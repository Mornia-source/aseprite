// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/ui/conlang_font.h"

#include "app/fonts/font_data.h"
#include "app/fonts/font_info.h"
#include "app/fonts/fonts.h"
#include "app/i18n/strings.h"
#include "app/resource_finder.h"
#include "text/font.h"
#include "text/font_mgr.h"

#include <cstring>
#include <memory>
#include <string>

namespace app { namespace mods {

namespace {

struct ConlangFont {
  const char* langId;  // must match the data/strings/<id>.ini file name
  const char* name;    // registered under this name so theme reloads reuse it
  const char* file;    // relative to data/
};

// Which font each constructed-script language draws with.
//
//   sarkaz   EndfieldByButan covers all printable ASCII, so even numbers become
//            Kazdel glyphs. Strings are the English ones.
//   seaborn  AgeFonts001 covers letters and digits but almost no punctuation,
//            which the shaper substitutes from a system font. Strings are the
//            English ones.
//   farnorth FarNorthRunes covers ASCII plus the Nordic letters (ÆØÅÄÖ and even
//            Þ and Ð), which is why its strings are Norwegian Nynorsk rather
//            than English -- the accented characters render in-script instead
//            of falling back.
constexpr ConlangFont kFonts[] = {
  { "sarkaz",   "EndfieldByButan",     "mods/fonts/EndfieldByButan.ttf"     },
  { "seaborn",  "AgeFonts001",         "mods/fonts/AgeFonts001.ttf"         },
  { "farnorth", "FarNorthRunes-Heavy", "mods/fonts/FarNorthRunes-Heavy.ttf" },
};

// Which entry of kFonts the theme on screen was built with, or null for none.
const ConlangFont* g_applied = nullptr;

const ConlangFont* active_conlang()
{
  const std::string lang = Strings::instance()->currentLanguage();
  for (const auto& f : kFonts) {
    if (lang == f.langId)
      return &f;
  }
  return nullptr;
}

} // anonymous namespace

bool apply_conlang_font(Fonts* fonts,
                        text::FontMgrRef& fontMgr,
                        const float size,
                        text::FontRef& outFont,
                        FontInfo& outFontInfo)
{
  if (!fonts)
    return false;

  const ConlangFont* conlang = active_conlang();
  if (!conlang) {
    g_applied = nullptr;
    return false;
  }

  FontData* fontData = fonts->fontDataByName(conlang->name);
  if (!fontData) {
    ResourceFinder rf;
    rf.includeDataDir(conlang->file);
    if (!rf.findFirst())
      return false;

    auto newFont = std::make_unique<FontData>(text::FontType::FreeType);
    newFont->setName(conlang->name);
    newFont->setFilename(rf.filename());
    // The rest of the theme is a pixel font, so keep the edges hard instead of
    // letting a smooth outline sit next to it.
    newFont->setAntialias(false);
    newFont->setHinting(text::FontHinting::Normal);

    fontData = newFont.get();
    fonts->addFontData(std::move(newFont));
  }

  text::FontRef font = fontData->getFont(fontMgr, size);
  if (!font)
    return false;

  outFont = font;
  outFontInfo = FontInfo(fontData, size);
  g_applied = conlang;
  return true;
}

bool conlang_font_out_of_sync()
{
  return (g_applied != active_conlang());
}

}} // namespace app::mods
