// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/updater/update_download.h"

#include "base/fs.h"
#include "base/fstream_path.h"
#include "net/http_request.h"
#include "net/http_response.h"

#include "json11.hpp"

#include <cctype>
#include <fstream>
#include <sstream>

namespace app { namespace mods {

namespace {

// A streambuf that writes through to another one and counts the bytes, so the
// download can report progress without net-lib having to know about progress.
class CountingBuf : public std::streambuf {
public:
  CountingBuf(std::streambuf* out, int64_t total, Downloader* owner, const std::atomic<bool>* abort)
    : m_out(out)
    , m_total(total)
    , m_owner(owner)
    , m_abort(abort)
  {
  }

  int64_t count() const { return m_count; }
  bool aborted() const { return m_aborted; }

protected:
  std::streamsize xsputn(const char* s, std::streamsize n) override
  {
    if (m_abort->load()) {
      m_aborted = true;
      return 0; // Makes the stream fail, which stops curl.
    }

    const std::streamsize written = m_out->sputn(s, n);
    m_count += written;

    // One report per 64 KiB rather than per curl callback: the UI cannot use
    // more than that, and every report costs a cross-thread hop.
    if (m_count - m_lastReported >= 64 * 1024) {
      m_lastReported = m_count;
      m_owner->Progress(m_count, m_total);
    }
    return written;
  }

  int overflow(int c) override
  {
    if (c == traits_type::eof())
      return traits_type::not_eof(c);
    const char ch = (char)c;
    return xsputn(&ch, 1) == 1 ? c : traits_type::eof();
  }

private:
  std::streambuf* m_out;
  int64_t m_total;
  Downloader* m_owner;
  const std::atomic<bool>* m_abort;
  int64_t m_count = 0;
  int64_t m_lastReported = 0;
  bool m_aborted = false;
};

bool starts_with_ci(const std::string& s, const char* prefix)
{
  const size_t n = std::char_traits<char>::length(prefix);
  if (s.size() < n)
    return false;
  for (size_t i = 0; i < n; ++i) {
    if (std::tolower((unsigned char)s[i]) != std::tolower((unsigned char)prefix[i]))
      return false;
  }
  return true;
}

} // anonymous namespace

std::string manifest_url_for(const std::string& packageUrl)
{
  // Keep any query string out of the way: "a.zip?t=1" -> "a.zip.json?t=1".
  const size_t q = packageUrl.find('?');
  if (q == std::string::npos)
    return packageUrl + ".json";
  return packageUrl.substr(0, q) + ".json" + packageUrl.substr(q);
}

bool is_secure_update_url(const std::string& url)
{
  return starts_with_ci(url, "https://");
}

bool fetch_manifest(const std::string& packageUrl, UpdateManifest& manifest, std::string& error)
{
  const std::string url = manifest_url_for(packageUrl);
  if (!is_secure_update_url(url)) {
    error = "The update manifest is not served over https.";
    return false;
  }

  std::stringstream body;
  net::HttpRequest request(url);
  net::HttpResponse response(&body);
  if (!request.send(response)) {
    error = "Could not reach the update server.";
    return false;
  }
  if (response.status() != 200) {
    error = "The update server answered " + std::to_string(response.status()) + ".";
    return false;
  }

  // Skip a UTF-8 BOM: a JSON parser rejects one, and several editors and
  // PowerShell itself write one without asking.
  std::string text = body.str();
  if (text.compare(0, 3, "\xef\xbb\xbf") == 0)
    text.erase(0, 3);

  std::string jsonError;
  const json11::Json json = json11::Json::parse(text, jsonError);
  if (!jsonError.empty()) {
    error = "The update manifest is not valid JSON: " + jsonError;
    return false;
  }

  manifest.sha256 = json["sha256"].string_value();
  manifest.size = (int64_t)json["size"].number_value();
  manifest.version = json["version"].string_value();

  if (!manifest.valid()) {
    error = "The update manifest has no usable sha256 and size.";
    return false;
  }
  return true;
}

Downloader::~Downloader()
{
  abort();
}

bool Downloader::start(const std::string& url, const std::string& destFile, int64_t expectedSize)
{
  if (m_running)
    return false;

  m_abort = false;
  m_running = true;
  m_thread = std::thread([this, url, destFile, expectedSize] { run(url, destFile, expectedSize); });
  return true;
}

void Downloader::abort()
{
  m_abort = true;
  if (m_thread.joinable())
    m_thread.join();
}

void Downloader::run(std::string url, std::string destFile, int64_t expectedSize)
{
  std::string error;
  {
    std::ofstream out(FSTREAM_PATH(destFile), std::ios::binary);
    if (!out) {
      error = "Could not write to " + destFile;
    }
    else {
      CountingBuf counter(out.rdbuf(), expectedSize, this, &m_abort);
      std::ostream stream(&counter);

      net::HttpRequest request(url);
      net::HttpResponse response(&stream);
      const bool sent = request.send(response);
      stream.flush();

      if (counter.aborted())
        error = "Download cancelled.";
      else if (!sent)
        error = "The download was interrupted.";
      else if (response.status() != 200)
        error = "The update server answered " + std::to_string(response.status()) + ".";
      else if (expectedSize > 0 && counter.count() != expectedSize)
        error = "The download is incomplete.";
      else
        Progress(counter.count(), expectedSize);
    }
  }

  if (!error.empty() && base::is_file(destFile))
    base::delete_file(destFile);

  m_running = false;
  Done(error);
}

}} // namespace app::mods
