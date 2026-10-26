// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/symmetry/symmetry_settings.h"

#include "app/ini_file.h"

#include <algorithm>

namespace app { namespace mods {

namespace {

constexpr const char* kSection = "mods_symmetry";

bool g_loaded = false;
SymmetrySettings g_settings;

void load()
{
  g_loaded = true;

  const int mode = get_config_int(kSection, "mode", int(SymMode::Official));
  g_settings.mode = (mode >= int(SymMode::Official) && mode <= int(SymMode::Spiral)) ?
                      SymMode(mode) :
                      SymMode::Official;
  g_settings.count = std::clamp(get_config_int(kSection, "count", 6), kMinSymCount, kMaxSymCount);
  g_settings.mandalaAngle = std::clamp(get_config_int(kSection, "mandala_angle", 0), -180, 180);
  g_settings.spiralAngle = std::clamp(get_config_int(kSection, "spiral_angle", 30), -180, 180);
}

} // anonymous namespace

SymmetrySettings& symmetry_settings()
{
  if (!g_loaded)
    load();
  return g_settings;
}

void save_symmetry_settings()
{
  const SymmetrySettings& s = symmetry_settings();
  set_config_int(kSection, "mode", int(s.mode));
  set_config_int(kSection, "count", s.count);
  set_config_int(kSection, "mandala_angle", s.mandalaAngle);
  set_config_int(kSection, "spiral_angle", s.spiralAngle);
}

bool symmetry_mods_active()
{
  return symmetry_settings().mode != SymMode::Official &&
         Preferences::instance().symmetryMode.enabled();
}

SymSpec current_symmetry_spec()
{
  const SymmetrySettings& s = symmetry_settings();
  SymSpec spec;
  spec.count = std::clamp(s.count, kMinSymCount, kMaxSymCount);
  switch (s.mode) {
    case SymMode::Center:
      spec.kind = SymKind::Center;
      break;
    case SymMode::Mandala:
      spec.kind = SymKind::Mandala;
      spec.angle = s.mandalaAngle;
      break;
    case SymMode::Spiral:
      spec.kind = SymKind::Spiral;
      spec.angle = s.spiralAngle;
      break;
    case SymMode::Radial:
    case SymMode::Official:
    default:
      spec.kind = SymKind::Radial;
      break;
  }
  return spec;
}

gen::SymmetryMode symmetry_mode_for_tools(const gen::SymmetryMode docMode)
{
  return symmetry_mods_active() ? gen::SymmetryMode::BOTH : docMode;
}

gen::SymmetryMode symmetry_mode_for_guides(const gen::SymmetryMode docMode)
{
  return symmetry_mods_active() ? gen::SymmetryMode::NONE : docMode;
}

}} // namespace app::mods
