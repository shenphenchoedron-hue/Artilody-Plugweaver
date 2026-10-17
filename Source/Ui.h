// SPDX-License-Identifier: GPL-3.0-only
// Visual building blocks for the Plugweaver builder app. The generated plugin
// only uses EngineTemplate/Source/InstrumentView.h.
#pragma once
#include "InstrumentView.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include "BuilderAssets.h"
namespace ui {
using namespace juce;
namespace col {
const Colour bgTop{0xff0c1d55}, bgBottom{0xff050a24}, panel{0xff0a1540},
    panelEdge{0xff1f3a8c}, card{0xff0c1a4c}, field{0xff081235},
    fieldEdge{0xff284596}, text{0xfff2f4ff}, muted{0xff9fb2e6},
    dim{0xff6f82b8}, accent{0xffff8a2b}, accentHot{0xffffa24d},
    blue{0xff2f62ff}, blueEdge{0xff4f7cff}, selection{0xff1d48d8};
}
inline Font sans(float size, bool bold = false) {
  return Font(size, bold ? Font::bold : Font::plain);
}
inline Font serif(float size) { return Font("Georgia", size, Font::plain); }

// Vector Artilody ribbon (1024 x 1024 design grid), used as a fallback when
// the embedded logo image cannot be loaded.
inline void drawLogoRibbon(Graphics &g, Rectangle<float> area) {
  Path blue, gold;
  blue.startNewSubPath(200, 710);
  blue.cubicTo(330, 170, 490, 210, 520, 510);
  blue.cubicTo(550, 810, 700, 780, 835, 355);
  gold.startNewSubPath(190, 705);
  gold.cubicTo(360, 135, 475, 215, 510, 510);
  gold.cubicTo(545, 805, 680, 795, 835, 355);
  // Bounds of the stroked ribbon in icon coordinates.
  Rectangle<float> source(140, 195, 750, 610);
  auto t = RectanglePlacement(RectanglePlacement::centred)
               .getTransformToFit(source, area);
  float scale = std::sqrt(std::abs(t.getDeterminant()));
  blue.applyTransform(t);
  gold.applyTransform(t);
  PathStrokeType round(100 * scale, PathStrokeType::curved,
                       PathStrokeType::rounded);
  g.setColour(Colour(0x5a456cff));
  g.strokePath(blue, PathStrokeType(116 * scale, PathStrokeType::curved,
                                    PathStrokeType::rounded));
  g.setColour(Colour(0xff456cff));
  g.strokePath(blue, round);
  auto gb = gold.getBounds();
  ColourGradient gradient(Colour(0xffffe0a1), 0, gb.getY(), Colour(0xffe85b21),
                          0, gb.getBottom(), false);
  gradient.addColour(.5, Colour(0xffff922e));
  g.setGradientFill(gradient);
  g.strokePath(gold, PathStrokeType(62 * scale, PathStrokeType::curved,
                                    PathStrokeType::rounded));
}

// The Plugweaver logo from Assets/logo.png (embedded in the app).
inline const Image &appLogo() {
  static const Image logo = ImageFileFormat::loadFrom(
      BuilderAssets::logo_png, static_cast<size_t>(BuilderAssets::logo_pngSize));
  return logo;
}
inline void drawAppLogo(Graphics &g, Rectangle<float> area) {
  const auto &logo = appLogo();
  if (!logo.isValid())
    return drawLogoRibbon(g, area);
  Graphics::ScopedSaveState state(g);
  g.setImageResamplingQuality(Graphics::highResamplingQuality);
  g.drawImage(logo, area, RectanglePlacement::centred);
}
inline void drawIcon(Graphics &g, const String &name, Rectangle<float> r,
                     Colour c) {
  g.setColour(c);
  float s = jmin(r.getWidth(), r.getHeight());
  r = r.withSizeKeepingCentre(s, s);
  float lw = jmax(1.4f, s * .09f);
  PathStrokeType stroke(lw, PathStrokeType::curved, PathStrokeType::rounded);
  auto x = [&](float f) { return r.getX() + f * s; };
  auto y = [&](float f) { return r.getY() + f * s; };
  Path p;
  if (name == "folder") {
    p.startNewSubPath(x(.08f), y(.28f));
    p.lineTo(x(.38f), y(.28f));
    p.lineTo(x(.46f), y(.38f));
    p.lineTo(x(.92f), y(.38f));
    p.lineTo(x(.92f), y(.82f));
    p.lineTo(x(.08f), y(.82f));
    p.closeSubPath();
    g.fillPath(p);
  } else if (name == "save") {
    p.addRoundedRectangle(x(.12f), y(.12f), s * .76f, s * .76f, s * .1f);
    g.strokePath(p, stroke);
    g.fillRect(Rectangle<float>(x(.3f), y(.12f), s * .4f, s * .22f));
    g.drawRect(Rectangle<float>(x(.28f), y(.52f), s * .44f, s * .3f), lw);
  } else if (name == "cube") {
    p.startNewSubPath(x(.5f), y(.08f));
    p.lineTo(x(.88f), y(.28f));
    p.lineTo(x(.88f), y(.72f));
    p.lineTo(x(.5f), y(.92f));
    p.lineTo(x(.12f), y(.72f));
    p.lineTo(x(.12f), y(.28f));
    p.closeSubPath();
    p.startNewSubPath(x(.12f), y(.28f));
    p.lineTo(x(.5f), y(.48f));
    p.lineTo(x(.88f), y(.28f));
    p.startNewSubPath(x(.5f), y(.48f));
    p.lineTo(x(.5f), y(.92f));
    g.strokePath(p, stroke);
  } else if (name == "layers") {
    for (int i = 2; i >= 0; --i) {
      Path l;
      float o = float(i) * .17f;
      l.startNewSubPath(x(.5f), y(.12f + o));
      l.lineTo(x(.9f), y(.32f + o));
      l.lineTo(x(.5f), y(.52f + o));
      l.lineTo(x(.1f), y(.32f + o));
      l.closeSubPath();
      if (i == 0)
        g.fillPath(l);
      else
        g.strokePath(l, stroke);
    }
  } else if (name == "wave") {
    const float h[] = {.18f, .42f, .7f, .32f, .55f, .24f};
    for (int i = 0; i < 6; ++i) {
      float xx = x(.12f + float(i) * .152f);
      g.drawLine(xx, y(.5f - h[i] * .5f), xx, y(.5f + h[i] * .5f), lw * 1.2f);
    }
  } else if (name == "palette") {
    p.addEllipse(x(.08f), y(.1f), s * .84f, s * .8f);
    g.fillPath(p);
    g.setColour(col::bgBottom);
    for (auto pt : {Point<float>(.3f, .36f), Point<float>(.5f, .25f),
                    Point<float>(.7f, .36f), Point<float>(.3f, .6f)})
      g.fillEllipse(x(pt.x) - s * .07f, y(pt.y) - s * .07f, s * .14f, s * .14f);
    g.fillEllipse(x(.58f), y(.58f), s * .2f, s * .2f);
  } else if (name == "upload" || name == "import") {
    p.startNewSubPath(x(.5f), y(.62f));
    p.lineTo(x(.5f), y(.12f));
    p.startNewSubPath(x(.3f), y(.3f));
    p.lineTo(x(.5f), y(.1f));
    p.lineTo(x(.7f), y(.3f));
    p.startNewSubPath(x(.14f), y(.6f));
    p.lineTo(x(.14f), y(.86f));
    p.lineTo(x(.86f), y(.86f));
    p.lineTo(x(.86f), y(.6f));
    g.strokePath(p, stroke);
  } else if (name == "image") {
    p.addRoundedRectangle(x(.1f), y(.16f), s * .8f, s * .68f, s * .08f);
    g.strokePath(p, stroke);
    Path m;
    m.startNewSubPath(x(.18f), y(.74f));
    m.lineTo(x(.42f), y(.46f));
    m.lineTo(x(.58f), y(.62f));
    m.lineTo(x(.68f), y(.52f));
    m.lineTo(x(.82f), y(.74f));
    g.strokePath(m, stroke);
    g.fillEllipse(x(.62f), y(.28f), s * .12f, s * .12f);
  } else if (name == "gear") {
    for (int i = 0; i < 8; ++i) {
      float a = float(i) * MathConstants<float>::twoPi / 8;
      g.drawLine(x(.5f) + std::cos(a) * s * .3f, y(.5f) + std::sin(a) * s * .3f,
                 x(.5f) + std::cos(a) * s * .44f,
                 y(.5f) + std::sin(a) * s * .44f, s * .14f);
    }
    g.fillEllipse(x(.2f), y(.2f), s * .6f, s * .6f);
    g.setColour(col::panel);
    g.fillEllipse(x(.38f), y(.38f), s * .24f, s * .24f);
  } else if (name == "refresh") {
    p.addCentredArc(x(.5f), y(.5f), s * .34f, s * .34f, 0, .6f, 5.6f, true);
    g.strokePath(p, stroke);
    Path a;
    a.addTriangle(x(.62f), y(.08f), x(.86f), y(.22f), x(.62f), y(.36f));
    a.applyTransform(AffineTransform::rotation(.5f, x(.7f), y(.22f)));
    g.fillPath(a);
  } else if (name == "info") {
    p.addEllipse(x(.1f), y(.1f), s * .8f, s * .8f);
    g.strokePath(p, PathStrokeType(lw * .8f));
    g.fillEllipse(x(.45f), y(.25f), s * .1f, s * .1f);
    g.fillRect(x(.455f), y(.42f), s * .09f, s * .32f);
  } else if (name == "chevron") {
    p.startNewSubPath(x(.25f), y(.38f));
    p.lineTo(x(.5f), y(.62f));
    p.lineTo(x(.75f), y(.38f));
    g.strokePath(p, stroke);
  } else if (name == "left" || name == "right") {
    bool l = name == "left";
    p.startNewSubPath(x(l ? .6f : .4f), y(.25f));
    p.lineTo(x(l ? .36f : .64f), y(.5f));
    p.lineTo(x(l ? .6f : .4f), y(.75f));
    g.strokePath(p, stroke);
  } else if (name == "plus") {
    g.drawLine(x(.5f), y(.18f), x(.5f), y(.82f), lw * 1.2f);
    g.drawLine(x(.18f), y(.5f), x(.82f), y(.5f), lw * 1.2f);
  } else if (name == "trash") {
    p.startNewSubPath(x(.2f), y(.26f));
    p.lineTo(x(.8f), y(.26f));
    p.startNewSubPath(x(.4f), y(.26f));
    p.lineTo(x(.42f), y(.14f));
    p.lineTo(x(.58f), y(.14f));
    p.lineTo(x(.6f), y(.26f));
    p.startNewSubPath(x(.26f), y(.3f));
    p.lineTo(x(.32f), y(.86f));
    p.lineTo(x(.68f), y(.86f));
    p.lineTo(x(.74f), y(.3f));
    g.strokePath(p, stroke);
  } else if (name == "play") {
    p.addTriangle(x(.28f), y(.18f), x(.82f), y(.5f), x(.28f), y(.82f));
    g.fillPath(p);
  } else if (name == "dots") {
    for (float f : {.22f, .5f, .78f})
      g.fillEllipse(x(.5f) - s * .07f, y(f) - s * .07f, s * .14f, s * .14f);
  } else if (name == "text") {
    g.setFont(serif(s * .95f));
    g.drawText("T", r, Justification::centred);
  } else if (name == "knob") {
    p.addCentredArc(x(.5f), y(.52f), s * .36f, s * .36f, 0, -2.4f, 2.4f, true);
    g.strokePath(p, stroke);
    g.drawLine(x(.5f), y(.52f), x(.5f), y(.24f), lw);
  } else if (name == "slider") {
    g.drawLine(x(.1f), y(.5f), x(.9f), y(.5f), lw);
    g.fillEllipse(x(.48f), y(.36f), s * .28f, s * .28f);
  } else if (name == "switch") {
    p.addRoundedRectangle(x(.08f), y(.3f), s * .84f, s * .4f, s * .2f);
    g.strokePath(p, stroke);
    g.fillEllipse(x(.56f), y(.36f), s * .28f, s * .28f);
  } else if (name == "logo") {
    drawAppLogo(g, r);
  } else if (name == "alignLeft" || name == "alignCentre" ||
             name == "alignRight") {
    const float w[] = {.76f, .5f, .76f, .5f};
    for (int i = 0; i < 4; ++i) {
      float ww = s * w[i], yy = y(.22f + float(i) * .19f);
      float xx = name == "alignLeft"     ? x(.12f)
                 : name == "alignRight" ? x(.88f) - ww
                                        : x(.5f) - ww * .5f;
      g.drawLine(xx, yy, xx + ww, yy, lw);
    }
  } else if (name == "vst3" || name == "au" || name == "clap") {
    p.addEllipse(x(.12f), y(.12f), s * .76f, s * .76f);
    g.strokePath(p, PathStrokeType(lw * .9f));
    if (name == "vst3")
      g.fillEllipse(x(.32f), y(.32f), s * .36f, s * .36f);
    else {
      Path n;
      n.startNewSubPath(x(.3f), y(.66f));
      n.lineTo(x(.5f), y(.3f));
      n.lineTo(x(.7f), y(.66f));
      if (name == "clap")
        n.lineTo(x(.3f), y(.66f));
      g.strokePath(n, stroke);
    }
  } else if (name == "speaker") {
    p.startNewSubPath(x(.12f), y(.38f));
    p.lineTo(x(.3f), y(.38f));
    p.lineTo(x(.52f), y(.18f));
    p.lineTo(x(.52f), y(.82f));
    p.lineTo(x(.3f), y(.62f));
    p.lineTo(x(.12f), y(.62f));
    p.closeSubPath();
    g.fillPath(p);
    Path w;
    w.addCentredArc(x(.56f), y(.5f), s * .18f, s * .18f, 0, .5f, 2.6f, true);
    w.addCentredArc(x(.56f), y(.5f), s * .32f, s * .32f, 0, .5f, 2.6f, true);
    g.strokePath(w, stroke);
  }
}

// Draws a panel/card like the reference design.
inline void drawPanel(Graphics &g, Rectangle<float> r, float radius = 14.f,
                      bool raised = false) {
  g.setColour(Colours::black.withAlpha(.35f));
  g.fillRoundedRectangle(r.translated(0, 3), radius);
  g.setGradientFill(ColourGradient(
      (raised ? col::card : col::panel).brighter(.06f).withAlpha(.96f), 0,
      r.getY(), col::bgBottom.withAlpha(.94f), 0, r.getBottom(), false));
  g.fillRoundedRectangle(r, radius);
  g.setColour(col::panelEdge.withAlpha(.85f));
  g.drawRoundedRectangle(r.reduced(.5f), radius, 1.f);
}
inline void drawTitle(Graphics &g, const String &title, Rectangle<int> r,
                      const String &icon = {}, float size = 19.f) {
  if (icon.isNotEmpty()) {
    drawIcon(g, icon, r.removeFromLeft(int(size) + 6).toFloat().reduced(1),
             icon == "gear" ? col::text : col::text);
    r.removeFromLeft(6);
  }
  g.setColour(col::text);
  g.setFont(sans(size, true));
  g.drawText(title, r, Justification::centredLeft);
}
inline void drawSubheading(Graphics &g, const String &text, Rectangle<int> r) {
  g.setColour(col::text.withAlpha(.92f));
  g.setFont(sans(15.f));
  g.drawText(text, r, Justification::centredLeft);
}
inline void drawCover(Graphics &g, const Image &image, Rectangle<float> r,
                      float radius) {
  Graphics::ScopedSaveState state(g);
  Path clip;
  clip.addRoundedRectangle(r, radius);
  g.reduceClipRegion(clip);
  g.drawImageWithin(image, roundToInt(r.getX()), roundToInt(r.getY()),
                    roundToInt(r.getWidth()), roundToInt(r.getHeight()),
                    RectanglePlacement::fillDestination);
}
inline String noteName(int midi) {
  static const char *names[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                "F#", "G",  "G#", "A",  "A#", "B"};
  return String(names[((midi % 12) + 12) % 12]) + String(midi / 12 - 1);
}

class AppLook : public pw::RoyalLook {
public:
  AppLook() {
    ink = col::text;
    setColour(Label::textColourId, col::text);
    setColour(TextEditor::backgroundColourId, col::field);
    setColour(TextEditor::textColourId, col::text);
    setColour(TextEditor::outlineColourId, col::fieldEdge);
    setColour(TextEditor::focusedOutlineColourId, col::blueEdge);
    setColour(TextEditor::highlightColourId, col::selection);
    setColour(CaretComponent::caretColourId, col::accent);
    setColour(ComboBox::backgroundColourId, col::field);
    setColour(ComboBox::textColourId, col::text);
    setColour(ComboBox::outlineColourId, col::fieldEdge);
    setColour(PopupMenu::backgroundColourId, Colour(0xff0b1847));
    setColour(PopupMenu::textColourId, col::text);
    setColour(PopupMenu::highlightedBackgroundColourId, col::selection);
    setColour(ListBox::backgroundColourId, Colours::transparentBlack);
    setColour(ScrollBar::thumbColourId, col::fieldEdge);
    setColour(ToggleButton::textColourId, col::text);
    setColour(ToggleButton::tickColourId, Colours::white);
    setColour(AlertWindow::backgroundColourId, Colour(0xff0b1847));
    setColour(AlertWindow::textColourId, col::text);
    setColour(AlertWindow::outlineColourId, col::panelEdge);
    setColour(ResizableWindow::backgroundColourId, Colour(0xff0b1847));
    setColour(TableHeaderComponent::backgroundColourId, col::panel);
    setColour(TableHeaderComponent::textColourId, col::muted);
    setColour(TableHeaderComponent::outlineColourId, col::panelEdge);
    setColour(ColourSelector::backgroundColourId, Colour(0xff0b1847));
  }
  Font getTextButtonFont(TextButton &, int) override { return sans(15.f); }
  Font getComboBoxFont(ComboBox &) override { return sans(15.f); }
  Font getPopupMenuFont() override { return sans(15.f); }
  Font getLabelFont(Label &l) override { return l.getFont(); }
  void drawButtonBackground(Graphics &g, Button &b, const Colour &, bool hover,
                            bool down) override {
    auto box = b.getLocalBounds().toFloat().reduced(1);
    String style = b.getProperties()["style"].toString();
    bool on = b.getToggleState();
    if (style == "tab") {
      if (on || hover) {
        g.setGradientFill(ColourGradient(
            col::blue.withAlpha(on ? .22f : .1f), box.getCentreX(),
            box.getBottom(), col::blue.withAlpha(0.f), box.getCentreX(),
            box.getY(), false));
        g.fillRect(box);
      }
      if (on) {
        auto line = box.removeFromBottom(3).reduced(4, 0);
        g.setColour(col::accent.withAlpha(.35f));
        g.fillRoundedRectangle(line.expanded(0, 2), 2);
        g.setColour(col::accent);
        g.fillRoundedRectangle(line, 1.5f);
      }
      return;
    }
    if (style == "flat") {
      if (hover || on) {
        g.setColour(col::selection.withAlpha(on ? .9f : .35f));
        g.fillRoundedRectangle(box, 7);
      }
      return;
    }
    if (style == "segment") {
      g.setColour(on ? col::selection : col::field);
      g.fillRoundedRectangle(box, 7);
      g.setColour(on ? col::blueEdge : col::fieldEdge);
      g.drawRoundedRectangle(box, 7, 1);
      return;
    }
    Colour top, bottom, edge;
    float radius = 9;
    if (style == "primary") {
      top = col::accentHot;
      bottom = Colour(0xffef6a12);
      edge = Colour(0xffffc07a);
      radius = 10;
    } else if (style == "blue") {
      top = Colour(0xff3d72ff);
      bottom = Colour(0xff1d43d6);
      edge = Colour(0xff7096ff);
    } else {
      top = Colour(0xff13235c);
      bottom = Colour(0xff0a1540);
      edge = col::fieldEdge;
    }
    if (hover) {
      top = top.brighter(.12f);
      bottom = bottom.brighter(.12f);
    }
    if (down) {
      top = top.darker(.15f);
      bottom = bottom.darker(.15f);
    }
    float alpha = b.isEnabled() ? 1.f : .4f;
    if (style == "primary") {
      g.setColour(col::accent.withAlpha(.18f * alpha));
      g.fillRoundedRectangle(box.expanded(3), radius + 3);
    }
    g.setGradientFill(ColourGradient(top.withMultipliedAlpha(alpha), 0,
                                     box.getY(),
                                     bottom.withMultipliedAlpha(alpha), 0,
                                     box.getBottom(), false));
    g.fillRoundedRectangle(box, radius);
    g.setColour(edge.withMultipliedAlpha(alpha * .9f));
    g.drawRoundedRectangle(box, radius, 1);
  }
  void drawButtonText(Graphics &g, TextButton &b, bool hover, bool) override {
    String style = b.getProperties()["style"].toString(),
           icon = b.getProperties()["icon"].toString();
    bool on = b.getToggleState();
    auto colour = col::text;
    if (style == "tab")
      colour = on ? Colours::white : (hover ? col::text : col::muted);
    if (!b.isEnabled())
      colour = colour.withAlpha(.4f);
    auto r = b.getLocalBounds().reduced(8, 4);
    auto text = b.getButtonText();
    bool bold = style == "primary" || style == "blue";
    auto font = sans(style == "tab" ? 17.f : (bold ? 16.f : 15.f), bold);
    float iconSize = style == "tab" ? 24.f : 18.f;
    if (text.isEmpty() && icon.isNotEmpty()) {
      drawIcon(g, icon, r.toFloat().withSizeKeepingCentre(iconSize, iconSize),
               colour);
      return;
    }
    float textW = font.getStringWidthFloat(text);
    float total = textW + (icon.isNotEmpty() ? iconSize + 10 : 0);
    float x0 = float(r.getCentreX()) - total * .5f;
    if (b.getProperties()["leftAlign"])
      x0 = float(r.getX() + 4);
    if (icon.isNotEmpty()) {
      drawIcon(g, icon,
               {x0, float(r.getCentreY()) - iconSize * .5f, iconSize, iconSize},
               style == "tab" && on ? col::text : colour);
      x0 += iconSize + 10;
    }
    g.setColour(colour);
    g.setFont(font);
    g.drawText(text, Rectangle<float>(x0, float(r.getY()), textW + 2,
                                      float(r.getHeight())),
               Justification::centredLeft, false);
  }
  void drawToggleButton(Graphics &g, ToggleButton &b, bool hover,
                        bool) override {
    auto r = b.getLocalBounds().toFloat();
    bool on = b.getToggleState();
    if (b.getProperties()["style"].toString() == "switch") {
      auto pill = r.removeFromRight(46).withSizeKeepingCentre(46, 26);
      g.setColour(on ? col::accent : Colour(0xff1a2650));
      g.fillRoundedRectangle(pill, 13);
      if (on) {
        g.setColour(col::accent.withAlpha(.25f));
        g.drawRoundedRectangle(pill.expanded(2), 15, 2);
      }
      auto knob = Rectangle<float>(20, 20).withCentre(
          {on ? pill.getRight() - 13 : pill.getX() + 13, pill.getCentreY()});
      g.setColour(Colours::white);
      g.fillEllipse(knob);
      auto icon = b.getProperties()["icon"].toString();
      if (icon.isNotEmpty())
        drawIcon(g, icon, r.removeFromLeft(22).withSizeKeepingCentre(20, 20),
                 col::text);
      r.removeFromLeft(12);
      g.setColour(col::text);
      g.setFont(sans(16.f));
      g.drawText(b.getButtonText(), r, Justification::centredLeft);
      return;
    }
    auto box = r.removeFromLeft(r.getHeight()).withSizeKeepingCentre(22, 22);
    g.setColour(on ? col::blue : col::field);
    if (hover)
      g.setColour((on ? col::blue : col::field).brighter(.1f));
    g.fillRoundedRectangle(box, 5);
    g.setColour(on ? col::blueEdge : col::fieldEdge);
    g.drawRoundedRectangle(box, 5, 1);
    if (on) {
      Path tick;
      tick.startNewSubPath(box.getX() + 5.5f, box.getCentreY());
      tick.lineTo(box.getX() + 9.5f, box.getBottom() - 6.f);
      tick.lineTo(box.getRight() - 5.f, box.getY() + 6.f);
      g.setColour(Colours::white);
      g.strokePath(tick, PathStrokeType(2.2f, PathStrokeType::curved,
                                        PathStrokeType::rounded));
    }
    r.removeFromLeft(10);
    g.setColour(col::text);
    g.setFont(sans(15.f));
    g.drawText(b.getButtonText(), r, Justification::centredLeft);
  }
  void drawComboBox(Graphics &g, int w, int h, bool, int, int, int, int,
                    ComboBox &box) override {
    auto r = Rectangle<float>(0, 0, float(w), float(h)).reduced(.5f);
    g.setColour(col::field);
    g.fillRoundedRectangle(r, 8);
    g.setColour(box.hasKeyboardFocus(true) ? col::blueEdge : col::fieldEdge);
    g.drawRoundedRectangle(r, 8, 1);
    auto icon = box.getProperties()["icon"].toString();
    if (icon.isNotEmpty())
      drawIcon(g, icon, Rectangle<float>(14, 0, 22, float(h)).withSizeKeepingCentre(22, 22),
               Colour(0xff8fb0ff));
    drawIcon(g, "chevron",
             Rectangle<float>(float(w - 30), 0, 22, float(h))
                 .withSizeKeepingCentre(16, 16),
             col::text);
  }
  void positionComboBoxText(ComboBox &box, Label &label) override {
    int left = box.getProperties()["icon"].toString().isNotEmpty() ? 46 : 12;
    label.setBounds(left, 1, box.getWidth() - left - 30, box.getHeight() - 2);
    label.setFont(getComboBoxFont(box));
  }
  void fillTextEditorBackground(Graphics &g, int w, int h,
                                TextEditor &) override {
    g.setColour(col::field);
    g.fillRoundedRectangle(0, 0, float(w), float(h), 8);
  }
  void drawTextEditorOutline(Graphics &g, int w, int h,
                             TextEditor &e) override {
    g.setColour(e.hasKeyboardFocus(true) ? col::blueEdge : col::fieldEdge);
    g.drawRoundedRectangle(.5f, .5f, float(w) - 1, float(h) - 1, 8, 1);
  }
  void drawPopupMenuBackground(Graphics &g, int w, int h) override {
    g.fillAll(Colour(0xff0b1847));
    g.setColour(col::panelEdge);
    g.drawRect(0, 0, w, h);
  }
  void drawScrollbar(Graphics &g, ScrollBar &, int x, int y, int w, int h,
                     bool vertical, int thumbStart, int thumbSize, bool hover,
                     bool) override {
    Rectangle<float> thumb =
        vertical ? Rectangle<float>(float(x + w / 2 - 2), float(thumbStart),
                                    4.f, float(thumbSize))
                 : Rectangle<float>(float(thumbStart), float(y + h / 2 - 2),
                                    float(thumbSize), 4.f);
    g.setColour(col::fieldEdge.withAlpha(hover ? 1.f : .8f));
    g.fillRoundedRectangle(thumb, 2);
  }
  void drawTableHeaderBackground(Graphics &g, TableHeaderComponent &h) override {
    g.setColour(col::field.withAlpha(.6f));
    g.fillRect(h.getLocalBounds());
    g.setColour(col::panelEdge);
    g.fillRect(0, h.getHeight() - 1, h.getWidth(), 1);
  }
  void drawTableHeaderColumn(Graphics &g, TableHeaderComponent &,
                             const String &name, int, int w, int h, bool, bool,
                             int) override {
    g.setColour(col::muted);
    g.setFont(sans(14.f));
    g.drawText(name, 10, 0, w - 12, h, Justification::centredLeft);
  }
  void drawCallOutBoxBackground(CallOutBox &box, Graphics &g, const Path &path,
                                Image &) override {
    g.setColour(Colour(0xff0b1847));
    g.fillPath(path);
    g.setColour(col::panelEdge);
    g.strokePath(path, PathStrokeType(1));
    ignoreUnused(box);
  }
};

inline void style(Button &b, const String &s, const String &icon = {}) {
  b.getProperties().set("style", s);
  if (icon.isNotEmpty())
    b.getProperties().set("icon", icon);
}

// Rotary parameter knob with name and value text, as in the mockups.
class ParamKnob : public Component {
public:
  Slider slider;
  String id;
  std::function<void(const String &, float)> onChange;
  bool large = false;
  ParamKnob(const String &parameter, const String &name) : id(parameter) {
    title = name;
    addAndMakeVisible(slider);
    slider.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    for (const auto &p : pw::parameters())
      if (p.id == id) {
        slider.setNormalisableRange({p.min, p.max, p.step, p.skew});
        slider.setDoubleClickReturnValue(true, p.initial);
        slider.setValue(p.initial, dontSendNotification);
      }
    slider.onValueChange = [this] {
      repaint();
      if (onChange)
        onChange(id, float(slider.getValue()));
    };
  }
  void setValue(float v) {
    if (std::abs(float(slider.getValue()) - v) > 1.0e-6f && !slider.isMouseButtonDown()) {
      slider.setValue(v, dontSendNotification);
      repaint();
    }
  }
  void resized() override {
    auto r = getLocalBounds();
    r.removeFromBottom(large ? 40 : 42);
    slider.setBounds(r.withSizeKeepingCentre(jmin(r.getWidth(), r.getHeight()),
                                             r.getHeight()));
  }
  void paint(Graphics &g) override {
    auto r = getLocalBounds();
    auto value = r.removeFromBottom(20);
    auto name = r.removeFromBottom(22);
    g.setColour(col::text);
    g.setFont(sans(16.f));
    g.drawText(title, name, Justification::centred);
    g.setColour(col::text.withAlpha(.85f));
    g.setFont(sans(15.f));
    g.drawText(pw::formatValue(id, slider.getValue()), value,
               Justification::centred);
  }

private:
  String title;
};

// Click-to-choose colour swatch with ColourSelector call-out.
class Swatch : public Component {
public:
  Colour colour;
  std::function<void(Colour)> onChange;
  void paint(Graphics &g) override {
    auto r = getLocalBounds().toFloat().reduced(1);
    g.setColour(colour);
    g.fillRoundedRectangle(r, 7);
    g.setColour(Colours::white.withAlpha(.25f));
    g.drawRoundedRectangle(r, 7, 1);
  }
  void mouseUp(const MouseEvent &) override {
    struct Picker : public ColourSelector, private ChangeListener {
      Swatch *owner;
      Component::SafePointer<Swatch> safe;
      explicit Picker(Swatch *s)
          : ColourSelector(ColourSelector::showColourspace |
                           ColourSelector::showSliders |
                           ColourSelector::showColourAtTop),
            owner(s), safe(s) {
        setCurrentColour(s->colour, dontSendNotification);
        setSize(300, 330);
        addChangeListener(this);
      }
      void changeListenerCallback(ChangeBroadcaster *) override {
        if (safe && safe->onChange)
          safe->onChange(getCurrentColour().withAlpha(1.f));
      }
    };
    CallOutBox::launchAsynchronously(std::make_unique<Picker>(this),
                                     getScreenBounds(), nullptr);
  }
};

// Image thumbnail with placeholder.
class Thumb : public Component {
public:
  Image image;
  String placeholder = "No image";
  bool logo = false;
  std::function<void()> onClick;
  void paint(Graphics &g) override {
    auto r = getLocalBounds().toFloat().reduced(1);
    g.setColour(col::field);
    g.fillRoundedRectangle(r, 9);
    if (image.isValid()) {
      if (logo)
        g.drawImageWithin(image, roundToInt(r.getX() + 8),
                          roundToInt(r.getY() + 6),
                          roundToInt(r.getWidth() - 16),
                          roundToInt(r.getHeight() - 12),
                          RectanglePlacement::centred);
      else
        drawCover(g, image, r, 9);
    } else if (placeholder.isEmpty()) {
      drawAppLogo(g, r.reduced(r.getHeight() * .14f));
    } else {
      g.setColour(col::dim);
      g.setFont(sans(13.f));
      g.drawFittedText(placeholder, r.toNearestInt().reduced(6),
                       Justification::centred, 2);
    }
    g.setColour(col::fieldEdge);
    g.drawRoundedRectangle(r, 9, 1);
  }
  void mouseUp(const MouseEvent &) override {
    if (onClick)
      onClick();
  }
};

// Built-in colour theme preview card.
class ThemeCard : public Component {
public:
  String name;
  Colour base, accent, ink;
  bool selected = false;
  Image art;
  std::function<void()> onClick;
  void paint(Graphics &g) override {
    auto r = getLocalBounds().toFloat().reduced(1.5f);
    auto pic = r.withTrimmedBottom(26).reduced(4, 4);
    g.setColour(col::field);
    g.fillRoundedRectangle(r, 9);
    {
      Graphics::ScopedSaveState state(g);
      Path clip;
      clip.addRoundedRectangle(pic, 6);
      g.reduceClipRegion(clip);
      g.setGradientFill(ColourGradient(base.brighter(.35f), pic.getX(),
                                       pic.getY(), base.darker(.6f),
                                       pic.getRight(), pic.getBottom(),
                                       false));
      g.fillRect(pic);
      if (art.isValid()) {
        g.setOpacity(.55f);
        g.drawImageWithin(art, roundToInt(pic.getX()), roundToInt(pic.getY()),
                          roundToInt(pic.getWidth()),
                          roundToInt(pic.getHeight()),
                          RectanglePlacement::fillDestination);
        g.setOpacity(1.f);
      }
      g.setGradientFill(ColourGradient(
          accent.withAlpha(.85f), pic.getRight() - pic.getWidth() * .25f,
          pic.getY() + pic.getHeight() * .35f, accent.withAlpha(0.f),
          pic.getX(), pic.getBottom(), true));
      g.fillRect(pic);
      Path wave;
      wave.startNewSubPath(pic.getX(), pic.getY() + pic.getHeight() * .55f);
      wave.cubicTo(pic.getX() + pic.getWidth() * .35f,
                   pic.getY() + pic.getHeight() * .75f,
                   pic.getX() + pic.getWidth() * .6f,
                   pic.getY() + pic.getHeight() * .35f, pic.getRight(),
                   pic.getY() + pic.getHeight() * .5f);
      g.setColour(accent);
      g.strokePath(wave, PathStrokeType(1.5f));
      // Mini keyboard.
      auto kb = pic.removeFromBottom(pic.getHeight() * .22f).reduced(4, 2);
      g.setColour(ink.withAlpha(.9f));
      g.fillRect(kb);
      g.setColour(Colours::black.withAlpha(.6f));
      for (int i = 1; i < 10; ++i)
        g.fillRect(kb.getX() + kb.getWidth() * float(i) / 10.f, kb.getY(), .8f,
                   kb.getHeight());
    }
    g.setColour(col::text);
    g.setFont(sans(14.f));
    g.drawFittedText(name, r.removeFromBottom(26).toNearestInt().reduced(2, 0),
                     Justification::centred, 1, .7f);
    if (selected) {
      g.setColour(col::accent.withAlpha(.3f));
      g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f), 10, 3);
      g.setColour(col::accent);
      g.drawRoundedRectangle(r, 9, 2);
    } else {
      g.setColour(col::fieldEdge);
      g.drawRoundedRectangle(r, 9, 1);
    }
  }
  void mouseUp(const MouseEvent &) override {
    if (onClick)
      onClick();
  }
};

// Visual picker: every style for one control type is drawn with the same code
// the plugin uses, so what you see is what you get.
class StyleGallery : public Component, public SettableTooltipClient {
  int hover = -1;
  float phase = .62f;

public:
  String type = "knob", current;
  Colour accent = col::accent, ink{0xfff4ecd9};
  std::function<void(const String &)> onPick;
  int columns = 3;
  int count() const { return pw::styleIds(type).size(); }
  // One row when the space is wide and low, two columns when tall.
  void resized() override {
    float ratio = float(getHeight()) / float(jmax(1, getWidth()));
    columns = ratio < .4f ? jmax(1, count()) : ratio > 1.1f ? 2 : 3;
  }
  int rows() const { return (count() + columns - 1) / jmax(1, columns); }
  Rectangle<float> tile(int i) const {
    float w = float(getWidth()) / float(columns),
          h = float(getHeight()) / float(jmax(1, rows()));
    return {float(i % columns) * w, float(i / columns) * h, w, h};
  }
  void paint(Graphics &g) override {
    const auto &ids = pw::styleIds(type);
    const auto &names = pw::styleNames(type);
    const float begin = MathConstants<float>::pi * 1.2f,
                end = MathConstants<float>::pi * 2.8f;
    for (int i = 0; i < ids.size(); ++i) {
      auto r = tile(i).reduced(3);
      bool selected = ids[i] == current;
      g.setColour(selected ? Colour(0xff10247a)
                           : (i == hover ? col::card.brighter(.15f) : col::field));
      g.fillRoundedRectangle(r, 9);
      g.setColour(selected ? col::accent
                           : (i == hover ? col::blueEdge : col::fieldEdge));
      g.drawRoundedRectangle(r.reduced(.5f), 9, selected ? 2.f : 1.f);
      auto label = r.removeFromBottom(20);
      auto art = r.reduced(6, 4);
      if (type == "knob")
        pw::drawKnob(g, art, phase, begin, end, accent, ids[i]);
      else if (type == "slider")
        pw::drawSliderControl(g, art, phase, ids[i] == "vertical", accent,
                              ids[i]);
      else
        pw::drawToggleControl(g, art, true, accent, ink, ids[i]);
      g.setColour(selected ? Colours::white : col::text);
      g.setFont(sans(13.f, selected));
      g.drawFittedText(names[i], label.toNearestInt(), Justification::centred,
                       1, .7f);
    }
  }
  int hit(Point<float> p) const {
    for (int i = 0; i < count(); ++i)
      if (tile(i).contains(p))
        return i;
    return -1;
  }
  void mouseMove(const MouseEvent &e) override {
    int h = hit(e.position);
    if (h != hover) {
      hover = h;
      setTooltip(h >= 0 ? "Use " + pw::styleNames(type)[h] : String());
      setMouseCursor(h >= 0 ? MouseCursor::PointingHandCursor
                            : MouseCursor::NormalCursor);
      repaint();
    }
  }
  void mouseExit(const MouseEvent &) override {
    hover = -1;
    repaint();
  }
  void mouseUp(const MouseEvent &e) override {
    int h = hit(e.position);
    if (h >= 0 && onPick)
      onPick(pw::styleIds(type)[h]);
  }
};

// Waveform display of the selected sample.
class WaveformView : public Component {
public:
  std::vector<std::pair<float, float>> peaks; // per column min/max
  double seconds = 0;
  String caption;
  void setAudio(const AudioBuffer<float> *buffer, double rate) {
    peaks.clear();
    seconds = 0;
    if (buffer == nullptr || buffer->getNumSamples() == 0) {
      repaint();
      return;
    }
    seconds = buffer->getNumSamples() / rate;
    const int columns = 1200;
    int n = buffer->getNumSamples();
    for (int c = 0; c < columns; ++c) {
      int a = int(int64(n) * c / columns), b = int(int64(n) * (c + 1) / columns);
      float lo = 0, hi = 0;
      for (int ch = 0; ch < buffer->getNumChannels(); ++ch) {
        auto range = FloatVectorOperations::findMinAndMax(
            buffer->getReadPointer(ch) + a, jmax(1, b - a));
        lo = jmin(lo, range.getStart());
        hi = jmax(hi, range.getEnd());
      }
      peaks.push_back({lo, hi});
    }
    // Normalise for display.
    float m = .0001f;
    for (auto p : peaks)
      m = jmax(m, std::abs(p.first), std::abs(p.second));
    for (auto &p : peaks)
      p = {p.first / m, p.second / m};
    repaint();
  }
  void paint(Graphics &g) override {
    auto r = getLocalBounds().toFloat();
    auto axis = r.removeFromBottom(22);
    auto plot = r.reduced(12, 8);
    g.setColour(col::panelEdge.withAlpha(.35f));
    for (int i = 0; i <= 6; ++i) {
      float x = plot.getX() + plot.getWidth() * float(i) / 6.f;
      g.fillRect(x, plot.getY(), 1.f, plot.getHeight());
    }
    g.fillRect(plot.getX(), plot.getCentreY(), plot.getWidth(), 1.f);
    if (peaks.empty()) {
      g.setColour(col::dim);
      g.setFont(sans(15.f));
      g.drawText("Import an SFZ instrument to see its samples", plot,
                 Justification::centred);
      return;
    }
    Path wave;
    float cy = plot.getCentreY(), h = plot.getHeight() * .48f;
    int n = int(peaks.size());
    wave.startNewSubPath(plot.getX(), cy);
    for (int i = 0; i < n; ++i)
      wave.lineTo(plot.getX() + plot.getWidth() * float(i) / float(n - 1),
                  cy - peaks[size_t(i)].second * h);
    for (int i = n - 1; i >= 0; --i)
      wave.lineTo(plot.getX() + plot.getWidth() * float(i) / float(n - 1),
                  cy - peaks[size_t(i)].first * h);
    wave.closeSubPath();
    g.setColour(col::accent.withAlpha(.25f));
    g.strokePath(wave, PathStrokeType(3.f));
    g.setGradientFill(ColourGradient(col::accentHot, 0, cy - h, col::accent,
                                     0, cy, false));
    g.fillPath(wave);
    // Start/end markers.
    g.setColour(col::accent);
    g.fillRect(plot.getX(), plot.getY(), 1.5f, plot.getHeight());
    g.fillEllipse(plot.getX() - 4, plot.getY() - 4, 8, 8);
    g.fillRect(plot.getRight() - 1.5f, plot.getY(), 1.5f, plot.getHeight());
    g.setColour(col::muted);
    g.setFont(sans(13.f));
    for (int i = 0; i <= 6; ++i) {
      float x = plot.getX() + plot.getWidth() * float(i) / 6.f;
      auto box = i == 0   ? Rectangle<float>(x, axis.getY(), 60, axis.getHeight())
                 : i == 6 ? Rectangle<float>(x - 60, axis.getY(), 60,
                                             axis.getHeight())
                          : Rectangle<float>(x - 30, axis.getY(), 60,
                                             axis.getHeight());
      g.drawText(String(seconds * i / 6., seconds < 10 ? 1 : 0) + "s", box,
                 i == 0   ? Justification::centredLeft
                 : i == 6 ? Justification::centredRight
                          : Justification::centred);
    }
  }
};

// Shared helpers for the editable envelope and filter graphs.
struct GraphBase : public Component {
  std::function<float(const String &)> get;
  std::function<void(const String &, float)> set;
  void paintGrid(Graphics &g, Rectangle<float> plot, int vx, int hy) {
    g.setColour(col::field.withAlpha(.7f));
    g.fillRoundedRectangle(plot.expanded(4), 6);
    g.setColour(col::panelEdge.withAlpha(.3f));
    for (int i = 1; i < vx; ++i)
      g.fillRect(plot.getX() + plot.getWidth() * float(i) / float(vx),
                 plot.getY(), 1.f, plot.getHeight());
    for (int i = 1; i < hy; ++i)
      g.fillRect(plot.getX(),
                 plot.getY() + plot.getHeight() * float(i) / float(hy),
                 plot.getWidth(), 1.f);
  }
  static void handle(Graphics &g, Point<float> p) {
    g.setColour(col::accent.withAlpha(.3f));
    g.fillEllipse(Rectangle<float>(16, 16).withCentre(p));
    g.setColour(Colour(0xfff4ecd9));
    g.fillEllipse(Rectangle<float>(10, 10).withCentre(p));
  }
};

class EnvelopeView : public GraphBase {
  int dragging = -1;
  Rectangle<float> plot() const {
    return getLocalBounds().toFloat().reduced(10, 10);
  }
  static float seg(float t, float max) {
    return std::log1p(t * 20.f) / std::log1p(max * 20.f);
  }
  static float unseg(float f, float max) {
    return std::expm1(jlimit(0.f, 1.f, f) * std::log1p(max * 20.f)) / 20.f;
  }
  std::array<Point<float>, 4> points() const {
    auto p = plot();
    float unit = p.getWidth() * .25f;
    float a = seg(get("attack"), 5) * unit, d = seg(get("decay"), 5) * unit,
          r = seg(get("release"), 8) * unit;
    float s = get("sustain");
    float x0 = p.getX();
    Point<float> pa(x0 + a, p.getY()),
        pd(pa.x + d, p.getBottom() - s * p.getHeight()),
        ps(pd.x + unit * .9f, pd.y), pr(ps.x + r, p.getBottom());
    return {pa, pd, ps, pr};
  }

public:
  void paint(Graphics &g) override {
    if (!get)
      return;
    auto p = plot();
    paintGrid(g, p, 8, 4);
    auto pts = points();
    Path curve;
    curve.startNewSubPath(p.getX(), p.getBottom());
    curve.quadraticTo(p.getX() + (pts[0].x - p.getX()) * .45f,
                      p.getY() + p.getHeight() * .05f, pts[0].x, pts[0].y);
    curve.quadraticTo(pts[0].x + (pts[1].x - pts[0].x) * .3f, pts[1].y,
                      pts[1].x, pts[1].y);
    curve.lineTo(pts[2]);
    curve.quadraticTo(pts[2].x + (pts[3].x - pts[2].x) * .25f, pts[3].y,
                      pts[3].x, pts[3].y);
    curve.lineTo(p.getRight(), p.getBottom());
    Path fill(curve);
    fill.lineTo(p.getRight(), p.getBottom());
    fill.lineTo(p.getX(), p.getBottom());
    fill.closeSubPath();
    g.setGradientFill(ColourGradient(col::accent.withAlpha(.35f), 0, p.getY(),
                                     col::accent.withAlpha(0.f), 0,
                                     p.getBottom(), false));
    g.fillPath(fill);
    pw::glowStroke(g, curve, col::accent, 2.f, 6.f);
    handle(g, {p.getX(), p.getBottom()});
    handle(g, pts[0]);
    handle(g, pts[1]);
    handle(g, pts[3]);
  }
  void mouseDown(const MouseEvent &e) override {
    auto pts = points();
    dragging = -1;
    float best = 18;
    for (int i : {0, 1, 3})
      if (pts[size_t(i)].getDistanceFrom(e.position) < best) {
        best = pts[size_t(i)].getDistanceFrom(e.position);
        dragging = i;
      }
  }
  void mouseDrag(const MouseEvent &e) override {
    if (dragging < 0 || !set)
      return;
    auto p = plot();
    auto pts = points();
    float unit = p.getWidth() * .25f;
    if (dragging == 0)
      set("attack", jlimit(.001f, 5.f, unseg((e.position.x - p.getX()) / unit, 5)));
    else if (dragging == 1) {
      set("decay", jlimit(.001f, 5.f, unseg((e.position.x - pts[0].x) / unit, 5)));
      set("sustain", jlimit(0.f, 1.f, (p.getBottom() - e.position.y) /
                                          p.getHeight()));
    } else
      set("release", jlimit(.01f, 8.f, unseg((e.position.x - pts[2].x) / unit, 8)));
    repaint();
  }
  void mouseUp(const MouseEvent &) override { dragging = -1; }
};

class FilterView : public GraphBase {
  bool dragging = false;
  Rectangle<float> plot() const {
    auto r = getLocalBounds().toFloat();
    r.removeFromBottom(18);
    return r.reduced(10, 8);
  }
  static float xOf(float f, Rectangle<float> p) {
    return p.getX() + p.getWidth() * std::log(f / 20.f) / std::log(1000.f);
  }
  static float fOf(float x, Rectangle<float> p) {
    return 20.f * std::pow(1000.f, jlimit(0.f, 1.f, (x - p.getX()) / p.getWidth()));
  }
  float response(float f) const {
    float x = f / get("cutoff"), q = get("resonance");
    int type = roundToInt(get("filtertype"));
    float den = std::sqrt(std::pow(1 - x * x, 2.f) + std::pow(x / q, 2.f));
    float num = type == 2 ? x * x : type == 1 ? x / q : 1.f;
    return Decibels::gainToDecibels(num / den, -60.f);
  }
  float yOf(float db, Rectangle<float> p) const {
    return jlimit(p.getY(), p.getBottom(),
                  p.getY() + p.getHeight() * (12.f - db) / 48.f);
  }

public:
  void paint(Graphics &g) override {
    if (!get)
      return;
    auto p = plot();
    paintGrid(g, p, 6, 4);
    Path curve;
    for (int i = 0; i <= 200; ++i) {
      float x = p.getX() + p.getWidth() * float(i) / 200.f;
      float y = yOf(response(fOf(x, p)), p);
      if (i == 0)
        curve.startNewSubPath(x, y);
      else
        curve.lineTo(x, y);
    }
    Path fill(curve);
    fill.lineTo(p.getRight(), p.getBottom());
    fill.lineTo(p.getX(), p.getBottom());
    fill.closeSubPath();
    g.setGradientFill(ColourGradient(col::accent.withAlpha(.3f), 0, p.getY(),
                                     col::accent.withAlpha(0.f), 0,
                                     p.getBottom(), false));
    g.fillPath(fill);
    pw::glowStroke(g, curve, col::accent, 2.f, 6.f);
    float fc = get("cutoff");
    handle(g, {xOf(fc, p), yOf(response(fc), p)});
    g.setColour(col::muted);
    g.setFont(sans(13.f));
    for (auto [f, t] : {std::pair<float, const char *>{20, "20"},
                        {100, "100"}, {1000, "1k"}, {10000, "10k"},
                        {20000, "20k"}}) {
      if (f == 20000 && p.getWidth() * .1f < 48)
        continue;
      float fx = xOf(f, p);
      g.drawText(t,
                 f == 20      ? Rectangle<float>(fx, p.getBottom() + 4, 40, 16)
                 : f == 20000 ? Rectangle<float>(fx - 40, p.getBottom() + 4, 40, 16)
                              : Rectangle<float>(fx - 20, p.getBottom() + 4, 40, 16),
                 f == 20 ? Justification::centredLeft
                 : f == 20000 ? Justification::centredRight
                             : Justification::centred);
    }
  }
  void mouseDown(const MouseEvent &e) override {
    auto p = plot();
    float fc = get("cutoff");
    dragging = Point<float>(xOf(fc, p), yOf(response(fc), p))
                   .getDistanceFrom(e.position) < 24 ||
               p.contains(e.position);
    mouseDrag(e);
  }
  void mouseDrag(const MouseEvent &e) override {
    if (!dragging || !set)
      return;
    auto p = plot();
    set("cutoff", jlimit(30.f, 20000.f, fOf(e.position.x, p)));
    float t = jlimit(0.f, 1.f, (p.getBottom() - e.position.y) / p.getHeight());
    set("resonance", jlimit(.1f, 1.5f, .1f + t * 1.4f));
    repaint();
  }
  void mouseUp(const MouseEvent &) override { dragging = false; }
};
} // namespace ui
