// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Opacity for tools whose ink does not normally offer it (pencil, contour...).
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_TOOLS_TOOL_OPACITY_H_INCLUDED
#define APP_MODS_TOOLS_TOOL_OPACITY_H_INCLUDED
#pragma once

namespace app { namespace tools {
class Tool;
}} // namespace app::tools

namespace app { namespace mods {

// Whether the context bar should offer the opacity field for `tool`.
//
// Upstream only shows it when the selected ink type already honours opacity
// (alpha compositing / lock alpha) or when the ink is an effect. Tools left on
// the default SIMPLE ink -- which is every paint tool out of the box -- have no
// opacity control at all, so the pencil and the contour tool appear to have no
// opacity setting. Effects such as the eraser already work and are unaffected.
bool tool_offers_opacity(const app::tools::Tool* tool);

// Makes `tool` actually honour `opacity`.
//
// SIMPLE ink replaces pixels outright and ToolLoop forces opacity back to 255
// for it, so showing the field alone would give a control that does nothing.
// For plain paint tools this promotes SIMPLE to ALPHA_COMPOSITING, which is the
// ink the equivalent manual workflow uses.
//
// Only ever promotes: returning the value to 255 leaves the ink alone, so a
// deliberate ink choice is never silently undone.
void promote_ink_for_opacity(app::tools::Tool* tool, int opacity);

}} // namespace app::mods

#endif
