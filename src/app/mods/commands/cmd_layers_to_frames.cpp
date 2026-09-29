// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Layer > Layers to Frames.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/doc_api.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "ui/alert.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"

#include <algorithm>
#include <set>
#include <vector>

namespace app {

using namespace doc;

namespace {

// Layers we can turn into a frame: ones that hold plain pixels. Tilemaps would
// need converting to pixels first, and reference layers are not artwork.
bool is_convertible(const Layer* layer)
{
  return layer->isImage() && !layer->isTilemap() && !layer->isReference();
}

// Appends the convertible layers under `layer` bottom-to-top, the order
// LayerGroup::layers() keeps them in.
void collect(Layer* layer, std::vector<Layer*>& out)
{
  if (layer->isGroup()) {
    for (Layer* child : static_cast<LayerGroup*>(layer)->layers())
      collect(child, out);
  }
  else if (is_convertible(layer)) {
    out.push_back(layer);
  }
}

// The layers to convert: the ones selected in the timeline if there are at
// least two, otherwise every layer in the sprite.
std::vector<Layer*> source_layers(const Site& site)
{
  std::vector<Layer*> result;

  const auto& range = site.range();
  if (range.enabled() && range.selectedLayers().size() >= 2) {
    std::set<Layer*> seen;
    for (Layer* layer : range.selectedLayers().toAllLayersList()) {
      std::vector<Layer*> leaves;
      collect(layer, leaves);
      for (Layer* leaf : leaves) {
        if (seen.insert(leaf).second)
          result.push_back(leaf);
      }
    }
    if (result.size() >= 2)
      return result;
    result.clear();
  }

  collect(site.sprite()->root(), result);
  return result;
}

} // anonymous namespace

// Turns layers into frames of one layer: the bottom layer becomes frame 1, the
// next one up frame 2, and so on -- the order Photoshop's "Make Frames From
// Layers" uses, which is how animations drawn as PSD layers expect to play.
//
// Each frame takes the cel the layer has at the current frame. The source
// layers are removed, along with any group they leave empty, so the result is
// a conversion rather than a copy; one undo restores everything.
//
// The order can be flipped: "order" = "ascending" (bottom layer first, the
// default above) or "descending" (top layer first). Without the parameter --
// i.e. from the menu -- the user is asked; scripts pass it to skip the prompt.
class LayersToFramesCommand : public Command {
public:
  LayersToFramesCommand() : Command(CommandId::LayersToFrames()) {}

protected:
  enum class Order { Ask, Ascending, Descending };

  void onLoadParams(const Params& params) override
  {
    const std::string order = params.get("order");
    if (order == "ascending")
      m_order = Order::Ascending;
    else if (order == "descending")
      m_order = Order::Descending;
    else
      m_order = Order::Ask;
  }

  bool onEnabled(Context* ctx) override
  {
    if (!ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable))
      return false;

    std::vector<Layer*> layers;
    collect(ctx->activeSite().sprite()->root(), layers);
    return layers.size() >= 2;
  }

  void onExecute(Context* ctx) override
  {
    bool descending = (m_order == Order::Descending);
    if (m_order == Order::Ask && ctx->isUIAvailable()) {
      // 1 = bottom to top, 2 = top to bottom, anything else = cancelled.
      const int ret = ui::Alert::show(Strings::alerts_mods_layers_to_frames_order());
      if (ret != 1 && ret != 2)
        return;
      descending = (ret == 2);
    }

    LayerImage* target = nullptr;
    Doc* doc = nullptr;
    {
      ContextWriter writer(ctx);
      const Site& site = writer.site();
      Sprite* sprite = site.sprite();
      doc = writer.document();

      std::vector<Layer*> sources = source_layers(site);
      if (sources.size() < 2)
        return;
      if (descending)
        std::reverse(sources.begin(), sources.end());

      const frame_t fromFrame = site.frame();
      Tx tx(writer, friendlyName());
      DocApi api = writer.document()->getApi(tx);

      target = api.newLayer(sprite->root(), Strings::commands_LayersToFrames_LayerName());
      api.addEmptyFramesTo(sprite, frame_t(sources.size()) - 1);

      for (size_t i = 0; i < sources.size(); ++i) {
        Layer* layer = sources[i];
        const Cel* src = layer->cel(fromFrame);
        if (!src || !src->image())
          continue; // Stays an empty frame, so later frames keep their places.

        ImageRef image(Image::createCopy(src->image()));
        auto* cel = new Cel(frame_t(i), image);
        cel->setPosition(src->position());

        // The target is one layer, so per-layer opacity has nowhere to live
        // except in the cel. Blend modes have no per-cel equivalent and are
        // dropped; hidden layers are included, since a PSD animation usually
        // keeps all but one of its frames hidden.
        const int layerOpacity = static_cast<LayerImage*>(layer)->opacity();
        cel->setOpacity(src->opacity() * layerOpacity / 255);

        api.addCel(target, cel);
      }

      // Remove the sources, then any group that removing them emptied. Groups
      // that were already empty are none of our business and stay.
      std::set<LayerGroup*> touched;
      for (Layer* layer : sources) {
        for (Layer* g = layer->parent(); g && g != sprite->root(); g = g->parent())
          touched.insert(static_cast<LayerGroup*>(g));
        api.removeLayer(layer);
      }

      bool removed = true;
      while (removed) { // Innermost first: a parent may only empty out later.
        removed = false;
        for (auto it = touched.begin(); it != touched.end(); ++it) {
          if ((*it)->layersCount() == 0) {
            api.removeLayer(*it);
            touched.erase(it);
            removed = true;
            break;
          }
        }
      }

      tx.commit();
    }

    // Outside the writer: changing the active layer notifies observers that may
    // want to read the document.
    ctx->setActiveLayer(target);
    ctx->setActiveFrame(0);
    update_screen_for_document(doc);
  }

private:
  Order m_order = Order::Ask;
};

Command* CommandFactory::createLayersToFramesCommand()
{
  return new LayersToFramesCommand();
}

} // namespace app
