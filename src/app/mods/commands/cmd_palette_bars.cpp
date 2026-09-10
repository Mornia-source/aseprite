// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// View > Palette Bars toggle.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context.h"
#include "app/mods/ui/palette_bars.h"
#include "app/ui/color_bar.h"

namespace app {

class ShowPaletteBarsCommand : public Command {
public:
  ShowPaletteBarsCommand() : Command(CommandId::ShowPaletteBars()) {}

protected:
  bool onChecked(Context* ctx) override { return mods::PaletteBars::enabled(); }

  void onExecute(Context* ctx) override
  {
    mods::PaletteBars::setEnabled(!mods::PaletteBars::enabled());

    // The widget lives in the color bar, which owns showing and hiding it.
    if (ColorBar* colorBar = ColorBar::instance())
      colorBar->updatePaletteBarsVisibility();
  }
};

Command* CommandFactory::createShowPaletteBarsCommand()
{
  return new ShowPaletteBarsCommand();
}

} // namespace app
