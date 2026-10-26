// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/symmetry/symmetry_guides.h"

#include "app/mods/symmetry/symmetry_settings.h"
#include "render/projection.h"
#include "ui/graphics.h"

#include <algorithm>
#include <cmath>

namespace app { namespace mods {

namespace {

constexpr double kPi = 3.14159265358979323846;

struct Canvas {
  ui::Graphics* g;
  gfx::Rect rect;
  const render::Projection* proj;
  double w, h; // Sprite size.

  gfx::Point toScreen(const double x, const double y) const
  {
    return gfx::Point(rect.x + int(proj->applyX<double>(x)), rect.y + int(proj->applyY<double>(y)));
  }

  // Liang-Barsky: clips the segment to the sprite and draws what is left.
  void segment(const gfx::Color color, double x0, double y0, double x1, double y1) const
  {
    const double dx = x1 - x0, dy = y1 - y0;
    double t0 = 0.0, t1 = 1.0;
    const double p[4] = { -dx, dx, -dy, dy };
    const double q[4] = { x0, w - x0, y0, h - y0 };
    for (int i = 0; i < 4; ++i) {
      if (p[i] == 0.0) {
        if (q[i] < 0.0)
          return; // Parallel to this edge and outside it.
      }
      else {
        const double r = q[i] / p[i];
        if (p[i] < 0.0) {
          if (r > t1)
            return;
          t0 = std::max(t0, r);
        }
        else {
          if (r < t0)
            return;
          t1 = std::min(t1, r);
        }
      }
    }
    g->drawLine(color, toScreen(x0 + t0 * dx, y0 + t0 * dy), toScreen(x0 + t1 * dx, y0 + t1 * dy));
  }

  // A direction that many degrees counter-clockwise on screen from the +x axis.
  static void direction(const double degrees, double& ux, double& uy)
  {
    const double a = degrees * kPi / 180.0;
    ux = std::cos(a);
    uy = -std::sin(a);
  }

  // From the center out to past the edge of the sprite.
  void ray(const gfx::Color color, const double cx, const double cy, const double degrees) const
  {
    double ux, uy;
    direction(degrees, ux, uy);
    const double len = (w + h) * 2.0;
    segment(color, cx, cy, cx + ux * len, cy + uy * len);
  }

  // A full line through the center.
  void line(const gfx::Color color, const double cx, const double cy, const double degrees) const
  {
    double ux, uy;
    direction(degrees, ux, uy);
    const double len = (w + h) * 2.0;
    segment(color, cx - ux * len, cy - uy * len, cx + ux * len, cy + uy * len);
  }
};

} // anonymous namespace

void draw_symmetry_guides(ui::Graphics* g,
                          const gfx::Rect& spriteRect,
                          const render::Projection& proj,
                          const int spriteWidth,
                          const int spriteHeight,
                          const double centerX,
                          const double centerY,
                          const gfx::Color solid,
                          const gfx::Color faint)
{
  if (!symmetry_mods_active())
    return;

  const Canvas canvas{ g, spriteRect, &proj, double(spriteWidth), double(spriteHeight) };
  const SymSpec spec = current_symmetry_spec();

  // The two lines the document lets the user drag. For the center mode they
  // are the whole symmetry; for the others they only mark where it is.
  const gfx::Color crossColor = (spec.kind == SymKind::Center ? solid : faint);
  canvas.line(crossColor, centerX, centerY, 0.0);
  canvas.line(crossColor, centerX, centerY, 90.0);

  switch (spec.kind) {
    case SymKind::Center: break;

    case SymKind::Radial:
      for (int k = 0; k < spec.count; ++k)
        canvas.ray(solid, centerX, centerY, 360.0 * k / spec.count);
      break;

    case SymKind::Mandala:
      for (int k = 0; k < spec.count; ++k)
        canvas.line(solid, centerX, centerY, spec.angle + 180.0 * k / spec.count);
      break;

    case SymKind::Spiral: {
      // The curve the copies sit on: each step turns by the angle and shrinks
      // by kSpiralScale. Drawn past both ends of the copies to show where it
      // goes.
      const double r0 = std::min(canvas.w, canvas.h) / 2.0;
      constexpr double kStart = -12.0, kEnd = 30.0, kStep = 0.2;
      bool havePrev = false;
      double px = 0, py = 0;
      for (double t = kStart; t <= kEnd; t += kStep) {
        const double r = r0 * std::pow(kSpiralScale, t);
        double ux, uy;
        Canvas::direction(spec.angle * t, ux, uy);
        const double x = centerX + ux * r, y = centerY + uy * r;
        // Skip the part that has spiralled out to nowhere.
        if (havePrev && r < canvas.w + canvas.h)
          canvas.segment(solid, px, py, x, y);
        px = x;
        py = y;
        havePrev = true;
      }
      break;
    }
  }
}

}} // namespace app::mods
