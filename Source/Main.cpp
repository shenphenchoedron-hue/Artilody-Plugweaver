// SPDX-License-Identifier: GPL-3.0-only
#include "Exporter.h"
#include "SfzImport.h"
#include "Ui.h"
#include <iostream>
#include <thread>
using namespace juce;
using namespace ui;

namespace {
struct BuiltInTheme {
  const char *name;
  uint32 base, accent, ink;
  const char *knobStyle;
};
const BuiltInTheme builtInThemes[] = {
    {"Royal Amber", 0xff0a1640, 0xffff8a2b, 0xfff4ecd9, "classic"},
    {"Violet Luxe", 0xff160a3c, 0xffb46bff, 0xfff3e9ff, "classic"},
    {"Graphite Glow", 0xff14171e, 0xff8fd3ff, 0xffeef1f6, "minimal"}};

String elementName(const pw::Control &c) {
  if (c.type == "label")
    return c.text.contains("{instrument}") ? "Instrument title"
           : c.text.contains("{maker}")    ? "Maker line"
                                           : "Text";
  if (c.type == "logo")
    return "Logo";
  if (c.type == "toggle")
    return "Switch";
  for (const auto &p : pw::parameters())
    if (p.id == c.binding)
      return (c.type == "knob" ? "Knob - " : "Slider - ") + p.name;
  return c.type == "knob" ? "Knob" : "Slider";
}
String elementIcon(const pw::Control &c) {
  return c.type == "label"    ? "text"
         : c.type == "logo"   ? "logo"
         : c.type == "toggle" ? "switch"
         : c.type == "slider" ? "slider"
                              : "knob";
}
} // namespace

class Main final : public AudioAppComponent,
                   private ListBoxModel,
                   private TableListBoxModel,
                   private Timer {
  pw::Document document;
  pw::Engine engine;
  AppLook look;
  pw::PreviewHost preview{document, engine.values, engine.keyboard};
  Viewport previewViewport;
  CriticalSection audioLock;
  MidiMessageCollector midiCollector;
  std::thread worker;
  bool busy = false, dirty = false;
  int tab = 1, selected = -1, zone = -1;
  std::unique_ptr<FileChooser> chooser;
  File projectFile;
  ApplicationProperties settings;
  String statusText = "Ready - import an SFZ instrument to begin";
  std::vector<File> templates;
  std::vector<Component *> managed;
  struct Caption {
    Rectangle<int> area;
    String text;
  };
  std::vector<Caption> captions;
  struct PanelInfo {
    Rectangle<int> area;
    String title, icon;
  };
  std::vector<PanelInfo> panels;
  Rectangle<int> headerCard, instrumentInfo, footer, previewBar, exportInfo,
      inspectorHead, waveCaption;
  Image backgroundImage;
  // Text shown in paint(), computed in refresh() so painting stays cheap.
  String infoSamples, infoRange, infoCode;
  String waveAsset;

  // Header
  ComboBox projectBox;
  TextButton open{"Open project"}, save{"Save"}, build{"Build plugin"},
      audioSettings{"Audio / MIDI"};
  TextButton samplesTab{"Samples"}, soundTab{"Sound"}, designTab{"Design"},
      exportTab{"Export"};
  // Instrument panel
  Thumb instrumentThumb;
  TextButton import{"Import SFZ"};
  ListBox sampleList{"Samples", this};
  // Centre
  WaveformView waveform;
  TextButton prevZone, nextZone;
  EnvelopeView envelope;
  FilterView filterGraph;
  ComboBox filterType;
  ParamKnob attack{"attack", "Attack"}, decay{"decay", "Decay"},
      sustain{"sustain", "Sustain"}, release{"release", "Release"},
      cutoff{"cutoff", "Cutoff"}, resonance{"resonance", "Resonance"},
      reverb{"reverb", "Reverb"}, master{"gain", "Master volume"};
  TableListBox regionTable{"Regions", this};
  TextButton exportBuild{"Build plugin"}, exportSource{"Export source project"},
      openPlugins{"Show plugin folders"};
  TextEditor buildLog;
  // Plugin settings
  TextEditor instrumentName, maker;
  ToggleButton vst3{"VST3"}, au{"AU"}, clap{"CLAP"};
  TextButton buildSide{"Build plugin"};
  // Bottom keyboard
  pw::Piano piano{engine.keyboard};
  // Design: appearance
  ThemeCard themeCards[3];
  ComboBox templateBox;
  TextButton importTemplate, saveTemplate;
  Swatch baseSwatch, accentSwatch, inkSwatch, textSwatch;
  TextEditor baseHex, accentHex, inkHex, textHex;
  Thumb backgroundThumb, logoThumb;
  TextButton chooseBackground{"Choose image"}, clearBackground,
      uploadLogo{"Upload logo"}, clearLogo;
  ComboBox fitBox, areaBox;
  // Design: preview
  ComboBox zoomBox, modeBox;
  // Design: inspector
  TextEditor elementText, fontSize, spacing, x, y, w, h;
  ComboBox fontBox, bindingBox;
  TextButton alignLeft, alignCentre, alignRight, controlImage{"Control image"},
      clearImage, deleteElement, inspectorLogo{"Upload logo"};
  StyleGallery styleGallery;
  TextButton applyStyleToAll;
  Rectangle<int> snapFallback;
  TextButton addKnob{"+ Knob"}, addSlider{"+ Slider"}, addToggle{"+ Switch"},
      addLabel{"+ Text"};
  ToggleButton snap{"Snap to grid"};
  TextButton resetDesign{"Reset design"};

  //==========================================================================
  File templateFolder() {
    auto f = File::getSpecialLocation(File::userApplicationDataDirectory)
                 .getChildFile("Artilody/Plugweaver/Templates");
    f.createDirectory();
    return f;
  }
  template <typename C> C &manage(C &c) {
    addChildComponent(c);
    managed.push_back(&c);
    if (auto *e = dynamic_cast<TextEditor *>(&c)) {
      e->setJustification(Justification::centredLeft);
      e->setIndents(12, 0);
    }
    return c;
  }
  void place(Component &c, Rectangle<int> r) {
    c.setBounds(r);
    c.setVisible(true);
  }
  void caption(Rectangle<int> r, const String &t) { captions.push_back({r, t}); }
  void message(const String &s) {
    statusText = s;
    repaint();
  }
  void changed() {
    dirty = true;
    message("Unsaved changes");
  }
  void error(const String &s) {
    message(s);
    AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Plugweaver", s);
  }
  pw::Control *current() {
    return selected >= 0 && selected < int(document.theme.controls.size())
               ? &document.theme.controls[size_t(selected)]
               : nullptr;
  }
  float param(const String &id) { return engine.values.at(id).load(); }
  void setParam(const String &id, float v) {
    engine.values.at(id) = v;
    document.defaults[id] = v;
    envelope.repaint();
    filterGraph.repaint();
    if (!dirty)
      changed();
  }

  //==========================================================================
  void refresh(bool rebuild = true) {
    if (rebuild)
      preview.view.rebuild(tab == 2 && modeBox.getSelectedId() != 2);
    preview.view.snap = snap.getToggleState();
    preview.view.select(selected);
    instrumentName.setText(document.name, false);
    maker.setText(document.maker, false);
    auto &t = document.theme;
    baseSwatch.colour = t.base;
    accentSwatch.colour = t.accent;
    inkSwatch.colour = t.ink;
    baseHex.setText("#" + t.base.toDisplayString(false), false);
    accentHex.setText("#" + t.accent.toDisplayString(false), false);
    inkHex.setText("#" + t.ink.toDisplayString(false), false);
    for (int i = 0; i < 3; ++i)
      themeCards[i].selected =
          t.base == Colour(builtInThemes[i].base) &&
          t.accent == Colour(builtInThemes[i].accent) &&
          t.ink == Colour(builtInThemes[i].ink);
    updateGallery();
    fitBox.setSelectedId(t.fit == "contain" ? 2 : t.fit == "stretch" ? 3 : 1,
                         dontSendNotification);
    areaBox.setSelectedId(t.area == "full" ? 2 : 1, dontSendNotification);
    auto bg = preview.view.images.get(document, t.background),
         lg = preview.view.images.get(document, t.logo);
    backgroundThumb.image = bg;
    backgroundImage = bg;
    logoThumb.image = lg;
    instrumentThumb.image = bg.isValid() ? bg : lg;
    instrumentThumb.logo = !bg.isValid() && lg.isValid();
    for (auto &card : themeCards)
      card.art = bg;
    piano.covered = document.coverage();
    piano.repaint();
    {
      std::set<String> files;
      for (auto &z : document.zones)
        files.insert(z.asset);
      infoSamples = String(files.size()) +
                    (files.size() == 1 ? " sample" : " samples");
      int lo = piano.covered.findNextSetBit(0),
          hi = piano.covered.getHighestBit();
      infoRange = lo >= 0 ? noteName(lo) + " - " + noteName(hi)
                          : String("No samples yet");
      infoCode = pw::pluginCode(document);
    }
    piano.accent = col::accent;
    templateBox.setText(t.name, dontSendNotification);
    for (auto *b : {&samplesTab, &soundTab, &designTab, &exportTab})
      b->setToggleState(false, dontSendNotification);
    (tab == 0   ? samplesTab
     : tab == 1 ? soundTab
     : tab == 2 ? designTab
                : exportTab)
        .setToggleState(true, dontSendNotification);
    filterType.setSelectedId(roundToInt(param("filtertype")) + 1,
                             dontSendNotification);
    sampleList.updateContent();
    regionTable.updateContent();
    if (zone >= int(document.zones.size()))
      zone = -1;
    if (zone < 0 && !document.zones.empty())
      showZone(0);
    else if (document.zones.empty())
      showZone(-1);
    updateProjectBox();
    updateInspector();
    resized();
    repaint();
  }
  void updateInspector() {
    auto *c = current();
    if (!c)
      return;
    elementText.setText(c->text, false);
    bindingBox.setSelectedId(pw::parameterIDs.indexOf(c->binding) + 1,
                             dontSendNotification);
    fontBox.setText(c->font, dontSendNotification);
    fontSize.setText(String(c->fontSize, 0), false);
    spacing.setText(String(c->spacing, 2), false);
    auto colour = pw::colourOr(c->colour, document.theme.ink);
    textSwatch.colour = colour;
    textSwatch.repaint();
    textHex.setText("#" + colour.toDisplayString(false), false);
    alignLeft.setToggleState(c->align == "left", dontSendNotification);
    alignCentre.setToggleState(c->align == "centre", dontSendNotification);
    alignRight.setToggleState(c->align == "right", dontSendNotification);
    x.setText(String(c->bounds.getX()), false);
    y.setText(String(c->bounds.getY()), false);
    w.setText(String(c->bounds.getWidth()), false);
    h.setText(String(c->bounds.getHeight()), false);
  }
  void applyControl() {
    auto *c = current();
    if (!c)
      return;
    const auto previous = *c;
    c->text = elementText.getText();
    if (c->type == "label") {
      c->font = fontBox.getText().trim().isEmpty() ? String("Georgia")
                                                   : fontBox.getText().trim();
      c->fontSize = jlimit(8.f, 120.f, fontSize.getText().getFloatValue());
      c->spacing = jlimit(0.f, 1.f, spacing.getText().getFloatValue());
    }
    if (c->type == "toggle")
      c->binding = "enabled";
    else if (c->type == "knob" || c->type == "slider")
      c->binding = pw::parameterIDs[jmax(0, bindingBox.getSelectedId() - 1)];
    int ww = jlimit(30, 1000, w.getText().getIntValue()),
        hh = jlimit(24, 490, h.getText().getIntValue());
    c->bounds = {jlimit(0, 1000 - ww, x.getText().getIntValue()),
                 jlimit(0, 490 - hh, y.getText().getIntValue()), ww, hh};
    if (c->text == previous.text && c->font == previous.font &&
        std::abs(c->fontSize - previous.fontSize) < .001f &&
        std::abs(c->spacing - previous.spacing) < .001f &&
        c->binding == previous.binding && c->bounds == previous.bounds)
      return;
    changed();
    refresh();
  }
  void setAlign(const String &a) {
    if (auto *c = current()) {
      c->align = a;
      changed();
      refresh();
    }
  }
  static bool parseHex(String s, Colour &out) {
    s = s.trim().removeCharacters("#");
    if (s.length() != 6 || !s.containsOnly("0123456789abcdefABCDEF"))
      return false;
    out = Colour::fromString("ff" + s);
    return true;
  }
  // Style shown/selected in the gallery for the current selection.
  void updateGallery() {
    auto *c = current();
    auto &t = document.theme;
    styleGallery.accent = t.accent;
    styleGallery.ink = t.ink;
    if (c && (c->type == "knob" || c->type == "slider" || c->type == "toggle")) {
      styleGallery.type = c->type;
      styleGallery.current = c->style.isNotEmpty() ? c->style
                             : c->type == "knob"   ? t.knobStyle
                             : c->type == "toggle" ? String("pill")
                                                   : String("classic");
    } else {
      styleGallery.type = "knob";
      styleGallery.current = t.knobStyle;
    }
    styleGallery.repaint();
  }
  // Applies a style; switching a slider to/from vertical turns its box so
  // the new orientation has room.
  static void setControlStyle(pw::Control &c, const String &id) {
    bool wasVertical = c.style == "vertical", vertical = id == "vertical";
    c.style = id;
    if (c.type != "slider" || wasVertical == vertical)
      return;
    auto r = c.bounds;
    int w = vertical ? 100 : 220, h = vertical ? 220 : 90;
    w = jmin(w, 1000);
    h = jmin(h, 490);
    c.bounds = Rectangle<int>(jlimit(0, 1000 - w, r.getCentreX() - w / 2),
                              jlimit(0, 490 - h, r.getCentreY() - h / 2), w, h);
  }
  void pickStyle(const String &id) {
    if (auto *c = current()) {
      if (c->type != styleGallery.type)
        return;
      setControlStyle(*c, id);
    } else {
      document.theme.knobStyle = id;
      for (auto &other : document.theme.controls)
        if (other.type == "knob")
          other.style.clear();
    }
    changed();
    refresh();
  }
  void chooseTab(int n) {
    tab = n;
    refresh();
  }
  void add(const String &type) {
    if (document.theme.controls.size() >= 64) {
      error("Maximum 64 elements");
      return;
    }
    pw::Control c;
    c.type = type;
    c.binding = type == "toggle"  ? "enabled"
                : type == "label" ? ""
                                  : "gain";
    c.text = type == "label"    ? "Your text"
             : type == "toggle" ? "Enabled"
                                : "Volume";
    if (type == "label")
      c.fontSize = 28;
    c.bounds = {80, 250, type == "label" ? 300 : 120,
                type == "label" ? 60 : (type == "slider" ? 90 : 158)};
    document.theme.controls.push_back(c);
    selected = int(document.theme.controls.size()) - 1;
    if (modeBox.getSelectedId() == 2)
      modeBox.setSelectedId(1, dontSendNotification);
    changed();
    refresh();
  }
  void applyBuiltIn(int i) {
    auto &t = document.theme;
    t.base = Colour(builtInThemes[i].base);
    t.accent = Colour(builtInThemes[i].accent);
    t.ink = Colour(builtInThemes[i].ink);
    t.knobStyle = builtInThemes[i].knobStyle;
    t.name = builtInThemes[i].name;
    changed();
    refresh();
  }
  void scanTemplates() {
    templates.clear();
    templateBox.clear(dontSendNotification);
    templateBox.addItem("Royal Amber (built-in layout)", 1);
    for (auto f : templateFolder().findChildFiles(File::findFiles, false,
                                                  "*.artitheme")) {
      templates.push_back(f);
      templateBox.addItem(f.getFileNameWithoutExtension(),
                          int(templates.size()) + 1);
    }
    templateBox.setText(document.theme.name, dontSendNotification);
  }
  void applyTheme(pw::Document theme) {
    for (auto &[key, value] : theme.assets)
      document.assets[key] = std::move(value);
    document.theme = std::move(theme.theme);
    selected = -1;
    changed();
    refresh();
  }

  //== Recent projects ========================================================
  StringArray recentProjects() {
    StringArray list;
    list.addTokens(settings.getUserSettings()->getValue("recent"), "\n", "");
    list.removeEmptyStrings();
    StringArray existing;
    for (auto &s : list)
      if (File(s).existsAsFile())
        existing.add(s);
    return existing;
  }
  void remember(const File &f) {
    auto list = recentProjects();
    list.removeString(f.getFullPathName());
    list.insert(0, f.getFullPathName());
    while (list.size() > 8)
      list.remove(list.size() - 1);
    settings.getUserSettings()->setValue("recent", list.joinIntoString("\n"));
    settings.saveIfNeeded();
  }
  void updateProjectBox() {
    projectBox.clear(dontSendNotification);
    projectBox.addItem(document.name, 1);
    auto recent = recentProjects();
    if (!recent.isEmpty()) {
      projectBox.addSectionHeading("Recent projects");
      for (int i = 0; i < recent.size(); ++i)
        projectBox.addItem(File(recent[i]).getFileNameWithoutExtension(),
                           100 + i);
    }
    projectBox.addSeparator();
    projectBox.addItem("New instrument", 2);
    projectBox.addItem("Open project...", 3);
    projectBox.setSelectedId(1, dontSendNotification);
  }

  //== Background work =======================================================
  void run(std::function<std::function<void()>()> job) {
    if (busy)
      return;
    if (worker.joinable())
      worker.join();
    busy = true;
    setEnabled(false);
    Component::SafePointer<Main> safe(this);
    worker = std::thread([safe, job] {
      try {
        auto finish = job();
        MessageManager::callAsync([safe, finish] {
          if (safe) {
            safe->busy = false;
            safe->setEnabled(true);
            finish();
          }
        });
      } catch (const std::exception &e) {
        String s = e.what();
        MessageManager::callAsync([safe, s] {
          if (safe) {
            safe->busy = false;
            safe->setEnabled(true);
            safe->error(s);
          }
        });
      }
    });
  }
  void appendLog(const String &text) {
    Component::SafePointer<Main> safe(this);
    MessageManager::callAsync([safe, text] {
      if (safe) {
        safe->buildLog.moveCaretToEnd();
        safe->buildLog.insertTextAtCaret(text);
      }
    });
  }
  void replaceDocument(pw::Document d,
                       const pw::Engine::Prepared &sounds = {}) {
    {
      const ScopedLock lock(audioLock);
      engine.install(d, sounds);
      document = std::move(d);
    }
    selected = -1;
    zone = -1;
    waveAsset.clear();
    dirty = false;
    refresh();
  }
  void confirmDiscard(std::function<void()> action) {
    if (!dirty) {
      action();
      return;
    }
    Component::SafePointer<Main> safe(this);
    AlertWindow::showOkCancelBox(
        AlertWindow::WarningIcon, "Unsaved project",
        "Discard current changes? Save the project first if you want to keep "
        "them.",
        "Discard", "Cancel", this,
        ModalCallbackFunction::create([safe, action](int result) {
          if (safe && result)
            action();
        }));
  }

  //== Samples ===============================================================
  void showZone(int i) {
    zone = i;
    if (i < 0 || i >= int(document.zones.size())) {
      zone = -1;
      waveform.setAudio(nullptr, 44100);
      waveform.caption.clear();
      sampleList.deselectAllRows();
      repaint();
      return;
    }
    const auto &z = document.zones[size_t(i)];
    if (z.asset != waveAsset) {
      waveAsset = z.asset;
      // Reuse the audio the engine already decoded instead of decoding the
      // sample again on the message thread.
      std::shared_ptr<pw::SampleData> sample;
      {
        const ScopedLock lock(audioLock);
        for (int s = 0; s < engine.synth.getNumSounds() && !sample; ++s)
          if (auto *sound =
                  dynamic_cast<pw::Sound *>(engine.synth.getSound(s).get()))
            if (sound->zone.asset == z.asset)
              sample = sound->sample;
      }
      waveform.setAudio(sample ? &sample->data : nullptr,
                        sample ? sample->rate : 44100);
    }
    waveform.caption = noteName(z.root) + "  -  " + zoneFile(z);
    if (sampleList.getSelectedRow() != i)
      sampleList.selectRow(i, true, true);
    if (regionTable.getSelectedRow() != i)
      regionTable.selectRow(i, true, true);
    repaint();
  }
  static String zoneFile(const pw::Zone &z) {
    return z.file.isNotEmpty() ? z.file
                               : z.asset.fromFirstOccurrenceOf("/", false, false);
  }
  void audition(int note) {
    engine.keyboard.noteOn(1, note, .8f);
    Component::SafePointer<Main> safe(this);
    Timer::callAfterDelay(700, [safe, note] {
      if (safe)
        safe->engine.keyboard.noteOff(1, note, 0);
    });
  }
  void removeZone(int i) {
    if (i < 0 || i >= int(document.zones.size()) || document.zones.size() < 2) {
      error("An instrument needs at least one sample region");
      return;
    }
    auto snapshot = document;
    snapshot.zones.erase(snapshot.zones.begin() + i);
    std::set<String> used;
    for (auto &z : snapshot.zones)
      used.insert(z.asset);
    for (auto it = snapshot.assets.begin(); it != snapshot.assets.end();)
      if (it->first.startsWith("samples/") && !used.count(it->first))
        it = snapshot.assets.erase(it);
      else
        ++it;
    run([this, snapshot]() mutable {
      auto prepared = pw::Engine::decode(snapshot);
      return [this, snapshot, prepared]() mutable {
        replaceDocument(std::move(snapshot), prepared);
        dirty = true;
        message("Region removed");
      };
    });
  }
  // ListBoxModel (sample list)
  int getNumRows() override { return int(document.zones.size()); }
  void paintListBoxItem(int row, Graphics &g, int width, int height,
                        bool rowSelected) override {
    if (row < 0 || row >= int(document.zones.size()))
      return;
    const auto &z = document.zones[size_t(row)];
    auto r = Rectangle<int>(0, 0, width, height);
    if (rowSelected) {
      g.setGradientFill(ColourGradient(Colour(0xff2a5bff), 0, 0,
                                       Colour(0xff1d3fc4), float(width), 0,
                                       false));
      g.fillRoundedRectangle(r.toFloat().reduced(2, 1), 6);
      drawIcon(g, "play", r.removeFromLeft(30).toFloat().withSizeKeepingCentre(12, 12),
               col::text);
    } else {
      r.removeFromLeft(30);
      g.setColour(col::panelEdge.withAlpha(.3f));
      g.fillRect(8, height - 1, width - 16, 1);
    }
    g.setColour(col::text);
    g.setFont(sans(15.f, rowSelected));
    String note = z.low == z.high ? noteName(z.root)
                                  : noteName(z.low) + "-" + noteName(z.high);
    g.drawText(note, r.removeFromLeft(72), Justification::centredLeft);
    drawIcon(g, "dots", r.removeFromRight(30).toFloat().withSizeKeepingCentre(16, 16),
             col::muted);
    g.setFont(sans(15.f));
    g.drawText(zoneFile(z), r, Justification::centredLeft);
  }
  void selectedRowsChanged(int row) override {
    if (row >= 0 && row != zone)
      showZone(row);
  }
  void listBoxItemClicked(int row, const MouseEvent &e) override {
    if (row < 0 || row >= int(document.zones.size()))
      return;
    if (e.x > sampleList.getWidth() - 40) {
      PopupMenu m;
      m.addItem(1, "Play " + noteName(document.zones[size_t(row)].root));
      m.addItem(2, "Remove region", document.zones.size() > 1);
      Component::SafePointer<Main> safe(this);
      m.showMenuAsync(PopupMenu::Options(), [safe, row](int r) {
        if (!safe)
          return;
        if (r == 1)
          safe->audition(safe->document.zones[size_t(row)].root);
        if (r == 2)
          safe->removeZone(row);
      });
      return;
    }
    showZone(row);
    audition(document.zones[size_t(row)].root);
  }
  // TableListBoxModel (region table)
  void paintRowBackground(Graphics &g, int, int width, int height,
                          bool rowSelected) override {
    if (rowSelected) {
      g.setColour(col::selection.withAlpha(.85f));
      g.fillRoundedRectangle(0, 1, float(width), float(height - 2), 5);
    } else {
      g.setColour(col::panelEdge.withAlpha(.3f));
      g.fillRect(0, height - 1, width, 1);
    }
  }
  void paintCell(Graphics &g, int row, int column, int width, int height,
                 bool) override {
    if (row < 0 || row >= int(document.zones.size()))
      return;
    const auto &z = document.zones[size_t(row)];
    String t = column == 1   ? zoneFile(z)
               : column == 2 ? noteName(z.root)
               : column == 3 ? noteName(z.low)
               : column == 4 ? noteName(z.high)
                             : String(z.velLow) + " - " + String(z.velHigh);
    g.setColour(col::text);
    g.setFont(sans(15.f));
    g.drawText(t, 10, 0, width - 12, height, Justification::centredLeft);
  }
  void cellClicked(int row, int, const MouseEvent &) override {
    if (row >= 0 && row < int(document.zones.size())) {
      showZone(row);
      audition(document.zones[size_t(row)].root);
    }
  }

  //== Files ===================================================================
  enum Action {
    Import,
    Open,
    Save,
    ThemeOpen,
    ThemeSave,
    Background,
    Logo,
    KnobImage,
    Export,
    Build
  };
  void choose(Action action) {
    if (busy)
      return;
    if (action == Build) {
      buildPlugins();
      return;
    }
    if (action == Open) {
      confirmDiscard([this] { chooseActual(Open); });
      return;
    }
    if (action == Save && projectFile.existsAsFile()) {
      handleFile(Save, projectFile);
      return;
    }
    chooseActual(action);
  }
  void chooseActual(Action action) {
    bool folder = action == Export || action == Build;
    bool saving = action == Save || action == ThemeSave;
    String extension = action == Import                   ? "*.sfz"
                       : action == Open || action == Save ? "*.artiplug"
                       : action == ThemeOpen || action == ThemeSave
                           ? "*.artitheme"
                       : folder ? ""
                                : "*.png;*.jpg;*.jpeg";
    File initial = File::getSpecialLocation(File::userDocumentsDirectory);
    if (action == Save)
      initial = initial.getChildFile(
          File::createLegalFileName(document.name) + ".artiplug");
    if (action == ThemeSave)
      initial = initial.getChildFile("Design.artitheme");
    chooser = std::make_unique<FileChooser>(
        folder   ? "Choose a parent folder for the instrument project"
        : saving ? "Save file"
                 : "Choose file",
        initial, extension);
    Component::SafePointer<Main> safe(this);
    chooser->launchAsync(
        (saving ? FileBrowserComponent::saveMode |
                      FileBrowserComponent::warnAboutOverwriting
                : FileBrowserComponent::openMode) |
            (folder ? FileBrowserComponent::canSelectDirectories
                    : FileBrowserComponent::canSelectFiles),
        [safe, action, saving](const FileChooser &fc) {
          if (!safe || fc.getResult() == File{})
            return;
          auto file = fc.getResult();
          if (saving) {
            auto target = file.withFileExtension(action == Save ? "artiplug"
                                                                : "artitheme");
            if (target != file && target.existsAsFile()) {
              AlertWindow::showOkCancelBox(
                  AlertWindow::WarningIcon, "Replace file?",
                  target.getFileName(), "Replace", "Cancel",
                  safe.getComponent(),
                  ModalCallbackFunction::create(
                      [safe, target, action](int yes) {
                        if (safe && yes)
                          safe->handleFile(action, target);
                      }));
              return;
            }
            file = target;
          }
          safe->handleFile(action, file);
        });
  }
  void openProject(const File &file) {
    auto snapshot = document;
    message("Reading instrument...");
    run([this, snapshot, file]() mutable {
      snapshot = pw::readDocument(file);
      auto prepared = pw::Engine::decode(snapshot);
      return [this, snapshot, prepared, file]() mutable {
        replaceDocument(std::move(snapshot), prepared);
        projectFile = file;
        remember(file);
        refresh(false);
        message("Opened " + file.getFileName());
      };
    });
  }
  void handleFile(Action action, File file) {
    try {
      if (action == Background || action == Logo || action == KnobImage) {
        auto asset = pw::addImage(document, file);
        if (action == Background)
          document.theme.background = asset;
        else if (action == Logo) {
          document.theme.logo = asset;
          bool hasLogo = false;
          for (auto &c : document.theme.controls)
            hasLogo = hasLogo || c.type == "logo";
          // Without room for another element the logo uses the classic
          // fixed placement, keeping the theme within the 64-element limit.
          if (!hasLogo && document.theme.controls.size() < 64) {
            pw::Control c;
            c.type = "logo";
            c.binding = "";
            c.text = "Logo";
            c.bounds = {470, 40, 120, 120};
            document.theme.controls.push_back(c);
          }
        } else if (auto *c = current())
          c->image = asset;
        changed();
        refresh();
        return;
      }
      if (action == ThemeOpen) {
        auto d = pw::readDocument(file, true);
        auto dest = templateFolder().getNonexistentChildFile(
            file.getFileNameWithoutExtension(), ".artitheme", false);
        pw::saveDocument(d, dest, true);
        applyTheme(std::move(d));
        scanTemplates();
        message("Template imported into your gallery");
        return;
      }
      if (action == ThemeSave) {
        document.theme.name = file.getFileNameWithoutExtension();
        pw::saveDocument(document, file, true);
        auto dest = templateFolder().getNonexistentChildFile(
            file.getFileNameWithoutExtension(), ".artitheme", false);
        pw::require(file.copyFileTo(dest), "Could not add template to gallery");
        scanTemplates();
        message("Template saved - images included, samples excluded");
        return;
      }
      if (action == Save) {
        auto snapshot = document;
        message("Saving self-contained project...");
        run([this, snapshot, file] {
          pw::saveDocument(snapshot, file);
          return [this, file] {
            projectFile = file;
            dirty = false;
            remember(file);
            updateProjectBox();
            message("Project saved: " + file.getFileName());
          };
        });
        return;
      }
      if (action == Open) {
        openProject(file);
        return;
      }
      if (action == Import) {
        auto snapshot = document;
        message("Reading instrument...");
        run([this, snapshot, file]() mutable {
          pw::importSfz(snapshot, file);
          // Decode before committing, keeping a failed import from replacing
          // the current instrument.
          auto prepared = pw::Engine::decode(snapshot);
          return [this, snapshot, prepared]() mutable {
            replaceDocument(std::move(snapshot), prepared);
            dirty = true;
            projectFile = File{};
            message("Loaded " + String(document.zones.size()) + " regions");
          };
        });
        return;
      }
      if (action == Export)
        writeSourceProject(file);
    } catch (const std::exception &e) {
      error(e.what());
    }
  }
  // Builds the selected plugin formats from the engine built into Plugweaver
  // and installs them. Nothing has to be installed on the user's machine.
  void buildPlugins() {
    auto snapshot = document;
    bool v = vst3.getToggleState(), a = au.getToggleState(),
         c = clap.getToggleState();
    try {
      pw::validateForExport(snapshot, v, a, c);
    } catch (const std::exception &e) {
      error(e.what());
      return;
    }
    buildLog.clear();
    tab = 3;
    refresh(false);
    message("Building plugin...");
    run([this, snapshot, v, a, c] {
      auto log = [this](const String &line) { appendLog(line); };
      auto engineArchive = pw::openEngineArchive();
      pw::require(engineArchive != nullptr,
                  "The built-in plugin engine is missing. Reinstall "
                  "Artilody Plugweaver.");
      auto stage = File::getSpecialLocation(File::tempDirectory)
                       .getChildFile("Plugweaver-build")
                       .getChildFile(Uuid().toDashedString());
      appendLog("Building " + snapshot.name + "...\n");
      auto built =
          pw::createPlugins(snapshot, v, a, c, *engineArchive, stage, log);
      std::vector<File> installed;
      try {
        installed = pw::installPlugins(built, log);
      } catch (...) {
        stage.deleteRecursively();
        throw;
      }
      stage.deleteRecursively();
      return [this, installed] {
        message("Plugin installed - quit and reopen your DAW, then rescan "
                "plugins");
        appendLog("\nDone. Restart your DAW and rescan plugins.\n");
        if (!installed.empty())
          installed.front().revealToUser();
      };
    });
  }
  // Writes a complete C++/CMake project for developers. Building it needs a
  // compiler and CMake; Build plugin does not.
  void writeSourceProject(File parent) {
    auto snapshot = document;
    bool v = vst3.getToggleState(), a = au.getToggleState(),
         c = clap.getToggleState();
    buildLog.clear();
    tab = 3;
    refresh(false);
    message("Exporting source project...");
    run([this, snapshot, parent, v, a, c] {
      auto dir = pw::exportInstrument(snapshot, parent, v, a, c);
      appendLog("Source project: " + dir.getFullPathName() +
                "\nBuild it with the build script for your system "
                "(needs a C++ compiler and CMake).\n");
      return [this, dir] {
        message("Source project exported");
        dir.revealToUser();
      };
    });
  }

  //== Timer: keep editor controls in sync with the sound engine ==============
  void timerCallback() override {
    for (auto *k : {&attack, &decay, &sustain, &release, &cutoff, &resonance,
                    &reverb, &master})
      k->setValue(param(k->id));
    int type = roundToInt(param("filtertype")) + 1;
    if (filterType.getSelectedId() != type)
      filterType.setSelectedId(type, dontSendNotification);
  }

public:
  Main() {
    settings.setStorageParameters([] {
      PropertiesFile::Options o;
      o.applicationName = "Plugweaver";
      o.folderName = "Artilody/Plugweaver";
      o.filenameSuffix = ".settings";
      o.osxLibrarySubFolder = "Application Support";
      return o;
    }());
    setLookAndFeel(&look);
    LookAndFeel::setDefaultLookAndFeel(&look);
    // Header
    for (auto *b : {&open, &save, &build, &audioSettings, &samplesTab,
                    &soundTab, &designTab, &exportTab})
      manage(*b);
    manage(projectBox);
    projectBox.getProperties().set("icon", "folder");
    style(open, "", "folder");
    style(save, "", "save");
    style(build, "primary", "cube");
    style(audioSettings, "flat", "speaker");
    style(samplesTab, "tab", "layers");
    style(soundTab, "tab", "wave");
    style(designTab, "tab", "palette");
    style(exportTab, "tab", "cube");
    // Instrument panel
    manage(instrumentThumb);
    instrumentThumb.placeholder = "";
    manage(import);
    style(import, "blue", "upload");
    manage(sampleList);
    sampleList.setRowHeight(34);
    sampleList.setColour(ListBox::backgroundColourId,
                         Colours::transparentBlack);
    // Centre
    manage(waveform);
    for (auto *b : {&prevZone, &nextZone})
      manage(*b);
    style(prevZone, "", "left");
    style(nextZone, "", "right");
    manage(envelope);
    manage(filterGraph);
    for (auto *g : std::initializer_list<GraphBase *>{&envelope, &filterGraph}) {
      g->get = [this](const String &id) { return param(id); };
      g->set = [this](const String &id, float v) { setParam(id, v); };
    }
    manage(filterType);
    filterType.addItemList(pw::filterTypes, 1);
    filterType.onChange = [this] {
      setParam("filtertype", float(filterType.getSelectedId() - 1));
    };
    for (auto *k : {&attack, &decay, &sustain, &release, &cutoff, &resonance,
                    &reverb, &master}) {
      manage(*k);
      k->onChange = [this](const String &id, float v) { setParam(id, v); };
    }
    master.large = true;
    manage(regionTable);
    regionTable.setRowHeight(30);
    regionTable.setColour(ListBox::backgroundColourId,
                          Colours::transparentBlack);
    auto &header = regionTable.getHeader();
    header.addColumn("Sample", 1, 260, 80, 800, TableHeaderComponent::notSortable);
    header.addColumn("Root", 2, 80, 50, 140, TableHeaderComponent::notSortable);
    header.addColumn("Low key", 3, 80, 50, 140, TableHeaderComponent::notSortable);
    header.addColumn("High key", 4, 80, 50, 140, TableHeaderComponent::notSortable);
    header.addColumn("Velocity", 5, 90, 50, 160, TableHeaderComponent::notSortable);
    header.setStretchToFitActive(true);
    regionTable.setHeaderHeight(30);
    for (auto *b : {&exportBuild, &exportSource, &openPlugins})
      manage(*b);
    style(exportBuild, "primary", "cube");
    style(exportSource, "", "folder");
    style(openPlugins, "", "folder");
    manage(buildLog);
    buildLog.setMultiLine(true);
    buildLog.setJustification(Justification::topLeft);
    buildLog.setIndents(12, 10);
    buildLog.setTextToShowWhenEmpty(
        "Build output appears here. Build plugin creates and installs the "
        "selected formats in a few seconds - no other software is needed.",
        col::dim);
    buildLog.setReadOnly(true);
    buildLog.setScrollbarsShown(true);
    buildLog.setFont(Font(Font::getDefaultMonospacedFontName(), 12.f,
                          Font::plain));
    buildLog.setColour(TextEditor::textColourId, col::muted);
    // Plugin settings
    for (auto *e : {&instrumentName, &maker})
      manage(*e).setFont(sans(16.f));
    for (auto *t : {&vst3, &au, &clap}) {
      manage(*t);
      t->setToggleState(true, dontSendNotification);
      t->getProperties().set("style", "switch");
    }
    vst3.getProperties().set("icon", "vst3");
    au.getProperties().set("icon", "au");
    clap.getProperties().set("icon", "clap");
#if !JUCE_MAC
    au.setToggleState(false, dontSendNotification);
    au.setEnabled(false);
#endif
    manage(buildSide);
    style(buildSide, "primary", "cube");
    // Keyboard
    manage(piano);
    piano.labelsBelow = true;
    // Design appearance
    for (int i = 0; i < 3; ++i) {
      manage(themeCards[i]);
      themeCards[i].name = builtInThemes[i].name;
      themeCards[i].base = Colour(builtInThemes[i].base);
      themeCards[i].accent = Colour(builtInThemes[i].accent);
      themeCards[i].ink = Colour(builtInThemes[i].ink);
      themeCards[i].onClick = [this, i] { applyBuiltIn(i); };
    }
    manage(templateBox);
    for (auto *b : {&importTemplate, &saveTemplate, &chooseBackground,
                    &clearBackground, &uploadLogo, &clearLogo})
      manage(*b);
    style(importTemplate, "", "import");
    style(saveTemplate, "", "save");
    importTemplate.setTooltip("Import template");
    saveTemplate.setTooltip("Save design as template");
    style(chooseBackground, "", "image");
    style(uploadLogo, "", "upload");
    style(clearBackground, "flat", "trash");
    style(clearLogo, "flat", "trash");
    for (auto *s : {&baseSwatch, &accentSwatch, &inkSwatch, &textSwatch})
      manage(*s);
    for (auto *e : {&baseHex, &accentHex, &inkHex, &textHex})
      manage(*e).setFont(sans(15.f));
    manage(backgroundThumb);
    manage(logoThumb);
    backgroundThumb.placeholder = "No background";
    logoThumb.placeholder = "No logo";
    logoThumb.logo = true;
    backgroundThumb.onClick = [this] { choose(Background); };
    logoThumb.onClick = [this] { choose(Logo); };
    manage(fitBox);
    fitBox.addItem("Fill / crop", 1);
    fitBox.addItem("Fit inside", 2);
    fitBox.addItem("Stretch", 3);
    manage(areaBox);
    areaBox.addItem("Header wave", 1);
    areaBox.addItem("Full design area", 2);
    // Design preview
    manage(zoomBox);
    zoomBox.addItem("Fit", 1);
    zoomBox.addItem("75%", 2);
    zoomBox.addItem("100%", 3);
    zoomBox.addItem("125%", 4);
    zoomBox.setSelectedId(1, dontSendNotification);
    manage(modeBox);
    modeBox.addItem("Edit layout", 1);
    modeBox.addItem("Plugin UI", 2);
    modeBox.setSelectedId(1, dontSendNotification);
    manage(previewViewport);
    previewViewport.setViewedComponent(&preview, false);
    previewViewport.setScrollBarsShown(false, false, true, true);
    preview.shadow = true;
    // Inspector
    for (auto *e : {&elementText, &fontSize, &spacing, &x, &y, &w, &h})
      manage(*e).setFont(sans(15.f));
    for (auto *e : {&x, &y, &w, &h, &fontSize, &spacing}) {
      e->setJustification(Justification::centred);
      e->setIndents(2, 0);
    }
    manage(fontBox);
    fontBox.addItemList(StringArray{"Georgia", "Times New Roman", "Didot",
                                    "Baskerville", "Optima", "Futura",
                                    "Avenir Next", "Helvetica Neue", "Arial",
                                    "Verdana", "Courier New"},
                        1);
    fontBox.setEditableText(true);
    manage(bindingBox);
    for (int i = 0; i < pw::parameterIDs.size(); ++i)
      bindingBox.addItem(pw::parameters()[size_t(i)].name, i + 1);
    for (auto *b : {&alignLeft, &alignCentre, &alignRight, &controlImage,
                    &clearImage, &deleteElement, &inspectorLogo, &addKnob,
                    &addSlider, &addToggle, &addLabel, &resetDesign})
      manage(*b);
    style(alignLeft, "segment", "alignLeft");
    style(alignCentre, "segment", "alignCentre");
    style(alignRight, "segment", "alignRight");
    style(controlImage, "", "image");
    style(clearImage, "flat", "trash");
    style(deleteElement, "flat", "trash");
    deleteElement.setTooltip("Delete element");
    style(inspectorLogo, "", "upload");
    addKnob.setTooltip("Add a knob");
    addSlider.setTooltip("Add a slider");
    addToggle.setTooltip("Add an on/off switch");
    addLabel.setTooltip("Add a text element");
    style(resetDesign, "", "refresh");
    manage(styleGallery);
    styleGallery.onPick = [this](const String &id) { pickStyle(id); };
    manage(applyStyleToAll);
    style(applyStyleToAll, "", "layers");
    applyStyleToAll.onClick = [this] {
      auto *c = current();
      if (!c)
        return;
      auto type = c->type, chosen = styleGallery.current;
      for (auto &other : document.theme.controls)
        if (other.type == type)
          setControlStyle(other, chosen);
      if (type == "knob") {
        // Make it the default for knobs added later as well.
        document.theme.knobStyle = chosen;
        for (auto &other : document.theme.controls)
          if (other.type == "knob")
            other.style.clear();
      }
      changed();
      refresh();
      message("Style used for all " +
              (type == "toggle" ? String("switches") : type + "s"));
    };
    manage(snap);
    snap.setToggleState(true, dontSendNotification);

    // Callbacks
    open.onClick = [this] { choose(Open); };
    save.onClick = [this] { choose(Save); };
    build.onClick = buildSide.onClick = exportBuild.onClick = [this] {
      choose(Build);
    };
    exportSource.onClick = [this] { choose(Export); };
    openPlugins.onClick = [] {
      auto root = pw::pluginInstallRoot();
#if JUCE_LINUX || JUCE_BSD
      root = root.getChildFile(".vst3");
#endif
      root.createDirectory();
      root.revealToUser();
    };
    import.onClick = [this] { choose(Import); };
    chooseBackground.onClick = [this] { choose(Background); };
    uploadLogo.onClick = inspectorLogo.onClick = [this] { choose(Logo); };
    controlImage.onClick = [this] { choose(KnobImage); };
    importTemplate.onClick = [this] { choose(ThemeOpen); };
    saveTemplate.onClick = [this] { choose(ThemeSave); };
    projectBox.onChange = [this] {
      int id = projectBox.getSelectedId();
      projectBox.setSelectedId(1, dontSendNotification);
      if (id == 2)
        confirmDiscard([this] {
          replaceDocument(pw::Document{});
          projectFile = File{};
          message("New instrument");
        });
      else if (id == 3)
        choose(Open);
      else if (id >= 100) {
        auto recent = recentProjects();
        if (id - 100 < recent.size()) {
          File f(recent[id - 100]);
          confirmDiscard([this, f] { openProject(f); });
        }
      }
    };
    samplesTab.onClick = [this] { chooseTab(0); };
    soundTab.onClick = [this] { chooseTab(1); };
    designTab.onClick = [this] { chooseTab(2); };
    exportTab.onClick = [this] { chooseTab(3); };
    prevZone.onClick = [this] {
      if (zone > 0)
        showZone(zone - 1);
    };
    nextZone.onClick = [this] {
      if (zone + 1 < int(document.zones.size()))
        showZone(zone + 1);
    };
    addKnob.onClick = [this] { add("knob"); };
    addSlider.onClick = [this] { add("slider"); };
    addToggle.onClick = [this] { add("toggle"); };
    addLabel.onClick = [this] { add("label"); };
    deleteElement.onClick = [this] {
      if (current()) {
        document.theme.controls.erase(document.theme.controls.begin() +
                                      selected);
        selected = -1;
        changed();
        refresh();
      }
    };
    clearImage.onClick = [this] {
      if (auto *c = current()) {
        c->image.clear();
        changed();
        refresh();
      }
    };
    clearBackground.onClick = [this] {
      document.theme.background.clear();
      changed();
      refresh();
    };
    clearLogo.onClick = [this] {
      document.theme.logo.clear();
      changed();
      refresh();
    };
    fitBox.onChange = [this] {
      document.theme.fit = fitBox.getSelectedId() == 2   ? "contain"
                           : fitBox.getSelectedId() == 3 ? "stretch"
                                                         : "cover";
      changed();
      refresh();
    };
    areaBox.onChange = [this] {
      document.theme.area = areaBox.getSelectedId() == 2 ? "full" : "header";
      changed();
      refresh();
    };
    zoomBox.onChange = [this] { resized(); };
    modeBox.onChange = [this] { refresh(); };
    snap.onClick = [this] {
      preview.view.snap = snap.getToggleState();
      preview.view.rebuild(tab == 2 && modeBox.getSelectedId() != 2);
    };
    templateBox.onChange = [this] {
      try {
        int i = templateBox.getSelectedId();
        if (i == 1) {
          auto keepBackground = document.theme.background,
               keepLogo = document.theme.logo;
          document.theme = pw::Theme{};
          document.theme.background = keepBackground;
          document.theme.logo = keepLogo;
          selected = -1;
          changed();
          refresh();
        } else if (i > 1)
          applyTheme(pw::readDocument(
              templates.at(static_cast<std::size_t>(i - 2)), true));
      } catch (const std::exception &e) {
        error(e.what());
      }
    };
    resetDesign.onClick = [this] {
      Component::SafePointer<Main> safe(this);
      AlertWindow::showOkCancelBox(
          AlertWindow::QuestionIcon, "Reset design",
          "Reset layout, colours and images to the Royal Amber default?",
          "Reset", "Cancel", this,
          ModalCallbackFunction::create([safe](int ok) {
            if (safe && ok) {
              safe->document.theme = pw::Theme{};
              safe->selected = -1;
              safe->changed();
              safe->refresh();
            }
          }));
    };
    instrumentName.onReturnKey = instrumentName.onFocusLost = [this] {
      auto name = instrumentName.getText().trim();
      if (name.isNotEmpty() && name != document.name) {
        document.name = name;
        changed();
        refresh();
      }
    };
    maker.onReturnKey = maker.onFocusLost = [this] {
      if (maker.getText().trim() != document.maker) {
        document.maker = maker.getText().trim();
        changed();
        refresh();
      }
    };
    for (auto *e : {&elementText, &x, &y, &w, &h, &fontSize, &spacing})
      e->onReturnKey = e->onFocusLost = [this] {
        if (tab == 2)
          applyControl();
      };
    bindingBox.onChange = [this] { applyControl(); };
    fontBox.onChange = [this] { applyControl(); };
    alignLeft.onClick = [this] { setAlign("left"); };
    alignCentre.onClick = [this] { setAlign("centre"); };
    alignRight.onClick = [this] { setAlign("right"); };
    auto themeColour = [this](Colour pw::Theme::*member, Colour c) {
      document.theme.*member = c;
      changed();
      refresh();
    };
    baseSwatch.onChange = [themeColour](Colour c) {
      themeColour(&pw::Theme::base, c);
    };
    accentSwatch.onChange = [themeColour](Colour c) {
      themeColour(&pw::Theme::accent, c);
    };
    inkSwatch.onChange = [themeColour](Colour c) {
      themeColour(&pw::Theme::ink, c);
    };
    textSwatch.onChange = [this](Colour c) {
      if (auto *ctl = current()) {
        ctl->colour = c.toString();
        changed();
        refresh();
      }
    };
    std::pair<TextEditor *, Colour pw::Theme::*> hexFields[] = {
        {&baseHex, &pw::Theme::base},
        {&accentHex, &pw::Theme::accent},
        {&inkHex, &pw::Theme::ink}};
    for (auto [editor, member] : hexFields)
      editor->onReturnKey = editor->onFocusLost = [this, editor = editor,
                                                   member = member,
                                                   themeColour] {
        Colour c;
        if (!parseHex(editor->getText(), c)) {
          refresh(false);
          return;
        }
        if (c != document.theme.*member)
          themeColour(member, c);
      };
    textHex.onReturnKey = textHex.onFocusLost = [this] {
      Colour c;
      auto *ctl = current();
      if (!ctl || !parseHex(textHex.getText(), c)) {
        updateInspector();
        return;
      }
      if (c.toString() != ctl->colour) {
        ctl->colour = c.toString();
        changed();
        refresh();
      }
    };
    preview.view.selected = [this](int n) {
      selected = n;
      updateInspector();
      updateGallery();
      resized();
      repaint();
    };
    preview.view.moved = [this] {
      changed();
      updateInspector();
    };
    preview.view.changed = [this](const String &id, float value) {
      document.defaults[id] = value;
      if (!dirty)
        changed();
    };
    audioSettings.onClick = [this] {
      auto *panel = new AudioDeviceSelectorComponent(
          deviceManager, 0, 0, 2, 2, true, false, true, false);
      panel->setSize(540, 440);
      DialogWindow::LaunchOptions opts;
      opts.content.setOwned(panel);
      opts.dialogTitle = "Audio / MIDI";
      opts.dialogBackgroundColour = Colour(0xff0b1847);
      opts.useNativeTitleBar = true;
      opts.resizable = false;
      opts.launchAsync();
    };
    scanTemplates();
    setSize(1480, 900);
    setAudioChannels(0, 2);
    deviceManager.addMidiInputDeviceCallback({}, &midiCollector);
    refresh();
    startTimerHz(30);
  }
  ~Main() override {
    stopTimer();
    if (worker.joinable())
      worker.join();
    deviceManager.removeMidiInputDeviceCallback({}, &midiCollector);
    shutdownAudio();
    previewViewport.setViewedComponent(nullptr, false);
    LookAndFeel::setDefaultLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
  }
#ifdef PW_TESTING_VIEW
  void qaPreview(int screen, const File &sfz = {}, const File &background = {},
                 const File &logoFile = {}) {
    if (sfz.existsAsFile()) {
      pw::Document d;
      pw::importSfz(d, sfz);
      d.name = "Velvet Grand";
      if (background.existsAsFile())
        d.theme.background = pw::addImage(d, background);
      if (logoFile.existsAsFile())
        d.theme.logo = pw::addImage(d, logoFile);
      replaceDocument(d, pw::Engine::decode(d));
    } else {
      document.name = "Velvet Grand";
      document.zones = {{"", 24, 34, 24, 1, 127}, {"", 36, 95, 36, 1, 127}};
    }
    document.maker = "Artilody";
    selected = 1;
    chooseTab(screen);
    engine.keyboard.noteOn(1, 60, .8f);
    engine.keyboard.noteOn(1, 64, .8f);
    engine.keyboard.noteOn(1, 67, .8f);
    showZone(jmin(2, int(document.zones.size()) - 1));
  }
#endif
#ifdef PW_TESTING_VIEW
  // Adds sliders and switches in several styles and selects element n.
  void qaStyles(int n) {
    const char *sliders[] = {"fader", "bar", "led"};
    const char *toggles[] = {"rocker", "button", "lever"};
    for (int i = 0; i < 3; ++i) {
      pw::Control s;
      s.type = "slider";
      s.binding = "gain";
      s.text = "Volume";
      s.style = sliders[i];
      s.bounds = {40 + i * 210, 424, 190, 64};
      document.theme.controls.push_back(s);
    }
    for (int i = 0; i < 3; ++i) {
      pw::Control t;
      t.type = "toggle";
      t.binding = "enabled";
      t.text = "Power";
      t.style = toggles[i];
      t.bounds = {680 + i * 105, 424, 100, 64};
      document.theme.controls.push_back(t);
    }
    document.theme.controls[4].style = "vintage";
    document.theme.controls[5].style = "ledring";
    document.theme.controls[6].style = "metal";
    document.theme.controls[7].style = "flat";
    selected = n;
    refresh();
  }
#endif
  void requestClose() {
    if (busy) {
      message("Please wait for the current operation to finish");
      return;
    }
    confirmDiscard([] { JUCEApplication::getInstance()->quit(); });
  }
  void prepareToPlay(int block, double sr) override {
    const ScopedLock lock(audioLock);
    engine.prepare(sr, block);
    midiCollector.reset(sr);
  }
  void releaseResources() override {}
  void getNextAudioBlock(const AudioSourceChannelInfo &info) override {
    const ScopedLock lock(audioLock);
    MidiBuffer midi;
    midiCollector.removeNextBlockOfMessages(midi, info.numSamples);
    AudioBuffer<float> sub(info.buffer->getArrayOfWritePointers(),
                           info.buffer->getNumChannels(), info.startSample,
                           info.numSamples);
    engine.render(sub, midi);
  }

  //== Painting ================================================================
  void paintHeader(Graphics &g) {
    float W = float(getWidth());
    // Decorative ribbons sweeping through the header.
    Path orange, blue, blue2;
    orange.startNewSubPath(0, 112);
    orange.cubicTo(W * .2f, 132, W * .33f, 96, W * .47f, -4);
    blue.startNewSubPath(0, 122);
    blue.cubicTo(W * .22f, 136, W * .36f, 100, W * .5f, -10);
    blue2.startNewSubPath(W * .52f, 128);
    blue2.cubicTo(W * .7f, 100, W * .82f, 160, W, 60);
    pw::glowStroke(g, blue, Colour(0xff3d6bff).withAlpha(.65f), 1.6f, 10);
    pw::glowStroke(g, orange, col::accent.withAlpha(.75f), 1.8f, 12);
    pw::glowStroke(g, blue2, Colour(0xff3d6bff).withAlpha(.35f), 1.2f, 8);
    Path orange2;
    orange2.startNewSubPath(W * .7f, 128);
    orange2.cubicTo(W * .8f, 110, W * .9f, 140, W, 96);
    pw::glowStroke(g, orange2, col::accent.withAlpha(.45f), 1.4f, 8);
    g.setGradientFill(ColourGradient(Colour(0x00000000), 0, 120,
                                     col::panelEdge.withAlpha(.6f), W * .5f,
                                     120, false));
    g.fillRect(0.f, 127.f, W, 1.f);
    // Logo.
    drawAppLogo(g, {16, 18, 122, 88});
    g.setColour(Colours::white);
    g.setFont(sans(44.f, true));
    g.drawText("Artilody", 142, 14, 280, 50, Justification::centredLeft);
    g.setGradientFill(ColourGradient(Colour(0xff6f9bff), 142, 0,
                                     Colour(0xff3f6dff), 400, 0, false));
    g.setFont(sans(36.f, true));
    g.drawText("Plugweaver", 142, 58, 300, 44, Justification::centredLeft);
  }
  void paintInstrumentInfo(Graphics &g) {
    if (instrumentInfo.isEmpty())
      return;
    auto r = instrumentInfo;
    g.setColour(col::text);
    g.setFont(serif(24.f));
    g.drawFittedText(document.name, r.removeFromTop(34),
                     Justification::centredLeft, 1);
    g.setFont(sans(16.f));
    g.setColour(col::text.withAlpha(.85f));
    g.drawText(infoSamples, r.removeFromTop(24), Justification::centredLeft);
    g.drawText(infoRange, r.removeFromTop(24), Justification::centredLeft);
  }
  void paintHeaderCard(Graphics &g) {
    if (headerCard.isEmpty())
      return;
    auto r = headerCard.toFloat();
    const auto &bg = backgroundImage;
    if (bg.isValid()) {
      Graphics::ScopedSaveState state(g);
      Path clip;
      clip.addRoundedRectangle(r, 14);
      g.reduceClipRegion(clip);
      auto art = r.withLeft(r.getX() + r.getWidth() * .42f);
      g.drawImageWithin(bg, roundToInt(art.getX()), roundToInt(art.getY()),
                        roundToInt(art.getWidth()), roundToInt(art.getHeight()),
                        RectanglePlacement::fillDestination);
      g.setGradientFill(ColourGradient(col::panel, art.getX(), 0,
                                       col::panel.withAlpha(0.f),
                                       art.getX() + art.getWidth() * .45f, 0,
                                       false));
      g.fillRect(art);
      g.setGradientFill(ColourGradient(col::panel.withAlpha(0.f), 0,
                                       r.getBottom() - 30, col::panel, 0,
                                       r.getBottom(), false));
      g.fillRect(art);
    }
    auto text = headerCard.reduced(26, 14);
    g.setColour(col::muted);
    g.setFont(sans(15.f));
    g.drawText(tab == 0   ? "Sample mapping"
               : tab == 1 ? "Sample editor"
                          : "Build & export",
               text.removeFromTop(22), Justification::centredLeft);
    g.setColour(col::text);
    g.setFont(serif(jmin(54.f, float(text.getHeight()) * .95f)));
    g.drawFittedText(document.name, text, Justification::centredLeft, 1);
  }
  void paintExportInfo(Graphics &g) {
    if (exportInfo.isEmpty())
      return;
    auto r = exportInfo.reduced(24, 18);
    drawTitle(g, "Formats and installation", r.removeFromTop(28), "cube", 18);
    r.removeFromTop(10);
    String safe = document.name;
    struct Row {
      ToggleButton *t;
      String label, format;
    } rows[] = {{&vst3, "VST3", "vst3"},
                {&au, "AU (Logic, GarageBand)", "au"},
                {&clap, "CLAP", "clap"}};
    auto root = pw::pluginInstallRoot().getFullPathName();
#if !JUCE_WINDOWS
    auto home = File::getSpecialLocation(File::userHomeDirectory)
                    .getFullPathName();
    if (root.startsWith(home))
      root = "~" + root.substring(home.length());
#endif
    g.setFont(sans(15.f));
    for (auto &row : rows) {
      auto line = r.removeFromTop(26);
      bool on = row.t->getToggleState();
      g.setColour(on ? col::accent : col::dim);
      g.fillEllipse(line.removeFromLeft(14).toFloat().withSizeKeepingCentre(8, 8));
      line.removeFromLeft(6);
      g.setColour(on ? col::text : col::dim);
      g.drawText(row.label, line.removeFromLeft(200), Justification::centredLeft);
      g.setColour(on ? col::muted : col::dim);
      auto sep = File::getSeparatorString();
      g.drawFittedText(on ? root + sep + pw::pluginFolderName(row.format) +
                                sep + safe + pw::pluginExtension(row.format)
                          : String(row.t->isEnabled() ? "not selected"
                                                      : "macOS only"),
                       line, Justification::centredLeft, 1);
    }
    r.removeFromTop(8);
    g.setColour(col::muted);
    g.setFont(sans(14.f));
    g.drawFittedText(
        "Plugin identity: manufacturer Arti, code " + infoCode +
            ". Every build gets a new version number so hosts notice the "
            "update.\nAfter building: quit and reopen your DAW. Logic: "
            "Plug-in Manager > Reset & Rescan. REAPER: Preferences > "
            "Plug-ins > Re-scan. FL Studio: Options > Manage plugins > Find "
            "installed plugins. Find it among instruments (" +
            (document.maker.isNotEmpty() ? document.maker : String("maker")) +
            "), not effects.",
        r.removeFromTop(78), Justification::topLeft, 5);
  }
  void paint(Graphics &g) override {
    g.setGradientFill(ColourGradient(col::bgTop, 0, 0, col::bgBottom, 0,
                                     float(getHeight()), false));
    g.fillAll();
    g.setGradientFill(ColourGradient(Colour(0x55234ad8), float(getWidth()) * .5f,
                                     float(getHeight()) * .35f,
                                     Colour(0x00234ad8), 0, float(getHeight()),
                                     true));
    g.fillAll();
    paintHeader(g);
    for (auto &p : panels) {
      drawPanel(g, p.area.toFloat(), 14, p.title.isEmpty());
      if (p.title.isNotEmpty())
        drawTitle(g, p.title,
                  p.area.reduced(18, 0).withTop(p.area.getY() + 14).withHeight(28),
                  p.icon);
    }
    paintHeaderCard(g);
    paintInstrumentInfo(g);
    paintExportInfo(g);
    for (auto &c : captions)
      drawSubheading(g, c.text, c.area);
    if (!waveCaption.isEmpty())
      drawSubheading(g,
                     waveform.caption.isEmpty() ? String("Waveform")
                                                : waveform.caption,
                     waveCaption);
    if (!previewBar.isEmpty()) {
      auto r = previewBar;
      g.setColour(col::text);
      g.setFont(sans(19.f, true));
      g.drawText("Live preview", r, Justification::centredLeft);
      g.setColour(col::muted);
      g.setFont(sans(15.f));
      g.drawText("Preview mode", modeBox.getX() - 112, r.getY(), 104,
                 r.getHeight(), Justification::centredRight);
      g.setColour(col::panelEdge);
      g.fillRect(modeBox.getX() - 124, r.getY() + 6, 1, r.getHeight() - 12);
    }
    if (!inspectorHead.isEmpty()) {
      auto r = inspectorHead;
      if (auto *c = current()) {
        auto box = r.removeFromLeft(r.getHeight()).toFloat();
        g.setColour(col::field);
        g.fillRoundedRectangle(box, 8);
        g.setColour(col::fieldEdge);
        g.drawRoundedRectangle(box.reduced(.5f), 8, 1);
        drawIcon(g, elementIcon(*c), box.reduced(10), col::text);
        r.removeFromLeft(14);
        g.setColour(col::text);
        g.setFont(sans(18.f, true));
        g.drawFittedText(elementName(*c), r.withTrimmedRight(40),
                         Justification::centredLeft, 1);
      } else {
        g.setColour(col::muted);
        g.setFont(sans(15.f));
        g.drawFittedText("Click an element in the live preview to edit it. "
                         "Drag to move; drag the lower-right corner to resize.",
                         r.withHeight(r.getHeight() * 2), Justification::topLeft,
                         4);
      }
    }
    if (!footer.isEmpty()) {
      drawPanel(g, footer.toFloat(), 12);
      auto r = footer.reduced(18, 0);
      drawIcon(g, "info", r.removeFromLeft(22).toFloat().withSizeKeepingCentre(20, 20),
               col::muted);
      r.removeFromLeft(12);
      g.setColour(col::muted);
      g.setFont(sans(15.f));
      g.drawFittedText(tab == 2 && statusText.startsWith("Ready")
                           ? String("Changes appear instantly in the preview")
                           : statusText,
                       r.withTrimmedRight(220), Justification::centredLeft, 1);
    }
    // Status line beside the tabs.
    if (tab < 2 || footer.isEmpty()) {
      g.setColour(col::muted);
      g.setFont(sans(14.f));
      g.drawFittedText(statusText,
                       Rectangle<int>(exportTab.getRight() + 24, 92,
                                      getWidth() - exportTab.getRight() - 46,
                                      28),
                       Justification::centredRight, 1);
    }
    if (tab < 2 && !piano.getBounds().isEmpty()) {
      auto r = piano.getBounds().expanded(10, 10).withTrimmedBottom(10);
      Path rim;
      rim.addRoundedRectangle(r.toFloat(), 12);
      g.setColour(Colour(0xff060a1c));
      g.fillPath(rim);
      g.setColour(col::panelEdge);
      g.strokePath(rim, PathStrokeType(1));
    }
  }

  //== Layout ==================================================================
  void layoutInstrumentPanel(Rectangle<int> r) {
    panels.push_back({r, "Instrument", ""});
    auto in = r.reduced(16).withTrimmedTop(36);
    auto top = in.removeFromTop(96);
    place(instrumentThumb, top.removeFromLeft(128));
    top.removeFromLeft(16);
    instrumentInfo = top.withTrimmedTop(6);
    in.removeFromTop(14);
    place(import, in.removeFromTop(46));
    in.removeFromTop(14);
    auto head = in.removeFromTop(28);
    caption(head.withTrimmedLeft(34).withWidth(60), "Note");
    caption(head.withTrimmedLeft(106), "Sample");
    place(sampleList, in);
  }
  void layoutSettingsPanel(Rectangle<int> r) {
    panels.push_back({r, "Plugin settings", "gear"});
    auto in = r.reduced(18).withTrimmedTop(44);
    caption(in.removeFromTop(26), "Instrument name");
    place(instrumentName, in.removeFromTop(42));
    in.removeFromTop(12);
    caption(in.removeFromTop(26), "Maker");
    place(maker, in.removeFromTop(42));
    in.removeFromTop(16);
    caption(in.removeFromTop(26), "Export formats");
    for (auto *t : {&vst3, &au, &clap}) {
      place(*t, in.removeFromTop(44).reduced(6, 4));
    }
    in.removeFromTop(14);
    place(buildSide, in.removeFromTop(54));
    in.removeFromTop(10);
    if (in.getHeight() > 40)
      place(audioSettings, in.removeFromTop(36));
  }
  void layoutKeyboard(Rectangle<int> r) {
    auto knob = r.removeFromRight(150);
    place(master, knob.reduced(10, 0));
    place(piano, r.reduced(10, 10).withTrimmedBottom(0));
  }
  void resized() override {
    for (auto *c : managed)
      c->setVisible(false);
    captions.clear();
    panels.clear();
    headerCard = instrumentInfo = footer = previewBar = exportInfo =
        inspectorHead = waveCaption = {};
    const int W = getWidth(), H = getHeight(), gap = 14;
    // Header
    int right = W - 22;
    place(build, {right - 230, 30, 230, 48});
    right -= 230 + 18;
    place(save, {right - 130, 32, 130, 44});
    right -= 130 + 16;
    place(open, {right - 176, 32, 176, 44});
    right -= 176 + 30;
    int comboW = jlimit(200, 390, right - 470);
    place(projectBox, {right - comboW, 32, comboW, 44});
    int tabsW = 4 * 160 + 3 * 20;
    int tx = (W - tabsW) / 2;
    for (auto *t : {&samplesTab, &soundTab, &designTab, &exportTab}) {
      place(*t, {tx, 84, 160, 42});
      tx += 180;
    }
    auto body = Rectangle<int>(18, 138, W - 36, H - 138 - 14);
    if (tab == 2) {
      footer = body.removeFromBottom(46);
      body.removeFromBottom(gap);
      layoutDesign(body);
      place(resetDesign, footer.reduced(8, 6).removeFromRight(190));
      return;
    }
    if (tab == 3) {
      footer = body.removeFromBottom(46);
      body.removeFromBottom(gap);
    } else {
      auto keys = body.removeFromBottom(jlimit(120, 150, H / 6));
      body.removeFromBottom(gap);
      layoutKeyboard(keys);
    }
    int side = jlimit(270, 360, W / 5);
    layoutInstrumentPanel(body.removeFromLeft(side));
    body.removeFromLeft(gap);
    layoutSettingsPanel(body.removeFromRight(jlimit(260, 330, W / 5)));
    body.removeFromRight(gap);
    // Centre
    panels.push_back({body, "", ""});
    auto in = body.reduced(14);
    headerCard = in.removeFromTop(jlimit(84, 120, body.getHeight() / 5));
    in.removeFromTop(8);
    if (tab == 3) {
      auto card = in.removeFromTop(206);
      panels.push_back({card, "", ""});
      exportInfo = card;
      in.removeFromTop(12);
      auto buttons = in.removeFromTop(48);
      place(exportBuild, buttons.removeFromLeft(220));
      buttons.removeFromLeft(12);
      place(exportSource, buttons.removeFromLeft(220));
      buttons.removeFromLeft(12);
      place(openPlugins, buttons.removeFromLeft(210));
      in.removeFromTop(12);
      place(buildLog, in);
      return;
    }
    // Waveform card
    int waveH = tab == 0 ? jmax(150, in.getHeight() * 2 / 5)
                         : jlimit(130, 210, in.getHeight() / 3);
    auto wave = in.removeFromTop(waveH);
    panels.push_back({wave, "", ""});
    auto waveHead = wave.reduced(14, 8).removeFromTop(30);
    place(nextZone, waveHead.removeFromRight(32));
    waveHead.removeFromRight(6);
    place(prevZone, waveHead.removeFromRight(32));
    waveCaption = waveHead.withTrimmedRight(12);
    place(waveform, wave.reduced(8, 6).withTrimmedTop(32));
    in.removeFromTop(12);
    if (tab == 0) {
      panels.push_back({in, "Regions", "layers"});
      place(regionTable, in.reduced(14).withTrimmedTop(36));
      return;
    }
    // Sound: ADSR + filter
    auto adsr = in.removeFromLeft(in.getWidth() * 3 / 5);
    in.removeFromLeft(12);
    auto filter = in;
    panels.push_back({adsr, "", ""});
    panels.push_back({filter, "", ""});
    caption(adsr.reduced(18, 0).withTop(adsr.getY() + 12).withHeight(26),
            String::fromUTF8("\xe2\x97\x8f  ADSR Envelope"));
    caption(filter.reduced(18, 0).withTop(filter.getY() + 12).withHeight(26),
            String::fromUTF8("\xe2\x97\x8f  Filter"));
    place(filterType, filter.reduced(14, 0)
                          .withTop(filter.getY() + 10)
                          .withHeight(30)
                          .removeFromRight(140));
    int knobH = jlimit(96, 130, adsr.getHeight() / 2);
    auto adsrIn = adsr.reduced(14, 10).withTrimmedTop(36);
    auto adsrKnobs = adsrIn.removeFromBottom(knobH);
    place(envelope, adsrIn);
    int kw = adsrKnobs.getWidth() / 4;
    for (auto *k : {&attack, &decay, &sustain, &release})
      place(*k, adsrKnobs.removeFromLeft(kw));
    auto filterIn = filter.reduced(14, 10).withTrimmedTop(36);
    auto filterKnobs = filterIn.removeFromBottom(knobH);
    place(filterGraph, filterIn);
    int fw = filterKnobs.getWidth() / 3;
    for (auto *k : {&cutoff, &resonance, &reverb})
      place(*k, filterKnobs.removeFromLeft(fw));
  }
  void layoutDesign(Rectangle<int> body) {
    const int gap = 14;
    int leftW = jlimit(300, 380, int(getWidth() / 4.6));
    int rightW = jlimit(320, 380, int(getWidth() / 4.5));
    // Appearance
    auto left = body.removeFromLeft(leftW);
    body.removeFromLeft(gap);
    panels.push_back({left, "Appearance", ""});
    auto in = left.reduced(16).withTrimmedTop(34);
    caption(in.removeFromTop(24), "Theme");
    auto cards = in.removeFromTop(jlimit(90, 112, in.getHeight() / 6));
    int cw = (cards.getWidth() - 16) / 3;
    for (auto &card : themeCards) {
      place(card, cards.removeFromLeft(cw));
      cards.removeFromLeft(8);
    }
    in.removeFromTop(8);
    auto tpl = in.removeFromTop(32);
    place(saveTemplate, tpl.removeFromRight(38));
    tpl.removeFromRight(6);
    place(importTemplate, tpl.removeFromRight(38));
    tpl.removeFromRight(8);
    place(templateBox, tpl);
    in.removeFromTop(10);
    caption(in.removeFromTop(24), "Colours");
    std::pair<const char *, std::pair<Swatch *, TextEditor *>> rows[] = {
        {"Background", {&baseSwatch, &baseHex}},
        {"Accent", {&accentSwatch, &accentHex}},
        {"Text", {&inkSwatch, &inkHex}}};
    for (auto &[name, pair] : rows) {
      auto row = in.removeFromTop(38).reduced(0, 3);
      caption(row.removeFromLeft(110).withTrimmedLeft(4), name);
      place(*pair.first, row.removeFromLeft(52));
      row.removeFromLeft(10);
      place(*pair.second, row);
    }
    in.removeFromTop(10);
    caption(in.removeFromTop(24), "Background image");
    auto bgRow = in.removeFromTop(64);
    place(backgroundThumb, bgRow.removeFromLeft(96));
    bgRow.removeFromLeft(10);
    place(clearBackground, bgRow.removeFromRight(32).withSizeKeepingCentre(32, 32));
    bgRow.removeFromRight(4);
    place(chooseBackground, bgRow.withSizeKeepingCentre(bgRow.getWidth(), 42));
    in.removeFromTop(6);
    auto fitRow = in.removeFromTop(30);
    place(fitBox, fitRow.removeFromLeft(fitRow.getWidth() / 2 - 4));
    fitRow.removeFromLeft(8);
    place(areaBox, fitRow);
    in.removeFromTop(10);
    caption(in.removeFromTop(24), "Logo");
    auto logoRow = in.removeFromTop(64);
    place(logoThumb, logoRow.removeFromLeft(96));
    logoRow.removeFromLeft(10);
    place(clearLogo, logoRow.removeFromRight(32).withSizeKeepingCentre(32, 32));
    logoRow.removeFromRight(4);
    place(uploadLogo, logoRow.withSizeKeepingCentre(logoRow.getWidth(), 42));

    // Inspector column
    auto right = body.removeFromRight(rightW);
    body.removeFromRight(gap);
    auto *c = current();
    const bool styled =
        c == nullptr || c->type == "knob" || c->type == "slider" ||
        c->type == "toggle";
    // Space the inspector needs for the selected element; the style gallery
    // below gets the rest.
    int need = !c                   ? 130
               : c->type == "logo"  ? 230
               : c->type == "label" ? 400
               : c->type == "toggle" ? 296
                                     : 338;
    int controlsH = styled ? jmax(250, right.getHeight() - gap - need) : 150;
    auto controlsPanel = right.removeFromBottom(controlsH);
    right.removeFromBottom(gap);
    panels.push_back({right, "Selected element", ""});
    auto ins = right.reduced(18).withTrimmedTop(36);
    inspectorHead = ins.removeFromTop(48);
    if (c) {
      place(deleteElement, inspectorHead.withLeft(inspectorHead.getRight() - 36)
                               .withSizeKeepingCentre(36, 36));
      ins.removeFromTop(10);
      auto field = [&](const String &label, Component &comp) {
        caption(ins.removeFromTop(24), label);
        place(comp, ins.removeFromTop(36));
        ins.removeFromTop(8);
      };
      // Compact rows (caption left, field right) for knob/slider/switch.
      auto row = [&](const String &label, Component &comp) {
        auto r = ins.removeFromTop(36);
        caption(r.removeFromLeft(110), label);
        place(comp, r);
        ins.removeFromTop(6);
      };
      if (c->type == "logo") {
        place(inspectorLogo, ins.removeFromTop(40));
        ins.removeFromTop(10);
      } else if (c->type == "label")
        field("Text  ({instrument}, {maker})", elementText);
      else
        row("Label", elementText);
      if (c->type == "knob" || c->type == "slider")
        row("Parameter", bindingBox);
      if (c->type == "label") {
        auto heads = ins.removeFromTop(24);
        caption(heads.removeFromLeft(heads.getWidth() - 140), "Typeface");
        caption(heads.removeFromLeft(70), "Size");
        caption(heads, "Spacing");
        auto line = ins.removeFromTop(36);
        place(spacing, line.removeFromRight(62));
        line.removeFromRight(8);
        place(fontSize, line.removeFromRight(62));
        line.removeFromRight(8);
        place(fontBox, line);
        ins.removeFromTop(10);
        auto alignRow = ins.removeFromTop(36);
        caption(alignRow.removeFromLeft(110), "Alignment");
        int bw = alignRow.getWidth() / 3;
        place(alignLeft, alignRow.removeFromLeft(bw).reduced(2, 0));
        place(alignCentre, alignRow.removeFromLeft(bw).reduced(2, 0));
        place(alignRight, alignRow.reduced(2, 0));
        ins.removeFromTop(10);
      }
      if (c->type != "logo") {
        auto colourRow = ins.removeFromTop(36);
        caption(colourRow.removeFromLeft(110), "Text colour");
        place(textSwatch, colourRow.removeFromLeft(48));
        colourRow.removeFromLeft(10);
        place(textHex, colourRow);
        ins.removeFromTop(c->type == "label" ? 10 : 6);
      }
      if (c->type == "knob" || c->type == "slider" || c->type == "toggle") {
        auto imgRow = ins.removeFromTop(36);
        place(clearImage, imgRow.removeFromRight(34).withSizeKeepingCentre(34, 34));
        imgRow.removeFromRight(6);
        place(controlImage, imgRow);
        ins.removeFromTop(8);
      }
      if (ins.getHeight() >= 58) {
        caption(ins.removeFromTop(24), "X / Y / Width / Height");
        auto geo = ins.removeFromTop(34);
        int gw = (geo.getWidth() - 18) / 4;
        for (auto *e : {&x, &y, &w, &h}) {
          place(*e, geo.removeFromLeft(gw));
          geo.removeFromLeft(6);
        }
      }
    }
    panels.push_back({controlsPanel, "Controls", ""});
    auto ctl = controlsPanel.reduced(18).withTrimmedTop(34);
    auto addRow = ctl.removeFromBottom(34);
    // Used for the snap toggle when the preview bar is too narrow.
    snapFallback = addRow.withY(controlsPanel.getY() + 12).withHeight(28)
                       .withTrimmedLeft(addRow.getWidth() - 140);
    int aw = (addRow.getWidth() - 18) / 4;
    for (auto *b : {&addKnob, &addSlider, &addToggle, &addLabel}) {
      place(*b, addRow.removeFromLeft(aw));
      addRow.removeFromLeft(6);
    }
    ctl.removeFromBottom(10);
    if (styled) {
      String typeName = !c                    ? String("knobs")
                        : c->type == "slider" ? String("slider")
                        : c->type == "toggle" ? String("switch")
                                              : String("knob");
      caption(ctl.removeFromTop(24),
              c ? "Style for this " + typeName + " (click to use)"
                : String("Style for all knobs (click to use)"));
      if (c) {
        auto all = ctl.removeFromBottom(32);
        applyStyleToAll.setButtonText(
            c->type == "slider"   ? "Use for all sliders"
            : c->type == "toggle" ? "Use for all switches"
                                  : "Use for all knobs");
        place(applyStyleToAll, all);
        ctl.removeFromBottom(8);
      }
      place(styleGallery, ctl);
    } else
      caption(ctl.removeFromTop(48).withHeight(24),
              "Select a knob, slider or switch to see its styles");

    // Live preview
    panels.push_back({body, "", ""});
    auto pv = body.reduced(18, 14);
    previewBar = pv.removeFromTop(36);
    auto bar = previewBar;
    if (bar.getWidth() >= 700)
      place(snap, bar.withTrimmedLeft(140).withWidth(150).reduced(0, 4));
    else
      place(snap, snapFallback);
    place(modeBox, bar.removeFromRight(150).reduced(0, 2));
    bar.removeFromRight(136);
    place(zoomBox, bar.removeFromRight(100).reduced(0, 2));
    pv.removeFromTop(10);
    place(previewViewport, pv);
    const float zooms[] = {0.f, .75f, 1.f, 1.25f};
    preview.zoom = zooms[jlimit(0, 3, zoomBox.getSelectedId() - 1)];
    if (preview.zoom <= 0)
      preview.setSize(pv.getWidth(), pv.getHeight());
    else
      preview.setSize(jmax(pv.getWidth(), roundToInt(1000 * preview.zoom) + 20),
                      jmax(pv.getHeight(), roundToInt(600 * preview.zoom) + 40));
  }
};

#ifndef PW_TESTING_VIEW
class PlugweaverApp final : public JUCEApplication {
  class Window : public DocumentWindow {
    Main *main;

  public:
    Window()
        : DocumentWindow("Artilody Plugweaver", Colour(0xff0b1847),
                         allButtons) {
      setUsingNativeTitleBar(true);
      main = new Main;
      setContentOwned(main, true);
      setResizable(true, true);
      setResizeLimits(1280, 780, 2800, 1800);
      auto *screen = Desktop::getInstance().getDisplays().getPrimaryDisplay();
      auto area = screen ? screen->userArea : Rectangle<int>(0, 0, 1440, 900);
      centreWithSize(jlimit(1280, 1600, area.getWidth() - 30),
                     jlimit(780, 940, area.getHeight() - 40));
      setVisible(true);
    }
    void closeButtonPressed() override { main->requestClose(); }
  };
  std::unique_ptr<Window> window;

public:
  const String getApplicationName() override { return "Artilody Plugweaver"; }
  const String getApplicationVersion() override {
    return PW_APP_VERSION;
  }
  bool moreThanOneInstanceAllowed() override { return false; }
  void initialise(const String &commandLine) override {
    auto args = StringArray::fromTokens(commandLine, true);
    args.trim();
    for (auto &a : args)
      a = a.unquoted();
    int i = args.indexOf("--export-test");
    if (i >= 0) {
      setApplicationReturnValue(
          exportTest(File(args[i + 1]), args.contains("--install")) ? 0 : 1);
      quit();
      return;
    }
    window = std::make_unique<Window>();
  }
  // Release check without UI: builds plugins from a generated instrument with
  // the engine embedded in this app (and installs them with --install), and
  // writes the result to result.txt.
  static bool exportTest(const File &folder, bool install) {
    String result;
    bool ok = false;
    try {
      pw::require(File::isAbsolutePath(folder.getFullPathName()) &&
                      folder.createDirectory().wasOk(),
                  "Usage: --export-test <absolute folder>");
      auto wav = folder.getChildFile("tone.wav");
      {
        wav.deleteFile();
        WavAudioFormat format;
        std::unique_ptr<AudioFormatWriter> writer(format.createWriterFor(
            wav.createOutputStream().release(), 44100, 1, 16, {}, 0));
        pw::require(writer != nullptr, "Cannot write test sample");
        AudioBuffer<float> audio(1, 22050);
        for (int n = 0; n < audio.getNumSamples(); ++n)
          audio.setSample(0, n, .2f * std::sin(float(n) * .02f));
        writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples());
      }
      auto sfz = folder.getChildFile("Test.sfz");
      sfz.replaceWithText("<region> key=60 sample=tone.wav\n");
      pw::Document d;
      pw::importSfz(d, sfz);
      d.name = "Plugweaver Export Test";
      auto engine = pw::openEngineArchive();
      pw::require(engine != nullptr, "Built-in engine not found");
      auto built = pw::createPlugins(d, true, pw::hasAudioUnits, true, *engine,
                                     folder.getChildFile("plugins"));
      for (auto &b : built)
        result << b.format << ": " << b.file.getFullPathName() << "\n";
      ok = !built.empty();
      if (install)
        for (auto &file : pw::installPlugins(built)) {
          pw::require(file.exists(), "Not installed: " + file.getFullPathName());
          result << "installed: " << file.getFullPathName() << "\n";
        }
    } catch (const std::exception &e) {
      result << "FAILED: " << e.what() << "\n";
    }
    if (folder.isDirectory())
      folder.getChildFile("result.txt")
          .replaceWithText((ok ? "OK\n" : "") + result);
    std::cout << result << std::flush;
    return ok;
  }
  void shutdown() override { window.reset(); }
  void systemRequestedQuit() override {
    if (window)
      window->closeButtonPressed();
    else
      quit();
  }
  void anotherInstanceStarted(const String &) override {}
};
START_JUCE_APPLICATION(PlugweaverApp)
#endif
