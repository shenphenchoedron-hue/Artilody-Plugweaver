// SPDX-License-Identifier: GPL-3.0-only
// Offscreen renderer: Visual <out.png> <screen 0-3, or 9 = plugin UI>
//                            [instrument.sfz] [background.png] [logo.png]
#include "../Source/Main.cpp"
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  auto arg = [&](int i) {
    return argc > i ? juce::File(juce::String(argv[i])) : juce::File();
  };
  int screen = argc > 2 ? juce::String(argv[2]).getIntValue() : 2;
  juce::Image image;
  if (screen == 9) {
    pw::Document d;
    if (arg(3).existsAsFile())
      pw::importSfz(d, arg(3));
    d.name = "Velvet Grand";
    d.maker = "Artilody";
    if (arg(4).existsAsFile())
      d.theme.background = pw::addImage(d, arg(4));
    if (arg(5).existsAsFile())
      d.theme.logo = pw::addImage(d, arg(5));
    pw::Engine engine;
    engine.values.setDefaults(d);
    pw::RoyalLook look;
    juce::LookAndFeel::setDefaultLookAndFeel(&look);
    pw::PreviewHost host(d, engine.values, engine.keyboard);
    host.setSize(1500, 900);
    engine.keyboard.noteOn(1, 60, .8f);
    engine.keyboard.noteOn(1, 64, .8f);
    image = host.createComponentSnapshot(host.getLocalBounds());
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
  } else {
    Main view;
    view.setSize(argc > 6 ? juce::String(argv[6]).getIntValue() : 1480,
                 argc > 7 ? juce::String(argv[7]).getIntValue() : 900);
    view.qaPreview(screen, arg(3), arg(4), arg(5));
    if (argc > 8)
      view.qaStyles(juce::String(argv[8]).getIntValue());
    image = view.createComponentSnapshot(view.getLocalBounds());
  }
  juce::File file{juce::String(argv[1])};
  file.deleteFile();
  auto out = file.createOutputStream();
  juce::PNGImageFormat png;
  return png.writeImageToStream(image, *out) ? 0 : 1;
}
