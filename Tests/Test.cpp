// SPDX-License-Identifier: GPL-3.0-only
#include "Engine.h"
#include "Exporter.h"
#include "SfzImport.h"
// Keep test assertions enabled without changing JUCE's release/debug configuration.
#undef NDEBUG
#include <cassert>
#include <iostream>
int main(int argc, char **argv) {
  using namespace juce;
  try {
    File root = argc > 1 ? File(String(argv[1]))
                         : File::getSpecialLocation(File::tempDirectory)
                               .getChildFile("pw-tests");
    root.createDirectory();
    File wav = root.getChildFile("tone.wav");
    wav.deleteFile();
    auto stream = wav.createOutputStream();
    WavAudioFormat format;
    std::unique_ptr<AudioFormatWriter> writer(
        format.createWriterFor(stream.release(), 44100, 2, 24, {}, 0));
    assert(writer);
    AudioBuffer<float> audio(2, 44100);
    for (int i = 0; i < audio.getNumSamples(); ++i) {
      float sample =
          .2f * std::sin(float(i) *
                         float(2 * MathConstants<double>::pi * 65.406 / 44100));
      audio.setSample(0, i, sample);
      audio.setSample(1, i, sample * .5f);
    }
    assert(writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples()));
    writer.reset();
    auto sfz = root.getChildFile("Test.sfz");
    sfz.replaceWithText("<group> loop_mode=no_loop\n<region> key=36 "
                        "sample=tone.wav\n<region> key=38 sample=tone.wav\n");
    pw::Document d;
    pw::importSfz(d, sfz);
    assert(d.zones.size() == 2 && d.assets.size() == 1 && d.coverage()[36] &&
           !d.coverage()[37] && d.coverage()[38]);
    d.defaults["reverb"] = 0;
    d.theme.controls[1].bounds = {101, 200, 120, 130};
    d.theme.name = "My shared design";
    d.theme.controls[4].style = "vintage";
    Image image(Image::ARGB, 8, 8, true);
    Graphics graphics(image);
    graphics.fillAll(Colours::orange);
    MemoryOutputStream png;
    PNGImageFormat pngFormat;
    assert(pngFormat.writeImageToStream(image, png));
    d.assets["images/test.png"] = png.getMemoryBlock();
    d.theme.background = "images/test.png";
    auto project = root.getChildFile("test.artiplug"),
         theme = root.getChildFile("test.artitheme");
    pw::saveDocument(d, project);
    pw::saveDocument(d, theme, true);
    auto restored = pw::readDocument(project);
    assert(restored.zones.size() == 2 && restored.uuid == d.uuid &&
           restored.theme.controls[1].bounds.getX() == 101 &&
           restored.theme.controls[4].style == "vintage" &&
           restored.assets.size() == 2);
    auto t = pw::readDocument(theme, true);
    assert(t.zones.empty() && t.assets.size() == 1 &&
           t.assets.count("images/test.png"));
    pw::Engine engine;
    engine.load(restored);
    engine.prepare(44100, 512);
    AudioBuffer<float> output(2, 512);
    MidiBuffer midi;
    midi.addEvent(MidiMessage::noteOn(1, 36, juce::uint8(100)), 0);
    engine.render(output, midi);
    assert(output.getRMSLevel(0, 0, 512) > .001f);
    engine.synth.allNotesOff(0, false);
    engine.prepare(44100, 512);
    midi.clear();
    midi.addEvent(MidiMessage::noteOn(1, 37, juce::uint8(100)), 0);
    engine.render(output, midi);
    assert(output.getRMSLevel(0, 0, 512) < .00001f);
    auto bad = root.getChildFile("unsupported.sfz");
    bad.replaceWithText("<region> key=36 tune=40 sample=tone.wav");
    bool rejected = false;
    try {
      pw::importSfz(d, bad);
    } catch (const std::exception &) {
      rejected = true;
    }
    assert(rejected && d.zones.size() == 2);
    MemoryOutputStream archive;
    ZipFile::Builder zip;
    zip.addEntry(new MemoryInputStream("x", 1, true), 6, "../bad",
                 Time::getCurrentTime());
    assert(zip.writeToStream(archive, nullptr));
    MemoryInputStream malicious(archive.getData(), archive.getDataSize(),
                                false);
    rejected = false;
    try {
      pw::readZip(malicious);
    } catch (const std::exception &) {
      rejected = true;
    }
    assert(rejected);
    auto exported = pw::exportInstrument(restored, root, true, true, true);
    assert(exported.getChildFile("Source/Plugin.cpp").existsAsFile() &&
           exported.getChildFile("instrument.zip").existsAsFile() &&
           exported.getChildFile(".github/workflows/build.yml").existsAsFile() &&
           exported.getChildFile("Build-Mac.command").existsAsFile() &&
           exported.getChildFile("Build-Linux.sh").existsAsFile() &&
           exported.getChildFile("Build-Windows.bat").existsAsFile());
    assert(pw::pluginFolderName("vst3").isNotEmpty() &&
           pw::pluginExtension("au") == ".component" &&
           pw::pluginInstallRoot().getFullPathName().isNotEmpty());
    // Plugins built from the prebuilt engine: two instruments loaded side by
    // side must keep their own name, maker, id and sound.
    ScopedJuceInitialiser_GUI gui;
    const File engineZip(PW_ENGINE_ZIP);
    assert(engineZip.existsAsFile());
    struct Loaded {
      std::unique_ptr<AudioPluginInstance> plugin;
      int uid = 0;
    };
    auto buildAndLoad = [&](const String &name, const String &maker) {
      auto doc = restored;
      doc.name = name;
      doc.maker = maker;
      doc.uuid = Uuid().toString();
      FileInputStream engineIn(engineZip);
      assert(engineIn.openedOk());
      auto stage = root.getChildFile("plugins").getChildFile(
          String::toHexString(name.hashCode()));
      auto built = pw::createPlugins(doc, true, pw::hasAudioUnits, true, engineIn,
                                     stage);
      assert(built.size() == (pw::hasAudioUnits ? 3u : 2u));
      File vst3File, clapFile;
      for (auto &b : built) {
        assert(b.file.exists() && b.file.getFileNameWithoutExtension() == name);
        if (b.format == "vst3")
          vst3File = b.file;
        if (b.format == "clap")
          clapFile = b.file;
      }
#if !JUCE_MAC
      // Single-file CLAP: identity is appended to the binary.
      MemoryBlock tail;
      {
        FileInputStream in(clapFile);
        in.setPosition(in.getTotalLength() - pw_payload::trailerSize);
        in.readIntoMemoryBlock(tail, pw_payload::trailerSize);
        pw_payload::Trailer t;
        assert(pw_payload::parseTrailer(
            static_cast<const unsigned char *>(tail.getData()),
            std::uint64_t(in.getTotalLength()), t));
      }
#else
      assert(clapFile.getChildFile("Contents/Resources/instrument.pwi")
                 .existsAsFile());
#endif
      VST3PluginFormat format;
      OwnedArray<PluginDescription> types;
      format.findAllTypesForFile(types, vst3File.getFullPathName());
      assert(types.size() == 1);
      std::cout << "VST3: " << types[0]->name << " / "
                << types[0]->manufacturerName << " / "
                << String::toHexString(types[0]->uniqueId) << "\n";
      assert(types[0]->name == name && types[0]->manufacturerName == maker &&
             types[0]->isInstrument);
      String error;
      Loaded loaded;
      loaded.plugin =
          format.createInstanceFromDescription(*types[0], 44100, 512, error);
      assert(loaded.plugin != nullptr);
      loaded.uid = types[0]->uniqueId;
      return loaded;
    };
    auto first = buildAndLoad("PW Test A", "Maker A");
    auto second = buildAndLoad(CharPointer_UTF8("PW Test \xc3\x98 Flygel"),
                               CharPointer_UTF8("M\xc3\xa6rke B"));
    assert(first.uid != second.uid);
    for (auto *loaded : {&first, &second}) {
      auto &plugin = *loaded->plugin;
      plugin.enableAllBuses();
      plugin.prepareToPlay(44100, 512);
      AudioBuffer<float> pluginOut(2, 512);
      float rms = 0;
      for (int block = 0; block < 4 && rms < .001f; ++block) {
        pluginOut.clear();
        MidiBuffer notes;
        if (block == 0)
          notes.addEvent(MidiMessage::noteOn(1, 36, juce::uint8(100)), 0);
        plugin.processBlock(pluginOut, notes);
        rms = std::max(rms, pluginOut.getRMSLevel(0, 0, 512));
      }
      std::cout << "Sound from " << plugin.getName() << ": rms " << rms << "\n";
      assert(rms > .001f);
      plugin.releaseResources();
    }
    first.plugin.reset();
    second.plugin.reset();
    std::cout
        << "PASS: SFZ, coverage gaps, audio, project roundtrip, asset-only "
           "template, unsafe/unsupported input rejection, source export, "
           "prebuilt engine plugins (identity, side by side, sound)\n"
        << "Exported: " << exported.getFullPathName() << "\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
