// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_SCRIPT_PANEL_API_H_INCLUDED
#define APP_MODS_SCRIPT_PANEL_API_H_INCLUDED
#pragma once

struct lua_State;

namespace app { namespace mods {

// app.panel{ dialog=..., id=..., title=..., side=... }
int App_panel(lua_State* L);

// app.closePanel("id")
int App_closePanel(lua_State* L);

}} // namespace app::mods

#endif
