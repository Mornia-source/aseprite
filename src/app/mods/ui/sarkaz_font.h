// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// The Sarkaz joke language: English strings drawn in the Kazdel script.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_UI_SARKAZ_FONT_H_INCLUDED
#define APP_MODS_UI_SARKAZ_FONT_H_INCLUDED
#pragma once

#include "text/fwd.h"

namespace app {
class Fonts;
class FontInfo;
} // namespace app

namespace app { namespace mods {

// True when the UI language is the Sarkaz one.
bool sarkaz_language_active();

// Replaces the theme font with Sarkaz.ttf while that language is selected.
//
// data/strings/sarkaz.ini is a verbatim copy of the English strings, so on its
// own it would look like English. The script comes from the font:
// EndfieldByButan.ttf maps the whole printable ASCII range -- letters, digits
// and punctuation -- onto Kazdel glyphs, so ordinary English text renders as
// Sarkaz. Note that this means numbers are unreadable too; only text outside
// ASCII (the Chinese half of the language's own display name, for instance)
// falls back to a system font.
//
// Returns false and leaves the arguments untouched when the language is not
// active or the font could not be loaded.
bool apply_sarkaz_font(Fonts* fonts,
                       text::FontMgrRef& fontMgr,
                       float size,
                       text::FontRef& outFont,
                       FontInfo& outFontInfo);

}} // namespace app::mods

#endif
