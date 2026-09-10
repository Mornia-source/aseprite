// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Three gradient ramps shown above the palette.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_UI_PALETTE_BARS_H_INCLUDED
#define APP_MODS_UI_PALETTE_BARS_H_INCLUDED
#pragma once

#include "app/color.h"
#include "ui/widget.h"

#include <array>

namespace app { namespace mods {

// Three independent ramps. Each row is a strip with a square swatch at each
// end; when both ends hold a color the space between them shows two gradient
// bands to pick from.
//
//   [A]===================================[B]
//        ^ upper band: RGB interpolation
//        v lower band: HSV interpolation
//
// Clicking a swatch stores the current foreground (left button) or background
// (right button) color into it. Clicking a band picks the color under the
// cursor into the foreground (left) or background (right).
//
// The colors are a tool, not document data, so they live in the global config
// rather than in the sprite -- switching documents keeps the ramps you built.
class PaletteBars : public ui::Widget {
public:
  static constexpr int kRows = 3;

  PaletteBars();

  // Reads the "[Mods] PaletteBars" config entry.
  static bool enabled();
  static void setEnabled(bool state);

  void reloadColors();

protected:
  void onInitTheme(ui::InitThemeEvent& ev) override;
  void onSizeHint(ui::SizeHintEvent& ev) override;
  void onPaint(ui::PaintEvent& ev) override;
  bool onProcessMessage(ui::Message* msg) override;

private:
  struct Row {
    app::Color left = app::Color::fromMask();
    app::Color right = app::Color::fromMask();
    bool complete() const { return left.getType() != app::Color::MaskType &&
                                   right.getType() != app::Color::MaskType; }
  };

  // Which part of the widget a point falls in.
  enum class Part { None, LeftSwatch, RightSwatch, RgbBand, HsvBand };

  int rowHeight() const;
  int rowGap() const;
  int swatchSize() const;
  gfx::Rect rowBounds(int row) const;
  gfx::Rect leftSwatchBounds(int row) const;
  gfx::Rect rightSwatchBounds(int row) const;
  gfx::Rect bandBounds(int row, bool upper) const;
  Part hitTest(const gfx::Point& pos, int& row, double& t) const;

  app::Color sampleRgb(const Row& r, double t) const;
  app::Color sampleHsv(const Row& r, double t) const;

  void saveColors() const;

  // Hover state, so the end swatches light up like the palette entries do.
  void updateHot(const gfx::Point& pos);

  std::array<Row, kRows> m_rows;
  int m_hotRow = -1;
  Part m_hotPart = Part::None;
};

}} // namespace app::mods

#endif
