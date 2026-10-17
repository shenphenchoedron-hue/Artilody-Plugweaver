// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Model.h"
namespace pw {
inline int midiNumber(String s) {
  s = s.trim();
  if (s.containsOnly("0123456789") && s.isNotEmpty())
    return s.getIntValue();
  String names = "c d ef g a b";
  int base = names.indexOfChar(s.toLowerCase()[0]);
  require(base >= 0 && s.length() >= 2, "Invalid SFZ note: " + s);
  int at = 1;
  if (s[at] == '#') {
    ++base;
    ++at;
  } else if (s[at] == 'b') {
    --base;
    ++at;
  }
  auto oct = s.substring(at);
  require(oct.isNotEmpty() && oct.containsOnly("-0123456789"),
          "Invalid SFZ octave");
  return (oct.getIntValue() + 1) * 12 + base;
}
inline void importSfz(Document &d, const File &file) {
  require(file.getSize() > 0 && file.getSize() < 2 * 1024 * 1024,
          "SFZ must be under 2 MB");
  String clean;
  for (auto line : StringArray::fromLines(file.loadFileAsString())) {
    auto trimmed = line.trimStart();
    require(!trimmed.startsWithChar('#'),
            "SFZ macros/includes are not supported in baseline-01");
    clean += line.upToFirstOccurrenceOf("//", false, false) + "\n";
  }
  std::string text = clean.toStdString();
  std::regex tokens(R"((<[^>]+>|[A-Za-z_][A-Za-z_0-9]*\s*=))");
  struct Token {
    std::string name, value;
  };
  std::vector<Token> list;
  std::vector<std::smatch> matches;
  for (auto it = std::sregex_iterator(text.begin(), text.end(), tokens);
       it != std::sregex_iterator(); ++it)
    matches.push_back(*it);
  for (std::size_t i = 0; i < matches.size(); ++i) {
    auto &m = matches[i];
    auto start = static_cast<std::size_t>(m.position() + m.length());
    auto end = i + 1 < matches.size()
                   ? static_cast<std::size_t>(matches[i + 1].position())
                   : text.size();
    list.push_back({m.str(), text.substr(start, end - start)});
  }
  std::map<String, String> global, group, region;
  String section, defaultPath;
  std::vector<std::map<String, String>> regions;
  auto flush = [&] {
    if (section == "<region>")
      regions.push_back(region);
  };
  const StringArray supported{"sample",          "key",   "lokey", "hikey",
                              "pitch_keycenter", "lovel", "hivel", "loop_mode"};
  for (auto token : list) {
    String name(token.name), value = String(token.value).trim().unquoted();
    name = name.trim();
    if (name.startsWithChar('<')) {
      flush();
      require(name == "<global>" || name == "<group>" || name == "<region>" ||
                  name == "<control>",
              "Unsupported SFZ section: " + name);
      section = name;
      if (name == "<group>")
        group = global;
      if (name == "<region>")
        region = group.empty() ? global : group;
      continue;
    }
    name = name.dropLastCharacters(1).trim();
    if (section == "<control>" && name == "default_path") {
      defaultPath = value.replaceCharacter('\\', '/');
      if (defaultPath.isNotEmpty() && !defaultPath.endsWithChar('/'))
        defaultPath += "/";
      continue;
    }
    require(supported.contains(name),
            "Unsupported SFZ opcode: " + name +
                ". Re-export with Sampleforge or simplify the SFZ.");
    require(section == "<global>" || section == "<group>" ||
                section == "<region>",
            "SFZ opcode outside a region/group");
    if (name == "loop_mode")
      require(value == "no_loop",
              "Looped SFZ instruments are not supported yet");
    auto &target = section == "<global>"  ? global
                   : section == "<group>" ? group
                                          : region;
    if (name == "key") {
      target["lokey"] = value;
      target["hikey"] = value;
      target["pitch_keycenter"] = value;
    } else
      target[name] = value;
  }
  flush();
  require(!regions.empty() && regions.size() <= 256, "Use 1-256 SFZ regions");
  Document next = d;
  next.zones.clear();
  for (auto it = next.assets.begin(); it != next.assets.end();)
    if (it->first.startsWith("samples/"))
      it = next.assets.erase(it);
    else
      ++it;
  std::map<String, String> imported;
  int64 total = 0;
  AudioFormatManager fm;
  fm.registerBasicFormats();
  for (auto values : regions) {
    auto get = [&](String k, int fallback) {
      auto it = values.find(k);
      return it == values.end() ? fallback : midiNumber(it->second);
    };
    Zone z;
    z.low = get("lokey", 0);
    z.high = get("hikey", 127);
    z.root = get("pitch_keycenter", 60);
    z.velLow = get("lovel", 1);
    z.velHigh = get("hivel", 127);
    require(z.low >= 0 && z.high <= 127 && z.low <= z.high && z.root >= 0 &&
                z.root <= 127 && z.velLow >= 1 && z.velHigh <= 127 &&
                z.velLow <= z.velHigh,
            "Invalid SFZ MIDI range");
    auto relative =
        (defaultPath + values["sample"]).replaceCharacter('\\', '/');
    require(safePath(relative),
            "Use relative sample paths within the SFZ folder");
    File f = file.getParentDirectory().getChildFile(relative);
    for (auto check = f; check != file.getParentDirectory();
         check = check.getParentDirectory())
      require(!check.isSymbolicLink(), "Linked sample paths are not supported");
    auto known = imported.find(relative);
    if (known == imported.end()) {
      std::unique_ptr<AudioFormatReader> reader(fm.createReaderFor(f));
      require(bool(reader), "Cannot decode sample: " + relative);
      require(reader->numChannels >= 1 && reader->numChannels <= 2 &&
                  reader->sampleRate >= 8000 && reader->sampleRate <= 192000 &&
                  reader->lengthInSamples > 0 &&
                  reader->lengthInSamples < 240 * reader->sampleRate,
              "Use mono/stereo samples under four minutes at 8-192 kHz");
      total += reader->lengthInSamples * reader->numChannels * 4;
      require(total <= 256LL * 1024 * 1024,
              "Decoded instrument exceeds 256 MB");
      MemoryBlock bytes;
      require(f.getSize() <= 256LL * 1024 * 1024 && f.loadFileAsData(bytes),
              "Cannot read sample");
      z.asset = "samples/" + String(static_cast<int>(imported.size())) +
                f.getFileExtension();
      next.assets[z.asset] = std::move(bytes);
      imported[relative] = z.asset;
    } else
      z.asset = known->second;
    z.file = f.getFileName();
    next.zones.push_back(z);
  }
  next.name = file.getFileNameWithoutExtension();
  d = std::move(next);
}
} // namespace pw
