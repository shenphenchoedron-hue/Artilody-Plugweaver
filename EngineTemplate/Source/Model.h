// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <map>
#include <regex>
#include <set>
namespace pw {
using namespace juce;
inline void require(bool ok, const String &message) {
  if (!ok)
    throw std::runtime_error(message.toStdString());
}
inline var object() { return var(new DynamicObject); }
inline void put(var &v, const Identifier &key, const var &value) {
  v.getDynamicObject()->setProperty(key, value);
}
inline String json(const var &v) { return JSON::toString(v); }
inline bool safePath(const String &name) {
  if (name.isEmpty() || name.startsWithChar('/') || name.containsChar('\\') ||
      name.containsChar(':'))
    return false;
  for (auto part : StringArray::fromTokens(name, "/", ""))
    if (part == ".." || part == "." || part.isEmpty())
      return false;
  return true;
}
inline const StringArray parameterIDs{
    "gain",      "attack", "decay",   "sustain",   "release",
    "cutoff",    "resonance", "reverb", "enabled", "filtertype"};
struct Param {
  String id, name;
  float min, max, initial, skew, step = 0;
};
inline const StringArray filterTypes{"Low Pass", "Band Pass", "High Pass"};
inline const std::vector<Param> &parameters() {
  static const std::vector<Param> p{
      {"gain", "Volume", 0, 1, .7f, 1},
      {"attack", "Attack", .001f, 5, .01f, .35f},
      {"decay", "Decay", .001f, 5, .3f, .35f},
      {"sustain", "Sustain", 0, 1, .8f, 1},
      {"release", "Release", .01f, 8, .5f, .4f},
      {"cutoff", "Cutoff", 30, 20000, 18000, .3f},
      {"resonance", "Resonance", .1f, 1.5f, .7f, 1},
      {"reverb", "Reverb", 0, 1, .15f, 1},
      {"enabled", "Enabled", 0, 1, 1, 1, 1},
      {"filtertype", "Filter type", 0, 2, 0, 1, 1}};
  return p;
}
// Visual styles for knobs, sliders and switches. The plugin draws each style in
// InstrumentView.h; the builder shows the same drawings as a picker.
inline const StringArray knobStyleIds{"classic", "minimal", "vintage",
                                      "ledring", "metal",   "flat"};
inline const StringArray knobStyleNames{"Classic",  "Minimal", "Vintage",
                                        "LED ring", "Metal",   "Flat"};
inline const StringArray sliderStyleIds{"classic", "fader", "bar",
                                        "minimal", "led",   "vertical"};
inline const StringArray sliderStyleNames{"Classic", "Fader",     "Bar",
                                          "Minimal", "LED meter", "Vertical"};
inline const StringArray toggleStyleIds{"pill", "rocker", "button",
                                        "lever", "led",   "slide"};
inline const StringArray toggleStyleNames{"Pill",  "Rocker",  "Push button",
                                          "Lever", "LED key", "Slide"};
inline const StringArray &styleIds(const String &type) {
  static const StringArray none;
  return type == "knob"     ? knobStyleIds
         : type == "slider" ? sliderStyleIds
         : type == "toggle" ? toggleStyleIds
                            : none;
}
inline const StringArray &styleNames(const String &type) {
  static const StringArray none;
  return type == "knob"     ? knobStyleNames
         : type == "slider" ? sliderStyleNames
         : type == "toggle" ? toggleStyleNames
                            : none;
}
// Control types: knob, slider, toggle, label (text) and logo (the theme logo
// image as a freely placed element).
struct Control {
  String type = "knob", binding = "gain", text = "Volume", image;
  Rectangle<int> bounds{60, 300, 100, 120};
  String font = "Georgia";
  float fontSize = 32.f;
  String align = "centre"; // left, centre, right (labels)
  String colour;           // optional eight-digit ARGB; empty = theme text
  float spacing = 0.f;     // extra letter spacing for labels, 0..1
  String style; // knob/slider/toggle style; empty = default (theme knob style)
};
inline bool validHexColour(const String &s) {
  return s.length() == 8 && s.containsOnly("0123456789abcdefABCDEF");
}
struct Theme {
  String name = "Royal Amber", author = "", background, logo, fit = "cover";
  // header: artwork fills the top area with a wave-shaped lower edge;
  // full: artwork fills the complete 1000 x 490 design area.
  String area = "header";
  String knobStyle = "classic"; // classic, minimal
  Colour base{0xff0a1640}, accent{0xffff8a2b}, ink{0xfff4ecd9};
  std::vector<Control> controls;
  static Control label(String text, Rectangle<int> r, float size,
                       float spacing = 0) {
    Control c;
    c.type = "label";
    c.binding = "";
    c.text = text;
    c.bounds = r;
    c.fontSize = size;
    c.spacing = spacing;
    return c;
  }
  Theme() {
    Control logoElement;
    logoElement.type = "logo";
    logoElement.binding = "";
    logoElement.text = "Logo";
    logoElement.bounds = {470, 40, 120, 120};
    controls.push_back(logoElement);
    controls.push_back(label("{instrument}", {40, 50, 400, 82}, 58));
    controls.push_back(label("{maker}", {40, 136, 400, 30}, 19, .55f));
    const StringArray ids{"attack",  "decay",  "sustain",
                          "release", "cutoff", "reverb"};
    const int centres[] = {125, 270, 415, 560, 725, 875};
    for (int i = 0; i < ids.size(); ++i) {
      Control c;
      c.binding = ids[i];
      c.text = ids[i].substring(0, 1).toUpperCase() + ids[i].substring(1);
      c.bounds = {centres[i] - 60, 262, 120, 158};
      controls.push_back(c);
    }
  }
  var toVar() const {
    auto v = object();
    put(v, "name", name);
    put(v, "author", author);
    put(v, "background", background);
    put(v, "logo", logo);
    put(v, "fit", fit);
    put(v, "area", area);
    put(v, "knobStyle", knobStyle);
    put(v, "base", base.toString());
    put(v, "accent", accent.toString());
    put(v, "ink", ink.toString());
    Array<var> a;
    for (const auto &c : controls) {
      auto x = object();
      put(x, "type", c.type);
      put(x, "binding", c.binding);
      put(x, "text", c.text);
      put(x, "image", c.image);
      put(x, "font", c.font);
      put(x, "fontSize", c.fontSize);
      put(x, "align", c.align);
      put(x, "colour", c.colour);
      put(x, "spacing", c.spacing);
      put(x, "style", c.style);
      put(x, "x", c.bounds.getX());
      put(x, "y", c.bounds.getY());
      put(x, "w", c.bounds.getWidth());
      put(x, "h", c.bounds.getHeight());
      a.add(x);
    }
    put(v, "controls", a);
    return v;
  }
  static Theme fromVar(const var &v) {
    require(v.isObject(), "Missing template design");
    Theme t;
    t.controls.clear();
    t.name = v["name"].toString();
    t.author = v["author"].toString();
    t.background = v["background"].toString();
    t.logo = v["logo"].toString();
    t.fit = v["fit"].toString();
    require(t.fit == "cover" || t.fit == "contain" || t.fit == "stretch",
            "Unknown background scaling mode");
    // Older templates had no wave header; keep their artwork full size.
    t.area = v["area"].isVoid() ? "full" : v["area"].toString();
    require(t.area == "header" || t.area == "full", "Unknown artwork area");
    t.knobStyle =
        v["knobStyle"].isVoid() ? "classic" : v["knobStyle"].toString();
    require(knobStyleIds.contains(t.knobStyle), "Unknown knob style");
    for (auto key : {"base", "accent", "ink"})
      require(validHexColour(v[key].toString()), "Invalid theme colour");
    t.base = Colour::fromString(v["base"].toString());
    t.accent = Colour::fromString(v["accent"].toString());
    t.ink = Colour::fromString(v["ink"].toString());
    auto *a = v["controls"].getArray();
    require(a && a->size() <= 64,
            "Template has too many controls or no control array");
    for (auto x : *a) {
      Control c;
      c.type = x["type"].toString();
      c.binding = x["binding"].toString();
      c.text = x["text"].toString();
      c.image = x["image"].toString();
      c.font = x["font"].isVoid() ? "Georgia" : x["font"].toString();
      c.fontSize = x["fontSize"].isVoid() ? 32.f : float(x["fontSize"]);
      require(std::isfinite(c.fontSize) && c.fontSize >= 8 && c.fontSize <= 120,
              "Invalid font size");
      c.align = x["align"].isVoid() ? "centre" : x["align"].toString();
      require(c.align == "left" || c.align == "centre" || c.align == "right",
              "Invalid text alignment");
      c.colour = x["colour"].toString();
      require(c.colour.isEmpty() || validHexColour(c.colour),
              "Invalid element colour");
      c.spacing = x["spacing"].isVoid() ? 0.f : float(x["spacing"]);
      require(std::isfinite(c.spacing) && c.spacing >= 0 && c.spacing <= 1,
              "Invalid letter spacing");
      c.style = x["style"].toString();
      require(c.type != "toggle" || c.binding == "enabled",
              "Switches must bind to enabled");
      require(c.type == "knob" || c.type == "slider" || c.type == "toggle" ||
                  c.type == "label" || c.type == "logo",
              "Unsupported control type");
      require(c.type == "label" || c.type == "logo" ||
                  parameterIDs.contains(c.binding),
              "Unknown parameter binding");
      require(c.style.isEmpty() || styleIds(c.type).contains(c.style),
              "Unknown control style");
      c.bounds = {int(x["x"]), int(x["y"]), int(x["w"]), int(x["h"])};
      require(c.bounds.getWidth() >= 30 && c.bounds.getHeight() >= 24 &&
                  Rectangle<int>(0, 0, 1000, 490).contains(c.bounds),
              "Control outside the 1000 x 490 design area");
      t.controls.push_back(c);
    }
    return t;
  }
};
struct Zone {
  String asset;
  int low = 60, high = 60, root = 60, velLow = 1, velHigh = 127;
  String file; // original sample file name, for display only
};
struct Document {
  String name = "My instrument", maker = "Artilody", uuid = Uuid().toString();
  Theme theme;
  std::vector<Zone> zones;
  std::map<String, MemoryBlock> assets;
  std::map<String, float> defaults;
  Document() {
    for (const auto &p : parameters())
      defaults[p.id] = p.initial;
  }
  BigInteger coverage() const {
    BigInteger bits;
    for (auto z : zones)
      bits.setRange(z.low, z.high - z.low + 1, true);
    return bits;
  }
  var toVar(bool themeOnly = false) const {
    auto v = object();
    put(v, "format", themeOnly ? "artilody.theme" : "artilody.plugweaver");
    put(v, "version", 1);
    put(v, "theme", theme.toVar());
    if (themeOnly)
      return v;
    put(v, "name", name);
    put(v, "maker", maker);
    put(v, "uuid", uuid);
    Array<var> a;
    for (auto z : zones) {
      auto x = object();
      put(x, "asset", z.asset);
      put(x, "low", z.low);
      put(x, "high", z.high);
      put(x, "root", z.root);
      put(x, "velLow", z.velLow);
      put(x, "velHigh", z.velHigh);
      put(x, "file", z.file);
      a.add(x);
    }
    put(v, "zones", a);
    auto d = object();
    for (auto [k, x] : defaults)
      put(d, k, x);
    put(v, "defaults", d);
    return v;
  }
  void applyVar(const var &v, bool themeOnly = false) {
    require(v["format"].toString() ==
                    (themeOnly ? "artilody.theme" : "artilody.plugweaver") &&
                int(v["version"]) == 1,
            "Unsupported file format/version");
    theme = Theme::fromVar(v["theme"]);
    if (themeOnly)
      return;
    name = v["name"].toString();
    maker = v["maker"].toString();
    uuid = v["uuid"].toString();
    require(name.isNotEmpty() && uuid.isNotEmpty(),
            "Missing instrument name/identity");
    auto *a = v["zones"].getArray();
    require(a && a->size() <= 256, "Invalid zone list");
    zones.clear();
    for (auto x : *a) {
      Zone z{x["asset"].toString(), int(x["low"]),    int(x["high"]),
             int(x["root"]),        int(x["velLow"]), int(x["velHigh"]),
             x["file"].toString()};
      require(z.low >= 0 && z.high <= 127 && z.low <= z.high && z.root >= 0 &&
                  z.root <= 127 && z.velLow >= 1 && z.velHigh <= 127 &&
                  z.velLow <= z.velHigh,
              "Invalid MIDI range");
      zones.push_back(z);
    }
    for (const auto &p : parameters()) {
      auto x = v["defaults"][Identifier(p.id)];
      float f = x.isVoid() ? p.initial : float(x);
      require(std::isfinite(f) && f >= p.min && f <= p.max,
              "Invalid parameter value");
      defaults[p.id] = f;
    }
  }
  std::set<String> requiredAssets(bool themeOnly = false) const {
    std::set<String> names;
    if (theme.background.isNotEmpty())
      names.insert(theme.background);
    if (theme.logo.isNotEmpty())
      names.insert(theme.logo);
    for (const auto &c : theme.controls)
      if (c.image.isNotEmpty())
        names.insert(c.image);
    if (!themeOnly)
      for (const auto &z : zones)
        names.insert(z.asset);
    return names;
  }
  void validateAssets(bool themeOnly = false) const {
    for (auto name : {theme.background, theme.logo})
      require(name.isEmpty() || name.startsWith("images/"),
              "Image path must be inside images/");
    for (const auto &c : theme.controls)
      require(c.image.isEmpty() || c.image.startsWith("images/"),
              "Control image path must be inside images/");
    if (!themeOnly)
      for (const auto &z : zones)
        require(z.asset.startsWith("samples/"),
                "Audio path must be inside samples/");

    for (const auto &n : requiredAssets(themeOnly))
      require(safePath(n) && assets.count(n) > 0, "Missing asset: " + n);
  }
};
inline std::map<String, MemoryBlock> readZip(InputStream &input) {
  ZipFile zip(input);
  require(zip.getNumEntries() > 0 && zip.getNumEntries() <= 400,
          "Invalid archive or too many entries");
  std::map<String, MemoryBlock> out;
  int64 total = 0;
  for (int i = 0; i < zip.getNumEntries(); ++i) {
    auto *e = zip.getEntry(i);
    require(e && !e->isSymbolicLink && safePath(e->filename),
            "Invalid archive path");
    total += e->uncompressedSize;
    require(e->uncompressedSize >= 0 && total <= 512LL * 1024 * 1024,
            "Archive exceeds 512 MB");
    require(!out.count(e->filename), "Duplicate archive entry");
    std::unique_ptr<InputStream> stream(zip.createStreamForEntry(i));
    require(bool(stream), "Cannot read archive entry");
    MemoryBlock data;
    stream->readIntoMemoryBlock(data, e->uncompressedSize);
    require(int64(data.getSize()) == e->uncompressedSize,
            "Incomplete archive entry");
    out.emplace(e->filename, std::move(data));
  }
  return out;
}
inline Document readDocument(InputStream &input, bool themeOnly = false) {
  Document d;
  d.assets = readZip(input);
  auto it = d.assets.find("document.json");
  require(it != d.assets.end(), "Missing document.json");
  require(it->second.getSize() < 1024 * 1024, "Document metadata too large");
  d.applyVar(JSON::parse(it->second.toString()), themeOnly);
  d.assets.erase(it);
  d.validateAssets(themeOnly);
  auto expected = d.requiredAssets(themeOnly);
  require(d.assets.size() == expected.size(),
          "Archive contains unexpected assets");
  return d;
}
inline Document readDocument(const File &file, bool themeOnly = false) {
  auto in = file.createInputStream();
  require(bool(in), "Cannot open file");
  return readDocument(*in, themeOnly);
}
inline void saveDocument(const Document &d, const File &file,
                         bool themeOnly = false) {
  d.validateAssets(themeOnly);
  ZipFile::Builder zip;
  auto meta = json(d.toVar(themeOnly));
  zip.addEntry(
      new MemoryInputStream(meta.toRawUTF8(), meta.getNumBytesAsUTF8(), true),
      6, "document.json", Time::getCurrentTime());
  for (const auto &n : d.requiredAssets(themeOnly)) {
    const auto &data = d.assets.at(n);
    zip.addEntry(new MemoryInputStream(data, false), 6, n,
                 Time::getCurrentTime());
  }
  TemporaryFile temp(file);
  auto out = temp.getFile().createOutputStream();
  require(bool(out) && zip.writeToStream(*out, nullptr),
          "Could not write archive");
  out->flush();
  require(out->getStatus().wasOk(), "Archive write failed");
  out.reset();
  require(temp.overwriteTargetFileWithTemporary(), "Could not commit file");
}
inline Image assetImage(const Document &d, const String &asset) {
  auto it = d.assets.find(asset);
  if (it == d.assets.end())
    return {};
  return ImageFileFormat::loadFrom(it->second.getData(), it->second.getSize());
}
inline String addImage(Document &d, const File &f) {
  require(f.getSize() > 0 && f.getSize() < 24 * 1024 * 1024,
          "Use an image under 24 MB");
  MemoryBlock b;
  require(f.loadFileAsData(b), "Cannot read image");
  auto image = ImageFileFormat::loadFrom(b.getData(), b.getSize());
  require(image.isValid() && image.getWidth() <= 8192 &&
              image.getHeight() <= 8192,
          "Use PNG/JPEG up to 8192 pixels");
  String id = "images/" + Uuid().toString() + f.getFileExtension();
  d.assets[id] = std::move(b);
  return id;
}
} // namespace pw
