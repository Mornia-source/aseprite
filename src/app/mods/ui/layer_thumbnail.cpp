// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Feature F2: per-layer thumbnail column in the timeline.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/ui/layer_thumbnail.h"

#include "app/ini_file.h"
#include "app/resource_finder.h"
#include "app/thumbnails.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/object_id.h"
#include "doc/object_version.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/graphics.h"

#include <map>
#include <tuple>

namespace app { namespace mods {

namespace {

// The stock cel thumbnail helper re-renders through render::Render on every
// call, which is far too expensive for a column that repaints on every
// timeline invalidation. Key the result on the cel + image versions so any
// edit invalidates the entry naturally.
struct CacheKey {
  doc::ObjectId celId = 0;
  doc::ObjectVersion celVer = 0;
  doc::ObjectVersion imgVer = 0;
  int w = 0;
  int h = 0;

  bool operator<(const CacheKey& o) const
  {
    return std::tie(celId, celVer, imgVer, w, h) < std::tie(o.celId, o.celVer, o.imgVer, o.w, o.h);
  }
};

// Small enough that the whole cache stays trivial, large enough to cover a
// tall timeline plus a couple of frames of scrolling.
constexpr int kMaxCacheEntries = 256;

std::map<CacheKey, os::SurfaceRef>& cache()
{
  static std::map<CacheKey, os::SurfaceRef> instance;
  return instance;
}

const char* kConfigSection = "Mods";
const char* kConfigKey = "LayerThumbnails";

} // anonymous namespace

bool layer_thumbnails_enabled()
{
  return get_config_bool(kConfigSection, kConfigKey, true);
}

void set_layer_thumbnails_enabled(const bool state)
{
  set_config_bool(kConfigSection, kConfigKey, state);
  clear_layer_thumbnail_cache();
}

int layer_thumbnail_width(const int layerBoxHeight)
{
  if (!layer_thumbnails_enabled())
    return 0;

  // A square cell, so the thumbnail keeps the row height as its budget.
  return layerBoxHeight;
}

void draw_layer_thumbnail(ui::Graphics* g,
                          const gfx::Rect& bounds,
                          const doc::Layer* layer,
                          const doc::frame_t frame)
{
  if (!layer_thumbnails_enabled() || bounds.isEmpty())
    return;

  // Leave a 1px gutter so the thumbnail doesn't touch the row separator.
  gfx::Rect inner = bounds;
  inner.shrink(1);
  if (inner.w < 1 || inner.h < 1)
    return;

  // Group layers have no cel of their own; nothing to show for them (and
  // nothing to composite cheaply either).
  const doc::Cel* cel = layer->cel(frame);
  if (!cel || !cel->image())
    return;

  const CacheKey key{ cel->id(),
                      cel->version(),
                      cel->image()->version(),
                      inner.w,
                      inner.h };

  auto& c = cache();
  auto it = c.find(key);
  if (it == c.end()) {
    if (c.size() >= kMaxCacheEntries)
      c.clear();

    // scaleUpToFit=false: tiny cels stay pixel-exact instead of being blown
    // up into a blurry mess, which matters for pixel art.
    os::SurfaceRef surface =
      thumb::get_cel_thumbnail(g->display(), cel, false, inner.size());
    if (!surface)
      return;

    it = c.emplace(key, surface).first;
  }

  os::Surface* surface = it->second.get();
  if (!surface)
    return;

  // Center inside the cell.
  const int x = inner.x + (inner.w - surface->width()) / 2;
  const int y = inner.y + (inner.h - surface->height()) / 2;
  g->drawRgbaSurface(surface, x, y);
}

void draw_layer_thumbnails_toggle(ui::Graphics* g,
                                  const gfx::Rect& bounds,
                                  const bool enabled,
                                  const gfx::Color color)
{
  // Loaded once; a missing or corrupt file just means no glyph, never a crash.
  static bool tried = false;
  static os::SurfaceRef icon;
  if (!tried) {
    tried = true;
    ResourceFinder rf;
    rf.includeDataDir("mods/icons/layer_thumbnails.png");
    if (rf.findFirst())
      icon = os::System::instance()->loadRgbaSurface(rf.filename().c_str());
  }
  if (!icon)
    return;

  // Two frames side by side; frame 0 = on, frame 1 = off.
  const int fw = icon->width() / 2;
  const int fh = icon->height();
  if (fw < 1 || fh < 1)
    return;

  const int srcx = (enabled ? 0 : fw);
  const int x = bounds.x + (bounds.w - fw) / 2;
  const int y = bounds.y + (bounds.h - fh) / 2;
  g->drawColoredRgbaSurface(icon.get(), color, srcx, 0, x, y, fw, fh);
}

void clear_layer_thumbnail_cache()
{
  cache().clear();
}

}} // namespace app::mods
