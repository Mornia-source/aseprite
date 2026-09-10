// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Live re-translation of the UI when the language changes.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_I18N_RETRANSLATE_H_INCLUDED
#define APP_MODS_I18N_RETRANSLATE_H_INCLUDED
#pragma once

#include "ui/property.h"

#include <string>
#include <utility>

namespace ui {
class Widget;
}

namespace app { namespace mods {

// Remembers which string a widget's text came from.
//
// XmlTranslator resolves "@id" to its translation while the widget is built and
// only the resulting text is stored, so afterwards nothing knows which string
// to look up again. Upstream therefore re-translates the menu bar alone on a
// language change and leaves every other widget showing the old language until
// the next start.
class I18nTextProperty : public ui::Property {
public:
  static constexpr const char* Name = "ModsI18nText";

  explicit I18nTextProperty(std::string id) : ui::Property(Name), m_id(std::move(id)) {}

  const std::string& id() const { return m_id; }

private:
  std::string m_id;
};

// Attaches the property above. Does nothing when `stringId` is empty, which is
// the case for literal text that must not be translated.
void remember_text_string_id(ui::Widget* widget, const std::string& stringId);

// Looks the remembered strings up again for `root` and all its descendants.
// Returns the number of widgets whose text was replaced.
int retranslate_all(ui::Widget* root);

}} // namespace app::mods

#endif
