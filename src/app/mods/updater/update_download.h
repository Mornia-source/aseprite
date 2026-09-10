// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Fetching the update manifest and the package it points at.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_UPDATER_UPDATE_DOWNLOAD_H_INCLUDED
#define APP_MODS_UPDATER_UPDATE_DOWNLOAD_H_INCLUDED
#pragma once

#include "base/ints.h"
#include "obs/signal.h"

#include <atomic>
#include <string>
#include <thread>

namespace app { namespace mods {

// What the server says about the update, read from "<package url>.json":
//
//   { "sha256": "<64 hex chars>", "size": 17512345, "version": "1.3.18.5-99" }
//
// A sidecar file rather than extra attributes on the <update> XML, because the
// XML is parsed by src/updater/check_update.cpp -- an upstream file we would
// then have to keep patched at every merge.
struct UpdateManifest {
  std::string sha256;
  int64_t size = 0;
  std::string version;

  bool valid() const { return sha256.size() == 64 && size > 0; }
};

// The URL of the manifest for a package URL.
std::string manifest_url_for(const std::string& packageUrl);

// True if `url` is one we are willing to download code from. Only https: an
// update is arbitrary code executed on this machine, and plain http lets
// anyone on the path choose what that code is.
bool is_secure_update_url(const std::string& url);

// Downloads a URL on a background thread.
//
// Every signal is emitted from that thread; the UI has to hop back with
// ui::execute_from_ui_thread().
class Downloader {
public:
  Downloader() = default;
  ~Downloader();

  // `total` is 0 when the size is not known in advance.
  obs::signal<void(int64_t /*now*/, int64_t /*total*/)> Progress;
  // `error` is empty on success.
  obs::signal<void(const std::string& /*error*/)> Done;

  // Downloads `url` into `destFile`. `expectedSize` only drives the progress
  // report. Returns false if a download is already running.
  bool start(const std::string& url, const std::string& destFile, int64_t expectedSize);

  // Asks the download to stop and waits for the thread. Safe to call twice.
  void abort();

  bool running() const { return m_running; }

private:
  void run(std::string url, std::string destFile, int64_t expectedSize);

  std::thread m_thread;
  std::atomic<bool> m_running{ false };
  std::atomic<bool> m_abort{ false };
};

// Fetches and parses the manifest. Blocking; call it off the UI thread.
// Returns false and fills `error` on any problem.
bool fetch_manifest(const std::string& packageUrl, UpdateManifest& manifest, std::string& error);

}} // namespace app::mods

#endif
