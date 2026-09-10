// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/ui/palette_bars.h"

#include "app/color_utils.h"
#include "app/ini_file.h"
#include "app/modules/gfx.h"
#include "app/ui/color_bar.h"
#include "app/ui/skin/skin_theme.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/theme.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace app { namespace mods {

using namespace ui;
using namespace app::skin;

namespace {

const char* kSection = "Mods";
const char* kEnabledKey = "PaletteBars";

std::string color_key(int row, bool left)
{
  return "PaletteBar" + std::to_string(row) + (left ? "L" : "R");
}

// Shortest way around the hue circle, so a red to magenta ramp does not travel
// the long way through green.
double lerp_hue(double a, double b, double t)
{
  double d = b - a;
  if (d > 180.0)
    d -= 360.0;
  else if (d < -180.0)
    d += 360.0;

  double h = a + d * t;
  if (h < 0.0)
    h += 360.0;
  else if (h >= 360.0)
    h -= 360.0;
  return h;
}

double lerp(double a, double b, double t)
{
  return a + (b - a) * t;
}

} // anonymous namespace

PaletteBars::PaletteBars() : Widget(kGenericWidget)
{
  setFocusStop(false);
  reloadColors();
  initTheme();
}

bool PaletteBars::enabled()
{
  return get_config_bool(kSection, kEnabledKey, true);
}

void PaletteBars::setEnabled(const bool state)
{
  set_config_bool(kSection, kEnabledKey, state);
}

void PaletteBars::reloadColors()
{
  for (int i = 0; i < kRows; ++i) {
    const std::string l = get_config_string(kSection, color_key(i, true).c_str(), "");
    const std::string r = get_config_string(kSection, color_key(i, false).c_str(), "");
    m_rows[i].left = (l.empty() ? app::Color::fromMask() : app::Color::fromString(l));
    m_rows[i].right = (r.empty() ? app::Color::fromMask() : app::Color::fromString(r));
  }
}

void PaletteBars::saveColors() const
{
  for (int i = 0; i < kRows; ++i) {
    const std::string l = (m_rows[i].left.getType() == app::Color::MaskType ?
                             std::string() : m_rows[i].left.toString());
    const std::string r = (m_rows[i].right.getType() == app::Color::MaskType ?
                             std::string() : m_rows[i].right.toString());
    set_config_string(kSection, color_key(i, true).c_str(), l.c_str());
    set_config_string(kSection, color_key(i, false).c_str(), r.c_str());
  }
}

int PaletteBars::swatchSize() const
{
  return 12 * guiscale();
}

int PaletteBars::rowGap() const
{
  return 2 * guiscale();
}

int PaletteBars::rowHeight() const
{
  return swatchSize();
}

void PaletteBars::onInitTheme(InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);
  setBorder(gfx::Border(2 * guiscale()));
}

void PaletteBars::onSizeHint(SizeHintEvent& ev)
{
  const gfx::Border b = border();
  ev.setSizeHint(
    gfx::Size(b.width(), b.height() + kRows * rowHeight() + (kRows - 1) * rowGap()));
}

gfx::Rect PaletteBars::rowBounds(const int row) const
{
  // Client coordinates: onPaint() draws in them, and onProcessMessage()
  // converts the mouse position into them. childrenBounds() would be relative
  // to the parent instead, putting everything outside the visible area.
  const gfx::Rect rc = clientChildrenBounds();
  return gfx::Rect(rc.x, rc.y + row * (rowHeight() + rowGap()), rc.w, rowHeight());
}

gfx::Rect PaletteBars::leftSwatchBounds(const int row) const
{
  const gfx::Rect rc = rowBounds(row);
  return gfx::Rect(rc.x, rc.y, swatchSize(), rc.h);
}

gfx::Rect PaletteBars::rightSwatchBounds(const int row) const
{
  const gfx::Rect rc = rowBounds(row);
  return gfx::Rect(rc.x + rc.w - swatchSize(), rc.y, swatchSize(), rc.h);
}

gfx::Rect PaletteBars::bandBounds(const int row, const bool upper) const
{
  const gfx::Rect rc = rowBounds(row);
  const int x = rc.x + swatchSize();
  const int w = rc.w - 2 * swatchSize();
  const int half = rc.h / 2;
  if (upper)
    return gfx::Rect(x, rc.y, w, half);
  return gfx::Rect(x, rc.y + half, w, rc.h - half);
}

app::Color PaletteBars::sampleRgb(const Row& r, const double t) const
{
  return app::Color::fromRgb(int(std::lround(lerp(r.left.getRed(), r.right.getRed(), t))),
                             int(std::lround(lerp(r.left.getGreen(), r.right.getGreen(), t))),
                             int(std::lround(lerp(r.left.getBlue(), r.right.getBlue(), t))),
                             int(std::lround(lerp(r.left.getAlpha(), r.right.getAlpha(), t))));
}

app::Color PaletteBars::sampleHsv(const Row& r, const double t) const
{
  return app::Color::fromHsv(lerp_hue(r.left.getHsvHue(), r.right.getHsvHue(), t),
                             lerp(r.left.getHsvSaturation(), r.right.getHsvSaturation(), t),
                             lerp(r.left.getHsvValue(), r.right.getHsvValue(), t),
                             int(std::lround(lerp(r.left.getAlpha(), r.right.getAlpha(), t))));
}

namespace {

// The frame drawn around palette entries and color buttons, so the ramps read
// as part of the color bar instead of as a foreign widget.
void draw_colorbar_frame(Graphics* g, SkinTheme* theme, const gfx::Rect& rc)
{
  theme->drawRect(g,
                  rc,
                  theme->parts.colorbar0()->bitmapNW(),
                  theme->parts.colorbar0()->bitmapN(),
                  theme->parts.colorbar1()->bitmapNE(),
                  theme->parts.colorbar1()->bitmapE(),
                  theme->parts.colorbar3()->bitmapSE(),
                  theme->parts.colorbar2()->bitmapS(),
                  theme->parts.colorbar2()->bitmapSW(),
                  theme->parts.colorbar0()->bitmapW());
}

// The colorbar frame is a rounded rectangle, but the color underneath it is a
// plain rect, so the four corners of the fill poke out past the rounding. Paint
// them back over with the widget background.
void clear_rounded_corners(Graphics* g, const gfx::Rect& rc, const gfx::Color bg)
{
  const int s = guiscale();
  if (rc.w < 4 * s || rc.h < 4 * s)
    return;

  const int x0 = rc.x + s;
  const int y0 = rc.y + s;
  const int x1 = rc.x + rc.w - 2 * s;
  const int y1 = rc.y + rc.h - 2 * s;

  g->fillRect(bg, gfx::Rect(x0, y0, s, s));
  g->fillRect(bg, gfx::Rect(x1, y0, s, s));
  g->fillRect(bg, gfx::Rect(x0, y1, s, s));
  g->fillRect(bg, gfx::Rect(x1, y1, s, s));
}

} // anonymous namespace

void PaletteBars::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  auto* theme = SkinTheme::get(this);
  const int scale = guiscale();

  g->fillRect(theme->colors.workspace(), clientBounds());

  for (int i = 0; i < kRows; ++i) {
    const Row& row = m_rows[i];

    // --- the two gradient bands, framed like a wide color button ---------
    gfx::Rect area = bandBounds(i, true);
    area |= bandBounds(i, false);

    if (area.w > 0) {
      // NOTE gfx::Rect::shrink() modifies in place and returns *this, so it
      // must be applied to a copy or the frame would end up drawn on the
      // already-shrunk rect, letting the gradient bleed past its border.
      gfx::Rect inner = area;
      inner.shrink(scale);

      if (row.complete()) {
        for (int b = 0; b < 2; ++b) {
          gfx::Rect bb = bandBounds(i, b == 0);
          bb.shrink(scale);
          // shrink() also eats the shared edge between the two bands, so give
          // each one back the half it should keep.
          if (b == 0)
            bb.h += scale;
          else {
            bb.y -= scale;
            bb.h += scale;
          }
          bb &= inner;
          if (bb.w <= 0 || bb.h <= 0)
            continue;

          for (int x = 0; x < bb.w; ++x) {
            const double t = (bb.w > 1 ? double(x) / double(bb.w - 1) : 0.0);
            const app::Color c = (b == 0 ? sampleRgb(row, t) : sampleHsv(row, t));
            g->fillRect(color_utils::color_for_ui(c), gfx::Rect(bb.x + x, bb.y, 1, bb.h));
          }
        }
      }
      else {
        // Waiting for both ends to be filled in.
        g->fillRect(theme->colors.face(), inner);
      }

      draw_colorbar_frame(g, theme, area);
      clear_rounded_corners(g, area, theme->colors.workspace());
    }

    // --- the two end swatches, drawn exactly like a color button ---------
    for (int s = 0; s < 2; ++s) {
      const gfx::Rect sb = (s == 0 ? leftSwatchBounds(i) : rightSwatchBounds(i));
      const app::Color& c = (s == 0 ? row.left : row.right);
      const Part part = (s == 0 ? Part::LeftSwatch : Part::RightSwatch);
      const bool hot = (m_hotRow == i && m_hotPart == part);

      draw_color_button(g, sb, c, doc::ColorMode::RGB, hot, false);
      clear_rounded_corners(g, sb, theme->colors.workspace());
    }
  }
}

PaletteBars::Part PaletteBars::hitTest(const gfx::Point& pos, int& row, double& t) const
{
  for (int i = 0; i < kRows; ++i) {
    if (!rowBounds(i).contains(pos))
      continue;

    row = i;
    if (leftSwatchBounds(i).contains(pos))
      return Part::LeftSwatch;
    if (rightSwatchBounds(i).contains(pos))
      return Part::RightSwatch;

    for (int b = 0; b < 2; ++b) {
      const gfx::Rect bb = bandBounds(i, b == 0);
      if (bb.w > 0 && bb.contains(pos)) {
        t = (bb.w > 1 ? double(pos.x - bb.x) / double(bb.w - 1) : 0.0);
        t = std::clamp(t, 0.0, 1.0);
        return (b == 0 ? Part::RgbBand : Part::HsvBand);
      }
    }
    break;
  }
  return Part::None;
}

void PaletteBars::updateHot(const gfx::Point& pos)
{
  int row = -1;
  double t = 0.0;
  const Part part = hitTest(pos, row, t);

  if (row != m_hotRow || part != m_hotPart) {
    m_hotRow = row;
    m_hotPart = part;
    invalidate();
  }
}

bool PaletteBars::onProcessMessage(Message* msg)
{
  if (msg->type() == kMouseMoveMessage) {
    auto* mouseMsg = static_cast<MouseMessage*>(msg);
    updateHot(mouseMsg->positionForDisplay(display()) - bounds().origin());
  }
  else if (msg->type() == kMouseLeaveMessage) {
    if (m_hotRow != -1 || m_hotPart != Part::None) {
      m_hotRow = -1;
      m_hotPart = Part::None;
      invalidate();
    }
  }
  else if (msg->type() == kMouseDownMessage) {
    auto* mouseMsg = static_cast<MouseMessage*>(msg);
    // Into client coordinates, matching rowBounds().
    const gfx::Point pos = mouseMsg->positionForDisplay(display()) - bounds().origin();

    int row = -1;
    double t = 0.0;
    const Part part = hitTest(pos, row, t);

    ColorBar* colorBar = ColorBar::instance();
    if (part != Part::None && row >= 0 && colorBar) {
      const bool leftButton = mouseMsg->left();

      switch (part) {
        // Clicking an end swatch stores the current color into it.
        case Part::LeftSwatch:
        case Part::RightSwatch: {
          const app::Color c = (leftButton ? colorBar->getFgColor() : colorBar->getBgColor());
          if (part == Part::LeftSwatch)
            m_rows[row].left = c;
          else
            m_rows[row].right = c;
          saveColors();
          invalidate();
          break;
        }

        // Clicking a band picks the color under the cursor.
        case Part::RgbBand:
        case Part::HsvBand: {
          if (m_rows[row].complete()) {
            const app::Color c = (part == Part::RgbBand ? sampleRgb(m_rows[row], t) :
                                                          sampleHsv(m_rows[row], t));
            if (leftButton)
              colorBar->setFgColor(c);
            else
              colorBar->setBgColor(c);
          }
          break;
        }

        case Part::None: break;
      }
      return true;
    }
  }
  return Widget::onProcessMessage(msg);
}

}} // namespace app::mods
