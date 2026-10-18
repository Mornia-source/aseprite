// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Select > Isolated Pixels.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_mask.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/status_bar.h"
#include "doc/cel.h"
#include "doc/color.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/primitives.h"

namespace app {

using namespace doc;

namespace {

// Whether a pixel counts as drawn. Transparent pixels are the absence of a
// pixel, so they are never isolated and never count as a matching neighbour.
// A background layer has no transparency: every pixel there is drawn.
bool is_drawn(const Image* image, const color_t c, const bool background)
{
  if (background)
    return true;
  switch (image->pixelFormat()) {
    case IMAGE_RGB:       return rgba_geta(c) != 0;
    case IMAGE_GRAYSCALE: return graya_geta(c) != 0;
    default:              return c != image->maskColor(); // Indexed.
  }
}

} // anonymous namespace

// Selects every pixel of the active cel that has no pixel of exactly the same
// color directly above, below, left or right of it. Diagonals do not count:
// a stray pixel touching its own color only at a corner is still a stray.
//
// Typical use is cleaning up noise after scaling, dithering or importing: the
// selection can then be deleted, recolored or inspected.
class SelectIsolatedPixelsCommand : public Command {
public:
  SelectIsolatedPixelsCommand() : Command(CommandId::SelectIsolatedPixels()) {}

protected:
  bool onEnabled(Context* ctx) override
  {
    // Tilemap cels hold tile indices, not colors.
    return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                           ContextFlags::ActiveLayerIsImage) &&
           !ctx->checkFlags(ContextFlags::ActiveLayerIsTilemap);
  }

  void onExecute(Context* ctx) override
  {
    Doc* doc = nullptr;
    int count = 0;
    {
      ContextWriter writer(ctx);
      doc = writer.document();

      const Cel* cel = writer.cel();
      if (!cel || !cel->image())
        return;

      const Image* image = cel->image();
      const bool background = writer.layer()->isBackground();
      const int w = image->width();
      const int h = image->height();

      // Start from the cel's full rectangle (all bits set) and clear every
      // pixel that is not isolated.
      Mask mask;
      mask.replace(gfx::Rect(0, 0, w, h));
      Image* bits = mask.bitmap();

      for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
          const color_t c = get_pixel(image, x, y);
          bool isolated = is_drawn(image, c, background);
          if (isolated) {
            // A neighbour outside the image is no pixel at all, so it cannot
            // be a match.
            if ((x > 0 && get_pixel(image, x - 1, y) == c) ||
                (x < w - 1 && get_pixel(image, x + 1, y) == c) ||
                (y > 0 && get_pixel(image, x, y - 1) == c) ||
                (y < h - 1 && get_pixel(image, x, y + 1) == c))
              isolated = false;
          }
          if (isolated)
            ++count;
          else
            put_pixel(bits, x, y, 0);
        }
      }

      mask.offsetOrigin(cel->x(), cel->y());
      mask.shrink();

      Tx tx(writer, friendlyName(), DoesntModifyDocument);
      tx(new cmd::SetMask(doc, &mask));
      doc->resetTransformation();
      tx.commit();
    }

    if (ctx->isUIAvailable()) {
      if (auto* statusBar = StatusBar::instance())
        statusBar->showTip(1500, Strings::commands_SelectIsolatedPixels_Count(count));
    }
    update_screen_for_document(doc);
  }
};

Command* CommandFactory::createSelectIsolatedPixelsCommand()
{
  return new SelectIsolatedPixelsCommand();
}

} // namespace app
