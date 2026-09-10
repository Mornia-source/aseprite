// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// The download/install dialog for our own update server.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_UI_UPDATE_DIALOG_H_INCLUDED
#define APP_MODS_UI_UPDATE_DIALOG_H_INCLUDED
#pragma once

#include "app/mods/updater/update_download.h"
#include "app/mods/updater/update_install.h"

#include "aseprite_update.xml.h"

#include <atomic>
#include <string>
#include <thread>

namespace app { namespace mods {

// Downloads an update, checks it against the digest the server published, and
// stages it; the OK button then hands over to the helper and closes the
// program. See docs/MODDING_NOTES.md §33.
//
// It borrows the official aseprite_update.xml layout, which is built but
// unreachable without the closed-source drm library, so this adds no widget of
// its own to keep in step with upstream.
class UpdateDialog : public app::gen::AsepriteUpdate {
public:
  UpdateDialog(const std::string& url, const std::string& version);
  ~UpdateDialog();

protected:
  void onBeforeClose(ui::CloseEvent& ev) override;

private:
  void log(std::string text);
  void fail(const std::string& error);
  void startDownload();
  void startInstall();

  std::string m_url;
  std::string m_version;
  UpdateManifest m_manifest;
  Downloader m_download;
  Installer m_install;
  std::thread m_manifestThread;
  std::atomic<bool> m_ready{ false }; // Staged and waiting for the OK button.
  bool m_closing = false;
};

}} // namespace app::mods

#endif
