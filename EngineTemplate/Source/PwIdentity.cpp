// SPDX-License-Identifier: GPL-3.0-only
// Reads the instrument identity for the prebuilt engine. Plain C++ only: the
// VST3 and CLAP wrappers ask for the identity while the module is loading,
// before JUCE's own static objects can be relied on.
#include "PwIdentity.h"
#include "PwPayload.h"
#include <cstdio>
#include <string>
#include <vector>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {
struct Identity {
  std::string name = "Plugweaver Engine", maker = "Artilody",
              description = "Artilody Plugweaver instrument",
              version = "1.0.0", clapId = "org.artilody.engine", path;
  unsigned int code = pw_payload::fourCC("Pwen");
  int versionCode = 0x10000;
  std::uint64_t zipOffset = 0, zipSize = 0;
};

// Path of the module (plugin binary) that contains this code, as UTF-8.
std::string modulePath() {
#if defined(_WIN32)
  HMODULE module = nullptr;
  if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCWSTR>(&modulePath), &module))
    return {};
  std::vector<wchar_t> wide(32768);
  DWORD n = GetModuleFileNameW(module, wide.data(), DWORD(wide.size()));
  if (n == 0 || n >= wide.size())
    return {};
  int bytes = WideCharToMultiByte(CP_UTF8, 0, wide.data(), int(n), nullptr, 0,
                                  nullptr, nullptr);
  std::string utf8(size_t(bytes), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.data(), int(n), utf8.data(), bytes,
                      nullptr, nullptr);
  return utf8;
#else
  Dl_info info{};
  if (dladdr(reinterpret_cast<void *>(&modulePath), &info) == 0 ||
      info.dli_fname == nullptr)
    return {};
  return info.dli_fname;
#endif
}

std::FILE *openFile(const std::string &utf8) {
#if defined(_WIN32)
  int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
  std::wstring wide(size_t(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wide.data(), n);
  return _wfopen(wide.c_str(), L"rb");
#else
  return std::fopen(utf8.c_str(), "rb");
#endif
}

bool seek(std::FILE *f, std::uint64_t pos) {
#if defined(_WIN32)
  return _fseeki64(f, static_cast<__int64>(pos), SEEK_SET) == 0;
#else
  return fseeko(f, static_cast<off_t>(pos), SEEK_SET) == 0;
#endif
}

std::uint64_t fileSize(std::FILE *f) {
#if defined(_WIN32)
  if (_fseeki64(f, 0, SEEK_END) != 0)
    return 0;
  return static_cast<std::uint64_t>(_ftelli64(f));
#else
  if (fseeko(f, 0, SEEK_END) != 0)
    return 0;
  return static_cast<std::uint64_t>(ftello(f));
#endif
}

// macOS keeps the payload as a resource inside the signed bundle; elsewhere it
// is appended to the plugin binary.
std::string payloadPath() {
  auto path = modulePath();
#if defined(__APPLE__)
  auto slash = path.rfind('/');                     // .../Contents/MacOS/<exe>
  auto macos = slash == std::string::npos ? slash : path.rfind('/', slash - 1);
  if (macos == std::string::npos)
    return {};
  return path.substr(0, macos) + "/Resources/" + pw_payload::resourceName;
#else
  return path;
#endif
}

Identity load() {
  Identity id;
  auto path = payloadPath();
  if (path.empty())
    return id;
  std::FILE *f = openFile(path);
  if (f == nullptr)
    return id;
  unsigned char trailer[pw_payload::trailerSize];
  pw_payload::Trailer t;
  std::uint64_t size = fileSize(f);
  bool ok = size > pw_payload::trailerSize &&
            seek(f, size - pw_payload::trailerSize) &&
            std::fread(trailer, 1, sizeof(trailer), f) == sizeof(trailer) &&
            pw_payload::parseTrailer(trailer, size, t);
  std::string text;
  if (ok) {
    text.resize(size_t(t.identitySize));
    ok = seek(f, size - pw_payload::trailerSize - t.identitySize) &&
         std::fread(text.data(), 1, text.size(), f) == text.size();
  }
  std::fclose(f);
  if (!ok)
    return id;
  auto value = [&](const char *key, std::string &target) {
    auto v = pw_payload::identityValue(text, key);
    if (!v.empty())
      target = v;
  };
  value("name", id.name);
  value("maker", id.maker);
  value("description", id.description);
  value("version", id.version);
  value("clapId", id.clapId);
  if (auto code = pw_payload::fourCC(pw_payload::identityValue(text, "code")))
    id.code = code;
  id.versionCode = pw_payload::versionCode(id.version);
  id.path = path;
  id.zipSize = t.zipSize;
  id.zipOffset = size - pw_payload::trailerSize - t.identitySize - t.zipSize;
  return id;
}

const Identity &identity() {
  static const Identity id = load();
  return id;
}
} // namespace

namespace pw_id {
const char *name() noexcept { return identity().name.c_str(); }
const char *maker() noexcept { return identity().maker.c_str(); }
const char *description() noexcept { return identity().description.c_str(); }
const char *versionString() noexcept { return identity().version.c_str(); }
int versionCode() noexcept { return identity().versionCode; }
unsigned int code() noexcept { return identity().code; }
const char *clapId() noexcept { return identity().clapId.c_str(); }
Payload payload() noexcept {
  const auto &id = identity();
  return {id.path.c_str(), id.zipOffset, id.zipSize};
}
} // namespace pw_id
