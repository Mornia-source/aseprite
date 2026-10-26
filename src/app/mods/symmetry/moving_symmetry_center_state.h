// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Dragging the symmetry center directly.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_SYMMETRY_MOVING_SYMMETRY_CENTER_STATE_H_INCLUDED
#define APP_MODS_SYMMETRY_MOVING_SYMMETRY_CENTER_STATE_H_INCLUDED
#pragma once

#include "app/pref/preferences.h"
#include "app/ui/editor/standby_state.h"

namespace app {
class Editor;
}

namespace app { namespace mods {

// The built-in handles move one guide line each, so putting the center of a
// radial or mandala pattern somewhere takes two drags. This moves both at
// once: grab the handle where the two lines cross.
//
// Positions snap to half pixels and stay off the very edge, as the built-in
// dragging does.
class MovingSymmetryCenterState : public StandbyState {
public:
  MovingSymmetryCenterState(Editor* editor, ui::MouseMessage* msg);

  bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
  bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
  bool onUpdateStatusBar(Editor* editor) override;

  bool requireBrushPreview() override { return false; }

private:
  Option<double>& m_xAxis;
  Option<double>& m_yAxis;
  double m_xStart;
  double m_yStart;
  gfx::PointF m_mouseStart;
};

}} // namespace app::mods

#endif
