// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/updater/sha256.h"

#include "base/file_handle.h"
#include "base/ints.h"

#include <cctype>
#include <cstdio>
#include <vector>

#if LAF_WINDOWS
  #include <windows.h>

  #include <bcrypt.h>
#endif

namespace app { namespace mods {

namespace {

const char kHex[] = "0123456789abcdef";

#if LAF_WINDOWS

// A BCrypt hash, closed in the right order however we leave the function.
class Hasher {
public:
  Hasher()
  {
    if (::BCryptOpenAlgorithmProvider(&m_alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
      m_alg = nullptr;
    else if (::BCryptCreateHash(m_alg, &m_hash, nullptr, 0, nullptr, 0, 0) < 0)
      m_hash = nullptr;
  }

  ~Hasher()
  {
    if (m_hash)
      ::BCryptDestroyHash(m_hash);
    if (m_alg)
      ::BCryptCloseAlgorithmProvider(m_alg, 0);
  }

  bool ok() const { return m_hash != nullptr; }

  bool update(const uint8_t* data, size_t size)
  {
    return ::BCryptHashData(m_hash, const_cast<PUCHAR>(data), (ULONG)size, 0) >= 0;
  }

  bool finish(uint8_t digest[32])
  {
    return ::BCryptFinishHash(m_hash, digest, 32, 0) >= 0;
  }

private:
  BCRYPT_ALG_HANDLE m_alg = nullptr;
  BCRYPT_HASH_HANDLE m_hash = nullptr;
};

#endif // LAF_WINDOWS

} // anonymous namespace

std::string sha256_file(const std::string& filename)
{
#if LAF_WINDOWS
  Hasher hasher;
  if (!hasher.ok())
    return std::string();

  base::FileHandle file(base::open_file(filename, "rb"));
  if (!file)
    return std::string();

  std::vector<uint8_t> buf(64 * 1024);
  while (true) {
    const size_t n = std::fread(&buf[0], 1, buf.size(), file.get());
    if (n == 0)
      break;
    if (!hasher.update(&buf[0], n))
      return std::string();
  }
  if (std::ferror(file.get()))
    return std::string();

  uint8_t digest[32];
  if (!hasher.finish(digest))
    return std::string();

  std::string result;
  result.reserve(64);
  for (int i = 0; i < 32; ++i) {
    result.push_back(kHex[digest[i] >> 4]);
    result.push_back(kHex[digest[i] & 0x0f]);
  }
  return result;
#else
  // No digest means no install: see sha256_equal().
  return std::string();
#endif
}

bool sha256_equal(const std::string& a, const std::string& b)
{
  if (a.size() != 64 || b.size() != 64)
    return false;

  for (size_t i = 0; i < 64; ++i) {
    if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i]))
      return false;
  }
  return true;
}

}} // namespace app::mods
