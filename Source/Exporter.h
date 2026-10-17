// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "BuilderAssets.h"
#include "Model.h"
#include "PwPayload.h"
#include <functional>
namespace pw {
// Four-character plugin code (AU subtype / VST3 class id) derived from the
// instrument UUID. Shared by the exporter and the builder's Export tab.
inline String pluginCode(const Document &d) {
  auto hash = static_cast<uint32>(d.uuid.hashCode());
  static constexpr char letters[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  String code = String::charToString(static_cast<juce_wchar>('A' + hash % 26));
  for (int i = 0; i < 3; ++i) {
    hash /= 26;
    code += String::charToString(static_cast<juce_wchar>(letters[hash % 62]));
  }
  return code;
}
// Plugin version that increases every minute, based on UTC so daylight saving
// and time zone changes never make it go backwards. JUCE packs minor and patch
// into 8 bits each for the AU version, so the minute count is split base 256.
inline String pluginVersion(Time when = Time::getCurrentTime()) {
  const int64 epoch = 1767225600000LL; // 2026-01-01 00:00 UTC
  const int64 minutes = std::max<int64>(0, (when.toMilliseconds() - epoch) / 60000);
  return String(1 + minutes / 65536) + "." + String((minutes / 256) % 256) +
         "." + String(minutes % 256);
}
// Per-user plugin folders that hosts scan by default, so installing never
// needs administrator rights:
//   macOS:   ~/Library/Audio/Plug-Ins/{VST3,Components,CLAP}
//   Windows: %LOCALAPPDATA%\Programs\Common\{VST3,CLAP}
//   Linux:   ~/.vst3, ~/.clap
// The folder names match the install rules in EngineTemplate/CMakeLists.txt.
inline File pluginInstallRoot() {
#if JUCE_MAC
  return File::getSpecialLocation(File::userHomeDirectory)
      .getChildFile("Library/Audio/Plug-Ins");
#elif JUCE_WINDOWS
  return File::getSpecialLocation(File::windowsLocalAppData)
      .getChildFile("Programs")
      .getChildFile("Common");
#else
  return File::getSpecialLocation(File::userHomeDirectory);
#endif
}
// format is "vst3", "au" or "clap".
inline String pluginFolderName(const String &format) {
#if JUCE_MAC
  return format == "au" ? "Components" : format.toUpperCase();
#elif JUCE_WINDOWS
  return format.toUpperCase();
#else
  return "." + format;
#endif
}
inline String pluginExtension(const String &format) {
  return format == "au" ? ".component" : "." + format;
}
// CMake used to build exported instruments: the one Plugweaver was built
// with, a standard install location, or the first one found on PATH.
inline File findCMake() {
  File built(PW_CMAKE_PATH);
  if (built.existsAsFile())
    return built;
#if JUCE_WINDOWS
  const String exe = "cmake.exe";
  StringArray candidates{"C:\\Program Files\\CMake\\bin\\cmake.exe"};
  const String separator = ";";
#else
  const String exe = "cmake";
  StringArray candidates{"/Applications/CMake.app/Contents/bin/cmake",
                         "/opt/homebrew/bin/cmake", "/usr/local/bin/cmake",
                         "/usr/bin/cmake"};
  const String separator = ":";
#endif
  for (auto &dir : StringArray::fromTokens(
           SystemStats::getEnvironmentVariable("PATH", {}), separator, "\""))
    if (File::isAbsolutePath(dir))
      candidates.add(File(dir).getChildFile(exe).getFullPathName());
  for (auto &path : candidates)
    if (File(path).existsAsFile())
      return File(path);
  return {};
}
// The name becomes the plugin file/bundle name and is written into CMake files
// for source exports, so characters with meaning to CMake ($ { } [ ] \ ") and
// names Windows cannot store (trailing dot) are not allowed.
inline void validateForExport(const Document &d, bool vst3, bool au,
                              bool clap) {
  require(!d.zones.empty(), "Import an SFZ instrument before building");
  require(d.name.trim().isNotEmpty() && d.name == d.name.trim() &&
              d.name.length() <= 80 && d.maker.length() <= 80 &&
              d.name == File::createLegalFileName(d.name) &&
              !d.name.endsWithChar('.') &&
              !d.name.containsAnyOf(";\r\n${}[]\\\"") &&
              !d.maker.containsAnyOf(";\r\n"),
          "Use an instrument name under 80 characters without path separators, "
          "line breaks or the characters $ { } [ ] \\ \"");
  // Names Windows reserves for devices, also with an extension (CON.vst3).
  auto base = d.name.upToFirstOccurrenceOf(".", false, false).trim().toUpperCase();
  static const StringArray reserved{"CON", "PRN", "AUX", "NUL"};
  bool device = reserved.contains(base) ||
                ((base.startsWith("COM") || base.startsWith("LPT")) &&
                 base.length() == 4 && base.getLastCharacter() >= '0' &&
                 base.getLastCharacter() <= '9');
  require(!device, "\"" + d.name + "\" is a reserved name on Windows; "
                   "choose another instrument name");
  require(vst3 || au || clap, "Select at least one plugin format");
}
// This only extracts the trusted code archive compiled into Plugweaver, never a
// user template.
inline File exportInstrument(const Document &d, const File &parent, bool vst3,
                             bool au, bool clap) {
  validateForExport(d, vst3, au, clap);
  auto safeName = d.name.retainCharacters(
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_");
  if (safeName.isEmpty())
    safeName = "Instrument";
  File target = parent.getNonexistentChildFile(safeName + "-Source", "", false);
  require(target.createDirectory().wasOk(), "Cannot create export directory");
  try {
    MemoryInputStream in(
        BuilderAssets::engine_template_zip,
        static_cast<std::size_t>(BuilderAssets::engine_template_zipSize),
        false);
    ZipFile zip(in);
    require(zip.uncompressTo(target, false).wasOk(),
            "Could not unpack source template");
    saveDocument(d, target.getChildFile("instrument.zip"));
    // Bracket quoting prevents instrument names being interpreted as CMake
    // code.
    auto quoted = [](String s) {
      require(!s.contains("]====]"), "Unsupported text in instrument name");
      return "[====[" + s + "]====]";
    };
    String code = pluginCode(d);
    String id = "org.artilody.instrument." +
                d.uuid.retainCharacters("0123456789abcdef");
    String version = pluginVersion();
    String config = "set(PW_NAME " + quoted(d.name) + ")\nset(PW_MAKER " +
                    quoted(d.maker) + ")\nset(PW_ID " + quoted(id) +
                    ")\nset(PW_CODE " + code + ")\nset(PW_VST3 " +
                    String(vst3 ? "ON" : "OFF") + ")\nset(PW_AU " +
                    String(au ? "ON" : "OFF") + ")\nset(PW_CLAP " +
                    String(clap ? "ON" : "OFF") + ")\nset(PW_VERSION " +
                    version + ")\n";
    require(
        target.getChildFile("InstrumentConfig.cmake").replaceWithText(config),
        "Cannot write instrument configuration");
    target.getChildFile("Build-Mac.command").setExecutePermission(true);
    target.getChildFile("Build-Linux.sh").setExecutePermission(true);
    return target;
  } catch (...) {
    target.deleteRecursively();
    throw;
  }
}

//== Building plugins from the prebuilt engine ===================================
// Plugweaver embeds one finished plugin per format ("the engine", see
// EngineTemplate/Source/PwPayload.h). Building an instrument copies the engine,
// gives it the instrument's name, identity and data, and installs it. No
// compiler, CMake or network access is needed.
using Log = std::function<void(const String &)>;
// Implemented in EngineBlob.cpp (app only): the engine archive in the app.
std::unique_ptr<InputStream> openEngineArchive();

inline const String engineName = "Plugweaver Engine";
// Audio Units exist only on macOS.
#if JUCE_MAC
inline constexpr bool hasAudioUnits = true;
#else
inline constexpr bool hasAudioUnits = false;
#endif

struct BuiltPlugin {
  String format; // "vst3", "au" or "clap"
  File file;     // finished bundle (directory) or single file
};

inline String pluginMaker(const Document &d) {
  return d.maker.trim().isNotEmpty() ? d.maker.trim() : String("Artilody");
}
inline String pluginId(const Document &d) {
  return "org.artilody.instrument." +
         d.uuid.retainCharacters("0123456789abcdef");
}
// Identity read by the engine (PwIdentity.cpp). VST3 class id, AU subtype and
// CLAP id are derived exactly as in exported source projects, so a plugin
// built either way is the same plugin for the host.
inline String identityText(const Document &d, const String &version) {
  return "name=" + d.name + "\nmaker=" + pluginMaker(d) +
         "\ndescription=Artilody Plugweaver instrument\nversion=" + version +
         "\ncode=" + pluginCode(d) + "\nclapId=" + pluginId(d) + "\n";
}

// Runs a helper program and returns its exit code; output goes to the log.
inline int runTool(const StringArray &args, const Log &log) {
  if (log)
    log("$ " + args.joinIntoString(" ") + "\n");
  ChildProcess process;
  if (!process.start(args, ChildProcess::wantStdOut | ChildProcess::wantStdErr))
    return -1;
  auto output = process.readAllProcessOutput();
  process.waitForProcessToFinish(120000);
  if (log && output.isNotEmpty())
    log(output.endsWithChar('\n') ? output : output + "\n");
  return int(process.getExitCode());
}

// Writes the payload: [project zip][identity][trailer].
inline void writePayload(FileOutputStream &out, const File &projectZip,
                         const String &identity) {
  FileInputStream zip(projectZip);
  require(zip.openedOk(), "Cannot read the instrument archive");
  auto zipSize = zip.getTotalLength();
  require(out.writeFromInputStream(zip, -1) == zipSize,
          "Could not write instrument data");
  require(out.write(identity.toRawUTF8(), identity.getNumBytesAsUTF8()),
          "Could not write instrument identity");
  pw_payload::Trailer t;
  t.identitySize = identity.getNumBytesAsUTF8();
  t.zipSize = std::uint64_t(zipSize);
  unsigned char trailer[pw_payload::trailerSize];
  pw_payload::makeTrailer(trailer, t);
  require(out.write(trailer, sizeof(trailer)), "Could not write payload");
  out.flush();
  require(out.getStatus().wasOk(), "Could not write instrument data");
}
inline void appendPayload(const File &binary, const File &projectZip,
                          const String &identity) {
  FileOutputStream out(binary); // opens at the end of the file
  require(out.openedOk(), "Cannot write " + binary.getFullPathName());
  writePayload(out, projectZip, identity);
}

// Plist helpers for the macOS bundles produced by JUCE (XML property lists).
inline XmlElement *plistValue(XmlElement &dict, const String &key) {
  for (auto *child = dict.getFirstChildElement(); child != nullptr;
       child = child->getNextElement())
    if (child->hasTagName("key") && child->getAllSubText() == key)
      return child->getNextElement();
  return nullptr;
}
inline void setPlistValue(XmlElement &dict, const String &key,
                          const String &value, const String &type = "string") {
  auto *v = plistValue(dict, key);
  if (v == nullptr) {
    dict.createNewChildElement("key")->addTextElement(key);
    v = dict.createNewChildElement(type);
  }
  v->setTagName(type);
  v->deleteAllChildElements();
  v->addTextElement(value);
}
inline void writeBundlePlist(const File &plist, const Document &d,
                             const String &format, const String &version) {
  auto xml = XmlDocument::parse(plist);
  require(xml && xml->hasTagName("plist"), "Engine Info.plist is invalid");
  auto *dict = xml->getChildByName("dict");
  require(dict != nullptr, "Engine Info.plist is invalid");
  setPlistValue(*dict, "CFBundleExecutable", d.name);
  setPlistValue(*dict, "CFBundleIdentifier", pluginId(d));
  setPlistValue(*dict, "CFBundleName", d.name);
  setPlistValue(*dict, "CFBundleDisplayName", d.name);
  setPlistValue(*dict, "CFBundleShortVersionString", version);
  setPlistValue(*dict, "CFBundleVersion", version);
  if (format == "au") {
    auto *components = plistValue(*dict, "AudioComponents");
    auto *component =
        components != nullptr ? components->getChildByName("dict") : nullptr;
    require(component != nullptr, "Engine Audio Unit description is missing");
    setPlistValue(*component, "name", pluginMaker(d) + ": " + d.name);
    setPlistValue(*component, "description", d.name);
    setPlistValue(*component, "manufacturer", "Arti");
    setPlistValue(*component, "subtype", pluginCode(d));
    setPlistValue(*component, "version",
                  String(pw_payload::versionCode(version.toStdString())),
                  "integer");
  }
  XmlElement::TextFormat format_;
  format_.dtd = "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
                "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">";
  require(xml->writeTo(plist, format_), "Cannot write Info.plist");
}

// Creates the finished plugins for this platform inside `stage`.
inline std::vector<BuiltPlugin>
createPlugins(const Document &d, bool vst3, bool au, bool clap,
              InputStream &engineArchive, const File &stage,
              const Log &log = {}) {
  validateForExport(d, vst3, au, clap);
  require(hasAudioUnits || !au, "Audio Units can only be built on macOS");
  require(stage.deleteRecursively() && stage.createDirectory().wasOk(),
          "Cannot create " + stage.getFullPathName());
  auto engineDir = stage.getChildFile("engine");
  {
    ZipFile zip(engineArchive);
    require(zip.getNumEntries() > 0 &&
                zip.uncompressTo(engineDir, true).wasOk(),
            "The built-in plugin engine is missing or damaged");
  }
  auto projectZip = stage.getChildFile("instrument.zip");
  saveDocument(d, projectZip);
  const String version = pluginVersion(), identity = identityText(d, version);
  auto outDir = stage.getChildFile("plugins");
  require(outDir.createDirectory().wasOk(), "Cannot create output folder");

  struct Format {
    bool wanted;
    String id, folder;
  } formats[] = {{vst3, "vst3", "VST3"}, {au, "au", "AU"}, {clap, "clap", "CLAP"}};
  std::vector<BuiltPlugin> built;
  for (const auto &f : formats) {
    if (!f.wanted)
      continue;
    const String ext = pluginExtension(f.id);
    auto source = engineDir.getChildFile(f.folder).getChildFile(engineName + ext);
    require(source.exists(), "The built-in engine has no " +
                                 f.folder.toUpperCase() + " plugin");
    auto target = outDir.getChildFile(d.name + ext);
    require(source.moveFileTo(target), "Cannot prepare " + target.getFileName());
#if JUCE_MAC
    auto contents = target.getChildFile("Contents");
    auto exe = contents.getChildFile("MacOS").getChildFile(engineName);
    auto renamed = exe.getSiblingFile(d.name);
    require(exe.moveFileTo(renamed), "Cannot rename the plugin binary");
    renamed.setExecutePermission(true);
    writeBundlePlist(contents.getChildFile("Info.plist"), d, f.id, version);
    auto resources = contents.getChildFile("Resources");
    resources.createDirectory();
    // moduleinfo.json describes the engine's class id; hosts read the real
    // identity from the plugin instead.
    resources.getChildFile("moduleinfo.json").deleteFile();
    contents.getChildFile("_CodeSignature").deleteRecursively();
    FileOutputStream out(resources.getChildFile(pw_payload::resourceName));
    require(out.openedOk(), "Cannot write instrument data");
    writePayload(out, projectZip, identity);
#else
    if (target.isDirectory()) { // VST3 bundle: Contents/<arch>/<name>.<so|vst3>
      auto contents = target.getChildFile("Contents");
      contents.getChildFile("Resources").getChildFile("moduleinfo.json").deleteFile();
      int binaries = 0;
      for (auto arch : contents.findChildFiles(File::findDirectories, false))
        for (auto bin : arch.findChildFiles(File::findFiles, false))
          if (bin.getFileNameWithoutExtension() == engineName) {
            auto renamed = bin.getSiblingFile(d.name + bin.getFileExtension());
            require(bin.moveFileTo(renamed), "Cannot rename the plugin binary");
            appendPayload(renamed, projectZip, identity);
            ++binaries;
          }
      require(binaries > 0, "The built-in VST3 engine has no binary");
    } else {
      appendPayload(target, projectZip, identity);
    }
#endif
    built.push_back({f.id, target});
  }
#if JUCE_MAC
  // Ad-hoc signature, as required for plugins on Apple Silicon. codesign is
  // part of macOS, so this needs no developer tools.
  for (const auto &b : built) {
    runTool({"/usr/bin/xattr", "-cr", b.file.getFullPathName()}, log);
    require(runTool({"/usr/bin/codesign", "--force", "--deep", "--sign", "-",
                     b.file.getFullPathName()},
                    log) == 0,
            "Could not sign " + b.file.getFileName());
    require(runTool({"/usr/bin/codesign", "--verify", "--deep", "--strict",
                     b.file.getFullPathName()},
                    log) == 0,
            "Signature check failed for " + b.file.getFileName());
  }
#endif
  engineDir.deleteRecursively();
  projectZip.deleteFile();
  return built;
}

// Copies finished plugins into the user's plugin folders. Older copies are
// removed first, so no stale files are left inside a bundle.
inline std::vector<File> installPlugins(const std::vector<BuiltPlugin> &built,
                                        const Log &log = {}) {
  std::vector<File> installed;
  bool audioUnit = false;
  for (const auto &b : built) {
    auto folder = pluginInstallRoot().getChildFile(pluginFolderName(b.format));
    require(folder.createDirectory().wasOk(),
            "Cannot create " + folder.getFullPathName());
    auto target = folder.getChildFile(b.file.getFileName());
    if (target.exists())
      require(target.deleteRecursively(),
              "Cannot replace " + target.getFullPathName() +
                  ". Close your music program and try again.");
#if JUCE_MAC
    require(runTool({"/usr/bin/ditto", b.file.getFullPathName(),
                     target.getFullPathName()},
                    log) == 0,
            "Cannot install " + target.getFullPathName());
#else
    bool ok = b.file.isDirectory() ? b.file.copyDirectoryTo(target)
                                   : b.file.copyFileTo(target);
    require(ok, "Cannot install " + target.getFullPathName() +
                    ". Close your music program and try again.");
#endif
    if (log)
      log("Installed: " + target.getFullPathName() + "\n");
    installed.push_back(target);
    audioUnit = audioUnit || b.format == "au";
  }
#if JUCE_MAC
  // Make macOS re-read its Audio Unit list so hosts see the new AU.
  if (audioUnit)
    runTool({"/usr/bin/killall", "-9", "AudioComponentRegistrar"}, log);
#else
  ignoreUnused(audioUnit);
#endif
  return installed;
}
} // namespace pw
