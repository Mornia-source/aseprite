// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Turning one stroke into its symmetrical copies.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_SYMMETRY_SYMMETRY_STROKES_H_INCLUDED
#define APP_MODS_SYMMETRY_SYMMETRY_STROKES_H_INCLUDED
#pragma once

#include "app/tools/stroke.h"

namespace app {
namespace tools {
class ToolLoop;
}
namespace mods {

// If one of our symmetry modes is active, appends `stroke` followed by its
// copies to `strokes` and returns true. Returns false, leaving `strokes`
// alone, when it is not, so the built-in symmetry can go ahead.
//
// (centerX, centerY) is the symmetry center in sprite coordinates: the place
// where the document's two guide lines cross.
bool generate_symmetry_strokes(const tools::Stroke& stroke,
                               tools::Strokes& strokes,
                               tools::ToolLoop* loop,
                               double centerX,
                               double centerY);

}} // namespace app::mods

#endif
