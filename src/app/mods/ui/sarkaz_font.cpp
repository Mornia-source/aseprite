// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/ui/sarkaz_font.h"

#include "app/fonts/font_data.h"
#include "app/fonts/font_info.h"
#include "app/fonts/fonts.h"
#include "app/i18n/strings.h"
#include "app/resource_finder.h"
#include "base/fs.h"
#include "text/font.h"
#include "text/font_mgr.h"

#include <memory>

namespace app { namespace mods {

namespace {

// Must match the file name of data/strings/sarkaz.ini.
const char* kSarkazLangId = "sarkaz";

// Registered once under this name so repeated theme reloads reuse it.
const char* kSarkazFontName = "EndfieldByButan";

} // anonymous namespace

bool sarkaz_language_active()
{
  return (Strings::instance()->currentLanguage() == kSarkazLangId);
}

bool apply_sarkaz_font(Fonts* fonts,
                       text::FontMgrRef& fontMgr,
                       const float size,
                       text::FontRef& outFont,
                       FontInfo& outFontInfo)
{
  if (!fonts || !sarkaz_language_active())
    return false;

  FontData* fontData = fonts->fontDataByName(kSarkazFontName);
  if (!fontData) {
    ResourceFinder rf;
    rf.includeDataDir("mods/fonts/EndfieldByButan.ttf");
    if (!rf.findFirst())
      return false;

    auto newFont = std::make_unique<FontData>(text::FontType::FreeType);
    newFont->setName(kSarkazFontName);
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
  return true;
}

}} // namespace app::mods
