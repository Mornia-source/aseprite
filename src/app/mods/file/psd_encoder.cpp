// Aseprite - private fork modification
//
// Part of the "mods" module: see docs/MODDING_NOTES.md
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/mods/file/psd_encoder.h"

#include "doc/blend_mode.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "render/render.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace app { namespace mods {

namespace {

using Bytes = std::vector<uint8_t>;

void put8(Bytes& out, uint8_t v)
{
  out.push_back(v);
}

void put16(Bytes& out, uint16_t v)
{
  out.push_back(uint8_t(v >> 8));
  out.push_back(uint8_t(v));
}

void put32(Bytes& out, uint32_t v)
{
  out.push_back(uint8_t(v >> 24));
  out.push_back(uint8_t(v >> 16));
  out.push_back(uint8_t(v >> 8));
  out.push_back(uint8_t(v));
}

void putRaw(Bytes& out, const void* data, size_t size)
{
  const uint8_t* p = static_cast<const uint8_t*>(data);
  out.insert(out.end(), p, p + size);
}

// PackBits, the run-length encoding PSD uses for its RLE channels. Runs of a
// repeated byte become a count and the byte; anything else is copied verbatim.
Bytes pack_bits(const uint8_t* src, size_t size)
{
  Bytes out;
  size_t i = 0;

  while (i < size) {
    // How long is the run starting here?
    size_t run = 1;
    while (i + run < size && run < 128 && src[i + run] == src[i])
      ++run;

    if (run >= 2) {
      out.push_back(uint8_t(257 - run)); // -(run-1) as a signed byte
      out.push_back(src[i]);
      i += run;
      continue;
    }

    // No run: gather literals up to the next run of three or more, which is
    // where encoding them as a run starts to pay off.
    size_t lit = 0;
    while (i + lit < size && lit < 128) {
      if (i + lit + 2 < size && src[i + lit] == src[i + lit + 1] &&
          src[i + lit] == src[i + lit + 2]) {
        break;
      }
      ++lit;
    }

    out.push_back(uint8_t(lit - 1));
    out.insert(out.end(), src + i, src + i + lit);
    i += lit;
  }
  return out;
}

// A Pascal string padded so its total length is a multiple of `padTo`.
Bytes pascal_string(const std::string& s, int padTo)
{
  Bytes out;
  const size_t len = (s.size() > 255 ? 255 : s.size());
  out.push_back(uint8_t(len));
  putRaw(out, s.data(), len);
  while (out.size() % padTo != 0)
    out.push_back(0);
  return out;
}

// The "luni" block, holding the layer name as UTF-16BE.
//
// The Pascal string above is in an unspecified codepage, so a non-ASCII name
// only survives a round trip through this block -- which is exactly the one the
// importer reads (see the decoder).
Bytes luni_block(const std::string& utf8)
{
  // UTF-8 to UTF-16BE.
  std::vector<uint16_t> utf16;
  for (size_t i = 0; i < utf8.size();) {
    const unsigned char c = uint8_t(utf8[i]);
    uint32_t cp;
    int extra;

    if (c < 0x80) {
      cp = c;
      extra = 0;
    }
    else if ((c & 0xE0) == 0xC0) {
      cp = c & 0x1F;
      extra = 1;
    }
    else if ((c & 0xF0) == 0xE0) {
      cp = c & 0x0F;
      extra = 2;
    }
    else if ((c & 0xF8) == 0xF0) {
      cp = c & 0x07;
      extra = 3;
    }
    else {
      cp = 0xFFFD;
      extra = 0;
      ++i;
      utf16.push_back(uint16_t(cp));
      continue;
    }

    if (i + extra >= utf8.size()) {
      utf16.push_back(0xFFFD);
      break;
    }
    for (int k = 1; k <= extra; ++k)
      cp = (cp << 6) | (uint8_t(utf8[i + k]) & 0x3F);
    i += extra + 1;

    if (cp >= 0x10000) {
      cp -= 0x10000;
      utf16.push_back(uint16_t(0xD800 + (cp >> 10)));
      utf16.push_back(uint16_t(0xDC00 + (cp & 0x3FF)));
    }
    else {
      utf16.push_back(uint16_t(cp));
    }
  }

  Bytes data;
  put32(data, uint32_t(utf16.size()));
  for (uint16_t u : utf16)
    put16(data, u);

  Bytes out;
  putRaw(out, "8BIM", 4);
  putRaw(out, "luni", 4);
  put32(out, uint32_t(data.size()));
  out.insert(out.end(), data.begin(), data.end());
  if (out.size() % 2)
    out.push_back(0);
  return out;
}

// A section divider block, which is how PSD marks a group.
Bytes lsct_block(uint32_t type)
{
  Bytes out;
  putRaw(out, "8BIM", 4);
  putRaw(out, "lsct", 4);
  put32(out, 4);
  put32(out, type);
  return out;
}

const char* blend_mode_key(doc::BlendMode mode)
{
  switch (mode) {
    case doc::BlendMode::MULTIPLY:       return "mul ";
    case doc::BlendMode::SCREEN:         return "scrn";
    case doc::BlendMode::OVERLAY:        return "over";
    case doc::BlendMode::DARKEN:         return "dark";
    case doc::BlendMode::LIGHTEN:        return "lite";
    case doc::BlendMode::COLOR_DODGE:    return "div ";
    case doc::BlendMode::COLOR_BURN:     return "idiv";
    case doc::BlendMode::HARD_LIGHT:     return "hLit";
    case doc::BlendMode::SOFT_LIGHT:     return "sLit";
    case doc::BlendMode::DIFFERENCE:     return "diff";
    case doc::BlendMode::EXCLUSION:      return "smud";
    case doc::BlendMode::HSL_HUE:        return "hue ";
    case doc::BlendMode::HSL_SATURATION: return "sat ";
    case doc::BlendMode::HSL_COLOR:      return "colr";
    case doc::BlendMode::HSL_LUMINOSITY: return "lum ";
    case doc::BlendMode::ADDITION:       return "lddg";
    case doc::BlendMode::SUBTRACT:       return "fsub";
    case doc::BlendMode::DIVIDE:         return "fdiv";
    default:                             return "norm";
  }
}

// One PSD layer record plus the channel data that belongs with it.
struct Record {
  Bytes header;
  Bytes channels;
};

// Reads a pixel of any sprite color mode as straight RGBA.
void get_rgba(const doc::Image* img,
              const doc::Palette* pal,
              doc::color_t transparentIndex,
              int x,
              int y,
              uint8_t rgba[4])
{
  const doc::color_t px = doc::get_pixel(img, x, y);

  switch (img->pixelFormat()) {
    case doc::IMAGE_RGB:
      rgba[0] = doc::rgba_getr(px);
      rgba[1] = doc::rgba_getg(px);
      rgba[2] = doc::rgba_getb(px);
      rgba[3] = doc::rgba_geta(px);
      break;

    case doc::IMAGE_GRAYSCALE: {
      const uint8_t v = doc::graya_getv(px);
      rgba[0] = rgba[1] = rgba[2] = v;
      rgba[3] = doc::graya_geta(px);
      break;
    }

    case doc::IMAGE_INDEXED: {
      if (px == transparentIndex || !pal || int(px) >= pal->size()) {
        rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0;
        break;
      }
      const doc::color_t c = pal->getEntry(px);
      rgba[0] = doc::rgba_getr(c);
      rgba[1] = doc::rgba_getg(c);
      rgba[2] = doc::rgba_getb(c);
      rgba[3] = doc::rgba_geta(c);
      break;
    }

    default:
      rgba[0] = rgba[1] = rgba[2] = 0;
      rgba[3] = 0;
      break;
  }
}

// Encodes one channel of an image as RLE, returning the per-scanline byte
// counts alongside the data. PSD stores those counts up front.
void encode_channel_rle(const std::vector<uint8_t>& plane,
                        int width,
                        int height,
                        Bytes& outCounts,
                        Bytes& outData)
{
  for (int y = 0; y < height; ++y) {
    Bytes row = pack_bits(plane.data() + size_t(y) * width, width);
    put16(outCounts, uint16_t(row.size()));
    outData.insert(outData.end(), row.begin(), row.end());
  }
}

// The four channel planes of `img`, in R G B A order.
std::vector<std::vector<uint8_t>> split_planes(const doc::Image* img,
                                               const doc::Palette* pal,
                                               doc::color_t transparentIndex)
{
  const int w = img->width();
  const int h = img->height();
  std::vector<std::vector<uint8_t>> planes(4, std::vector<uint8_t>(size_t(w) * h));

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      uint8_t c[4];
      get_rgba(img, pal, transparentIndex, x, y, c);
      const size_t i = size_t(y) * w + x;
      for (int k = 0; k < 4; ++k)
        planes[k][i] = c[k];
    }
  }
  return planes;
}

// Builds the channel data for a layer, RLE compressed, and the (id, length)
// pairs the layer record needs.
void build_channels(const doc::Image* img,
                    const doc::Palette* pal,
                    doc::color_t transparentIndex,
                    Bytes& outChannelData,
                    std::vector<std::pair<int16_t, uint32_t>>& outInfo)
{
  static const int16_t kIds[4] = { 0, 1, 2, -1 }; // R G B, and -1 for alpha

  if (!img) {
    // An empty layer still needs four channels, each an empty RLE stream.
    for (int k = 0; k < 4; ++k) {
      Bytes ch;
      put16(ch, 1); // RLE
      outInfo.emplace_back(kIds[k], uint32_t(ch.size()));
      outChannelData.insert(outChannelData.end(), ch.begin(), ch.end());
    }
    return;
  }

  auto planes = split_planes(img, pal, transparentIndex);

  for (int k = 0; k < 4; ++k) {
    Bytes counts, data;
    encode_channel_rle(planes[k], img->width(), img->height(), counts, data);

    Bytes ch;
    put16(ch, 1); // RLE
    ch.insert(ch.end(), counts.begin(), counts.end());
    ch.insert(ch.end(), data.begin(), data.end());

    outInfo.emplace_back(kIds[k], uint32_t(ch.size()));
    outChannelData.insert(outChannelData.end(), ch.begin(), ch.end());
  }
}

// Assembles one layer record.
Record make_record(const std::string& name,
                   const gfx::Rect& bounds,
                   doc::BlendMode blendMode,
                   int opacity,
                   bool visible,
                   const Bytes& extraBlocks,
                   const doc::Image* img,
                   const doc::Palette* pal,
                   doc::color_t transparentIndex)
{
  Record rec;

  std::vector<std::pair<int16_t, uint32_t>> info;
  build_channels(img, pal, transparentIndex, rec.channels, info);

  put32(rec.header, uint32_t(bounds.y));
  put32(rec.header, uint32_t(bounds.x));
  put32(rec.header, uint32_t(bounds.y + bounds.h));
  put32(rec.header, uint32_t(bounds.x + bounds.w));
  put16(rec.header, uint16_t(info.size()));
  for (const auto& ch : info) {
    put16(rec.header, uint16_t(ch.first));
    put32(rec.header, ch.second);
  }

  putRaw(rec.header, "8BIM", 4);
  putRaw(rec.header, blend_mode_key(blendMode), 4);
  put8(rec.header, uint8_t(opacity));
  put8(rec.header, 0);                    // clipping
  put8(rec.header, visible ? 0x00 : 0x02); // bit 1 means hidden
  put8(rec.header, 0);                    // filler

  Bytes extra;
  put32(extra, 0); // layer mask data
  put32(extra, 0); // blending ranges
  Bytes pname = pascal_string(name, 4);
  extra.insert(extra.end(), pname.begin(), pname.end());
  extra.insert(extra.end(), extraBlocks.begin(), extraBlocks.end());

  put32(rec.header, uint32_t(extra.size()));
  rec.header.insert(rec.header.end(), extra.begin(), extra.end());
  return rec;
}

// Walks the layer tree, emitting records bottom-to-top the way PSD stores them.
//
// A group becomes a bounding-section divider, then its children, then the named
// folder record -- the same shape the importer expects to read back.
void emit_layers(const doc::LayerGroup* group,
                 doc::frame_t frame,
                 const doc::Palette* pal,
                 doc::color_t transparentIndex,
                 std::vector<Record>& out)
{
  for (const doc::Layer* layer : group->layers()) {
    if (layer->isGroup()) {
      const auto* sub = static_cast<const doc::LayerGroup*>(layer);

      out.push_back(make_record("</Layer group>",
                                gfx::Rect(0, 0, 0, 0),
                                doc::BlendMode::NORMAL,
                                255,
                                true,
                                lsct_block(3),
                                nullptr,
                                pal,
                                transparentIndex));

      emit_layers(sub, frame, pal, transparentIndex, out);

      Bytes blocks = lsct_block(sub->isCollapsed() ? 2 : 1);
      Bytes luni = luni_block(layer->name());
      blocks.insert(blocks.end(), luni.begin(), luni.end());

      out.push_back(make_record(layer->name(),
                                gfx::Rect(0, 0, 0, 0),
                                doc::BlendMode::NORMAL,
                                255,
                                layer->isVisible(),
                                blocks,
                                nullptr,
                                pal,
                                transparentIndex));
      continue;
    }

    const doc::Cel* cel = layer->cel(frame);
    const doc::Image* img = (cel ? cel->image() : nullptr);
    const gfx::Rect bounds = (cel ? cel->bounds() : gfx::Rect(0, 0, 0, 0));

    // PSD has no per-cel opacity, so fold it into the layer's.
    int opacity = layer->opacity();
    if (cel)
      opacity = opacity * cel->opacity() / 255;

    out.push_back(make_record(layer->name(),
                              bounds,
                              layer->blendMode(),
                              opacity,
                              layer->isVisible(),
                              luni_block(layer->name()),
                              img,
                              pal,
                              transparentIndex));
  }
}

} // anonymous namespace

bool encode_psd(FILE* file, const doc::Sprite* sprite, const doc::frame_t frame, std::string& error)
{
  if (!file || !sprite) {
    error = "Nothing to write";
    return false;
  }
  if (sprite->width() > 30000 || sprite->height() > 30000) {
    error = "A PSD cannot be larger than 30000 pixels on a side";
    return false;
  }

  const doc::Palette* pal = sprite->palette(frame);
  const doc::color_t transparentIndex = sprite->transparentColor();

  // --- layer records -------------------------------------------------------
  std::vector<Record> records;
  emit_layers(sprite->root(), frame, pal, transparentIndex, records);

  Bytes layerInfo;
  put16(layerInfo, uint16_t(records.size()));
  for (const auto& r : records)
    layerInfo.insert(layerInfo.end(), r.header.begin(), r.header.end());
  for (const auto& r : records)
    layerInfo.insert(layerInfo.end(), r.channels.begin(), r.channels.end());
  if (layerInfo.size() % 2)
    layerInfo.push_back(0);

  Bytes layerAndMask;
  put32(layerAndMask, uint32_t(layerInfo.size()));
  layerAndMask.insert(layerAndMask.end(), layerInfo.begin(), layerInfo.end());
  put32(layerAndMask, 0); // global layer mask info, empty but not optional:
                          // leaving it out makes readers run off into the
                          // following bytes.

  // --- flattened image -----------------------------------------------------
  doc::ImageRef flat(doc::Image::create(doc::IMAGE_RGB, sprite->width(), sprite->height()));
  doc::clear_image(flat.get(), 0);
  {
    render::Render render;
    render.setNewBlend(true);
    render.renderSprite(flat.get(), sprite, frame);
  }

  auto flatPlanes = split_planes(flat.get(), pal, transparentIndex);

  Bytes mergedCounts, mergedData;
  for (int k = 0; k < 4; ++k)
    encode_channel_rle(flatPlanes[k], sprite->width(), sprite->height(), mergedCounts, mergedData);

  // --- write it out --------------------------------------------------------
  Bytes out;
  putRaw(out, "8BPS", 4);
  put16(out, 1);                          // version
  for (int i = 0; i < 6; ++i)             // reserved
    put8(out, 0);
  put16(out, 4);                          // channels: RGBA
  put32(out, uint32_t(sprite->height()));
  put32(out, uint32_t(sprite->width()));
  put16(out, 8);                          // bits per channel
  put16(out, 3);                          // color mode: RGB

  put32(out, 0); // color mode data
  put32(out, 0); // image resources

  put32(out, uint32_t(layerAndMask.size()));
  out.insert(out.end(), layerAndMask.begin(), layerAndMask.end());

  put16(out, 1); // merged image is RLE too
  out.insert(out.end(), mergedCounts.begin(), mergedCounts.end());
  out.insert(out.end(), mergedData.begin(), mergedData.end());

  if (std::fwrite(out.data(), 1, out.size(), file) != out.size()) {
    error = "Could not write the whole file";
    return false;
  }
  return true;
}

}} // namespace app::mods
