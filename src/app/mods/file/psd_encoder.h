// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
// Writing .psd files.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODS_FILE_PSD_ENCODER_H_INCLUDED
#define APP_MODS_FILE_PSD_ENCODER_H_INCLUDED
#pragma once

#include "doc/frame.h"

#include <cstdio>
#include <string>

namespace doc {
class Sprite;
}

namespace app { namespace mods {

// Writes `sprite` at `frame` as an 8-bit RGB Photoshop file.
//
// One frame only. Aseprite's layers and groups map onto PSD's, which is the
// part worth preserving; frames have no equivalent there, and turning them into
// groups would produce a file whose structure means nothing to Photoshop.
//
// Returns false and fills `error` when the sprite cannot be represented.
bool encode_psd(FILE* file, const doc::Sprite* sprite, doc::frame_t frame, std::string& error);

}} // namespace app::mods

#endif
