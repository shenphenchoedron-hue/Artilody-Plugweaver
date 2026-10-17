// SPDX-License-Identifier: GPL-3.0-only
#pragma once
// Instrument payload used by the prebuilt Plugweaver engine.
//
// Plugweaver ships one finished plugin per format ("the engine"). Exporting an
// instrument copies the engine and attaches a payload with the instrument's
// identity and its project archive, so no compiler is needed:
//   macOS:          <bundle>/Contents/Resources/instrument.pwi
//   Windows/Linux:  appended to the end of the plugin binary itself
//
// Layout (all integers little-endian):
//   [project zip][identity text][u64 identitySize][u64 zipSize][u64 version][8 byte magic]
// The identity text is UTF-8 "key=value" lines; values never contain line
// breaks. This header has no JUCE dependency, because the engine reads the
// identity while the plugin module is still being loaded.
#include <cstdint>
#include <cstring>
#include <string>

namespace pw_payload {
constexpr char magic[8] = {'P', 'W', 'I', 'N', 'S', 'T', '0', '1'};
constexpr std::uint64_t version = 1;
constexpr int trailerSize = 32;
constexpr const char *resourceName = "instrument.pwi";

struct Trailer {
  std::uint64_t identitySize = 0, zipSize = 0;
};

inline std::uint64_t readU64(const unsigned char *p) {
  std::uint64_t v = 0;
  for (int i = 7; i >= 0; --i)
    v = (v << 8) | p[i];
  return v;
}
inline void writeU64(unsigned char *p, std::uint64_t v) {
  for (int i = 0; i < 8; ++i, v >>= 8)
    p[i] = static_cast<unsigned char>(v & 0xff);
}
// Parses the last 32 bytes of a file. fileSize is the total size, so sizes can
// be checked against it.
inline bool parseTrailer(const unsigned char *t, std::uint64_t fileSize,
                         Trailer &out) {
  if (std::memcmp(t + 24, magic, 8) != 0 || readU64(t + 16) != version)
    return false;
  out.identitySize = readU64(t);
  out.zipSize = readU64(t + 8);
  return out.identitySize > 0 && out.identitySize < 64 * 1024 &&
         out.zipSize > 0 &&
         out.identitySize + out.zipSize + trailerSize <= fileSize;
}
inline void makeTrailer(unsigned char *t, const Trailer &in) {
  writeU64(t, in.identitySize);
  writeU64(t + 8, in.zipSize);
  writeU64(t + 16, version);
  std::memcpy(t + 24, magic, 8);
}
// Returns the value of "key=" in the identity text, or an empty string.
inline std::string identityValue(const std::string &text,
                                 const std::string &key) {
  size_t pos = 0;
  while (pos < text.size()) {
    size_t end = text.find('\n', pos);
    if (end == std::string::npos)
      end = text.size();
    if (text.compare(pos, key.size() + 1, key + "=") == 0)
      return text.substr(pos + key.size() + 1, end - pos - key.size() - 1);
    pos = end + 1;
  }
  return {};
}
// JUCE packs "a.b.c" as (a << 16) | (b << 8) | c.
inline int versionCode(const std::string &v) {
  int parts[3] = {0, 0, 0}, i = 0;
  for (char c : v) {
    if (c == '.') {
      if (++i > 2)
        break;
    } else if (c >= '0' && c <= '9')
      parts[i] = parts[i] * 10 + (c - '0');
  }
  return (parts[0] << 16) | ((parts[1] & 0xff) << 8) | (parts[2] & 0xff);
}
// Four-character code, first character in the highest byte, like JUCE.
inline std::uint32_t fourCC(const std::string &code) {
  if (code.size() != 4)
    return 0;
  std::uint32_t v = 0;
  for (char c : code)
    v = (v << 8) | static_cast<unsigned char>(c);
  return v;
}
} // namespace pw_payload
