// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/ui/plugin_panel.h"

#include "app/app.h"
#include "app/ui/dock.h"
#include "app/ui/main_window.h"
#include "ui/base.h"

#include <algorithm>
#include <vector>

namespace app { namespace mods {

namespace {

std::vector<PluginPanel*>& all_panels()
{
  static std::vector<PluginPanel*> panels;
  return panels;
}

} // anonymous namespace

PluginPanel::PluginPanel(const std::string& id, const std::string& title, const int side)
  : ui::VBox()
  , m_id(id)
  , m_side(side)
{
  setText(title);
  setExpansive(true);
  all_panels().push_back(this);
}

PluginPanel::~PluginPanel()
{
  close();
  auto& panels = all_panels();
  panels.erase(std::remove(panels.begin(), panels.end(), this), panels.end());
}

int PluginPanel::dockableAt() const
{
  return ui::LEFT | ui::RIGHT | ui::BOTTOM | ui::EXPANSIVE;
}

void PluginPanel::adopt(ui::Widget* content)
{
  if (!content || m_docked)
    return;

  MainWindow* mainWindow = App::instance()->mainWindow();
  Dock* dock = (mainWindow ? mainWindow->customizableDock() : nullptr);
  if (!dock)
    return;

  m_content = content;
  m_contentOldParent = content->parent();
  if (m_contentOldParent)
    m_contentOldParent->removeChild(content);
  addChild(content);

  dock->dock(m_side, this);
  m_docked = true;

  mainWindow->layout();
}

void PluginPanel::close()
{
  if (!m_docked)
    return;
  m_docked = false;

  MainWindow* mainWindow = App::instance()->mainWindow();
  if (Dock* dock = (mainWindow ? mainWindow->customizableDock() : nullptr))
    dock->undock(this);

  // Hand the content back. The dialog that owns it may still be used, and its
  // window expects to find its grid where it left it.
  if (m_content) {
    removeChild(m_content);
    if (m_contentOldParent)
      m_contentOldParent->addChild(m_content);
    m_content = nullptr;
    m_contentOldParent = nullptr;
  }

  if (mainWindow)
    mainWindow->layout();
}

// static
PluginPanel* PluginPanel::byId(const std::string& id)
{
  for (auto* panel : all_panels()) {
    if (panel->panelId() == id)
      return panel;
  }
  return nullptr;
}

// static
void PluginPanel::closeAll()
{
  // close() mutates nothing in the list, but be explicit about the copy anyway.
  auto panels = all_panels();
  for (auto* panel : panels)
    panel->close();
}

}} // namespace app::mods
