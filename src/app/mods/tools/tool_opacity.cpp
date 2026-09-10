// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/tools/tool_opacity.h"

#include "app/app.h"
#include "app/pref/preferences.h"
#include "app/tools/ink.h"
#include "app/tools/ink_type.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"

namespace app { namespace mods {

namespace {

// A tool whose ink paints and is not an effect is one whose ink type is
// remapped by ActiveToolManager::adjustToolInkDependingOnSelectedInkType().
// Effects (eraser, blur, jumble...) keep their own ink and already handle
// opacity themselves, so they must not be touched.
bool is_plain_paint_tool(const app::tools::Tool* tool)
{
  if (!tool)
    return false;

  // getInk() is not const in upstream.
  auto* t = const_cast<app::tools::Tool*>(tool);
  for (int i = 0; i < 2; ++i) {
    const app::tools::Ink* ink = t->getInk(i);
    if (ink && ink->isPaint() && !ink->isEffect())
      return true;
  }
  return false;
}

} // anonymous namespace

bool tool_offers_opacity(const app::tools::Tool* tool)
{
  return is_plain_paint_tool(tool);
}

void promote_ink_for_opacity(app::tools::Tool* tool, const int opacity)
{
  if (opacity >= 255)
    return;

  Preferences& pref = Preferences::instance();

  auto promote = [](Preferences& pref, app::tools::Tool* t) {
    if (!is_plain_paint_tool(t))
      return;
    auto& toolPref = pref.tool(t);
    if (toolPref.ink() == app::tools::InkType::SIMPLE)
      toolPref.ink(app::tools::InkType::ALPHA_COMPOSITING);
  };

  // Mirror how the opacity value itself is shared.
  if (pref.shared.shareInk()) {
    for (app::tools::Tool* t : *App::instance()->toolBox())
      promote(pref, t);
  }
  else {
    promote(pref, tool);
  }
}

}} // namespace app::mods
