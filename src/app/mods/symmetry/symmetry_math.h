// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// The geometry behind the extra symmetry modes. No dependency on the rest of
// the application, so it can be tested on its own.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_SYMMETRY_SYMMETRY_MATH_H_INCLUDED
#define APP_MODS_SYMMETRY_SYMMETRY_MATH_H_INCLUDED
#pragma once

#include <vector>

namespace app { namespace mods {

enum class SymKind {
  Center,  // 180 degrees about the center.
  Radial,  // N-fold rotation.
  Mandala, // N-fold rotation plus N mirror lines (a dihedral group of 2N).
  Spiral,  // Each copy rotated and scaled once more than the one before.
};

struct SymSpec {
  SymKind kind = SymKind::Radial;
  int count = 6;
  // Degrees, counter-clockwise on screen.
  //   Mandala: orientation of the first mirror line.
  //   Spiral:  rotation between one copy and the next.
  //   Others:  unused.
  int angle = 0;
};

constexpr int kMinSymCount = 2;
constexpr int kMaxSymCount = 32;

// How much smaller each spiral copy is than the one before it.
constexpr double kSpiralScale = 0.85;

// A linear map about the symmetry center, in screen coordinates (y points
// down): (dx, dy) -> (a*dx + b*dy, c*dx + d*dy).
struct SymCopy {
  double a = 1, b = 0, c = 0, d = 1;
};

// The copies to draw besides the original stroke, which is always kept as is.
std::vector<SymCopy> symmetry_copies(const SymSpec& spec);

// The eight rotations and reflections of a square. A copy that is one of them
// can reuse the brush image transforms Aseprite already has; any other angle
// moves the brush without turning it.
enum class SymElem {
  Other,
  Identity,
  FlipX,      // Mirror across a vertical line.
  FlipY,      // Mirror across a horizontal line.
  FlipXY,     // 180 degrees.
  RotFlip270, // Mirror across the "/" diagonal.
  RotFlip90,  // Mirror across the "\" diagonal.
  Rot270,     // 90 degrees counter-clockwise.
  Rot90,      // 90 degrees clockwise.
};

SymElem classify(const SymCopy& copy);

// True if the brush image is turned a quarter, which swaps its width and height.
bool elem_rotates_image(SymElem elem);

struct SymPoint {
  int x = 0, y = 0;
};

// Where the brush cursor goes for a copy.
//
// (px, py) is the cursor pixel of the original and (w, h) / (cx, cy) the size
// and center of the brush there, as Stroke and Brush report them; the brush
// covers [px - cx, px - cx + w) by [py - cy, py - cy + h). The copy is placed
// by moving the geometric center of the brush, so an even-sized brush mirrors
// exactly as the built-in symmetry does.
SymPoint map_cursor(int px,
                    int py,
                    int w,
                    int h,
                    int cx,
                    int cy,
                    double centerX,
                    double centerY,
                    const SymCopy& copy,
                    SymElem elem);

}} // namespace app::mods

#endif
