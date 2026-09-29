// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Help > File Association: open .ase/.aseprite with this build and show their
// canvas as File Explorer thumbnails.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context.h"
#include "app/i18n/strings.h"
#include "ui/alert.h"

#if LAF_WINDOWS
  #include "app/win/file_associations.h"
  #include "app/win/notify_shell.h"
  #include "app/win/thumbnails.h"
  #include "base/win/registry.h"
  #include "desktop/win/helpers.h"

  #include <shellapi.h>
#endif

namespace app {

#if LAF_WINDOWS

namespace {

using hkey = base::hkey;

// The ProgID the official installer and file_associations.cpp use, so the
// thumbnailer (which registers itself under it) and the thumbnail option in
// Preferences find what they expect, and an official install later simply
// takes it over.
constexpr const char* kProgId = "AsepriteFile";
constexpr const char* kClasses = "Software\\Classes\\";
const char* const kExtensions[] = { "ase", "aseprite" };

// Everything is per user (HKCU): no administrator rights, and nothing another
// account on this machine has to live with.
void register_types()
{
  // Upstream already knows how to register the type -- icon, open command and
  // DDE, so a double-click reaches a running instance instead of starting a
  // second one. It is what Preferences > File Types uses; we only call it for
  // both extensions at once.
  for (const char* ext : kExtensions)
    app::win::associate_file_type_with_asepritefile_class(ext);

  // Thumbnails with the icon overlay. Skipped if the DLL is not next to the
  // executable -- the association works without it.
  app::win::ThumbnailsOption thumbs;
  thumbs.enabled = true;
  thumbs.overlay = true;
  if (!app::win::get_thumbnailer_dll().empty())
    app::win::set_thumbnail_options("aseprite", thumbs);

  app::win::notify_shell_about_association_change_regen_thumbnails();
}

void unregister_types()
{
  hkey hkcu = hkey::current_user();

  for (const char* ext : kExtensions) {
    // hkey::open() throws for a key that does not exist, which here just
    // means there is nothing to undo.
    try {
      hkey k = hkcu.open(std::string(kClasses) + "." + ext, hkey::write);
      // Only undo what points at us: another program may have claimed the
      // extension since, and that is not ours to remove.
      if (k.exists("") && k.string("") == kProgId)
        k.delete_value("");
    }
    catch (const std::exception&) {
    }
  }

  desktop::win::unregister_thumbnailer();
  try {
    hkcu.delete_tree(std::string(kClasses) + kProgId);
  }
  catch (const std::exception&) {
    // Already gone.
  }

  app::win::notify_shell_about_association_change_regen_thumbnails();
}

// Windows keeps a per-user choice for each extension, protected by a hash that
// only the Settings app may write. If the user once picked another program for
// the extension, that choice wins over anything we register, and the honest
// fix is to send them to Settings rather than to forge the hash.
bool user_choice_overrides_us()
{
  for (const char* ext : kExtensions) {
    try {
      hkey k = hkey::current_user().open(
        std::string("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.") + ext +
          "\\UserChoice",
        hkey::read);
      if (k && k.exists("ProgId") && k.string("ProgId") != kProgId)
        return true;
    }
    catch (const std::exception&) {
      // No choice recorded for this extension.
    }
  }
  return false;
}

} // anonymous namespace

#endif // LAF_WINDOWS

class FileAssociationCommand : public Command {
public:
  FileAssociationCommand() : Command(CommandId::FileAssociation()) {}

protected:
  bool onEnabled(Context*) override
  {
#if LAF_WINDOWS
    return true;
#else
    return false;
#endif
  }

  void onExecute(Context*) override
  {
#if LAF_WINDOWS
    // 1 = associate, 2 = remove, anything else = cancelled.
    const int ret = ui::Alert::show(Strings::alerts_mods_file_association());
    try {
      if (ret == 1) {
        register_types();
        if (user_choice_overrides_us()) {
          if (ui::Alert::show(Strings::alerts_mods_file_association_user_choice()) == 1)
            ::ShellExecuteW(nullptr, L"open", L"ms-settings:defaultapps", nullptr, nullptr, SW_SHOWNORMAL);
        }
        else {
          ui::Alert::show(Strings::alerts_mods_file_association_done());
        }
      }
      else if (ret == 2) {
        unregister_types();
        ui::Alert::show(Strings::alerts_mods_file_association_removed());
      }
    }
    catch (const std::exception& e) {
      ui::Alert::show(Strings::alerts_mods_file_association_failed(e.what()));
    }
#endif
  }
};

Command* CommandFactory::createFileAssociationCommand()
{
  return new FileAssociationCommand();
}

} // namespace app
