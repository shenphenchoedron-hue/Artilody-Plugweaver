// SPDX-License-Identifier: GPL-3.0-only
#pragma once
// Runtime plugin identity for the prebuilt Plugweaver engine.
//
// The engine build passes this file as JUCE_APP_CONFIG_HEADER, so JUCE includes
// it from juce_TargetPlatform.h before any plugin wrapper (VST3, AU, CLAP) uses
// the JucePlugin_* macros. The macros JUCE defines on the command line are
// replaced here by functions that read the identity from the instrument
// payload attached at export time (see PwPayload.h). Every use of these macros
// in JUCE 7.0.12 and clap-juce-extensions accepts a runtime value.
#include <cstdint>

namespace pw_id {
const char *name() noexcept;
const char *maker() noexcept;
const char *description() noexcept;
const char *versionString() noexcept;
int versionCode() noexcept;
unsigned int code() noexcept;
const char *clapId() noexcept;
// Location of the instrument project archive. path is empty when the engine
// runs without a payload (for example while the engine itself is built).
struct Payload {
  const char *path = "";
  std::uint64_t offset = 0, size = 0;
};
Payload payload() noexcept;
} // namespace pw_id

#undef JucePlugin_Name
#define JucePlugin_Name pw_id::name()
#undef JucePlugin_Manufacturer
#define JucePlugin_Manufacturer pw_id::maker()
#undef JucePlugin_Desc
#define JucePlugin_Desc pw_id::description()
#undef JucePlugin_VersionString
#define JucePlugin_VersionString pw_id::versionString()
#undef JucePlugin_VersionCode
#define JucePlugin_VersionCode pw_id::versionCode()
#undef JucePlugin_PluginCode
#define JucePlugin_PluginCode pw_id::code()
#undef CLAP_ID
#define CLAP_ID pw_id::clapId()
