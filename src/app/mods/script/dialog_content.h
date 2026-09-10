// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_SCRIPT_DIALOG_CONTENT_H_INCLUDED
#define APP_MODS_SCRIPT_DIALOG_CONTENT_H_INCLUDED
#pragma once

struct lua_State;

namespace ui {
class Widget;
}

namespace app { namespace mods {

// The widget holding a Dialog's content, for the Lua value at `index`.
//
// Defined in src/app/script/dialog_class.cpp (seam S26) because the Dialog type
// lives there and nowhere else. The docked-panel API borrows this widget rather
// than growing a second implementation of every dialog control.
//
// Returns null when the value is not a Dialog.
ui::Widget* dialog_content(lua_State* L, int index);

}} // namespace app::mods

#endif
