// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Constructed-script languages: real strings drawn in an invented script.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_UI_CONLANG_FONT_H_INCLUDED
#define APP_MODS_UI_CONLANG_FONT_H_INCLUDED
#pragma once

#include "text/fwd.h"

namespace app {
class Fonts;
class FontInfo;
} // namespace app

namespace app { namespace mods {

// Replaces the theme font while one of the constructed-script languages is
// selected.
//
// Each of those languages ships an existing translation verbatim -- there is no
// invented vocabulary anywhere. The script comes entirely from the font, which
// maps Latin letters onto invented glyphs, so ordinary text renders as the
// fictional writing system. See kFonts in the .cpp for which font each language
// uses and what it covers.
//
// Returns false and leaves the arguments untouched when the current language is
// not one of them, or when the font could not be loaded.
bool apply_conlang_font(Fonts* fonts,
                        text::FontMgrRef& fontMgr,
                        float size,
                        text::FontRef& outFont,
                        FontInfo& outFontInfo);

// True when the font the theme is currently drawing with no longer matches the
// selected language, so the theme has to be regenerated.
//
// The font is chosen while the theme loads, and changing the language does not
// reload the theme by itself: without this the UI kept the invented glyphs
// after switching away.
bool conlang_font_out_of_sync();

}} // namespace app::mods

#endif
