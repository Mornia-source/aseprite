// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/i18n/retranslate.h"

#include "app/i18n/strings.h"
#include "ui/widget.h"

#include <memory>

namespace app { namespace mods {

void remember_text_string_id(ui::Widget* widget, const std::string& stringId)
{
  if (!widget || stringId.empty())
    return;

  widget->setProperty(std::make_shared<I18nTextProperty>(stringId));
}

int retranslate_all(ui::Widget* root)
{
  if (!root)
    return 0;

  int count = 0;

  if (const auto prop = root->getProperty(I18nTextProperty::Name)) {
    const auto* textProp = static_cast<const I18nTextProperty*>(prop.get());
    const std::string& translated = Strings::Translate(textProp->id().c_str());

    // Skip untouched widgets so this does not invalidate the whole tree.
    if (root->text() != translated)
      root->setText(translated);

    ++count;
  }

  for (auto* child : root->children())
    count += retranslate_all(child);

  return count;
}

}} // namespace app::mods
