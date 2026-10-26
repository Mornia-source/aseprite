// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Guide lines on the canvas for the extra symmetry modes.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_SYMMETRY_SYMMETRY_GUIDES_H_INCLUDED
#define APP_MODS_SYMMETRY_SYMMETRY_GUIDES_H_INCLUDED
#pragma once

#include "gfx/color.h"
#include "gfx/rect.h"

namespace render {
class Projection;
}
namespace ui {
class Graphics;
}

namespace app { namespace mods {

// Draws the guides for the active extra symmetry mode, if there is one:
// the two lines through the center that the document already lets the user
// drag, plus whatever shows the symmetry itself: spokes for radial, mirror
// lines for mandala, a spiral curve for spiral.
//
// spriteRect is where the sprite is on screen, in the coordinates of g.
// solid is for lines that are part of the symmetry, faint for the rest.
void draw_symmetry_guides(ui::Graphics* g,
                          const gfx::Rect& spriteRect,
                          const render::Projection& proj,
                          int spriteWidth,
                          int spriteHeight,
                          double centerX,
                          double centerY,
                          gfx::Color solid,
                          gfx::Color faint);

}} // namespace app::mods

#endif
