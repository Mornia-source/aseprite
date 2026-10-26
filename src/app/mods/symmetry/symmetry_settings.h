// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Settings for the extra symmetry modes, and the small adapters the official
// code calls to find out about them.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_SYMMETRY_SYMMETRY_SETTINGS_H_INCLUDED
#define APP_MODS_SYMMETRY_SYMMETRY_SETTINGS_H_INCLUDED
#pragma once

#include "app/mods/symmetry/symmetry_math.h"
#include "app/pref/preferences.h"

namespace app { namespace mods {

// Which symmetry is in charge. `Official` hands over to the document's
// built-in modes (horizontal, vertical, diagonals); the rest are ours.
enum class SymMode {
  Official = 0,
  Center,
  Radial,
  Mandala,
  Spiral,
};

struct SymmetrySettings {
  SymMode mode = SymMode::Official;
  int count = 6;
  int mandalaAngle = 0;
  int spiralAngle = 30;
};

// Global rather than per document: it is a way of drawing, like the brush, and
// not a property of one sprite. The center is still per document -- it reuses
// the built-in guide position, so it is saved with the document's preferences.
SymmetrySettings& symmetry_settings();
void save_symmetry_settings();

// True when one of our modes is drawing: chosen, and symmetry is switched on.
bool symmetry_mods_active();

SymSpec current_symmetry_spec();

// The built-in code decides what to do from the document's SymmetryMode. Our
// modes live outside it, so these two say what that code should see:
//
//   ..._for_tools:  BOTH while ours is active, so a Symmetry object is made
//                   and the guide handles are offered; the strokes are
//                   produced by us regardless of the value.
//   ..._for_guides: NONE while ours is active, so the built-in guide lines for
//                   the document's own mode are not drawn on top of ours.
gen::SymmetryMode symmetry_mode_for_tools(gen::SymmetryMode docMode);
gen::SymmetryMode symmetry_mode_for_guides(gen::SymmetryMode docMode);

}} // namespace app::mods

#endif
