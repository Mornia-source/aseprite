// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// SHA-256, for verifying downloaded updates.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_UPDATER_SHA256_H_INCLUDED
#define APP_MODS_UPDATER_SHA256_H_INCLUDED
#pragma once

#include <string>

namespace app { namespace mods {

// Lowercase hex digest of `filename`, or "" if the file cannot be read.
//
// An update is code that will run on this machine, so this is not optional:
// nothing downloaded gets installed without matching the digest the server
// published.
std::string sha256_file(const std::string& filename);

// Case-insensitive comparison of two hex digests. False if either is not a
// well-formed 64-character digest, so a truncated or missing digest can never
// be mistaken for a match.
bool sha256_equal(const std::string& a, const std::string& b);

}} // namespace app::mods

#endif
