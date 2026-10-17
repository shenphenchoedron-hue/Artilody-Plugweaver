// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Model.h"
namespace pw {
struct Values {
  std::atomic<float> gain{.7f}, attack{.01f}, decay{.3f}, sustain{.8f},
      release{.5f}, cutoff{18000}, resonance{.7f}, reverb{.15f}, enabled{1},
      filtertype{0};
  std::atomic<float> &at(const String &id) {
    if (id == "attack")
      return attack;
    if (id == "decay")
      return decay;
    if (id == "sustain")
      return sustain;
    if (id == "release")
      return release;
    if (id == "cutoff")
      return cutoff;
    if (id == "resonance")
      return resonance;
    if (id == "reverb")
      return reverb;
    if (id == "enabled")
      return enabled;
    if (id == "filtertype")
      return filtertype;
    return gain;
  }
  void setDefaults(const Document &d) {
    for (const auto &p : parameters())
      at(p.id) = d.defaults.at(p.id);
  }
};
struct SampleData {
  AudioBuffer<float> data;
  double rate = 44100;
};
struct Sound final : SynthesiserSound {
  std::shared_ptr<SampleData> sample;
  Zone zone;
  bool appliesToNote(int n) override { return n >= zone.low && n <= zone.high; }
  bool appliesToChannel(int) override { return true; }
};
class Voice final : public SynthesiserVoice {
  Values &values;
  std::shared_ptr<SampleData> sample;
  ADSR envelope;
  double pos = 0, step = 1, baseStep = 1;
  float velocity = 1;

public:
  explicit Voice(Values &v) : values(v) {}
  bool canPlaySound(SynthesiserSound *s) override {
    return dynamic_cast<Sound *>(s) != nullptr;
  }
  void startNote(int note, float vel, SynthesiserSound *s, int wheel) override {
    auto *sound = dynamic_cast<Sound *>(s);
    if (!sound)
      return;
    int v = roundToInt(vel * 127);
    if (v < sound->zone.velLow || v > sound->zone.velHigh) {
      clearCurrentNote();
      return;
    }
    sample = sound->sample;
    pos = 0;
    velocity = vel;
    baseStep = sample->rate / getSampleRate() *
               std::pow(2., (note - sound->zone.root) / 12.);
    pitchWheelMoved(wheel);
    envelope.setSampleRate(getSampleRate());
    envelope.setParameters({values.attack.load(), values.decay.load(),
                            values.sustain.load(), values.release.load()});
    envelope.noteOn();
  }
  void stopNote(float, bool tail) override {
    if (tail)
      envelope.noteOff();
    else {
      envelope.reset();
      clearCurrentNote();
      sample.reset();
    }
  }
  void pitchWheelMoved(int value) override {
    step = baseStep * std::pow(2., (value - 8192) / 8192. * 2. / 12.);
  }
  void controllerMoved(int, int) override {}
  void renderNextBlock(AudioBuffer<float> &out, int start, int count) override {
    if (!sample)
      return;
    envelope.setParameters({values.attack.load(), values.decay.load(),
                            values.sustain.load(), values.release.load()});
    for (int i = 0; i < count; ++i) {
      int a = static_cast<int>(pos);
      if (a >= sample->data.getNumSamples() || !envelope.isActive()) {
        clearCurrentNote();
        sample.reset();
        break;
      }
      int b = jmin(a + 1, sample->data.getNumSamples() - 1);
      float mix = static_cast<float>(pos - a),
            amp = envelope.getNextSample() * velocity;
      for (int ch = 0; ch < out.getNumChannels(); ++ch) {
        auto *x = sample->data.getReadPointer(
            jmin(ch, sample->data.getNumChannels() - 1));
        out.addSample(ch, start + i, (x[a] + (x[b] - x[a]) * mix) * amp);
      }
      pos += step;
    }
  }
};
class Engine {
public:
  Values values;
  Synthesiser synth;
  MidiKeyboardState keyboard;

private:
  dsp::StateVariableTPTFilter<float> filter;
  Reverb reverb;
  double rate = 44100;

public:
  Engine() {
    for (int i = 0; i < 32; ++i)
      synth.addVoice(new Voice(values));
    synth.setNoteStealingEnabled(true);
  }
  using Prepared = std::vector<SynthesiserSound::Ptr>;
  static Prepared decode(const Document &doc) {
    AudioFormatManager formats;
    formats.registerBasicFormats();
    std::map<String, std::shared_ptr<SampleData>> decoded;
    int64 bytes = 0;
    for (auto z : doc.zones)
      if (!decoded.count(z.asset)) {
        const auto &b = doc.assets.at(z.asset);
        std::unique_ptr<AudioFormatReader> r(formats.createReaderFor(
            std::make_unique<MemoryInputStream>(b, false)));
        require(bool(r) && r->lengthInSamples > 0 &&
                    r->lengthInSamples <= 46080000 && r->numChannels >= 1 &&
                    r->numChannels <= 2 && r->sampleRate >= 8000 &&
                    r->sampleRate <= 192000,
                "Invalid embedded sample");
        bytes += r->lengthInSamples * r->numChannels * 4;
        require(bytes <= 256LL * 1024 * 1024,
                "Decoded instrument exceeds 256 MB");
        auto data = std::make_shared<SampleData>();
        data->rate = r->sampleRate;
        data->data.setSize(static_cast<int>(r->numChannels),
                           static_cast<int>(r->lengthInSamples));
        require(
            r->read(&data->data, 0, data->data.getNumSamples(), 0, true, true),
            "Sample decoding failed");
        decoded[z.asset] = data;
      }
    Prepared result;
    for (auto z : doc.zones) {
      auto *sound = new Sound;
      sound->sample = decoded.at(z.asset);
      sound->zone = z;
      result.emplace_back(sound);
    }
    return result;
  }
  void install(const Document &doc, const Prepared &sounds) {
    synth.allNotesOff(0, false);
    synth.clearSounds();
    keyboard.reset();
    filter.reset();
    reverb.reset();
    for (auto sound : sounds)
      synth.addSound(sound);
    values.setDefaults(doc);
  }
  void load(const Document &doc) { install(doc, decode(doc)); }
  void prepare(double sr, int block) {
    rate = sr;
    synth.setCurrentPlaybackSampleRate(sr);
    filter.prepare({sr, static_cast<uint32>(jmax(1, block)), 2});
    filter.setType(dsp::StateVariableTPTFilterType::lowpass);
    reverb.setSampleRate(sr);
    reverb.reset();
  }
  void render(AudioBuffer<float> &out, MidiBuffer &midi) {
    ScopedNoDenormals denormals;
    out.clear();
    keyboard.processNextMidiBuffer(midi, 0, out.getNumSamples(), true);
    synth.renderNextBlock(out, midi, 0, out.getNumSamples());
    const int type = roundToInt(values.filtertype.load());
    filter.setType(type == 1   ? dsp::StateVariableTPTFilterType::bandpass
                   : type == 2 ? dsp::StateVariableTPTFilterType::highpass
                               : dsp::StateVariableTPTFilterType::lowpass);
    filter.setCutoffFrequency(
        jlimit(20.f, static_cast<float>(rate * .45), values.cutoff.load()));
    filter.setResonance(values.resonance.load());
    dsp::AudioBlock<float> b(out);
    dsp::ProcessContextReplacing<float> context(b);
    filter.process(context);
    Reverb::Parameters p;
    p.roomSize = .65f;
    p.damping = .5f;
    p.wetLevel = values.reverb.load() * .4f;
    p.dryLevel = 1.f;
    p.width = 1.f;
    reverb.setParameters(p);
    if (out.getNumChannels() == 2)
      reverb.processStereo(out.getWritePointer(0), out.getWritePointer(1),
                           out.getNumSamples());
    out.applyGain(values.enabled.load() > .5f ? values.gain.load() : 0.f);
  }
};
} // namespace pw
