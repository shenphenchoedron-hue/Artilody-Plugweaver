// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Engine.h"
namespace pw {
inline Colour colourOr(const String &hex, Colour fallback) {
  return validHexColour(hex) ? Colour::fromString(hex) : fallback;
}
inline String expandText(const String &text, const Document &doc) {
  return text.replace("{instrument}", doc.name).replace("{maker}", doc.maker);
}
inline String formatValue(const String &id, double value) {
  if (id == "attack" || id == "decay" || id == "release")
    return value < 1 ? String(value * 1000, 0) + " ms"
                     : String(value, 2) + " s";
  if (id == "cutoff")
    return value >= 1000 ? String(value / 1000, 1) + " kHz"
                         : String(value, 0) + " Hz";
  if (id == "sustain") {
    if (value <= .0001)
      return "-inf dB";
    return String(Decibels::gainToDecibels(value), 1) + " dB";
  }
  if (id == "gain")
    return value <= .0001 ? String("-inf dB")
                          : String(Decibels::gainToDecibels(value), 1) + " dB";
  if (id == "reverb")
    return String(value * 100, 0) + " %";
  if (id == "enabled")
    return value > .5 ? "On" : "Off";
  if (id == "filtertype")
    return filterTypes[jlimit(0, 2, roundToInt(value))];
  return String(value, 2);
}
// Soft glow: several translucent strokes under a crisp line.
inline void glowStroke(Graphics &g, const Path &p, Colour c, float width,
                       float glow = 10.f) {
  for (int i = 4; i >= 1; --i) {
    g.setColour(c.withAlpha(.07f * float(5 - i)));
    g.strokePath(p, PathStrokeType(width + glow * float(i) / 4.f,
                                   PathStrokeType::curved,
                                   PathStrokeType::rounded));
  }
  g.setColour(c);
  g.strokePath(p, PathStrokeType(width, PathStrokeType::curved,
                                 PathStrokeType::rounded));
}
// Lower edge of the artwork header in the 1000 x 600 plugin canvas.
inline Path headerWave(bool closed) {
  Path p;
  if (closed) {
    p.startNewSubPath(0, 0);
    p.lineTo(1000, 0);
    p.lineTo(1000, 205);
  } else
    p.startNewSubPath(1000, 205);
  p.cubicTo(800, 178, 720, 236, 520, 238);
  p.cubicTo(330, 240, 250, 196, 0, 198);
  if (closed)
    p.closeSubPath();
  return p;
}
inline Point<float> polar(Point<float> c, float radius, float angle) {
  return {c.x + std::sin(angle) * radius, c.y - std::cos(angle) * radius};
}
inline void strokeArc(Graphics &g, Point<float> c, float radius, float from,
                      float to, float width) {
  Path arc;
  arc.addCentredArc(c.x, c.y, radius, radius, 0, from, to, true);
  g.strokePath(arc, PathStrokeType(width, PathStrokeType::curved,
                                   PathStrokeType::rounded));
}
inline void glowArc(Graphics &g, Point<float> c, float radius, float from,
                    float to, float width, Colour accent) {
  if (to - from < .001f)
    return;
  for (int i = 3; i >= 1; --i) {
    g.setColour(accent.withAlpha(.12f * float(4 - i)));
    strokeArc(g, c, radius, from, to, width + float(i) * 3.f);
  }
  g.setGradientFill(ColourGradient(accent.brighter(.35f), c.x, c.y - radius,
                                   accent.darker(.25f), c.x, c.y + radius,
                                   false));
  strokeArc(g, c, radius, from, to, width);
}
inline void navyBody(Graphics &g, Point<float> c, float radius) {
  auto body = Rectangle<float>(radius * 2, radius * 2).withCentre(c);
  g.setGradientFill(ColourGradient(Colour(0xff2a3970), c.x, body.getY(),
                                   Colour(0xff050a1e), c.x, body.getBottom(),
                                   false));
  g.fillEllipse(body);
  g.setGradientFill(ColourGradient(Colour(0xff1b2a5c), c.x,
                                   body.getY() + radius * .3f,
                                   Colour(0xff0a1230), c.x, body.getBottom(),
                                   false));
  g.fillEllipse(body.reduced(radius * .1f));
  g.setColour(Colours::white.withAlpha(.12f));
  g.drawEllipse(body.reduced(.5f), 1.f);
}
inline void accentDot(Graphics &g, Point<float> p, float size, Colour accent) {
  g.setColour(accent.withAlpha(.35f));
  g.fillEllipse(Rectangle<float>(size * 2, size * 2).withCentre(p));
  g.setColour(accent.brighter(.2f));
  g.fillEllipse(Rectangle<float>(size, size).withCentre(p));
}
// Rotary knob in one of knobStyleIds. A control image replaces the knob body
// and rotates with the value; value rings/arcs stay visible.
inline void drawKnob(Graphics &g, Rectangle<float> box, float pos, float begin,
                     float end, Colour accent, const String &style,
                     const Image &image = {}) {
  float r = jmin(box.getWidth(), box.getHeight()) * .5f - 4.f;
  if (r < 6)
    return;
  auto c = box.getCentre();
  pos = jlimit(0.f, 1.f, pos);
  float angle = begin + pos * (end - begin);
  auto shadow = [&](float radius) {
    g.setColour(Colours::black.withAlpha(.45f));
    g.fillEllipse(Rectangle<float>(radius * 2, radius * 2)
                      .withCentre(c.translated(0, r * .08f))
                      .expanded(r * .04f));
  };
  auto imageBody = [&](float radius) {
    Graphics::ScopedSaveState state(g);
    g.addTransform(AffineTransform::rotation(angle, c.x, c.y));
    auto body = Rectangle<float>(radius * 2, radius * 2).withCentre(c);
    g.drawImageWithin(image, roundToInt(body.getX()), roundToInt(body.getY()),
                      roundToInt(body.getWidth()), roundToInt(body.getHeight()),
                      RectanglePlacement::centred);
  };
  const bool hasImage = image.isValid();
  if (style == "vintage") {
    g.setColour(Colour(0xffd9cfb8).withAlpha(.85f));
    for (int i = 0; i <= 10; ++i) {
      float a = begin + float(i) / 10.f * (end - begin);
      g.drawLine(Line<float>(polar(c, r * .98f, a), polar(c, r * .84f, a)),
                 i % 5 == 0 ? 2.f : 1.2f);
    }
    float skirt = r * .78f;
    shadow(skirt);
    if (hasImage)
      return imageBody(skirt);
    auto body = Rectangle<float>(skirt * 2, skirt * 2).withCentre(c);
    g.setGradientFill(ColourGradient(Colour(0xff3a3632), c.x, body.getY(),
                                     Colour(0xff0e0d0c), c.x, body.getBottom(),
                                     false));
    g.fillEllipse(body);
    float cap = r * .6f;
    auto face = Rectangle<float>(cap * 2, cap * 2).withCentre(c);
    g.setGradientFill(ColourGradient(Colour(0xfff6eedc), c.x - cap * .3f,
                                     c.y - cap * .4f, Colour(0xffbfae8c), c.x,
                                     face.getBottom(), true));
    g.fillEllipse(face);
    g.setColour(Colours::black.withAlpha(.25f));
    g.drawEllipse(face, 1.f);
    g.setColour(Colour(0xff1c1712));
    g.drawLine(Line<float>(polar(c, cap * .1f, angle), polar(c, cap * .9f, angle)),
               jmax(2.f, r * .07f));
    g.setColour(accent);
    g.drawLine(Line<float>(polar(c, cap * 1.02f, angle),
                           polar(c, skirt * .98f, angle)),
               jmax(2.f, r * .06f));
    return;
  }
  if (style == "ledring") {
    const int n = 15;
    float dot = jmax(1.6f, r * .075f);
    for (int i = 0; i < n; ++i) {
      float a = begin + float(i) / float(n - 1) * (end - begin);
      auto p = polar(c, r * .9f, a);
      bool lit = float(i) / float(n - 1) <= pos + .0001f;
      if (lit) {
        g.setColour(accent.withAlpha(.3f));
        g.fillEllipse(Rectangle<float>(dot * 4, dot * 4).withCentre(p));
      }
      g.setColour(lit ? accent.brighter(.25f) : Colour(0xff1a2550));
      g.fillEllipse(Rectangle<float>(dot * 2, dot * 2).withCentre(p));
    }
    float body = r * .66f;
    shadow(body);
    if (hasImage)
      return imageBody(body);
    navyBody(g, c, body);
    g.setColour(accent);
    g.drawLine(Line<float>(polar(c, body * .35f, angle), polar(c, body * .8f, angle)),
               jmax(2.f, r * .07f));
    return;
  }
  if (style == "metal") {
    g.setColour(accent.withAlpha(.25f));
    strokeArc(g, c, r * .98f, begin, end, 1.5f);
    g.setColour(accent);
    if (pos > .001f)
      strokeArc(g, c, r * .98f, begin, angle, 2.5f);
    float body = r * .86f;
    shadow(body);
    if (hasImage)
      return imageBody(body);
    auto outer = Rectangle<float>(body * 2, body * 2).withCentre(c);
    g.setGradientFill(ColourGradient(Colour(0xffeef1f5), c.x, outer.getY(),
                                     Colour(0xff6d737c), c.x, outer.getBottom(),
                                     false));
    g.fillEllipse(outer);
    g.setColour(Colours::black.withAlpha(.28f));
    for (int i = 0; i < 48; ++i) {
      float a = angle + float(i) * MathConstants<float>::twoPi / 48.f;
      g.drawLine(Line<float>(polar(c, body * .97f, a), polar(c, body * .86f, a)),
                 1.f);
    }
    float face = body * .78f;
    auto inner = Rectangle<float>(face * 2, face * 2).withCentre(c);
    g.setGradientFill(ColourGradient(Colour(0xffe2e6eb), c.x - face * .3f,
                                     c.y - face * .4f, Colour(0xff9aa1ab), c.x,
                                     inner.getBottom(), true));
    g.fillEllipse(inner);
    g.setColour(Colours::white.withAlpha(.5f));
    g.drawEllipse(inner, 1.f);
    g.setColour(accent);
    g.drawLine(Line<float>(polar(c, face * .3f, angle), polar(c, face * .92f, angle)),
               jmax(2.f, r * .07f));
    return;
  }
  if (style == "flat") {
    float ring = jmax(2.f, r * .09f);
    g.setColour(Colour(0xff0b1534));
    strokeArc(g, c, r - ring, begin, end, ring);
    g.setColour(accent);
    if (pos > .001f)
      strokeArc(g, c, r - ring, begin, angle, ring);
    float body = r * .7f;
    if (hasImage)
      return imageBody(body);
    auto disc = Rectangle<float>(body * 2, body * 2).withCentre(c);
    g.setColour(Colour(0xff15245c));
    g.fillEllipse(disc);
    g.setColour(accent.withAlpha(.6f));
    g.drawEllipse(disc, 1.5f);
    g.setColour(Colours::white);
    g.drawLine(Line<float>(c, polar(c, body * .82f, angle)), jmax(2.f, r * .08f));
    g.fillEllipse(Rectangle<float>(r * .16f, r * .16f).withCentre(c));
    return;
  }
  // classic and minimal
  const bool minimal = style == "minimal";
  float ring = jmax(3.f, r * .13f);
  float bodyR = minimal ? r * .92f : r - ring * 1.6f;
  shadow(bodyR);
  if (hasImage)
    imageBody(bodyR);
  else
    navyBody(g, c, bodyR);
  if (!minimal) {
    g.setColour(Colour(0xff0b1534).withAlpha(.9f));
    strokeArc(g, c, r - ring * .5f, begin, end, ring);
    glowArc(g, c, r - ring * .5f, begin, angle, ring, accent);
  } else {
    g.setColour(Colours::white.withAlpha(.08f));
    g.drawEllipse(Rectangle<float>(bodyR * 2, bodyR * 2).withCentre(c).expanded(3),
                  1.f);
  }
  if (!hasImage)
    accentDot(g, polar(c, bodyR * .68f, angle), jmax(4.f, bodyR * .2f), accent);
}
// Linear slider in one of sliderStyleIds. The area is the full slider area;
// vertical sliders run bottom (minimum) to top (maximum).
inline void drawSliderControl(Graphics &g, Rectangle<float> area, float p,
                              bool vertical, Colour accent, const String &style,
                              const Image &image = {}) {
  p = jlimit(0.f, 1.f, p);
  Graphics::ScopedSaveState state(g);
  if (vertical) {
    auto c = area.getCentre();
    g.addTransform(AffineTransform::rotation(-MathConstants<float>::halfPi,
                                             c.x, c.y));
    area = Rectangle<float>(area.getHeight(), area.getWidth()).withCentre(c);
  }
  const bool fader = style == "fader" || style == "vertical";
  float pad = fader ? 14.f : (style == "bar" || style == "led" ? 2.f : 10.f);
  float x0 = area.getX() + pad, x1 = area.getRight() - pad,
        cy = area.getCentreY();
  float x = x0 + (x1 - x0) * p;
  auto thumbImage = [&](float size) {
    g.drawImageWithin(image, roundToInt(x - size / 2), roundToInt(cy - size / 2),
                      roundToInt(size), roundToInt(size),
                      RectanglePlacement::centred);
  };
  if (fader) {
    g.setColour(Colour(0xffd9cfb8).withAlpha(.45f));
    for (int i = 0; i <= 10; ++i) {
      float tx = x0 + (x1 - x0) * float(i) / 10.f;
      float len = i % 5 == 0 ? 9.f : 5.f;
      g.drawLine(tx, cy - 12 - len, tx, cy - 12, 1.f);
      g.drawLine(tx, cy + 12, tx, cy + 12 + len, 1.f);
    }
    g.setColour(Colours::black.withAlpha(.8f));
    g.fillRoundedRectangle(x0, cy - 3, x1 - x0, 6, 3);
    g.setColour(accent.withAlpha(.8f));
    g.fillRoundedRectangle(x0, cy - 1.5f, x - x0, 3, 1.5f);
    float capH = jmin(area.getHeight() * .8f, 46.f), capW = 22.f;
    if (image.isValid())
      return thumbImage(capH);
    auto cap = Rectangle<float>(capW, capH).withCentre({x, cy});
    g.setColour(Colours::black.withAlpha(.5f));
    g.fillRoundedRectangle(cap.translated(2, 3), 3);
    g.setGradientFill(ColourGradient(Colour(0xffd4d8de), cap.getX(), 0,
                                     Colour(0xff7b818a), cap.getRight(), 0,
                                     false));
    g.fillRoundedRectangle(cap, 3);
    g.setColour(Colours::black.withAlpha(.35f));
    for (float gx : {-6.f, -3.f, 3.f, 6.f})
      g.drawLine(x + gx, cap.getY() + 5, x + gx, cap.getBottom() - 5, 1.f);
    g.setColour(accent);
    g.fillRect(x - 1, cap.getY() + 2, 2.f, cap.getHeight() - 4);
    return;
  }
  if (style == "bar") {
    float h = jmin(area.getHeight() * .55f, 28.f);
    auto bar = Rectangle<float>(x0, cy - h / 2, x1 - x0, h);
    g.setColour(Colour(0xff0b1534));
    g.fillRoundedRectangle(bar, h * .3f);
    if (p > .001f) {
      g.setGradientFill(ColourGradient(accent.darker(.35f), bar.getX(), 0,
                                       accent.brighter(.25f), x, 0, false));
      g.fillRoundedRectangle(bar.withRight(jmax(bar.getX() + h * .6f, x)),
                             h * .3f);
    }
    g.setColour(Colours::white.withAlpha(.18f));
    g.drawRoundedRectangle(bar, h * .3f, 1.f);
    if (image.isValid())
      return thumbImage(h * 1.4f);
    g.setColour(Colours::white.withAlpha(.9f));
    g.fillRect(jlimit(bar.getX() + 2, bar.getRight() - 4, x - 1), bar.getY() + 3,
               2.f, h - 6);
    return;
  }
  if (style == "led") {
    const int n = 20;
    float h = jmin(area.getHeight() * .45f, 22.f);
    float w = (x1 - x0) / float(n);
    for (int i = 0; i < n; ++i) {
      auto seg = Rectangle<float>(x0 + float(i) * w, cy - h / 2, w, h)
                     .reduced(1.2f, 0);
      bool lit = (float(i) + .5f) / float(n) <= p;
      Colour on = i >= n - 3 ? accent.brighter(.5f) : accent;
      if (lit) {
        g.setColour(on.withAlpha(.25f));
        g.fillRoundedRectangle(seg.expanded(1.5f), 2.5f);
      }
      g.setColour(lit ? on : Colour(0xff1a2550));
      g.fillRoundedRectangle(seg, 2);
    }
    if (image.isValid())
      thumbImage(h * 1.6f);
    return;
  }
  if (style == "minimal") {
    g.setColour(Colours::white.withAlpha(.18f));
    g.fillRect(x0, cy - 1, x1 - x0, 2.f);
    g.setColour(accent);
    g.fillRect(x0, cy - 1, x - x0, 2.f);
    if (image.isValid())
      return thumbImage(jmin(40.f, area.getHeight()));
    accentDot(g, {x, cy}, 9.f, accent);
    return;
  }
  // classic
  auto track = Rectangle<float>(x0, cy - 3, x1 - x0, 6);
  g.setColour(Colour(0xff0b1534));
  g.fillRoundedRectangle(track, 3);
  g.setGradientFill(ColourGradient(accent.darker(.2f), track.getX(), cy,
                                   accent.brighter(.3f), x, cy, false));
  g.fillRoundedRectangle(track.withRight(x), 3);
  if (image.isValid())
    return thumbImage(jmin(48.f, area.getHeight()));
  auto thumb = Rectangle<float>(18, 18).withCentre({x, cy});
  g.setColour(accent.withAlpha(.3f));
  g.fillEllipse(thumb.expanded(4));
  g.setGradientFill(ColourGradient(Colour(0xff2a3970), 0, thumb.getY(),
                                   Colour(0xff070c24), 0, thumb.getBottom(),
                                   false));
  g.fillEllipse(thumb);
  g.setColour(accent);
  g.drawEllipse(thumb, 1.5f);
}
// On/off switch in one of toggleStyleIds, drawn inside area.
inline void drawToggleControl(Graphics &g, Rectangle<float> area, bool on,
                              Colour accent, Colour ink, const String &style,
                              const Image &image = {}, bool hover = false) {
  auto c = area.getCentre();
  if (image.isValid()) {
    g.drawImageWithin(image, roundToInt(area.getX()), roundToInt(area.getY()),
                      roundToInt(area.getWidth()), roundToInt(area.getHeight()),
                      RectanglePlacement::centred);
    g.setColour(on ? accent : ink.withAlpha(.35f));
    g.fillEllipse(area.getRight() - 14, area.getY() + 4, 10, 10);
    return;
  }
  float s = jmin(area.getWidth(), area.getHeight());
  auto metal = [&](Rectangle<float> r, float radius) {
    g.setGradientFill(ColourGradient(Colour(0xffe6e9ee), 0, r.getY(),
                                     Colour(0xff7d838c), 0, r.getBottom(),
                                     false));
    g.fillRoundedRectangle(r, radius);
  };
  if (style == "rocker") {
    auto frame = Rectangle<float>(jmin(area.getWidth() * .5f, s * .62f),
                                  jmin(area.getHeight() * .9f, s * .95f))
                     .withCentre(c);
    g.setColour(Colour(0xff05070f));
    g.fillRoundedRectangle(frame.expanded(3), 6);
    auto face = frame.reduced(3);
    auto top = face.withHeight(face.getHeight() / 2),
         bottom = face.withTrimmedTop(face.getHeight() / 2);
    // The pressed half sits lower and darker; the raised half catches light.
    auto drawHalf = [&](Rectangle<float> r, bool pressed, bool upper) {
      g.setGradientFill(ColourGradient(
          pressed ? Colour(0xff161c30) : Colour(0xff3b4566), 0,
          upper ? r.getY() : r.getBottom(),
          pressed ? Colour(0xff0c1020) : Colour(0xff1d2440), 0,
          upper ? r.getBottom() : r.getY(), false));
      g.fillRoundedRectangle(r, 4);
    };
    drawHalf(top, on, true);
    drawHalf(bottom, !on, false);
    g.setColour(on ? accent : ink.withAlpha(.35f));
    g.drawLine(top.getCentreX(), top.getCentreY() - top.getHeight() * .22f,
               top.getCentreX(), top.getCentreY() + top.getHeight() * .22f,
               2.5f);
    g.setColour(!on ? ink.withAlpha(.8f) : ink.withAlpha(.35f));
    float o = bottom.getHeight() * .22f;
    g.drawEllipse(Rectangle<float>(o * 2, o * 2).withCentre(bottom.getCentre()),
                  2.f);
    if (on) {
      g.setColour(accent.withAlpha(.25f));
      g.drawRoundedRectangle(frame.expanded(4), 7, 2);
    }
    return;
  }
  if (style == "button") {
    float d = s * .78f;
    auto outer = Rectangle<float>(d, d).withCentre(c);
    if (on) {
      g.setColour(accent.withAlpha(.25f));
      g.fillEllipse(outer.expanded(5));
    }
    metal(outer, d / 2);
    g.setColour(on ? accent : Colour(0xff1a2550));
    g.drawEllipse(outer.reduced(d * .08f), jmax(2.f, d * .07f));
    auto face = outer.reduced(d * .17f);
    g.setGradientFill(ColourGradient(Colour(0xff2a3970), c.x, face.getY(),
                                     Colour(0xff070c24), c.x, face.getBottom(),
                                     false));
    g.fillEllipse(face);
    Path power;
    float pr = face.getWidth() * .26f;
    power.addCentredArc(c.x, c.y + pr * .1f, pr, pr, 0, .6f,
                        MathConstants<float>::twoPi - .6f, true);
    power.startNewSubPath(c.x, c.y - pr * 1.15f);
    power.lineTo(c.x, c.y - pr * .2f);
    g.setColour(on ? accent.brighter(.2f) : ink.withAlpha(.4f));
    g.strokePath(power, PathStrokeType(jmax(1.6f, d * .05f),
                                       PathStrokeType::curved,
                                       PathStrokeType::rounded));
    return;
  }
  if (style == "lever") {
    auto plate = Rectangle<float>(s * .42f, s * .9f).withCentre(c);
    metal(plate, 5);
    g.setColour(Colours::black.withAlpha(.3f));
    g.drawRoundedRectangle(plate, 5, 1);
    g.setFont(jmax(8.f, s * .11f));
    g.setColour(on ? accent : Colour(0xff2b2f3a));
    g.drawText("ON", plate.withHeight(s * .2f).toNearestInt(),
               Justification::centred);
    g.setColour(!on ? Colour(0xff2b2f3a) : Colour(0xff6b707a));
    g.drawText("OFF", plate.withTop(plate.getBottom() - s * .2f).toNearestInt(),
               Justification::centred);
    float nut = s * .15f;
    g.setColour(Colour(0xff3a3f48));
    g.fillEllipse(Rectangle<float>(nut * 2, nut * 2).withCentre(c));
    Point<float> tip(c.x, c.y + (on ? -1.f : 1.f) * s * .3f);
    g.setColour(Colours::black.withAlpha(.35f));
    g.drawLine(Line<float>(c.translated(2, 2), tip.translated(2, 2)), s * .09f);
    g.setGradientFill(ColourGradient(Colour(0xfff2f4f7), c.x - s * .05f, 0,
                                     Colour(0xff8d939c), c.x + s * .05f, 0,
                                     false));
    g.drawLine(Line<float>(c, tip), s * .08f);
    g.fillEllipse(Rectangle<float>(s * .14f, s * .14f).withCentre(tip));
    return;
  }
  if (style == "led") {
    auto key = Rectangle<float>(s * .7f, s * .62f).withCentre(c);
    g.setColour(Colours::black.withAlpha(.5f));
    g.fillRoundedRectangle(key.translated(0, 3), 6);
    g.setGradientFill(ColourGradient(Colour(0xff2c3658), 0, key.getY(),
                                     Colour(0xff0d1328), 0, key.getBottom(),
                                     false));
    g.fillRoundedRectangle(key, 6);
    g.setColour(Colours::white.withAlpha(hover ? .3f : .15f));
    g.drawRoundedRectangle(key, 6, 1);
    auto led = Rectangle<float>(key.getWidth() * .5f, jmax(3.f, s * .07f))
                   .withCentre({c.x, key.getY() + key.getHeight() * .28f});
    if (on) {
      g.setColour(accent.withAlpha(.35f));
      g.fillRoundedRectangle(led.expanded(4), 4);
    }
    g.setColour(on ? accent.brighter(.3f) : Colour(0xff05070f));
    g.fillRoundedRectangle(led, led.getHeight() / 2);
    return;
  }
  if (style == "slide") {
    auto slot = Rectangle<float>(s * .9f, s * .36f).withCentre(c);
    g.setColour(Colour(0xff05070f));
    g.fillRoundedRectangle(slot, 4);
    if (on) {
      g.setColour(accent.withAlpha(.85f));
      g.fillRoundedRectangle(slot.reduced(3).withWidth(slot.getWidth() * .55f),
                             3);
    }
    auto knob = Rectangle<float>(slot.getWidth() * .45f, slot.getHeight() + 6)
                    .withCentre({on ? slot.getRight() - slot.getWidth() * .23f
                                    : slot.getX() + slot.getWidth() * .23f,
                                 c.y});
    metal(knob, 4);
    g.setColour(Colours::black.withAlpha(.35f));
    for (float gx : {-.15f, 0.f, .15f})
      g.drawLine(knob.getCentreX() + knob.getWidth() * gx, knob.getY() + 4,
                 knob.getCentreX() + knob.getWidth() * gx, knob.getBottom() - 4,
                 1.f);
    return;
  }
  // pill
  auto pill = Rectangle<float>(jmin(46.f, s * 1.6f), jmin(24.f, s * .8f))
                  .withCentre(c);
  float radius = pill.getHeight() / 2;
  g.setColour(on ? accent : Colour(0xff1a2650));
  if (hover)
    g.setColour((on ? accent : Colour(0xff1a2650)).brighter(.1f));
  g.fillRoundedRectangle(pill, radius);
  g.setColour(Colours::white.withAlpha(.15f));
  g.drawRoundedRectangle(pill, radius, 1);
  float k = pill.getHeight() - 6;
  auto knob = Rectangle<float>(k, k).withCentre(
      {on ? pill.getRight() - radius : pill.getX() + radius, pill.getCentreY()});
  g.setColour(on ? Colours::white : ink.withAlpha(.7f));
  g.fillEllipse(knob);
}
// Decoded images per asset name. Asset names are unique (UUID based), so a
// cached image only needs re-decoding if the stored bytes change size.
class ImageCache {
  std::map<String, std::pair<std::size_t, Image>> images;

public:
  Image get(const Document &d, const String &asset) {
    if (asset.isEmpty())
      return {};
    auto it = d.assets.find(asset);
    if (it == d.assets.end())
      return {};
    auto &entry = images[asset];
    if (entry.first != it->second.getSize() || !entry.second.isValid())
      entry = {it->second.getSize(),
               ImageFileFormat::loadFrom(it->second.getData(),
                                         it->second.getSize())};
    return entry.second;
  }
};
class RoyalLook : public LookAndFeel_V4 {
public:
  Colour accent{0xffff8a2b}, ink{0xfff4ecd9};
  String style; // style id for the control using this look
  Image knobImage;
  RoyalLook() {
    setColour(Label::textColourId, ink);
    setColour(TextEditor::backgroundColourId, Colour(0xff081235));
    setColour(TextEditor::textColourId, ink);
    setColour(TextEditor::outlineColourId, Colour(0xff24409a));
    setColour(ComboBox::backgroundColourId, Colour(0xff0c1a4a));
    setColour(ComboBox::textColourId, ink);
    setColour(PopupMenu::backgroundColourId, Colour(0xff0c1a4a));
    setColour(PopupMenu::textColourId, ink);
    setColour(ToggleButton::textColourId, ink);
    setColour(ToggleButton::tickColourId, accent);
    setColour(Slider::thumbColourId, accent);
    setColour(Slider::trackColourId, accent);
    setColour(Slider::textBoxTextColourId, ink);
    setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
  }
  void drawToggleButton(Graphics &g, ToggleButton &b, bool hover,
                        bool) override {
    auto area = b.getLocalBounds().toFloat().withTrimmedBottom(22).reduced(2);
    drawToggleControl(g, area, b.getToggleState(), accent, ink, style,
                      knobImage, hover);
    g.setColour(ink);
    g.setFont(Font("Georgia", 17.f, Font::plain));
    g.drawFittedText(b.getButtonText(),
                     b.getLocalBounds().removeFromBottom(22),
                     Justification::centred, 1);
  }
  void drawLinearSlider(Graphics &g, int x, int y, int width, int height,
                        float, float, float, const Slider::SliderStyle kind,
                        Slider &slider) override {
    drawSliderControl(
        g, Rectangle<float>(float(x), float(y), float(width), float(height)),
        float(slider.valueToProportionOfLength(slider.getValue())),
        kind == Slider::LinearVertical, accent, style, knobImage);
  }
  void drawRotarySlider(Graphics &g, int x, int y, int w, int h, float pos,
                        float begin, float end, Slider &) override {
    drawKnob(g, Rectangle<float>(float(x), float(y), float(w), float(h)), pos,
             begin, end, accent, style, knobImage);
  }
};
class Piano : public Component, private Timer {
  struct Key {
    int midi;
    bool black;
    Rectangle<float> bounds;
  };
  std::vector<Key> keys;
  int held = -1;
  MidiKeyboardState &state;
  BigInteger shown; // notes drawn as pressed in the last paint
  BigInteger activeNotes() const {
    BigInteger notes;
    for (int m = 21; m <= 108; ++m)
      if (state.isNoteOnForChannels(0xffff, m))
        notes.setBit(m);
    return notes;
  }

public:
  BigInteger covered;
  Colour accent{0xffff8a2b};
  // Labels below the keyboard (builder) or small labels on the C keys.
  bool labelsBelow = false;
  // When false, every key can be played, also unmapped ones.
  bool onlyMapped = true;
  explicit Piano(MidiKeyboardState &s) : state(s) {
    setOpaque(true);
    startTimerHz(30);
  }
  // Only repaint when the pressed keys change, keeping idle CPU use low.
  void timerCallback() override {
    if (activeNotes() != shown)
      repaint();
  }
  void resized() override {
    keys.clear();
    float h = float(getHeight()) - (labelsBelow ? 22.f : 0.f);
    float w = float(getWidth()) / 52.f;
    int white = 0;
    for (int m = 21; m <= 108; ++m) {
      int k = m % 12;
      bool b = k == 1 || k == 3 || k == 6 || k == 8 || k == 10;
      keys.push_back({m,
                      b,
                      {(float(white) - (b ? .32f : 0.f)) * w, 0,
                       w * (b ? .64f : 1.f), h * (b ? .62f : 1.f)}});
      if (!b)
        ++white;
    }
  }
  void paint(Graphics &g) override {
    shown = activeNotes();
    float h = float(getHeight()) - (labelsBelow ? 22.f : 0.f);
    g.fillAll(Colour(0xff060a1c));
    g.setColour(Colour(0xff03060f));
    g.fillRect(0.f, 0.f, float(getWidth()), h);
    for (bool pass : {false, true})
      for (const auto &k : keys)
        if (k.black == pass) {
          bool mapped = covered[k.midi], active = shown[k.midi];
          auto r = k.bounds;
          if (!k.black) {
            r = r.reduced(.6f, 0).withTrimmedBottom(.5f);
            Colour top = mapped ? Colour(0xfffbf7ee) : Colour(0xffcdccc8),
                   bottom = mapped ? Colour(0xffe3dccb) : Colour(0xffa9a8a6);
            if (active) {
              top = accent.brighter(.35f);
              bottom = accent.darker(.15f);
            }
            g.setGradientFill(
                ColourGradient(top, 0, r.getY(), bottom, 0, r.getBottom(),
                               false));
            Path key;
            key.addRoundedRectangle(r.getX(), r.getY(), r.getWidth(),
                                    r.getHeight(), 3.f, 3.f, false, false,
                                    true, true);
            g.fillPath(key);
            g.setColour(Colours::black.withAlpha(.25f));
            g.fillRect(r.withHeight(4.f));
            g.setColour(Colours::white.withAlpha(active ? .35f : .55f));
            g.fillRect(r.withTop(r.getBottom() - 6).withHeight(1.5f));
            if (!labelsBelow && k.midi % 12 == 0 && r.getWidth() > 12) {
              g.setColour(Colour(0xff4a4f63).withAlpha(mapped ? .9f : .6f));
              g.setFont(jmin(10.f, r.getWidth() * .55f));
              g.drawText("C" + String(k.midi / 12 - 1),
                         r.withTop(r.getBottom() - 18).toNearestInt(),
                         Justification::centred);
            }
          } else {
            Colour top = mapped ? Colour(0xff363b4f) : Colour(0xff1d1f28),
                   bottom = Colour(0xff07080c);
            if (active) {
              top = accent.brighter(.2f);
              bottom = accent.darker(.4f);
            }
            g.setColour(Colours::black.withAlpha(.5f));
            g.fillRoundedRectangle(r.translated(1.f, 1.5f), 2.5f);
            g.setGradientFill(
                ColourGradient(top, 0, r.getY(), bottom, 0, r.getBottom(),
                               false));
            g.fillRoundedRectangle(r, 2.5f);
            g.setColour(Colours::white.withAlpha(active ? .3f : .14f));
            g.fillRoundedRectangle(r.reduced(r.getWidth() * .18f, 0)
                                       .withTrimmedTop(2)
                                       .withHeight(r.getHeight() * .72f),
                                   2.f);
          }
        }
    // Shadow from the keyboard frame onto the keys.
    g.setGradientFill(ColourGradient(Colours::black.withAlpha(.5f), 0, 0,
                                     Colours::transparentBlack, 0, 8, false));
    g.fillRect(0.f, 0.f, float(getWidth()), 8.f);
    if (labelsBelow) {
      g.setColour(Colour(0xff9fb2e6));
      g.setFont(13.f);
      for (const auto &k : keys)
        if (k.midi % 12 == 0)
          g.drawText("C" + String(k.midi / 12 - 1),
                     Rectangle<float>(k.bounds.getX() - 10, h + 4,
                                      k.bounds.getWidth() + 20, 16)
                         .toNearestInt(),
                     Justification::centred);
    }
  }
  void release() {
    if (held >= 0)
      state.noteOff(1, held, 0);
    held = -1;
  }
  void hit(Point<float> p, float velocity) {
    int found = -1;
    for (bool pass : {true, false}) {
      for (const auto &k : keys)
        if (k.black == pass && k.bounds.contains(p)) {
          found = k.midi;
          break;
        }
      if (found >= 0)
        break;
    }
    if (found == held)
      return;
    release();
    if (found >= 0 && (!onlyMapped || covered[found])) {
      held = found;
      state.noteOn(1, held, velocity);
    }
  }
  float velocityAt(Point<float> p) const {
    float h = float(getHeight()) - (labelsBelow ? 22.f : 0.f);
    return jlimit(.25f, 1.f, .35f + p.y / jmax(1.f, h) * .65f);
  }
  void mouseDown(const MouseEvent &e) override {
    hit(e.position, velocityAt(e.position));
  }
  void mouseDrag(const MouseEvent &e) override {
    hit(e.position, velocityAt(e.position));
  }
  void mouseUp(const MouseEvent &) override { release(); }
  ~Piano() override { release(); }
};
class Widget : public Component {
  Document &doc;
  Values &values;
  std::size_t index;
  RoyalLook look;
  Slider slider;
  ToggleButton toggle;
  Label label, valueLabel;
  Image logoImage;
  bool editing = false, resizing = false;
  Rectangle<int> initial;
  Colour ink;

public:
  std::function<void(int)> selected;
  std::function<void()> moved;
  std::function<void(const String &, float)> changed;
  std::function<void(const String &, bool)> gesture;
  bool highlighted = false, snap = true;
  Widget(Document &d, Values &v, std::size_t i, bool edit, ImageCache &images)
      : doc(d), values(v), index(i), editing(edit) {
    const auto &c = doc.theme.controls[index];
    ink = colourOr(c.colour, doc.theme.ink);
    look.accent = doc.theme.accent;
    look.ink = ink;
    look.style = c.style.isNotEmpty() ? c.style
                 : c.type == "knob"   ? doc.theme.knobStyle
                 : c.type == "toggle" ? String("pill")
                                      : String("classic");
    look.setColour(Slider::textBoxTextColourId, ink);
    look.setColour(ToggleButton::textColourId, ink);
    look.knobImage = images.get(doc, c.image);
    setLookAndFeel(&look);
    if (c.type == "logo")
      logoImage = images.get(doc, doc.theme.logo);
    addAndMakeVisible(label);
    label.setText(expandText(c.text, doc), dontSendNotification);
    label.setColour(Label::textColourId, ink);
    label.setBorderSize({0, 0, 0, 0});
    label.setMinimumHorizontalScale(.6f);
    if (c.type == "label") {
      Font f(c.font, c.fontSize, Font::plain);
      f.setExtraKerningFactor(c.spacing);
      label.setFont(f);
      label.setJustificationType(c.align == "left"    ? Justification::centredLeft
                                 : c.align == "right" ? Justification::centredRight
                                                      : Justification::centred);
    } else {
      label.setFont(Font("Georgia", 19.f, Font::plain));
      label.setJustificationType(Justification::centred);
    }
    if (c.type == "logo")
      label.setVisible(false);
    if (c.type == "knob" || c.type == "slider") {
      addAndMakeVisible(slider);
      addAndMakeVisible(valueLabel);
      valueLabel.setColour(Label::textColourId, ink.withAlpha(.85f));
      valueLabel.setFont(15.f);
      valueLabel.setJustificationType(Justification::centred);
      valueLabel.setBorderSize({0, 0, 0, 0});
      slider.setSliderStyle(c.type == "knob" ? Slider::RotaryHorizontalVerticalDrag
                            : look.style == "vertical" ? Slider::LinearVertical
                                                       : Slider::LinearHorizontal);
      slider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
      slider.setPopupDisplayEnabled(false, false, nullptr);
      for (const auto &p : parameters())
        if (p.id == c.binding) {
          slider.setNormalisableRange(
              NormalisableRange<double>(p.min, p.max, p.step, p.skew));
          slider.setDoubleClickReturnValue(true, p.initial);
          slider.setValue(values.at(p.id).load(), dontSendNotification);
        }
      updateValueText();
      slider.onValueChange = [this] {
        auto id = doc.theme.controls[index].binding;
        float value = float(slider.getValue());
        values.at(id) = value;
        updateValueText();
        if (changed)
          changed(id, value);
      };
      slider.onDragStart = [this] {
        if (gesture)
          gesture(doc.theme.controls[index].binding, true);
      };
      slider.onDragEnd = [this] {
        if (gesture)
          gesture(doc.theme.controls[index].binding, false);
      };
    } else if (c.type == "toggle") {
      addAndMakeVisible(toggle);
      label.setVisible(false);
      toggle.setButtonText(expandText(c.text, doc));
      toggle.setToggleState(values.at(c.binding) > .5f, dontSendNotification);
      toggle.onClick = [this] {
        auto id = doc.theme.controls[index].binding;
        float value = toggle.getToggleState() ? 1.f : 0.f;
        values.at(id) = value;
        if (gesture)
          gesture(id, true);
        if (changed)
          changed(id, value);
        if (gesture)
          gesture(id, false);
      };
    }
    if (editing) {
      slider.setInterceptsMouseClicks(false, false);
      toggle.setInterceptsMouseClicks(false, false);
      label.setInterceptsMouseClicks(false, false);
      valueLabel.setInterceptsMouseClicks(false, false);
    } else {
      label.setInterceptsMouseClicks(false, false);
      valueLabel.setInterceptsMouseClicks(false, false);
      if (c.type == "label" || c.type == "logo")
        setInterceptsMouseClicks(false, false);
    }
    setBounds(c.bounds);
  }
  void updateValueText() {
    valueLabel.setText(
        formatValue(doc.theme.controls[index].binding, slider.getValue()),
        dontSendNotification);
  }
  void refreshValue() {
    if (editing)
      return;
    const auto &c = doc.theme.controls[index];
    if (c.type == "knob" || c.type == "slider") {
      float v = values.at(c.binding).load();
      if (std::abs(float(slider.getValue()) - v) > 1.0e-6f) {
        slider.setValue(v, dontSendNotification);
        updateValueText();
      }
    }
    if (c.type == "toggle")
      toggle.setToggleState(values.at(c.binding) > .5f, dontSendNotification);
  }
  const String &type() const { return doc.theme.controls[index].type; }
  void resized() override {
    auto r = getLocalBounds();
    if (type() == "label" || type() == "logo")
      label.setBounds(r);
    else if (type() == "toggle")
      toggle.setBounds(r);
    else {
      valueLabel.setBounds(r.removeFromBottom(20));
      label.setBounds(r.removeFromBottom(26));
      slider.setBounds(r.reduced(type() == "knob" ? 4 : 8, 2));
    }
  }
  void paint(Graphics &g) override {
    if (type() != "logo")
      return;
    if (logoImage.isValid())
      g.drawImageWithin(logoImage, 0, 0, getWidth(), getHeight(),
                        RectanglePlacement::centred);
    else if (editing) {
      Path outline;
      outline.addRoundedRectangle(getLocalBounds().toFloat().reduced(3), 8);
      Path dashed;
      const float dashes[] = {6, 5};
      PathStrokeType(1.2f).createDashedStroke(dashed, outline, dashes, 2);
      g.setColour(ink.withAlpha(.45f));
      g.fillPath(dashed);
      g.setFont(14.f);
      g.drawFittedText("Logo\n(Upload logo)", getLocalBounds().reduced(6),
                       Justification::centred, 2);
    }
  }
  void paintOverChildren(Graphics &g) override {
    if (!editing)
      return;
    auto r = getLocalBounds().toFloat().reduced(1.5f);
    if (highlighted) {
      g.setColour(doc.theme.accent);
      g.drawRect(r, 1.6f);
      for (auto p : {r.getTopLeft(), r.getTopRight(), r.getBottomLeft(),
                     r.getBottomRight()}) {
        auto handle = Rectangle<float>(9, 9).withCentre(p);
        g.setColour(doc.theme.accent);
        g.fillEllipse(handle);
        g.setColour(Colours::white.withAlpha(.8f));
        g.drawEllipse(handle, 1);
      }
    } else {
      g.setColour(ink.withAlpha(.16f));
      g.drawRect(r, 1.f);
    }
  }
  void mouseDown(const MouseEvent &e) override {
    if (!editing)
      return;
    initial = getBounds();
    resizing = e.x > getWidth() - 16 && e.y > getHeight() - 16;
    if (selected)
      selected(int(index));
  }
  void mouseMove(const MouseEvent &e) override {
    if (editing)
      setMouseCursor(e.x > getWidth() - 16 && e.y > getHeight() - 16
                         ? MouseCursor::BottomRightCornerResizeCursor
                         : MouseCursor::DraggingHandCursor);
  }
  void mouseDrag(const MouseEvent &e) override {
    if (!editing)
      return;
    auto r = initial;
    // The preview may be scaled; drag distances are in local coordinates.
    int dx = e.getDistanceFromDragStartX(), dy = e.getDistanceFromDragStartY();
    auto grid = [this](int n) {
      return snap ? roundToInt(float(n) / 10.f) * 10 : n;
    };
    if (resizing)
      r.setSize(jlimit(30, 1000 - r.getX(), grid(initial.getWidth() + dx)),
                jlimit(24, 490 - r.getY(), grid(initial.getHeight() + dy)));
    else
      r.setPosition(jlimit(0, 1000 - r.getWidth(), grid(initial.getX() + dx)),
                    jlimit(0, 490 - r.getHeight(), grid(initial.getY() + dy)));
    doc.theme.controls[index].bounds = r;
    setBounds(r);
    if (moved)
      moved();
  }
  ~Widget() override { setLookAndFeel(nullptr); }
};
class InstrumentView : public Component, private Timer {
  Document &doc;
  Values &values;
  Image background, logo;
  std::vector<std::unique_ptr<Widget>> widgets;
  bool edit = false;
  int selection = -1;

public:
  ImageCache images;
  Piano piano;
  bool snap = true;
  std::function<void(int)> selected;
  std::function<void()> moved;
  std::function<void(const String &, float)> changed;
  std::function<void(const String &, bool)> gesture;
  static constexpr int width = 1000, height = 600;
  InstrumentView(Document &d, Values &v, MidiKeyboardState &s)
      : doc(d), values(v), piano(s) {
    addAndMakeVisible(piano);
    setSize(width, height);
    rebuild(false);
    startTimerHz(20);
  }
  void timerCallback() override {
    for (auto &w : widgets)
      w->refreshValue();
  }
  bool isEditing() const { return edit; }
  void rebuild(bool editing) {
    edit = editing;
    widgets.clear();
    background = images.get(doc, doc.theme.background);
    logo = images.get(doc, doc.theme.logo);
    piano.covered = doc.coverage();
    piano.accent = doc.theme.accent;
    piano.repaint();
    for (std::size_t i = 0; i < doc.theme.controls.size(); ++i) {
      auto w = std::make_unique<Widget>(doc, values, i, edit, images);
      w->highlighted = int(i) == selection;
      w->snap = snap;
      w->selected = [this](int n) {
        select(n);
        if (selected)
          selected(n);
      };
      w->moved = [this] {
        repaint();
        if (moved)
          moved();
      };
      w->changed = [this](const String &id, float val) {
        if (changed)
          changed(id, val);
      };
      w->gesture = [this](const String &id, bool begin) {
        if (gesture)
          gesture(id, begin);
      };
      addAndMakeVisible(*w);
      widgets.push_back(std::move(w));
    }
    resized();
    repaint();
  }
  void select(int n) {
    selection = n;
    for (std::size_t i = 0; i < widgets.size(); ++i) {
      widgets[i]->highlighted = int(i) == n;
      widgets[i]->repaint();
    }
  }
  void mouseDown(const MouseEvent &) override {
    if (edit) {
      select(-1);
      if (selected)
        selected(-1);
    }
  }
  static Rectangle<int> keyboardFrame() { return {14, 498, 972, 90}; }
  void resized() override { piano.setBounds(keyboardFrame().reduced(6, 6)); }
  void paint(Graphics &g) override {
    const auto &t = doc.theme;
    auto all = getLocalBounds().toFloat();
    Path frame;
    frame.addRoundedRectangle(all.reduced(2), 20);
    g.saveState();
    g.reduceClipRegion(frame);
    g.setGradientFill(ColourGradient(
        t.base.interpolatedWith(Colour(0xff1d3a9a), .16f), 0, 0,
        t.base.darker(.55f), 0, float(height), false));
    g.fillAll();
    // Warm glow behind the keyboard, as in the reference design.
    g.setGradientFill(ColourGradient(t.accent.withAlpha(.22f), 500, 600,
                                     t.accent.withAlpha(0.f), 500, 380, true));
    g.fillRect(0, 380, width, 220);
    const bool header = t.area == "header";
    if (background.isValid()) {
      Graphics::ScopedSaveState state(g);
      Rectangle<int> area(0, 0, width, header ? 240 : 490);
      if (header)
        g.reduceClipRegion(headerWave(true));
      else
        g.reduceClipRegion(area);
      RectanglePlacement place(
          t.fit == "stretch"   ? RectanglePlacement::stretchToFit
          : t.fit == "contain" ? RectanglePlacement::centred
                               : RectanglePlacement::fillDestination);
      g.drawImageWithin(background, area.getX(), area.getY(), area.getWidth(),
                        area.getHeight(), place);
      // Fade the artwork into the panel colour so it blends with the UI.
      g.setGradientFill(ColourGradient(
          t.base.withAlpha(0.f), 0, float(area.getBottom() - 70),
          t.base.darker(.3f).withAlpha(.85f), 0, float(area.getBottom()),
          false));
      g.fillRect(area);
    } else if (header) {
      Graphics::ScopedSaveState state(g);
      g.reduceClipRegion(headerWave(true));
      g.setGradientFill(ColourGradient(t.base.brighter(.25f), 0, 0,
                                       t.base.darker(.4f), 0, 240, false));
      g.fillAll();
      g.setGradientFill(ColourGradient(t.accent.withAlpha(.45f), 860, 60,
                                       t.accent.withAlpha(0.f), 560, 140,
                                       true));
      g.fillRect(0, 0, width, 240);
      g.setGradientFill(ColourGradient(Colour(0xff3f61ee).withAlpha(.35f), 120,
                                       0, Colour(0x003f61ee), 420, 220, true));
      g.fillRect(0, 0, width, 240);
    }
    if (header)
      glowStroke(g, headerWave(false), t.accent, 2.f, 12.f);
    // Thin separators between neighbouring knobs.
    std::vector<Rectangle<int>> knobs;
    for (const auto &c : t.controls)
      if (c.type == "knob")
        knobs.push_back(c.bounds);
    std::sort(knobs.begin(), knobs.end(),
              [](auto a, auto b) { return a.getX() < b.getX(); });
    for (std::size_t i = 0; i + 1 < knobs.size(); ++i) {
      auto a = knobs[i], b = knobs[i + 1];
      int top = jmax(a.getY(), b.getY()), bottom = jmin(a.getBottom(),
                                                        b.getBottom());
      int gap = b.getX() - a.getRight();
      if (bottom - top > 60 && gap >= 0 && gap < 90) {
        float x = float(a.getRight() + gap / 2);
        g.setGradientFill(ColourGradient(
            t.ink.withAlpha(0.f), x, float(top + 10), t.ink.withAlpha(.18f),
            x, float((top + bottom) / 2), false));
        g.fillRect(x, float(top + 20), 1.f, float(bottom - top - 40) * .5f);
        g.setGradientFill(ColourGradient(
            t.ink.withAlpha(.18f), x, float((top + bottom) / 2),
            t.ink.withAlpha(0.f), x, float(bottom - 10), false));
        g.fillRect(x, float((top + bottom) / 2), 1.f,
                   float(bottom - top - 40) * .5f);
      }
    }
    bool hasLogoElement = false;
    for (const auto &c : t.controls)
      hasLogoElement = hasLogoElement || c.type == "logo";
    // Templates made before the logo element keep the original placement.
    if (!hasLogoElement && logo.isValid())
      g.drawImageWithin(logo, 860, 28, 100, 90, RectanglePlacement::centred);
    // Keyboard bed with glowing rim.
    auto kb = keyboardFrame().toFloat();
    g.setColour(Colours::black.withAlpha(.55f));
    g.fillRoundedRectangle(kb.translated(0, 4).expanded(2), 12);
    g.setGradientFill(ColourGradient(Colour(0xff151b33), 0, kb.getY(),
                                     Colour(0xff05070f), 0, kb.getBottom(),
                                     false));
    g.fillRoundedRectangle(kb, 10);
    Path rim;
    rim.addRoundedRectangle(kb, 10);
    glowStroke(g, rim, t.accent.withAlpha(.85f), 1.4f, 9.f);
    g.restoreState();
    Path outer;
    outer.addRoundedRectangle(all.reduced(1.5f), 20);
    glowStroke(g, outer, t.accent.withAlpha(.9f), 1.6f, 6.f);
  }
};
class PreviewHost : public Component {
public:
  InstrumentView view;
  float zoom = 0; // 0 = fit to the available area
  bool shadow = false;
  PreviewHost(Document &d, Values &v, MidiKeyboardState &s) : view(d, v, s) {
    addAndMakeVisible(view);
  }
  float scale() const {
    return zoom > 0 ? zoom
                    : jmin(float(getWidth()) / float(InstrumentView::width),
                           float(getHeight()) / float(InstrumentView::height));
  }
  Rectangle<float> viewArea() const {
    float s = scale();
    return {jmax(0.f, (float(getWidth()) - InstrumentView::width * s) * .5f),
            jmax(0.f, (float(getHeight()) - InstrumentView::height * s) * .5f),
            InstrumentView::width * s, InstrumentView::height * s};
  }
  void resized() override {
    float s = scale();
    auto area = viewArea();
    view.setTransform(AffineTransform::scale(s));
    view.setTopLeftPosition(roundToInt(area.getX() / s),
                            roundToInt(area.getY() / s));
  }
  void paint(Graphics &g) override {
    if (!shadow)
      return;
    auto area = viewArea();
    // Soft reflection/shadow below the plugin.
    g.setGradientFill(ColourGradient(
        view.piano.accent.withAlpha(.18f), area.getCentreX(), area.getBottom(),
        view.piano.accent.withAlpha(0.f), area.getCentreX(),
        area.getBottom() + area.getHeight() * .12f, true));
    g.fillEllipse(area.getX() + area.getWidth() * .1f,
                  area.getBottom() - area.getHeight() * .04f,
                  area.getWidth() * .8f, area.getHeight() * .1f);
    for (int i = 1; i <= 4; ++i) {
      g.setColour(Colours::black.withAlpha(.08f));
      g.fillRoundedRectangle(area.translated(0, float(i) * 3.f)
                                 .expanded(float(i) * 2.f),
                             22.f);
    }
  }
};
} // namespace pw
