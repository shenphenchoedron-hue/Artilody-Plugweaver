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
    std::cout
        << "PASS: SFZ, coverage gaps, audio, project roundtrip, asset-only "
           "template, unsafe/unsupported input rejection, source export\n"
        << "Exported: " << exported.getFullPathName() << "\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
