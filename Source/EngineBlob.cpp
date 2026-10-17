// SPDX-License-Identifier: GPL-3.0-only
// Access to the prebuilt instrument engine archive embedded in the app:
//   macOS:   Contents/Resources/engine.zip inside the app bundle
//   Windows: RCDATA resource "PWENGINE" in the executable
//   Linux:   linked into the executable with .incbin
#include "Exporter.h"
#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif JUCE_LINUX || JUCE_BSD
__asm__(".section .rodata\n"
        ".global pw_engine_zip_begin\n"
        ".balign 16\n"
        "pw_engine_zip_begin:\n"
        ".incbin \"" PW_ENGINE_ZIP "\"\n"
        ".global pw_engine_zip_end\n"
        "pw_engine_zip_end:\n"
        ".byte 0\n"
        ".previous\n");
extern "C" const unsigned char pw_engine_zip_begin[];
extern "C" const unsigned char pw_engine_zip_end[];
#endif

namespace pw {
std::unique_ptr<InputStream> openEngineArchive() {
#if JUCE_MAC
  auto file = File::getSpecialLocation(File::currentApplicationFile)
                  .getChildFile("Contents/Resources/engine.zip");
  if (!file.existsAsFile())
    return {};
  return std::make_unique<FileInputStream>(file);
#elif JUCE_WINDOWS
  auto module = GetModuleHandleW(nullptr);
  auto resource = FindResourceW(module, L"PWENGINE", MAKEINTRESOURCEW(10)); // RT_RCDATA
  if (resource == nullptr)
    return {};
  auto handle = LoadResource(module, resource);
  auto size = SizeofResource(module, resource);
  auto *data = handle != nullptr ? LockResource(handle) : nullptr;
  if (data == nullptr || size == 0)
    return {};
  return std::make_unique<MemoryInputStream>(data, size_t(size), false);
#else
  return std::make_unique<MemoryInputStream>(
      pw_engine_zip_begin, size_t(pw_engine_zip_end - pw_engine_zip_begin),
      false);
#endif
}
} // namespace pw
