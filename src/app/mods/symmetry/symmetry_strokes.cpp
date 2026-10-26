// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/symmetry/symmetry_strokes.h"

#include "app/mods/symmetry/symmetry_math.h"
#include "app/mods/symmetry/symmetry_settings.h"
#include "app/tools/point_shape.h"
#include "app/tools/tool_loop.h"
#include "doc/brush.h"

namespace app { namespace mods {

namespace {

doc::SymmetryIndex to_index(const SymElem elem)
{
  switch (elem) {
    case SymElem::FlipX:      return doc::SymmetryIndex::FLIPPED_X;
    case SymElem::FlipY:      return doc::SymmetryIndex::FLIPPED_Y;
    case SymElem::FlipXY:     return doc::SymmetryIndex::FLIPPED_XY;
    case SymElem::RotFlip270: return doc::SymmetryIndex::ROT_FLIP_270;
    case SymElem::RotFlip90:  return doc::SymmetryIndex::ROT_FLIP_90;
    case SymElem::Rot270:     return doc::SymmetryIndex::ROTATED_270;
    case SymElem::Rot90:      return doc::SymmetryIndex::ROTATED_90;
    // Any other angle moves the brush but cannot turn its image.
    case SymElem::Identity:
    case SymElem::Other:
    default:                  return doc::SymmetryIndex::ORIGINAL;
  }
}

} // anonymous namespace

bool generate_symmetry_strokes(const tools::Stroke& stroke,
                               tools::Strokes& strokes,
                               tools::ToolLoop* loop,
                               const double centerX,
                               const double centerY)
{
  if (!symmetry_mods_active())
    return false;

  const std::vector<SymCopy> copies = symmetry_copies(current_symmetry_spec());

  // The brush as the built-in symmetry sees it: a flood fill has no footprint.
  gfx::Size brushSize(1, 1);
  gfx::Point brushCenter(0, 0);
  if (!loop->getPointShape()->isFloodFill()) {
    const doc::Brush* brush = loop->getBrush();
    brushSize = brush->bounds().size();
    brushCenter = brush->center();
  }
  const bool isDynamic = loop->getDynamics().isDynamic();

  strokes.push_back(stroke);

  for (const SymCopy& copy : copies) {
    const SymElem elem = classify(copy);
    const doc::SymmetryIndex index = to_index(elem);

    tools::Stroke copyStroke;
    for (const auto& pt : stroke) {
      gfx::Size size = brushSize;
      gfx::Point center = brushCenter;
      if (isDynamic) {
        size = gfx::Size(pt.size, pt.size);
        const int c = (size.w - size.w % 2) / 2;
        center = gfx::Point(c, c);
      }

      const SymPoint p =
        map_cursor(pt.x, pt.y, size.w, size.h, center.x, center.y, centerX, centerY, copy, elem);

      tools::Stroke::Pt pt2 = pt;
      pt2.x = p.x;
      pt2.y = p.y;
      pt2.symmetry = index;
      copyStroke.addPoint(pt2);
    }
    strokes.push_back(copyStroke);
  }
  return true;
}

}} // namespace app::mods
