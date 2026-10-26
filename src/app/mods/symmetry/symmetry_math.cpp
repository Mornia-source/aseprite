// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/symmetry/symmetry_math.h"

#include <algorithm>
#include <cmath>

namespace app { namespace mods {

namespace {

constexpr double kPi = 3.14159265358979323846;

double radians(const double degrees)
{
  return degrees * kPi / 180.0;
}

// Counter-clockwise on screen, where y points down.
SymCopy rotation(const double radiansCcw, const double scale = 1.0)
{
  const double cs = std::cos(radiansCcw) * scale;
  const double sn = std::sin(radiansCcw) * scale;
  SymCopy m;
  m.a = cs;
  m.b = sn;
  m.c = -sn;
  m.d = cs;
  return m;
}

// Mirror across a line through the center whose direction is radiansCcw
// counter-clockwise from the +x axis on screen.
SymCopy reflection(const double radiansCcw)
{
  const double cs = std::cos(2.0 * radiansCcw);
  const double sn = std::sin(2.0 * radiansCcw);
  SymCopy m;
  m.a = cs;
  m.b = -sn;
  m.c = -sn;
  m.d = -cs;
  return m;
}

bool near(const double v, const double target)
{
  return std::fabs(v - target) < 1e-9;
}

// -1, 0 or 1 if v is that to within rounding error, otherwise 2.
int snap(const double v)
{
  if (near(v, 0.0))
    return 0;
  if (near(v, 1.0))
    return 1;
  if (near(v, -1.0))
    return -1;
  return 2;
}

} // anonymous namespace

std::vector<SymCopy> symmetry_copies(const SymSpec& spec)
{
  std::vector<SymCopy> copies;
  const int n = std::clamp(spec.count, kMinSymCount, kMaxSymCount);
  const double step = 2.0 * kPi / n;

  switch (spec.kind) {
    case SymKind::Center: copies.push_back(rotation(kPi)); break;

    case SymKind::Radial:
      for (int k = 1; k < n; ++k)
        copies.push_back(rotation(step * k));
      break;

    case SymKind::Mandala:
      for (int k = 1; k < n; ++k)
        copies.push_back(rotation(step * k));
      // N mirror lines, pi / n apart.
      for (int k = 0; k < n; ++k)
        copies.push_back(reflection(radians(spec.angle) + kPi / n * k));
      break;

    case SymKind::Spiral: {
      const double turn = radians(spec.angle);
      double scale = 1.0;
      for (int k = 1; k < n; ++k) {
        scale *= kSpiralScale;
        copies.push_back(rotation(turn * k, scale));
      }
      break;
    }
  }
  return copies;
}

SymElem classify(const SymCopy& m)
{
  const int a = snap(m.a), b = snap(m.b), c = snap(m.c), d = snap(m.d);
  if (a == 2 || b == 2 || c == 2 || d == 2)
    return SymElem::Other;

  if (a == 1 && b == 0 && c == 0 && d == 1)
    return SymElem::Identity;
  if (a == -1 && b == 0 && c == 0 && d == 1)
    return SymElem::FlipX;
  if (a == 1 && b == 0 && c == 0 && d == -1)
    return SymElem::FlipY;
  if (a == -1 && b == 0 && c == 0 && d == -1)
    return SymElem::FlipXY;
  if (a == 0 && b == -1 && c == -1 && d == 0)
    return SymElem::RotFlip270;
  if (a == 0 && b == 1 && c == 1 && d == 0)
    return SymElem::RotFlip90;
  if (a == 0 && b == 1 && c == -1 && d == 0)
    return SymElem::Rot270;
  if (a == 0 && b == -1 && c == 1 && d == 0)
    return SymElem::Rot90;
  return SymElem::Other;
}

bool elem_rotates_image(const SymElem elem)
{
  return elem == SymElem::Rot90 || elem == SymElem::Rot270 || elem == SymElem::RotFlip90 ||
         elem == SymElem::RotFlip270;
}

SymPoint map_cursor(const int px,
                    const int py,
                    const int w,
                    const int h,
                    const int cx,
                    const int cy,
                    const double centerX,
                    const double centerY,
                    const SymCopy& m,
                    const SymElem elem)
{
  // Geometric center of the brush where it sits now.
  const double gx = px - cx + w / 2.0;
  const double gy = py - cy + h / 2.0;

  const double dx = gx - centerX;
  const double dy = gy - centerY;
  const double tx = centerX + m.a * dx + m.b * dy;
  const double ty = centerY + m.c * dx + m.d * dy;

  // The brush after the copy's transform: a quarter turn swaps its sides and,
  // as the built-in symmetry does, its pivot.
  int w2 = w, h2 = h, cx2 = cx, cy2 = cy;
  if (elem_rotates_image(elem)) {
    std::swap(w2, h2);
    std::swap(cx2, cy2);
  }

  // Round to the nearest pixel. The nudge keeps exact halves, which come from
  // half-pixel centers and even brushes, from flipping direction because of
  // floating point noise.
  SymPoint p;
  p.x = int(std::floor(tx - w2 / 2.0 + cx2 + 0.5 + 1e-7));
  p.y = int(std::floor(ty - h2 / 2.0 + cy2 + 0.5 + 1e-7));
  return p;
}

}} // namespace app::mods
