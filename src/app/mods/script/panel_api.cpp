// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// app.panel{} -- docks a script's Dialog into the main window.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/script/panel_api.h"

#include "app/app.h"
#include "app/context.h"
#include "app/mods/script/dialog_content.h"
#include "app/mods/ui/plugin_panel.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "ui/base.h"

#include <string>

namespace app { namespace mods {

namespace {

int side_from_string(const std::string& s)
{
  if (s == "left")
    return ui::LEFT;
  if (s == "bottom")
    return ui::BOTTOM;
  return ui::RIGHT;
}

std::string field_string(lua_State* L, int table, const char* name, const char* def)
{
  std::string result = def;
  if (lua_getfield(L, table, name) != LUA_TNIL) {
    if (const char* p = lua_tostring(L, -1))
      result = p;
  }
  lua_pop(L, 1);
  return result;
}

} // anonymous namespace

// app.panel{ dialog=dlg, id="...", title="...", side="right" }
//
// Takes a Dialog the script has already filled in and docks its content in the
// main window instead of floating it. Returns a handle with :close().
//
// The dialog is not shown and must stay referenced by the script for as long as
// the panel is open: the content belongs to the dialog's window, and letting it
// be collected would take the panel's widgets with it.
int App_panel(lua_State* L)
{
  if (!lua_istable(L, 1))
    return luaL_error(L, "app.panel() expects a table");

  auto* ctx = App::instance()->context();
  if (!ctx || !ctx->isUIAvailable())
    return luaL_error(L, "app.panel() needs the user interface");

  const std::string id = field_string(L, 1, "id", "");
  const std::string title = field_string(L, 1, "title", "");
  const int side = side_from_string(field_string(L, 1, "side", "right"));

  if (id.empty())
    return luaL_error(L, "app.panel() needs an id");

  // Re-registering the same id replaces the previous panel, so a script can be
  // re-run without stacking copies of its panel.
  if (PluginPanel* old = PluginPanel::byId(id)) {
    old->close();
    delete old;
  }

  lua_getfield(L, 1, "dialog");
  ui::Widget* content = dialog_content(L, -1);
  lua_pop(L, 1);

  if (!content)
    return luaL_error(L, "app.panel() needs a dialog=Dialog{...}");

  auto* panel = new PluginPanel(id, title, side);
  panel->adopt(content);

  if (!panel->isDocked()) {
    delete panel;
    return luaL_error(L, "app.panel() could not dock the panel");
  }

  lua_pushstring(L, id.c_str());
  return 1;
}

// app.closePanel("id")
int App_closePanel(lua_State* L)
{
  const char* id = lua_tostring(L, 1);
  if (!id)
    return luaL_error(L, "app.closePanel() needs an id");

  if (PluginPanel* panel = PluginPanel::byId(id)) {
    panel->close();
    delete panel;
    lua_pushboolean(L, 1);
  }
  else {
    lua_pushboolean(L, 0);
  }
  return 1;
}

}} // namespace app::mods
