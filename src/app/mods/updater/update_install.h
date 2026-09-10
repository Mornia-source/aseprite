// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Verifying, unpacking and applying a downloaded update.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_UPDATER_UPDATE_INSTALL_H_INCLUDED
#define APP_MODS_UPDATER_UPDATE_INSTALL_H_INCLUDED
#pragma once

#include "app/mods/updater/update_download.h"

#include "obs/signal.h"

#include <atomic>
#include <string>
#include <thread>

namespace app { namespace mods {

// The one directory every part of an update lives in, so a failed or cancelled
// update leaves exactly one thing to clean up.
std::string update_work_dir();

// Where the downloaded package is put.
std::string update_package_file();

// Turns a downloaded package into an installed one.
//
// The last step cannot happen while we are running -- Windows will not let a
// running executable be replaced -- so applying the update means handing the
// job to a small script that waits for us to exit, swaps the files, and starts
// the new build. See docs/MODDING_NOTES.md §33.
class Installer {
public:
  ~Installer();

  // Human-readable step, for the log in the dialog.
  obs::signal<void(const std::string&)> Step;
  obs::signal<void(int /*percent*/)> Progress;
  // `error` is empty when the update is staged and ready to apply.
  obs::signal<void(const std::string& /*error*/)> Done;

  // Verifies `packageFile` against `manifest` and unpacks it. Runs on a
  // background thread. Returns false if it is already running.
  bool start(const std::string& packageFile, const UpdateManifest& manifest);

  void abort();

  // Writes and launches the helper, then returns true. The caller is expected
  // to close the program immediately afterwards. Only valid after Done("").
  bool apply(std::string& error);

private:
  void run(std::string packageFile, UpdateManifest manifest);

  std::thread m_thread;
  std::atomic<bool> m_running{ false };
  std::atomic<bool> m_abort{ false };
  std::string m_stagedDir; // Holds the new build, ready to copy over ours.
};

}} // namespace app::mods

#endif
