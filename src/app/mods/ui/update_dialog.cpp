// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/ui/update_dialog.h"

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/context.h"
#include "app/mods/updater/sha256.h"
#include "base/fs.h"
#include "base/string.h"
#include "ver/info.h"
#include "ui/label.h"
#include "ui/system.h"

namespace app { namespace mods {

namespace {

std::string megabytes(int64_t bytes)
{
  return base::string_printf("%.1f MB", double(bytes) / (1024.0 * 1024.0));
}

} // anonymous namespace

UpdateDialog::UpdateDialog(const std::string& url, const std::string& version)
  : m_url(url)
  , m_version(version)
{
  okButton()->setText("Install and restart");
  okButton()->setEnabled(false);
  okButton()->Click.connect([this] {
    if (!m_ready)
      return;

    std::string error;
    if (!m_install.apply(error)) {
      log(error);
      return;
    }

    // The helper is now waiting for this process to go away. Exit through the
    // normal command so unsaved sprites still get their prompt; if the user
    // backs out there, the helper gives up on its own.
    log("Installing. " + std::string(get_app_name()) + " will restart.");
    closeWindow(this);
    if (auto* ctx = App::instance()->context())
      ctx->executeCommand(Commands::instance()->byId(CommandId::Exit()));
  });

  log(base::string_printf("Update %s is available.", version.c_str()));

  if (!is_secure_update_url(url)) {
    fail("The update is not served over https, so it will not be downloaded.");
    return;
  }

  log("Asking the server what to expect...");
  m_manifestThread = std::thread([this] {
    UpdateManifest manifest;
    std::string error;
    const bool ok = fetch_manifest(m_url, manifest, error);
    ui::execute_from_ui_thread([this, ok, manifest, error] {
      if (!ok) {
        fail(error);
        return;
      }
      m_manifest = manifest;
      log("Package is " + megabytes(m_manifest.size) + ".");
      startDownload();
    });
  });
}

UpdateDialog::~UpdateDialog()
{
  m_download.abort();
  m_install.abort();
  if (m_manifestThread.joinable())
    m_manifestThread.join();
}

void UpdateDialog::startDownload()
{
  const std::string file = update_package_file();
  base::make_all_directories(update_work_dir());

  m_download.Progress.connect([this](int64_t now, int64_t total) {
    ui::execute_from_ui_thread([this, now, total] {
      if (m_closing)
        return;
      if (total > 0)
        progress()->setValue(int(now * 100 / total));
    });
  });

  m_download.Done.connect([this](const std::string& error) {
    ui::execute_from_ui_thread([this, error] {
      if (m_closing)
        return;
      if (!error.empty()) {
        fail(error);
        return;
      }
      log("Downloaded.");
      startInstall();
    });
  });

  log("Downloading...");
  progress()->setValue(0);
  if (!m_download.start(m_url, file, m_manifest.size))
    fail("A download is already running.");
}

void UpdateDialog::startInstall()
{
  m_install.Step.connect([this](const std::string& text) {
    ui::execute_from_ui_thread([this, text] {
      if (!m_closing)
        log(text);
    });
  });

  m_install.Progress.connect([this](int pct) {
    ui::execute_from_ui_thread([this, pct] {
      if (!m_closing)
        progress()->setValue(pct);
    });
  });

  m_install.Done.connect([this](const std::string& error) {
    ui::execute_from_ui_thread([this, error] {
      if (m_closing)
        return;
      if (!error.empty()) {
        fail(error);
        return;
      }
      m_ready = true;
      okButton()->setEnabled(true);
      okButton()->requestFocus();
      log("The update is ready. It will be applied after the program closes.");
    });
  });

  m_install.start(update_package_file(), m_manifest);
}

void UpdateDialog::fail(const std::string& error)
{
  log(error.empty() ? std::string("The update failed.") : error);
  log("Nothing was changed.");
  progress()->setValue(0);
  okButton()->setEnabled(false);
}

void UpdateDialog::onBeforeClose(ui::CloseEvent& ev)
{
  m_closing = true;
  m_download.abort();
  m_install.abort();
}

void UpdateDialog::log(std::string text)
{
  if (text.empty())
    return;
  logitems()->addChild(new ui::Label(text));
  layout();
}

}} // namespace app::mods
