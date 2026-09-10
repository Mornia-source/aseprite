// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/updater/update_install.h"

#include "app/mods/updater/sha256.h"
#include "base/exception.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "base/fstream_path.h"
#include "base/process.h"
#include "base/replace_string.h"
#include "base/string.h"

#include "archive.h"
#include "archive_entry.h"

#include <algorithm>
#include <fstream>

#if LAF_WINDOWS
  #include <windows.h>
#endif

namespace app { namespace mods {

std::string update_work_dir()
{
  return base::join_path(base::get_temp_path(), "aseprite-mods-update");
}

std::string update_package_file()
{
  return base::join_path(update_work_dir(), "package.zip");
}

namespace {

std::string staged_dir()
{
  return base::join_path(update_work_dir(), "staged");
}

// True for an entry name we are willing to write. The digest only proves the
// package is the one the server meant to publish; it says nothing about
// whether the contents are sane, so the traversal check stays.
bool safe_entry_name(const std::string& name)
{
  if (name.empty())
    return false;
  if (name.find(':') != std::string::npos) // An absolute path or an NTFS stream.
    return false;
  if (name[0] == '/' || name[0] == '\\')
    return false;

  std::string n = name;
  base::replace_string(n, "\\", "/");
  size_t start = 0;
  for (;;) {
    const size_t end = n.find('/', start);
    const size_t stop = (end == std::string::npos ? n.size() : end);
    if (n.substr(start, stop - start) == "..")
      return false;
    if (end == std::string::npos)
      break;
    start = end + 1;
  }
  return true;
}

void remove_tree(const std::string& dir)
{
  if (!base::is_directory(dir))
    return;

  for (const auto& item : base::list_files(dir)) {
    const std::string full = base::join_path(dir, item);
    if (base::is_directory(full))
      remove_tree(full);
    else
      base::delete_file(full);
  }
  base::remove_directory(dir);
}

// The directory inside `root` that actually holds aseprite.exe. Our packages
// have a single top-level folder, but do not depend on its name.
std::string find_program_dir(const std::string& root)
{
  if (base::is_file(base::join_path(root, "aseprite.exe")))
    return root;

  for (const auto& item : base::list_files(root)) {
    const std::string full = base::join_path(root, item);
    if (base::is_directory(full) && base::is_file(base::join_path(full, "aseprite.exe")))
      return full;
  }
  return std::string();
}

// Quotes a path for a batch file. Batch has no escape for a quote inside a
// quoted string, so a path containing one cannot be handled, and has to be
// refused rather than silently producing a script that means something else.
bool batch_quote(const std::string& path, std::string& out)
{
  if (path.find('"') != std::string::npos || path.find('%') != std::string::npos)
    return false;
  out = "\"" + path + "\"";
  return true;
}

} // anonymous namespace

Installer::~Installer()
{
  abort();
}

bool Installer::start(const std::string& packageFile, const UpdateManifest& manifest)
{
  if (m_running)
    return false;

  m_abort = false;
  m_running = true;
  m_thread = std::thread([this, packageFile, manifest] { run(packageFile, manifest); });
  return true;
}

void Installer::abort()
{
  m_abort = true;
  if (m_thread.joinable())
    m_thread.join();
}

void Installer::run(std::string packageFile, UpdateManifest manifest)
{
  std::string error;

  try {
    Step("Verifying the download...");
    Progress(0);

    const std::string digest = sha256_file(packageFile);
    if (digest.empty())
      throw base::Exception("Could not compute the checksum of the download.");
    if (!sha256_equal(digest, manifest.sha256))
      throw base::Exception("The download does not match the checksum the server published, "
                            "so it will not be installed.");

    if (m_abort)
      throw base::Exception("Cancelled.");

    Step("Unpacking...");
    const std::string dest = staged_dir();
    remove_tree(dest);
    base::make_all_directories(dest);

    archive* in = archive_read_new();
    archive_read_support_format_zip(in);
    base::FileHandle file(base::open_file(packageFile, "rb"));
    if (!file || archive_read_open_FILE(in, file.get()) != ARCHIVE_OK) {
      archive_read_free(in);
      throw base::Exception("Could not open the downloaded package.");
    }

    archive* out = archive_write_disk_new();
    int64_t written = 0;
    std::string archError;

    for (;;) {
      archive_entry* entry = nullptr;
      const int err = archive_read_next_header(in, &entry);
      if (err == ARCHIVE_EOF)
        break;
      if (err != ARCHIVE_OK) {
        archError = "The package is damaged.";
        break;
      }
      if (m_abort) {
        archError = "Cancelled.";
        break;
      }

      const char* name = archive_entry_pathname(entry);
      if (!name || !safe_entry_name(name)) {
        archError = "The package contains an unsafe path.";
        break;
      }

      const std::string full = base::join_path(dest, name);
      archive_entry_set_pathname(entry, full.c_str());

      if (archive_write_header(out, entry) != ARCHIVE_OK) {
        archError = "Could not write the unpacked files.";
        break;
      }

      for (;;) {
        const void* buf;
        size_t size;
        int64_t offset;
        const int derr = archive_read_data_block(in, &buf, &size, &offset);
        if (derr == ARCHIVE_EOF)
          break;
        if (derr != ARCHIVE_OK || archive_write_data_block(out, buf, size, offset) != ARCHIVE_OK) {
          archError = "The package is damaged.";
          break;
        }
        written += (int64_t)size;
        if (manifest.size > 0) {
          // The unpacked size is bigger than the package, so this is a rough
          // ratio rather than a real percentage; cap it so it never goes back.
          Progress((int)std::min<int64_t>(99, written * 100 / (manifest.size * 3)));
        }
      }
      if (!archError.empty())
        break;
      archive_write_finish_entry(out);
    }

    archive_write_close(out);
    archive_write_free(out);
    archive_read_close(in);
    archive_read_free(in);

    if (!archError.empty())
      throw base::Exception("%s", archError.c_str());

    if (find_program_dir(dest).empty())
      throw base::Exception("The package does not contain aseprite.exe.");

    m_stagedDir = dest;
    Progress(100);
    Step("Ready to install.");
  }
  catch (const std::exception& e) {
    error = e.what();
    remove_tree(staged_dir());
    m_stagedDir.clear();
  }

  m_running = false;
  Done(error);
}

bool Installer::apply(std::string& error)
{
#if LAF_WINDOWS
  if (m_stagedDir.empty()) {
    error = "There is nothing staged to install.";
    return false;
  }

  const std::string source = find_program_dir(m_stagedDir);
  const std::string target = base::get_file_path(base::get_app_path());
  const std::string backup = base::join_path(update_work_dir(), "backup");
  const std::string script = base::join_path(update_work_dir(), "apply-update.cmd");

  std::string qSource, qTarget, qBackup, qExe, qWork;
  if (!batch_quote(source, qSource) || !batch_quote(target, qTarget) ||
      !batch_quote(backup, qBackup) || !batch_quote(base::get_app_path(), qExe) ||
      !batch_quote(update_work_dir(), qWork)) {
    error = "The install path contains characters this updater cannot handle.";
    return false;
  }

  const std::string pid = std::to_string((unsigned)base::get_current_process_id());

  // Windows will not let a running executable be replaced, so the swap has to
  // happen after we exit and therefore lives in a script:
  //
  //   1. wait for this process to go away
  //   2. keep a copy of the old build
  //   3. copy the new one over it
  //   4. if aseprite.exe did not survive that, put the old build back
  //   5. restart, then delete the work directory (including this script)
  //
  // robocopy reports success with any exit code below 8, which is why the
  // tests read "errorlevel 8" rather than "errorlevel 1".
  std::string cmd;
  cmd += "@echo off\r\n";
  cmd += "setlocal enabledelayedexpansion\r\n";
  cmd += "set /a tries=0\r\n";
  cmd += ":waitloop\r\n";
  cmd += "tasklist /fi \"PID eq " + pid + "\" /nh | find \"" + pid + "\" >nul\r\n";
  cmd += "if not errorlevel 1 (\r\n";
  cmd += "  set /a tries+=1\r\n";
  // The user can still cancel the exit (an unsaved sprite, say). Give up rather
  // than spin forever: about ten minutes, then clean up and leave the build
  // that is installed alone.
  cmd += "  if !tries! GTR 600 goto cleanup\r\n";
  cmd += "  ping -n 2 127.0.0.1 >nul\r\n";
  cmd += "  goto waitloop\r\n";
  cmd += ")\r\n";
  cmd += "robocopy " + qTarget + " " + qBackup + " /E /NFL /NDL /NJH /NJS /R:2 /W:1 >nul\r\n";
  cmd += "if errorlevel 8 goto failed\r\n";
  cmd += "robocopy " + qSource + " " + qTarget +
         " /E /IS /IT /NFL /NDL /NJH /NJS /R:3 /W:1 >nul\r\n";
  cmd += "if errorlevel 8 goto rollback\r\n";
  cmd += "if not exist " + qExe + " goto rollback\r\n";
  cmd += "start \"\" " + qExe + "\r\n";
  cmd += "goto cleanup\r\n";
  cmd += ":rollback\r\n";
  cmd += "robocopy " + qBackup + " " + qTarget +
         " /E /IS /IT /NFL /NDL /NJH /NJS /R:3 /W:1 >nul\r\n";
  cmd += "start \"\" " + qExe + "\r\n";
  cmd += ":failed\r\n";
  cmd += ":cleanup\r\n";
  // Detached, so it can delete the script that is running it.
  cmd += "start \"\" /min cmd /c \"ping -n 3 127.0.0.1 >nul & rmdir /s /q " + qWork + "\"\r\n";

  {
    std::ofstream file(FSTREAM_PATH(script), std::ios::binary);
    if (!file) {
      error = "Could not write the update helper.";
      return false;
    }
    file << cmd;
  }

  const std::wstring wScript = base::from_utf8(script);
  const std::wstring wWork = base::from_utf8(update_work_dir());
  const HINSTANCE result =
    ::ShellExecuteW(nullptr, L"open", wScript.c_str(), nullptr, wWork.c_str(), SW_HIDE);
  if ((INT_PTR)result <= 32) {
    error = "Could not start the update helper.";
    return false;
  }
  return true;
#else
  error = "Applying updates is only implemented on Windows.";
  return false;
#endif
}

}} // namespace app::mods
