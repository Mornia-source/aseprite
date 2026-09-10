// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Feature F2: per-layer thumbnail column in the timeline.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_UI_LAYER_THUMBNAIL_H_INCLUDED
#define APP_MODS_UI_LAYER_THUMBNAIL_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "gfx/color.h"
#include "gfx/rect.h"

namespace doc {
class Layer;
}

namespace ui {
class Display;
class Graphics;
} // namespace ui

namespace app { namespace mods {

// Whether the layer thumbnail column is shown at all. Reads the
// "mods.layer_thumbnails" preference so the column can be turned off
// without recompiling.
bool layer_thumbnails_enabled();

void set_layer_thumbnails_enabled(bool state);

// Width (in real pixels, guiscale already applied by the caller's
// layerBoxHeight) reserved for the thumbnail column. Returns 0 when the
// feature is disabled, which makes every other column fall back to the
// stock layout.
int layer_thumbnail_width(int layerBoxHeight);

// Renders the thumbnail of `layer` at `frame` inside `bounds`. The caller is
// responsible for having painted the row background already. Does nothing for
// group layers without a cel at that frame.
void draw_layer_thumbnail(ui::Graphics* g,
                          const gfx::Rect& bounds,
                          const doc::Layer* layer,
                          const doc::frame_t frame);

// Draws the header toggle button glyph inside `bounds`, tinted with `color` so
// it follows the active theme. `enabled` picks the on/off frame. The caller
// paints the button background with a stock timeline style first.
//
// The glyph comes from data/mods/icons/layer_thumbnails.png: a 24x12 mask with
// two 12x12 frames (frame 0 = on, frame 1 = off), black on transparent. Drawing
// it ourselves rather than adding it to the theme's sheet.png keeps that binary
// asset (and theme.xml) untouched, so upstream theme updates never conflict.
void draw_layer_thumbnails_toggle(ui::Graphics* g,
                                  const gfx::Rect& bounds,
                                  bool enabled,
                                  gfx::Color color);

// Drops every cached surface. Call when the theme/color space changes.
void clear_layer_thumbnail_cache();

}} // namespace app::mods

#endif
