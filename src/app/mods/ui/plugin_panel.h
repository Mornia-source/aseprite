// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Docked panels for scripts.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_UI_PLUGIN_PANEL_H_INCLUDED
#define APP_MODS_UI_PLUGIN_PANEL_H_INCLUDED
#pragma once

#include "app/ui/dockable.h"
#include "ui/box.h"

#include <string>

namespace app { namespace mods {

// A panel a script can dock into the main window, next to the timeline and the
// color bar, instead of only being able to open a floating Dialog.
//
// It does not build widgets of its own: it adopts the content of a Dialog the
// script has already filled in, so every widget Dialog offers works here with
// no second implementation to keep in step. The script keeps a reference to
// that Dialog for as long as the panel is docked -- the grid it borrows is a
// member of the dialog's window, so the dialog has to outlive the panel.
class PluginPanel : public ui::VBox,
                    public app::Dockable {
public:
  // `side` is ui::LEFT, ui::RIGHT or ui::BOTTOM.
  PluginPanel(const std::string& id, const std::string& title, int side);
  ~PluginPanel() override;

  const std::string& panelId() const { return m_id; }
  int side() const { return m_side; }

  // Moves `content` under this panel and docks the panel into the main window.
  // The widget keeps belonging to whoever created it; close() puts it back.
  void adopt(ui::Widget* content);

  // Undocks and returns the adopted widget to its previous parent.
  void close();

  bool isDocked() const { return m_docked; }

  // The panel with this id, or null.
  static PluginPanel* byId(const std::string& id);

  // Closes every panel. Used when scripts are reloaded, so a panel does not
  // outlive the script that made it.
  static void closeAll();

protected:
  int dockableAt() const override;

private:
  std::string m_id;
  int m_side;
  bool m_docked = false;
  ui::Widget* m_content = nullptr;
  ui::Widget* m_contentOldParent = nullptr;
};

}} // namespace app::mods

#endif
