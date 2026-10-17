// SPDX-License-Identifier: GPL-3.0-only
#include "InstrumentData.h"
#include "InstrumentView.h"
using namespace juce;
class InstrumentProcessor final : public AudioProcessor {
public:
  pw::Document document;
  pw::Engine engine;
  AudioProcessorValueTreeState state;
  static AudioProcessorValueTreeState::ParameterLayout layout() {
    AudioProcessorValueTreeState::ParameterLayout l;
    for (auto p : pw::parameters()) {
      // Discrete parameters are published as such, so hosts show proper
      // menus/switches for them.
      if (p.id == "filtertype")
        l.add(std::make_unique<AudioParameterChoice>(
            ParameterID(p.id, 1), p.name, pw::filterTypes,
            roundToInt(p.initial)));
      else if (p.id == "enabled")
        l.add(std::make_unique<AudioParameterBool>(ParameterID(p.id, 1),
                                                   p.name, p.initial > .5f));
      else
        l.add(std::make_unique<AudioParameterFloat>(
            ParameterID(p.id, 1), p.name,
            NormalisableRange<float>(p.min, p.max, p.step, p.skew),
            p.initial));
    }
    return l;
  }
  InstrumentProcessor()
      : AudioProcessor(BusesProperties().withOutput(
            "Output", AudioChannelSet::stereo(), true)),
        state(*this, nullptr, "PARAMETERS", layout()) {
    MemoryInputStream in(
        InstrumentData::instrument_zip,
        static_cast<std::size_t>(InstrumentData::instrument_zipSize), false);
    try {
      document = pw::readDocument(in);
      engine.load(document);
      for (auto p : pw::parameters())
        state.getParameter(p.id)->setValueNotifyingHost(
            state.getParameter(p.id)->convertTo0to1(
                document.defaults.at(p.id)));
    } catch (const std::exception &e) {
      loadError = e.what();
    }
  }
  String loadError;
  const String getName() const override { return JucePlugin_Name; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 12.; }
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const String getProgramName(int) override { return "Default"; }
  void changeProgramName(int, const String &) override {}
  void prepareToPlay(double sr, int block) override {
    engine.prepare(sr, block);
  }
  void releaseResources() override {}
  bool isBusesLayoutSupported(const BusesLayout &l) const override {
    return l.getMainOutputChannelSet() == AudioChannelSet::stereo();
  }
  void processBlock(AudioBuffer<float> &b, MidiBuffer &midi) override {
    for (auto p : pw::parameters())
      engine.values.at(p.id) = state.getRawParameterValue(p.id)->load();
    engine.render(b, midi);
    midi.clear();
  }
  bool hasEditor() const override { return true; }
  AudioProcessorEditor *createEditor() override;
  void getStateInformation(MemoryBlock &out) override {
    auto xml = state.copyState().createXml();
    copyXmlToBinary(*xml, out);
  }
  void setStateInformation(const void *data, int size) override {
    auto xml = getXmlFromBinary(data, size);
    if (xml && xml->hasTagName("PARAMETERS")) {
      state.replaceState(ValueTree::fromXml(*xml));
      for (auto p : pw::parameters())
        engine.values.at(p.id) = state.getRawParameterValue(p.id)->load();
    }
  }
};
class InstrumentEditor final : public AudioProcessorEditor {
  InstrumentProcessor &processor;
  pw::RoyalLook look;
  pw::PreviewHost preview;
  Label error;

public:
  explicit InstrumentEditor(InstrumentProcessor &p)
      : AudioProcessorEditor(p), processor(p),
        preview(p.document, p.engine.values, p.engine.keyboard) {
    setLookAndFeel(&look);
    addAndMakeVisible(preview);
    preview.view.changed = [this](const String &id, float value) {
      auto *param = processor.state.getParameter(id);
      param->setValueNotifyingHost(param->convertTo0to1(value));
    };
    preview.view.gesture = [this](const String &id, bool begin) {
      auto *param = processor.state.getParameter(id);
      if (begin)
        param->beginChangeGesture();
      else
        param->endChangeGesture();
    };
    if (p.loadError.isNotEmpty()) {
      addAndMakeVisible(error);
      error.setText(p.loadError, dontSendNotification);
      error.setColour(Label::backgroundColourId, Colours::darkred);
    }
    setResizable(true, true);
    setResizeLimits(700, 420, 1800, 1080);
    getConstrainer()->setFixedAspectRatio(1000. / 600.);
    setSize(1000, 600);
  }
  ~InstrumentEditor() override { setLookAndFeel(nullptr); }
  void resized() override {
    preview.setBounds(getLocalBounds());
    error.setBounds(20, 20, getWidth() - 40, 60);
  }
};
AudioProcessorEditor *InstrumentProcessor::createEditor() {
  return new InstrumentEditor(*this);
}
AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new InstrumentProcessor();
}
