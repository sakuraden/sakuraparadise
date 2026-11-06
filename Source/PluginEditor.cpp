#include "PluginEditor.h"

using namespace juce;

namespace
{
const Colour kPlum(0xff6a2c5a);
const Colour kGold(0xffe8b923);
const Colour kPinkP(0xffff9fc4);
const Colour kLipsP(0xffd4173f);
constexpr float twoPi = MathConstants<float>::twoPi;

inline Rectangle<float> R(float x, float y, float w, float h) { return { x, y, w, h }; }
inline Point<float> polar(Point<float> c, float r, float a) { return { c.x + r * std::sin(a), c.y - r * std::cos(a) }; }
}

//==============================================================================
void PearlLook::drawRotarySlider(Graphics& g, int x, int y, int w, int h, float pos,
                                 float a0, float a1, Slider&)
{
    auto area = Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(4.f);
    const float rad = jmin(area.getWidth(), area.getHeight()) / 2.f;
    const auto c = area.getCentre();
    const float ang = a0 + pos * (a1 - a0);
    const float arcR = rad - 4.f;
    const float body = rad * 0.72f;

    Path track;
    track.addCentredArc(c.x, c.y, arcR, arcR, 0.f, a0, a1, true);
    g.setColour(Colours::white.withAlpha(0.55f));
    g.strokePath(track, PathStrokeType(7.f, PathStrokeType::curved, PathStrokeType::rounded));

    Path arc;
    arc.addCentredArc(c.x, c.y, arcR, arcR, 0.f, a0, jmax(a0 + 0.001f, ang), true);
    g.setColour(kGold.withAlpha(0.3f));
    g.strokePath(arc, PathStrokeType(13.f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour(kGold);
    g.strokePath(arc, PathStrokeType(5.f, PathStrokeType::curved, PathStrokeType::rounded));

    g.setColour(Colour(0xffb59ac4));
    for (int i = 0; i <= 10; ++i)
    {
        float a = a0 + (a1 - a0) * (float) i / 10.f;
        auto p1 = polar(c, body + 3.f, a);
        auto p2 = polar(c, body + 8.f, a);
        g.drawLine(p1.x, p1.y, p2.x, p2.y, 1.5f);
    }

    g.setColour(Colour(0xff8a5a9a).withAlpha(0.25f));
    g.fillEllipse(c.x - body + 2.f, c.y - body + 4.f, body * 2.f, body * 2.f);
    ColourGradient mg(Colour(0xffffffff), c.x - body * 0.5f, c.y - body * 0.6f,
                      Colour(0xffd9c8e6), c.x + body, c.y + body, true);
    g.setGradientFill(mg);
    g.fillEllipse(c.x - body, c.y - body, body * 2.f, body * 2.f);
    g.setColour(Colour(0xffc9b4dc));
    g.drawEllipse(c.x - body, c.y - body, body * 2.f, body * 2.f, 2.f);
    g.setColour(kGold.withAlpha(0.8f));
    g.drawEllipse(c.x - body * 0.62f, c.y - body * 0.62f, body * 1.24f, body * 1.24f, 1.5f);

    auto q1 = polar(c, body * 0.25f, ang);
    auto q2 = polar(c, body * 0.9f, ang);
    g.setColour(kPinkP.withAlpha(0.5f));
    g.drawLine(q1.x, q1.y, q2.x, q2.y, 6.f);
    g.setColour(kPlum);
    g.drawLine(q1.x, q1.y, q2.x, q2.y, 2.5f);
}

//==============================================================================
SakuraParadiseAudioProcessorEditor::SakuraParadiseAudioProcessorEditor(SakuraParadiseAudioProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    setSize(760, 930);
    setupKnob(clean,   "clean",   "CLEAN  /  CLEAR SKY", "");
    setupKnob(silk,    "silk",    "SILK  /  RIBBONS", "");
    setupKnob(warmth,  "warmth",  "WARMTH  /  SUNRISE", "");
    setupKnob(air,     "air",     "AIR  /  DOVES", "");
    setupKnob(choir,   "choir",   "CHOIR  /  ANGELS", "");
    setupKnob(heaven,  "heaven",  "HEAVEN  /  GATE", "");
    setupKnob(shimmer, "shimmer", "SHIMMER  /  STARS", "");
    setupKnob(width,   "width",   "WIDTH  /  WINGS", "");
    setupKnob(output,  "output",  "OUTPUT", " dB");

    for (int i = 0; i < 40; ++i)
        petals.push_back({ rnd.nextFloat() * 760.f, rnd.nextFloat() * 930.f,
                           (rnd.nextFloat() - 0.4f) * 0.8f, 0.5f + rnd.nextFloat() * 1.0f,
                           rnd.nextFloat() * 6.28f, (rnd.nextFloat() - 0.5f) * 0.08f,
                           0.7f + rnd.nextFloat() * 0.9f, rnd.nextFloat() < 0.35f });
    startTimerHz(30);
}

SakuraParadiseAudioProcessorEditor::~SakuraParadiseAudioProcessorEditor()
{
    for (auto* k : { &clean, &silk, &warmth, &air, &choir, &heaven, &shimmer, &width, &output })
        k->s.setLookAndFeel(nullptr);
}

void SakuraParadiseAudioProcessorEditor::setupKnob(Knob& k, const String& id, const String& text, const String& suffix)
{
    k.s.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    k.s.setTextBoxStyle(Slider::TextBoxBelow, false, 80, 18);
    k.s.setTextValueSuffix(suffix);
    k.s.setNumDecimalPlacesToDisplay(id == "output" ? 1 : 2);
    k.s.setLookAndFeel(&look);
    k.s.setColour(Slider::textBoxTextColourId, Colour(0xff5a2a50));
    k.s.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    addAndMakeVisible(k.s);
    k.l.setText(text, dontSendNotification);
    k.l.setJustificationType(Justification::centred);
    k.l.setFont(Font(FontOptions(12.f, Font::bold)));
    k.l.setColour(Label::textColourId, Colour(0xff7a3a6a));
    addAndMakeVisible(k.l);
    k.att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, id, k.s);
}

void SakuraParadiseAudioProcessorEditor::resized()
{
    const int xs[4] = { 48, 224, 400, 576 };
    Knob* row1[4] = { &clean, &silk, &warmth, &air };
    Knob* row2[4] = { &choir, &heaven, &shimmer, &width };
    for (int i = 0; i < 4; ++i)
    {
        row1[i]->l.setBounds(xs[i] - 10, 538, 150, 20);
        row1[i]->s.setBounds(xs[i], 558, 130, 150);
        row2[i]->l.setBounds(xs[i] - 10, 722, 150, 20);
        row2[i]->s.setBounds(xs[i], 742, 130, 150);
    }
    output.l.setBounds(645, 6, 110, 18);
    output.s.setBounds(655, 24, 90, 82);
}

void SakuraParadiseAudioProcessorEditor::timerCallback()
{
    ++tick;
    const float target = jlimit(0.f, 1.f, proc.level.load() * 1.6f);
    lvl += (target - lvl) * (target > lvl ? 0.4f : 0.1f);

    for (auto& p : petals)
    {
        p.y += p.vy * (1.f + lvl * 1.5f);
        p.x += p.vx + std::sin(p.y * 0.02f + p.rot) * 0.6f;
        p.rot += p.vr;
        if (p.y > 942.f) { p.y = -12.f; p.x = rnd.nextFloat() * 760.f; }
    }
    repaint();
}

//==============================================================================
void SakuraParadiseAudioProcessorEditor::drawBackground(Graphics& g)
{
    ColourGradient bg(Colour(0xffe3d3ff), 0.f, 0.f, Colour(0xfffff0d8), 0.f, (float) getHeight(), false);
    bg.addColour(0.5, Colour(0xffffdfeb));
    g.setGradientFill(bg);
    g.fillRect(getLocalBounds());
}

void SakuraParadiseAudioProcessorEditor::drawPetals(Graphics& g)
{
    for (auto& p : petals)
    {
        Path pt;
        pt.startNewSubPath(0.f, -5.f);
        pt.quadraticTo(5.f, -4.f, 3.f, 3.f);
        pt.lineTo(0.f, 1.5f);
        pt.lineTo(-3.f, 3.f);
        pt.quadraticTo(-5.f, -4.f, 0.f, -5.f);
        pt.closeSubPath();
        auto tf = AffineTransform::scale(p.size).rotated(p.rot).translated(p.x, p.y);
        g.setColour((p.white ? Colours::white : Colour(0xffffb3d1)).withAlpha(0.8f));
        g.fillPath(pt, tf);
    }
}

void SakuraParadiseAudioProcessorEditor::drawHeader(Graphics& g)
{
    g.setFont(Font(FontOptions(40.f, Font::bold)));
    g.setColour(Colours::white.withAlpha(0.9f));
    g.drawText("SAKURAPARADISE", 32, 16, 600, 52, Justification::centredLeft);
    g.drawText("SAKURAPARADISE", 28, 20, 600, 52, Justification::centredLeft);
    g.setColour(kGold.withAlpha(0.8f));
    g.drawText("SAKURAPARADISE", 33, 19, 600, 52, Justification::centredLeft);
    g.setColour(kPlum);
    g.drawText("SAKURAPARADISE", 30, 18, 600, 52, Justification::centredLeft);

    g.setFont(Font(FontOptions(11.f)));
    g.setColour(kPlum.withAlpha(0.7f));
    g.drawText(String(L"\u685c\u5929\u56fd") + "  //  VOCAL HEAVEN PROCESSOR  //  CLEAN  SILK  AIR  REVERB",
               32, 68, 600, 18, Justification::centredLeft);

    const int lit = (int) (lvl * 24.f);
    for (int i = 0; i < 24; ++i)
    {
        Colour c = Colour(0xffffe08a).interpolatedWith(kPinkP, (float) i / 24.f);
        g.setColour(i < lit ? c : Colours::white.withAlpha(0.45f));
        g.fillRoundedRectangle(R(32.f + (float) i * 14.f, 90.f, 10.f, 8.f), 3.f);
    }
}

//==============================================================================
void SakuraParadiseAudioProcessorEditor::drawCloud(Graphics& g, float x, float y, float s, float a)
{
    static const float o[6][4] = { { -34.f, 4.f, 64.f, 28.f }, { -12.f, -14.f, 60.f, 38.f }, { 18.f, -8.f, 64.f, 34.f },
                                   { 44.f, 4.f, 54.f, 24.f }, { -4.f, 8.f, 70.f, 26.f }, { 26.f, 10.f, 60.f, 22.f } };
    g.setColour(Colour(0xffe9c8e8).withAlpha(a * 0.45f));
    g.fillEllipse(x - 30.f * s, y + 16.f * s, 96.f * s, 14.f * s);
    g.setColour(Colours::white.withAlpha(a));
    for (int k = 0; k < 6; ++k)
        g.fillEllipse(x + o[k][0] * s, y + o[k][1] * s, o[k][2] * s, o[k][3] * s);
}

void SakuraParadiseAudioProcessorEditor::drawTorii(Graphics& g, float cx, float baseY, float h, float glowA)
{
    const float top = baseY - h;
    const float hw = h * 0.45f;
    const Colour red(0xffd8433a);

    ColourGradient gg(Colours::white.withAlpha(glowA), cx, top + h * 0.5f,
                      Colours::white.withAlpha(0.f), cx, top + h * 1.05f, true);
    g.setGradientFill(gg);
    g.fillEllipse(cx - h * 0.55f, top, h * 1.1f, h * 1.1f);

    for (int s = -1; s <= 1; s += 2)
    {
        const float fs = (float) s;
        Path p;
        p.startNewSubPath(cx + fs * hw - 9.f, baseY);
        p.lineTo(cx + fs * (hw - 5.f) - 7.f, top + 8.f);
        p.lineTo(cx + fs * (hw - 5.f) + 7.f, top + 8.f);
        p.lineTo(cx + fs * hw + 9.f, baseY);
        p.closeSubPath();
        g.setColour(red);
        g.fillPath(p);
        g.setColour(Colours::white.withAlpha(0.25f));
        g.drawLine(cx + fs * hw - 4.f, baseY, cx + fs * (hw - 5.f) - 3.f, top + 10.f, 2.f);
        g.setColour(Colour(0xff2a1a22));
        g.fillRect(R(cx + fs * hw - 11.f, baseY - 14.f, 22.f, 14.f));
    }
    g.setColour(red);
    g.fillRect(R(cx - hw + 6.f, top + h * 0.3f, 2.f * hw - 12.f, 11.f));
    g.setColour(Colour(0xff2a1a22).withAlpha(0.5f));
    g.drawRect(R(cx - hw + 6.f, top + h * 0.3f, 2.f * hw - 12.f, 11.f), 1.f);
    g.setColour(red);
    g.fillRect(R(cx - 7.f, top + 24.f, 14.f, h * 0.3f - 20.f));
    g.setColour(kGold);
    g.fillRect(R(cx - 11.f, top + h * 0.3f - 24.f, 22.f, 24.f));
    g.setColour(Colour(0xff2a1a22));
    g.drawRect(R(cx - 11.f, top + h * 0.3f - 24.f, 22.f, 24.f), 1.2f);

    Path shim;
    shim.startNewSubPath(cx - hw - 26.f, top + 16.f);
    shim.quadraticTo(cx, top + 28.f, cx + hw + 26.f, top + 16.f);
    shim.lineTo(cx + hw + 26.f, top + 30.f);
    shim.quadraticTo(cx, top + 42.f, cx - hw - 26.f, top + 30.f);
    shim.closeSubPath();
    g.setColour(red);
    g.fillPath(shim);

    Path kas;
    kas.startNewSubPath(cx - hw - 34.f, top + 12.f);
    kas.quadraticTo(cx, top + 24.f, cx + hw + 34.f, top + 12.f);
    kas.lineTo(cx + hw + 42.f, top - 6.f);
    kas.quadraticTo(cx, top + 6.f, cx - hw - 42.f, top - 6.f);
    kas.closeSubPath();
    g.setColour(Colour(0xff2a1a22));
    g.fillPath(kas);
}

void SakuraParadiseAudioProcessorEditor::drawBlossoms(Graphics& g, float x0, float y0, float dir, float len, float phase)
{
    Path br;
    br.startNewSubPath(x0, y0);
    br.cubicTo(x0 + dir * len * 0.3f, y0 - len * 0.1f, x0 + dir * len * 0.7f, y0 - len * 0.5f,
               x0 + dir * len, y0 - len * 0.35f);
    g.setColour(Colour(0xff5a3a3a));
    g.strokePath(br, PathStrokeType(6.f, PathStrokeType::curved, PathStrokeType::rounded));

    const Colour cols[3] = { Colour(0xffffb7d0), Colour(0xffff93bb), Colour(0xffffd9e8) };
    for (int k = 0; k < 7; ++k)
    {
        const float tt = 0.25f + (float) k * 0.12f;
        const float px = x0 + dir * len * tt;
        const float py = y0 - len * tt * 0.4f;
        const float sw = std::sin((float) tick * 0.03f + phase + (float) k) * 2.f;
        for (int j = 0; j < 5; ++j)
        {
            const float ox = (float) ((j * 37 + k * 13) % 21) - 10.f;
            const float oy = (float) ((j * 53 + k * 29) % 17) - 14.f;
            const float rr = 6.f + (float) ((j + k) % 4) * 2.5f;
            g.setColour(cols[(j + k) % 3].withAlpha(0.92f));
            g.fillEllipse(px + ox + sw - rr, py + oy - rr, rr * 2.f, rr * 2.f);
        }
    }
}

void SakuraParadiseAudioProcessorEditor::drawDove(Graphics& g, float x, float y, float flap, float s)
{
    Graphics::ScopedSaveState ss(g);
    g.addTransform(AffineTransform::scale(s).translated(x, y));
    Path wing;
    wing.startNewSubPath(-2.f, 0.f);
    wing.quadraticTo(-10.f, -14.f + flap, -20.f, -6.f + flap * 0.5f);
    wing.quadraticTo(-10.f, -3.f, -2.f, 2.f);
    wing.closeSubPath();
    Path wing2;
    wing2.startNewSubPath(2.f, 0.f);
    wing2.quadraticTo(12.f, -16.f + flap, 22.f, -8.f + flap * 0.5f);
    wing2.quadraticTo(12.f, -3.f, 2.f, 2.f);
    wing2.closeSubPath();
    g.setColour(Colours::white);
    g.fillEllipse(-9.f, -3.f, 18.f, 8.f);
    g.fillEllipse(6.f, -7.f, 6.f, 6.f);
    g.fillPath(wing);
    g.fillPath(wing2);
    g.setColour(Colour(0xffc9b4dc));
    g.strokePath(wing, PathStrokeType(0.8f));
    g.strokePath(wing2, PathStrokeType(0.8f));
    Path tail;
    tail.addTriangle(-9.f, 0.f, -18.f, 3.f, -9.f, 4.f);
    g.setColour(Colours::white);
    g.fillPath(tail);
    Path beak;
    beak.addTriangle(11.f, -5.f, 15.f, -4.f, 11.f, -3.f);
    g.setColour(Colour(0xffff9a3c));
    g.fillPath(beak);
}

void SakuraParadiseAudioProcessorEditor::drawStar(Graphics& g, float x, float y, float s, float alpha, Colour c)
{
    Path st;
    st.startNewSubPath(0.f, -s);
    st.lineTo(s * 0.22f, -s * 0.22f);
    st.lineTo(s, 0.f);
    st.lineTo(s * 0.22f, s * 0.22f);
    st.lineTo(0.f, s);
    st.lineTo(-s * 0.22f, s * 0.22f);
    st.lineTo(-s, 0.f);
    st.lineTo(-s * 0.22f, -s * 0.22f);
    st.closeSubPath();
    g.setColour(c.withAlpha(alpha * 0.3f));
    g.fillEllipse(x - s * 1.2f, y - s * 1.2f, s * 2.4f, s * 2.4f);
    g.setColour(c.withAlpha(alpha));
    g.fillPath(st, AffineTransform::translation(x, y));
}

void SakuraParadiseAudioProcessorEditor::drawAngel(Graphics& g, float x, float y, float s)
{
    Graphics::ScopedSaveState ss(g);
    g.addTransform(AffineTransform::scale(s).translated(x, y));
    for (int i = -1; i <= 1; i += 2)
    {
        Path w;
        w.addEllipse(-7.f, -30.f, 14.f, 34.f);
        g.setColour(Colours::white.withAlpha(0.95f));
        g.fillPath(w, AffineTransform::rotation((float) i * 0.9f).translated((float) i * 6.f, -12.f));
    }
    g.setColour(Colours::white);
    Path body;
    body.startNewSubPath(-7.f, -14.f);
    body.quadraticTo(0.f, -18.f, 7.f, -14.f);
    body.lineTo(14.f, 18.f);
    body.quadraticTo(0.f, 24.f, -14.f, 18.f);
    body.closeSubPath();
    g.fillPath(body);
    g.setColour(Colour(0xffd9cbe8));
    g.strokePath(body, PathStrokeType(0.8f));
    g.setColour(kPinkP);
    g.fillRect(R(-9.f, -4.f, 18.f, 4.f));
    g.setColour(Colour(0xffdfe6f5));
    g.fillEllipse(-8.f, -34.f, 16.f, 20.f);
    g.setColour(Colour(0xff2a1d1a));
    Path e1, e2;
    e1.startNewSubPath(-5.f, -25.f); e1.quadraticTo(-3.f, -22.f, -1.f, -25.f);
    e2.startNewSubPath(1.f, -25.f);  e2.quadraticTo(3.f, -22.f, 5.f, -25.f);
    g.strokePath(e1, PathStrokeType(0.9f));
    g.strokePath(e2, PathStrokeType(0.9f));
    g.setColour(kLipsP);
    g.fillEllipse(-2.f, -20.f, 4.f, 1.8f);
    g.setColour(Colour(0xffff9a86).withAlpha(0.5f));
    g.fillEllipse(-7.f, -23.f, 4.f, 3.f);
    g.fillEllipse(3.f, -23.f, 4.f, 3.f);
    g.setColour(Colour(0xffffe08a).withAlpha(0.4f + lvl * 0.4f));
    g.drawEllipse(-11.f, -43.f, 22.f, 8.f, 4.f);
    g.setColour(Colour(0xffffd24a));
    g.drawEllipse(-10.f, -42.f, 20.f, 6.f, 1.8f);
}

void SakuraParadiseAudioProcessorEditor::drawSingerFace(Graphics& g)
{
    const float t = (float) tick;
    g.setColour(Colour(0xff3d2a20));
    g.fillEllipse(-24.f, -27.f, 48.f, 50.f);
    for (int i = 0; i < 7; ++i)
    {
        float a = -2.7f + (float) i * 0.45f;
        float sw = std::sin(t * 0.06f + (float) i) * 1.2f;
        g.fillEllipse(std::cos(a) * 23.f - 4.f + sw, std::sin(a) * 25.f - 4.f, 8.f, 9.f);
    }
    g.fillEllipse(-26.f, 0.f, 9.f, 22.f);
    g.fillEllipse(17.f, 0.f, 9.f, 22.f);

    Path face;
    face.addEllipse(-17.f, -21.f, 34.f, 42.f);
    ColourGradient mg(Colour(0xfffbfbff), -8.f, -14.f, Colour(0xffc4cdea), 12.f, 18.f, false);
    g.setGradientFill(mg);
    g.fillPath(face);
    g.setColour(Colour(0xff9aa4c4));
    g.strokePath(face, PathStrokeType(0.7f));

    g.setColour(Colour(0xff2a1d1a));
    for (int s = -1; s <= 1; s += 2)
    {
        Path b;
        b.addRoundedRectangle(-3.f, -1.1f, 6.f, 2.2f, 1.1f);
        g.fillPath(b, AffineTransform::rotation((float) s * -0.15f).translated((float) s * 7.5f, -14.f));
        Path e;
        const float ex = (float) s * 6.5f;
        e.startNewSubPath(ex - 4.5f, -5.f);
        e.quadraticTo(ex, -1.5f, ex + 4.5f, -5.f);
        g.strokePath(e, PathStrokeType(1.4f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    Path n;
    n.startNewSubPath(0.5f, -1.f);
    n.quadraticTo(-1.2f, 4.5f, 1.f, 5.2f);
    g.setColour(Colour(0xff7a82a8).withAlpha(0.6f));
    g.strokePath(n, PathStrokeType(0.7f));
    g.setColour(Colour(0xffff8f9a).withAlpha(0.5f));
    g.fillEllipse(-15.f, 3.f, 9.f, 6.f);
    g.fillEllipse(6.f, 3.f, 9.f, 6.f);

    if (lvl < 0.06f)
    {
        g.setColour(kLipsP);
        g.fillEllipse(-4.f, 11.f, 8.f, 3.2f);
    }
    else
    {
        const float mw = 2.f + lvl * 4.f, mh = 1.5f + lvl * 9.f;
        g.setColour(Colour(0xff3a0012));
        g.fillEllipse(-mw, 11.f, mw * 2.f, mh * 2.f);
        g.setColour(Colour(0xffff8fb8));
        g.fillEllipse(-mw * 0.6f, 11.f + mh * 1.2f, mw * 1.2f, mh * 0.7f);
        g.setColour(kLipsP);
        g.drawEllipse(-mw - 0.5f, 10.5f, mw * 2.f + 1.f, mh * 2.f + 1.f, 1.1f);
    }

    for (int i = 0; i < 2; ++i)
    {
        float sw = std::sin(t * 0.12f + (float) i) * 1.5f;
        Path s;
        s.startNewSubPath(15.f + (float) i * 1.5f, -12.f);
        s.quadraticTo(20.f + sw, 2.f, 17.f + (float) i * 2.f + sw, 18.f);
        g.setColour(Colour(0xffd4172f));
        g.strokePath(s, PathStrokeType(1.2f));
        g.fillEllipse(16.f + (float) i * 2.f + sw, 17.f, 2.2f, 2.2f);
    }
    const Colour pk[3] = { Colour(0xffffb7d0), Colour(0xffff93bb), Colour(0xffffd9e8) };
    for (int i = 0; i < 5; ++i)
    {
        float a = (float) i * twoPi / 5.f;
        g.setColour(pk[i % 3]);
        g.fillEllipse(-17.f + std::cos(a) * 3.f - 2.5f, -12.f + std::sin(a) * 3.f - 2.5f, 5.f, 5.f);
    }
    g.setColour(kGold);
    g.fillEllipse(-18.f, -13.f, 2.f, 2.f);
}

void SakuraParadiseAudioProcessorEditor::drawSinger(Graphics& g, float cx, float baseY)
{
    const float t = (float) tick;
    const float bob = std::sin(t * 0.05f) * 4.f;
    const float wid = val(width);
    const float K = 1.3f;

    g.setColour(Colour(0xff9a7ab8).withAlpha(0.25f));
    g.fillEllipse(cx - 55.f, baseY + 4.f, 110.f, 14.f);

    Graphics::ScopedSaveState ss(g);
    g.addTransform(AffineTransform::scale(K, K, cx, baseY).translated(0.f, bob));

    // halo glow
    {
        ColourGradient hg(Colour(0xffffe08a).withAlpha(0.35f + lvl * 0.4f), cx, baseY - 218.f,
                          Colour(0xffffe08a).withAlpha(0.f), cx + 70.f, baseY - 218.f, true);
        g.setGradientFill(hg);
        g.fillEllipse(cx - 70.f, baseY - 288.f, 140.f, 140.f);
    }

    // wings
    const float spread = 0.55f + wid * 0.75f + std::sin(t * 0.1f) * 0.03f * (1.f + lvl * 3.f);
    for (int s = -1; s <= 1; s += 2)
    {
        const float fs = (float) s;
        for (int k = 8; k >= 0; --k)
        {
            const float f = (float) k / 8.f;
            const float ang = fs * (0.25f + f * 1.5f * spread);
            const float len = 78.f + 30.f * f;
            Path fp;
            fp.startNewSubPath(0.f, 0.f);
            fp.quadraticTo(-8.f, -len * 0.5f, 0.f, -len);
            fp.quadraticTo(8.f, -len * 0.5f, 0.f, 0.f);
            fp.closeSubPath();
            auto tf = AffineTransform::rotation(ang).translated(cx + fs * 14.f, baseY - 150.f);
            g.setColour(Colours::white.interpolatedWith(Colour(0xffffc6dc), f * 0.7f).withAlpha(0.96f));
            g.fillPath(fp, tf);
            g.setColour(Colour(0xffd9cbe8));
            g.strokePath(fp, PathStrokeType(0.8f), tf);
        }
    }

    // robe
    Path robe;
    robe.startNewSubPath(cx - 24.f, baseY - 152.f);
    robe.quadraticTo(cx, baseY - 160.f, cx + 24.f, baseY - 152.f);
    robe.lineTo(cx + 30.f, baseY - 100.f);
    robe.quadraticTo(cx + 54.f, baseY - 50.f, cx + 60.f, baseY - 8.f);
    robe.quadraticTo(cx, baseY + 4.f, cx - 60.f, baseY - 8.f);
    robe.quadraticTo(cx - 54.f, baseY - 50.f, cx - 30.f, baseY - 100.f);
    robe.closeSubPath();
    ColourGradient rg(Colour(0xffffffff), cx - 30.f, baseY - 150.f, Colour(0xffe5d6f3), cx + 50.f, baseY, false);
    g.setGradientFill(rg);
    g.fillPath(robe);
    g.setColour(Colour(0xffc9b4dc).withAlpha(0.7f));
    g.strokePath(robe, PathStrokeType(1.f));
    g.setColour(Colour(0xffb59ac4).withAlpha(0.45f));
    g.drawLine(cx - 10.f, baseY - 98.f, cx - 24.f, baseY - 10.f, 1.2f);
    g.drawLine(cx + 6.f, baseY - 98.f, cx + 12.f, baseY - 8.f, 1.2f);
    g.drawLine(cx + 20.f, baseY - 96.f, cx + 40.f, baseY - 12.f, 1.2f);
    g.setColour(kPinkP.withAlpha(0.7f));
    for (int i = 0; i < 9; ++i)
    {
        float dx = cx - 42.f + (float) ((i * 23) % 84);
        float dy = baseY - 70.f + (float) ((i * 17) % 58);
        g.fillEllipse(dx - 3.f, dy - 3.f, 6.f, 6.f);
    }
    Path hem;
    hem.startNewSubPath(cx - 60.f, baseY - 8.f);
    hem.quadraticTo(cx, baseY + 4.f, cx + 60.f, baseY - 8.f);
    g.setColour(kGold);
    g.strokePath(hem, PathStrokeType(3.f));
    Path vneck;
    vneck.startNewSubPath(cx - 10.f, baseY - 155.f);
    vneck.lineTo(cx, baseY - 130.f);
    vneck.lineTo(cx + 10.f, baseY - 155.f);
    g.strokePath(vneck, PathStrokeType(2.f));
    g.setColour(kPinkP);
    g.fillRoundedRectangle(cx - 30.f, baseY - 104.f, 60.f, 13.f, 3.f);
    g.fillEllipse(cx + 12.f, baseY - 112.f, 20.f, 14.f);
    g.fillEllipse(cx + 22.f, baseY - 98.f, 12.f, 24.f);
    g.setColour(Colour(0xffd96a96));
    g.fillEllipse(cx + 18.f, baseY - 108.f, 8.f, 8.f);

    // arms
    const Colour skin(0xffe8c4a8);
    for (int s = -1; s <= 1; s += 2)
    {
        const float fs = (float) s;
        Path arm;
        arm.startNewSubPath(cx + fs * 22.f, baseY - 150.f);
        arm.lineTo(cx + fs * 52.f, baseY - 134.f);
        const float hx = cx + fs * 72.f;
        const float hy = baseY - 150.f + std::sin(t * 0.07f + fs) * 4.f;
        arm.lineTo(hx, hy);
        g.setColour(Colour(0xffc9b4dc));
        g.strokePath(arm, PathStrokeType(18.f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour(Colours::white);
        g.strokePath(arm, PathStrokeType(16.f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour(kGold);
        g.drawEllipse(hx - 8.f, hy - 8.f, 16.f, 16.f, 2.f);
        g.setColour(skin);
        g.fillEllipse(hx - 4.f + fs * 6.f, hy - 5.f, 10.f, 10.f);
        for (int i = 0; i < 4; ++i)
            g.drawLine(hx + fs * 9.f, hy - 4.f + (float) i * 2.8f, hx + fs * 15.f, hy - 7.f + (float) i * 4.f, 1.8f);
    }

    // neck + head + halo
    g.setColour(skin);
    g.fillRect(R(cx - 4.f, baseY - 168.f, 8.f, 10.f));
    {
        Graphics::ScopedSaveState hs(g);
        g.addTransform(AffineTransform::translation(cx, baseY - 184.f));
        drawSingerFace(g);
    }
    g.setColour(Colour(0xffffe08a).withAlpha(0.45f + lvl * 0.4f));
    g.drawEllipse(cx - 24.f, baseY - 224.f, 48.f, 12.f, 7.f);
    g.setColour(Colour(0xffffd24a));
    g.drawEllipse(cx - 23.f, baseY - 223.f, 46.f, 10.f, 3.f);
}

//==============================================================================
void SakuraParadiseAudioProcessorEditor::drawScene(Graphics& g)
{
    const auto sc = sceneRect();
    const float cx = sc.getCentreX();
    const float t = (float) tick;
    const float vClean = val(clean), vSilk = val(silk), vWarm = val(warmth), vAir = val(air);
    const float vChoir = val(choir), vHeaven = val(heaven), vShim = val(shimmer);
    const float floorY = sc.getBottom() - 70.f;

    g.setColour(Colours::white.withAlpha(0.35f));
    g.fillRoundedRectangle(sc.expanded(5.f), 18.f);

    Graphics::ScopedSaveState ss(g);
    Path clip;
    clip.addRoundedRectangle(sc.getX(), sc.getY(), sc.getWidth(), sc.getHeight(), 14.f);
    g.reduceClipRegion(clip, AffineTransform());

    // sky
    ColourGradient sky(Colour(0xff9cc4ff), cx, sc.getY(), Colour(0xffffe7b8), cx, sc.getBottom(), false);
    sky.addColour(0.5, Colour(0xffffd1e6));
    g.setGradientFill(sky);
    g.fillRect(sc);

    // sun + rays
    const float sy = sc.getY() + 170.f;
    const float sunR = 56.f + vWarm * 34.f + lvl * 8.f;
    {
        ColourGradient glow(Colour(0xffffe9a8).withAlpha(0.35f + vWarm * 0.5f), cx, sy,
                            Colour(0xffffe9a8).withAlpha(0.f), cx, sy + sunR * 4.f, true);
        g.setGradientFill(glow);
        g.fillEllipse(cx - sunR * 4.f, sy - sunR * 4.f, sunR * 8.f, sunR * 8.f);
        const float rayA = 0.05f + vHeaven * 0.16f + lvl * 0.05f;
        g.setColour(Colour(0xfffff3c4).withAlpha(rayA));
        for (int i = 0; i < 14; ++i)
        {
            const float a = (float) i * twoPi / 14.f + t * 0.004f;
            Path ray;
            ray.startNewSubPath(cx, sy);
            ray.lineTo(cx + std::sin(a - 0.05f) * 700.f, sy - std::cos(a - 0.05f) * 700.f);
            ray.lineTo(cx + std::sin(a + 0.05f) * 700.f, sy - std::cos(a + 0.05f) * 700.f);
            ray.closeSubPath();
            g.fillPath(ray);
        }
        ColourGradient sun(Colours::white, cx, sy, Colour(0xffffd77a), cx, sy + sunR, true);
        g.setGradientFill(sun);
        g.fillEllipse(cx - sunR, sy - sunR, sunR * 2.f, sunR * 2.f);
    }

    // far clouds
    const float W = sc.getWidth() + 300.f;
    for (int i = 0; i < 4; ++i)
    {
        float x = std::fmod((float) i * 210.f + t * (0.15f + (float) i * 0.05f), W) - 150.f + sc.getX();
        drawCloud(g, x, sc.getY() + 50.f + (float) i * 38.f, 0.9f + (float) (i % 2) * 0.3f, 0.7f);
    }

    // sparkles (shimmer)
    {
        const int count = (int) (vShim * 40.f);
        for (int i = 0; i < count; ++i)
        {
            float x = sc.getX() + std::fmod((float) i * 97.3f, sc.getWidth());
            float y = sc.getBottom() - std::fmod((float) i * 61.7f + t * (0.4f + (float) (i % 4) * 0.2f), sc.getHeight());
            float s = 2.5f + (float) (i % 3) * 1.5f;
            float a = 0.4f + 0.6f * std::abs(std::sin(t * 0.1f + (float) i));
            drawStar(g, x, y, s, a, (i % 2 == 0) ? Colour(0xffffe08a) : Colours::white);
        }
    }

    // torii gate of heaven
    drawTorii(g, cx, floorY - 6.f, 240.f, 0.1f + vHeaven * 0.75f + lvl * 0.15f);

    // cherry trees
    drawBlossoms(g, sc.getX() - 10.f, sc.getBottom() - 20.f, 1.f, 210.f, 0.f);
    drawBlossoms(g, sc.getX() - 10.f, sc.getBottom() - 120.f, 1.f, 150.f, 2.f);
    drawBlossoms(g, sc.getRight() + 10.f, sc.getBottom() - 20.f, -1.f, 210.f, 1.f);
    drawBlossoms(g, sc.getRight() + 10.f, sc.getBottom() - 120.f, -1.f, 150.f, 3.f);

    // silk ribbons
    {
        const int count = (int) (vSilk * 5.f + 0.99f);
        const Colour rc[5] = { Colour(0xffffb7d0), Colours::white, Colour(0xffffe08a), Colour(0xffc9b4ff), Colour(0xffff93bb) };
        for (int i = 0; i < count && vSilk > 0.001f; ++i)
        {
            Path rb;
            const float base = sc.getY() + 110.f + (float) i * 45.f;
            bool first = true;
            for (float x = sc.getX(); x <= sc.getRight(); x += 16.f)
            {
                float y = base + std::sin(x * 0.02f + t * 0.04f + (float) i) * 18.f;
                if (first) { rb.startNewSubPath(x, y); first = false; }
                else rb.lineTo(x, y);
            }
            g.setColour(rc[i % 5].withAlpha(0.55f));
            g.strokePath(rb, PathStrokeType(6.f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour(Colours::white.withAlpha(0.5f));
            g.strokePath(rb, PathStrokeType(1.5f, PathStrokeType::curved, PathStrokeType::rounded));
        }
    }

    // cloud floor
    for (int row = 0; row < 2; ++row)
        for (float x = sc.getX() - 60.f + (float) row * 45.f; x < sc.getRight() + 60.f; x += 90.f)
        {
            float y = floorY + 4.f + (float) row * 26.f + std::sin(t * 0.03f + x * 0.02f) * 3.f;
            g.setColour(Colours::white.withAlpha(0.96f));
            g.fillEllipse(x, y, 140.f, 74.f);
            g.setColour(Colour(0xffe9c8e8).withAlpha(0.35f));
            g.fillEllipse(x + 10.f, y + 40.f, 120.f, 30.f);
        }

    // choir of little angels
    {
        const float ap[6][2] = { { 110.f, 200.f }, { 600.f, 210.f }, { 190.f, 150.f }, { 510.f, 140.f }, { 90.f, 300.f }, { 620.f, 310.f } };
        const int count = (int) (vChoir * 6.f + 0.5f);
        for (int i = 0; i < count; ++i)
            drawAngel(g, sc.getX() + ap[i][0], sc.getY() + ap[i][1] + std::sin(t * 0.05f + (float) i) * 6.f, 0.9f);
    }

    // singer
    drawSinger(g, cx, floorY + 18.f);

    // doves
    {
        const int count = (int) (vAir * 6.f + 0.99f);
        for (int i = 0; i < count && vAir > 0.001f; ++i)
        {
            float x = sc.getX() + std::fmod(t * (1.2f + 0.3f * (float) i) + (float) i * 140.f, sc.getWidth() + 120.f) - 60.f;
            float y = sc.getY() + 70.f + (float) i * 38.f + std::sin(t * 0.07f + (float) i) * 10.f;
            drawDove(g, x, y, std::sin(t * 0.3f + (float) i) * 7.f, 1.1f);
        }
    }

    // dust grain (disappears with CLEAN)
    {
        const int count = (int) ((1.f - vClean) * 45.f);
        g.setColour(Colour(0xff8a8398).withAlpha(0.35f));
        for (int i = 0; i < count; ++i)
        {
            float x = sc.getX() + std::fmod((float) i * 83.7f + t * (0.2f + (float) (i % 3) * 0.15f), sc.getWidth());
            float y = sc.getY() + std::fmod((float) i * 47.3f + t * 0.1f * (float) (1 + i % 2), sc.getHeight());
            float s = 1.5f + (float) (i % 3);
            g.fillEllipse(x, y, s, s);
        }
        g.setColour(Colour(0xffb8b2c4).withAlpha((1.f - vClean) * 0.12f));
        g.fillRect(sc);
    }

    g.setColour(Colours::white.withAlpha(0.8f));
    g.drawRoundedRectangle(sc, 14.f, 2.f);
}

void SakuraParadiseAudioProcessorEditor::drawDeck(Graphics& g)
{
    const auto deck = R(20.f, 518.f, 720.f, 395.f);
    g.setColour(Colours::white.withAlpha(0.35f));
    g.fillRoundedRectangle(deck, 16.f);
    g.setColour(kGold.withAlpha(0.7f));
    g.drawRoundedRectangle(deck, 16.f, 2.f);

    const float xs[4] = { 48.f, 224.f, 400.f, 576.f };
    for (int row = 0; row < 2; ++row)
        for (int i = 0; i < 4; ++i)
        {
            auto plate = R(xs[i] - 10.f, 530.f + (float) row * 184.f, 150.f, 184.f);
            g.setColour(Colours::white.withAlpha(0.3f));
            g.fillRoundedRectangle(plate, 12.f);
            g.setColour(Colour(0xffc9b4dc).withAlpha(0.8f));
            g.drawRoundedRectangle(plate, 12.f, 1.5f);
        }
    auto op = R(645.f, 4.f, 110.f, 104.f);
    g.setColour(Colours::white.withAlpha(0.4f));
    g.fillRoundedRectangle(op, 12.f);
    g.setColour(Colour(0xffc9b4dc).withAlpha(0.8f));
    g.drawRoundedRectangle(op, 12.f, 1.5f);

    g.setFont(Font(FontOptions(10.f)));
    g.setColour(kPlum.withAlpha(0.6f));
    g.drawText("SAKURAPARADISE  //  FOR AMBIENT VOCALS", 20, 898, 720, 14, Justification::centred);
}

void SakuraParadiseAudioProcessorEditor::paint(Graphics& g)
{
    drawBackground(g);
    drawPetals(g);
    drawHeader(g);
    drawScene(g);
    drawDeck(g);
}
