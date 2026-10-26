// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// The symmetry controls that live permanently at the right end of the context
// bar.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_UI_SYMMETRY_BAR_H_INCLUDED
#define APP_MODS_UI_SYMMETRY_BAR_H_INCLUDED
#pragma once

#include "app/context_observer.h"
#include "ui/box.h"

#include <memory>

namespace ui {
class ComboBox;
class Label;
class TooltipManager;
} // namespace ui

namespace app {

namespace mods {

class SymmetryCountEntry;
class SymmetryOptionsButton;

// One place for everything about symmetry: the mode (the built-in ones and ours
// in a single list), how many copies, the angle, and where the center is.
//
// It replaces the built-in symmetry buttons, which only show up for some tools
// and have no room for modes that take parameters. It is always visible and
// pushed against the right edge by a filler placed before it in the context bar.
class SymmetryBar : public ui::HBox,
                    public ContextObserver {
public:
  explicit SymmetryBar(ui::TooltipManager* tooltipManager);
  ~SymmetryBar();

protected:
  // ContextObserver
  void onActiveSiteChange(const Site& site) override;
  // ui::Widget
  void onInitTheme(ui::InitThemeEvent& ev) override;

private:
  void refresh();
  void updateMinSize();
  void applyChoice(int choice);
  void applyCount(int count);
  void applyAngle(int angle);
  void retranslate();
  void showPositionMenu();

  ui::TooltipManager* m_tooltips;
  ui::ComboBox* m_mode;
  ui::Label* m_countLabel;
  SymmetryCountEntry* m_count;
  ui::Label* m_angleLabel;
  SymmetryCountEntry* m_angle;
  SymmetryOptionsButton* m_options;
  bool m_updating = false;
  obs::scoped_connection m_enabledConn;
  obs::scoped_connection m_languageConn;
};

} // namespace mods
} // namespace app

#endif
