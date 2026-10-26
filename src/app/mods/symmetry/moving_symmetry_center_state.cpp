// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/symmetry/moving_symmetry_center_state.h"

#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "fmt/format.h"
#include "ui/message.h"

#include <algorithm>
#include <cmath>

namespace app { namespace mods {

namespace {

// Half pixels, and at least one pixel from either edge.
double snap(const double pos, const double size)
{
  return std::clamp(std::round(pos * 2.0) / 2.0, 1.0, size - 1.0);
}

} // anonymous namespace

MovingSymmetryCenterState::MovingSymmetryCenterState(Editor* editor, ui::MouseMessage* msg)
  : m_xAxis(Preferences::instance().document(editor->document()).symmetry.xAxis)
  , m_yAxis(Preferences::instance().document(editor->document()).symmetry.yAxis)
  , m_xStart(m_xAxis())
  , m_yStart(m_yAxis())
{
  m_mouseStart = editor->screenToEditorF(msg->position());
  editor->captureMouse();
}

bool MovingSymmetryCenterState::onMouseUp(Editor* editor, ui::MouseMessage* msg)
{
  editor->backToPreviousState();
  editor->releaseMouse();
  return true;
}

bool MovingSymmetryCenterState::onMouseMove(Editor* editor, ui::MouseMessage* msg)
{
  const gfx::PointF delta = editor->screenToEditorF(msg->position()) - m_mouseStart;

  m_xAxis(snap(m_xStart + delta.x, editor->sprite()->width()));
  m_yAxis(snap(m_yStart + delta.y, editor->sprite()->height()));

  editor->invalidate();

  // Let the standby state do its usual work (cursor, status bar).
  return StandbyState::onMouseMove(editor, msg);
}

bool MovingSymmetryCenterState::onUpdateStatusBar(Editor* editor)
{
  StatusBar::instance()->setStatusText(0, fmt::format("X {:3.1f} Y {:3.1f}", m_xAxis(), m_yAxis()));
  return true;
}

}} // namespace app::mods
