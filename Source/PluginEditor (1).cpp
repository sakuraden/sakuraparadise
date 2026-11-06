#include "PluginEditor.h"

using namespace juce;

namespace
{
const Colour kPlum(0xff6a2c5a);
const Colour kGold(0xffe8b923);
const Colour kPinkP(0xffff9fc4);
const Colour kLipsP(0xffd4173f);
// colour of each knob's effects: clean, silk, warmth, air, choir, heaven, shimmer, width, anime
const Colour kPal[9] = { Colour(0xff9fe8ff), Colour(0xffff9fc4), Colour(0xffffc94a), Colour(0xffffffff), Colour(0xffffd9a0),
                         Colour(0xfffff3b0), Colour(0xffc9b4ff), Colour(0xffffb0e0), Colour(0xffff6fa8) };
constexpr float twoPi = MathConstants<float>::twoPi;

inline Rectangle<float> R(float x, float y, float w, float h) { return { x, y, w, h }; }
inline Point<float> polar(Point<float> c, float r, float a) { return { c.x + r * std::sin(a), c.y - r * std::cos(a) }; }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
}

//==============================================================================
void PearlLook::drawRotarySlider(Graphics& g, int x, int y, int w, int h, float pos,
                                 float a0, float a1, Slider& slider)
{
    auto area = Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(4.f);
    const float rad = jmin(area.getWidth(), area.getHeight()) / 2.f;
    const auto c = area.getCentre();
    const float ang = a0 + pos * (a1 - a0);
    const float arcR = rad - 4.f;
    const float body = rad * 0.72f;

    if (slider.isMouseOverOrDragging())
    {
        ColourGradient hot(kPinkP.withAlpha(0.5f), c.x, c.y, kPinkP.withAlpha(0.f), c.x + rad, c.y, true);
        g.setGradientFill(hot);
        g.fillEllipse(c.x - rad, c.y - rad, rad * 2.f, rad * 2.f);
    }

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
    setupKnob(anime,   "anime",   "ANIME  /  KAWAII", "");
    setupKnob(output,  "output",  "OUTPUT", " dB");

    for (int i = 0; i < 9; ++i) prevVal[i] = val(knobAt(i));

    for (int i = 0; i < 40; ++i)
        petals.push_back({ rnd.nextFloat() * 760.f, rnd.nextFloat() * 930.f,
                           (rnd.nextFloat() - 0.4f) * 0.8f, 0.5f + rnd.nextFloat() * 1.0f,
                           rnd.nextFloat() * 6.28f, (rnd.nextFloat() - 0.5f) * 0.08f,
                           0.7f + rnd.nextFloat() * 0.9f, rnd.nextFloat() < 0.35f });
    startTimerHz(30);
}

SakuraParadiseAudioProcessorEditor::~SakuraParadiseAudioProcessorEditor()
{
    for (auto* k : { &clean, &silk, &warmth, &air, &choir, &heaven, &shimmer, &width, &anime, &output })
        k->s.setLookAndFeel(nullptr);
}

SakuraParadiseAudioProcessorEditor::Knob& SakuraParadiseAudioProcessorEditor::knobAt(int i)
{
    switch (i)
    {
        case 0: return clean;
        case 1: return silk;
        case 2: return warmth;
        case 3: return air;
        case 4: return choir;
        case 5: return heaven;
        case 6: return shimmer;
        case 7: return width;
        default: return anime;
    }
}

float SakuraParadiseAudioProcessorEditor::kickMax() const
{
    float m = 0.f;
    for (int i = 0; i < 9; ++i) m = jmax(m, kick[i]);
    return m;
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
    anime.l.setBounds(525, 6, 110, 18);
    anime.s.setBounds(535, 24, 90, 82);
    output.l.setBounds(645, 6, 110, 18);
    output.s.setBounds(655, 24, 90, 82);
}

void SakuraParadiseAudioProcessorEditor::spawnSpark(int kind, int shape, float x, float y, float vx, float vy, float jitter)
{
    if (sparks.size() > 500) return;
    float size = 3.f;
    switch (shape)
    {
        case 0: size = 2.f + rnd.nextFloat() * 3.f; break;
        case 1: size = 3.f + rnd.nextFloat() * 4.f; break;
        case 2: size = 4.f + rnd.nextFloat() * 4.f; break;
        case 3: size = 3.f + rnd.nextFloat() * 3.f; break;
        default: size = 0.7f + rnd.nextFloat() * 0.5f; break;
    }
    sparks.push_back({ x, y, vx + (rnd.nextFloat() - 0.5f) * jitter, vy + (rnd.nextFloat() - 0.5f) * jitter,
                       1.f, size, kind, shape });
}

void SakuraParadiseAudioProcessorEditor::timerCallback()
{
    ++tick;
    const float target = jlimit(0.f, 1.f, proc.level.load() * 1.6f);
    lvl += (target - lvl) * (target > lvl ? 0.4f : 0.1f);

    const auto sc = sceneRect();
    const float cx = sc.getCentreX();
    const float floorY = sc.getBottom() - 70.f;
    const float singerY = floorY - 100.f;

    // every knob: how fast is it being turned -> kick (visual energy)
    for (int i = 0; i < 9; ++i)
    {
        Knob& k = knobAt(i);
        const float v = val(k);
        const float d = std::abs(v - prevVal[i]);
        prevVal[i] = v;
        kick[i] = jmax(kick[i] * 0.9f, jlimit(0.f, 1.f, d * 20.f));
        act[i] += ((k.s.isMouseButtonDown() ? 1.f : 0.f) - act[i]) * 0.2f;
        const float e = jmax(kick[i], act[i] * 0.5f);
        if (e > 0.1f)
        {
            const int n = (int) (e * 4.f) + 1;
            for (int j = 0; j < n; ++j)
            {
                const float side = rnd.nextFloat() < 0.5f ? -1.f : 1.f;
                switch (i)
                {
                    case 0: spawnSpark(0, 1, sc.getX() + rnd.nextFloat() * sc.getWidth(), sc.getY() + rnd.nextFloat() * sc.getHeight(), 6.f, 0.f, 2.f); break;
                    case 1: spawnSpark(1, 0, cx + (rnd.nextFloat() - 0.5f) * 140.f, singerY - 80.f, 0.f, -0.5f, 4.f); break;
                    case 2: spawnSpark(2, 0, cx + (rnd.nextFloat() - 0.5f) * 260.f, floorY, 0.f, -2.5f, 1.f); break;
                    case 3: spawnSpark(3, 3, sc.getX() + rnd.nextFloat() * sc.getWidth(), sc.getY() + 5.f, 0.f, 1.2f, 1.5f); break;
                    case 4: spawnSpark(4, 4, cx + (rnd.nextFloat() - 0.5f) * 80.f, singerY - 150.f, 0.f, -1.8f, 2.f); break;
                    case 5: spawnSpark(5, 0, cx + (rnd.nextFloat() - 0.5f) * 120.f, floorY - 20.f, 0.f, -4.5f, 1.5f); break;
                    case 6: spawnSpark(6, 1, cx, singerY - 100.f, 0.f, 0.f, 9.f); break;
                    case 7: spawnSpark(7, 3, cx + side * 70.f, singerY - 120.f, side * 3.f, -1.f, 2.f); break;
                    default: spawnSpark(8, 2, cx + (rnd.nextFloat() - 0.5f) * 60.f, singerY - 120.f, 0.f, -2.f, 3.f); break;
                }
            }
            if (kick[i] > 0.4f && tick % 4 == 0)
                rings.push_back({ cx, singerY - 40.f, 20.f, 1.f, i });
        }
    }

    // notes from the singer's mouth
    if (lvl > 0.12f && tick % 5 == 0)
        spawnSpark(4, 4, cx + 10.f, floorY - 204.f, (rnd.nextFloat() - 0.3f) * 1.2f, -1.5f, 1.f);

    for (auto& p : sparks)
    {
        p.x += p.vx; p.y += p.vy;
        p.life -= (p.shape == 3 ? 0.008f : 0.014f);
        p.vx *= 0.995f;
        if (p.shape == 3) { p.vy += 0.02f; p.vx += std::sin((float) tick * 0.1f + p.size) * 0.04f; }
    }
    sparks.erase(std::remove_if(sparks.begin(), sparks.end(), [](const Spark& s) { return s.life <= 0.f; }), sparks.end());

    for (auto& r : rings) { r.r += 4.5f; r.life -= 0.035f; }
    rings.erase(std::remove_if(rings.begin(), rings.end(), [](const Ring& r) { return r.life <= 0.f; }), rings.end());

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
    const float W = (float) getWidth();
    const float H = (float) getHeight();
    const float cx = W * 0.5f;
    const float t = (float) tick;
    const float sm = lvl;

    ColourGradient bg(Colour(0xffe3d3ff), 0.f, 0.f, Colour(0xfffff0d8), 0.f, H, false);
    bg.addColour(0.5, Colour(0xffffdfeb));
    g.setGradientFill(bg);
    g.fillRect(getLocalBounds());

    // golden aura grows with the sound
    {
        ColourGradient aura(Colour(0xffffe9a8).withAlpha(0.25f + 0.35f * sm), cx, 460.f,
                            Colour(0xffffe9a8).withAlpha(0.f), cx, 1000.f, true);
        g.setGradientFill(aura);
        g.fillRect(getLocalBounds());
    }

    // soft hair behind the mask
    g.setColour(Colour(0xff8a6a86).withAlpha(0.55f));
    g.fillEllipse(cx - 375.f, -30.f, 750.f, 960.f);
    for (int i = 0; i < 18; ++i)
    {
        float a = -2.9f + (float) i * 0.17f;
        float rx = cx + std::cos(a) * 375.f;
        float ry = 450.f + std::sin(a) * 480.f + std::sin(t * 0.05f + (float) i) * 3.f;
        g.fillEllipse(rx - 32.f, ry - 32.f, 64.f, 74.f);
    }
    for (int s = -1; s <= 1; s += 2)
        for (int i = 0; i < 4; ++i)
            g.fillEllipse(cx + (float) s * 355.f - 36.f, 480.f + (float) i * 110.f, 72.f, 150.f);

    // red strings
    for (int s = -1; s <= 1; s += 2)
    {
        float sw = std::sin(t * 0.04f + (float) s) * 6.f;
        Path str;
        str.startNewSubPath(cx + (float) s * 300.f, 270.f);
        str.quadraticTo(cx + (float) s * 350.f + sw, 560.f, cx + (float) s * 332.f + sw * 1.5f, 850.f);
        g.setColour(Colour(0xffd4172f).withAlpha(0.85f));
        g.strokePath(str, PathStrokeType(6.f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour(Colour(0xffff6a7f));
        g.strokePath(str, PathStrokeType(1.8f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour(kLipsP);
        g.fillEllipse(cx + (float) s * 332.f + sw * 1.5f - 8.f, 842.f, 16.f, 16.f);
    }

    // the mask face
    Path face;
    face.addEllipse(cx - 340.f, 20.f, 680.f, 890.f);
    ColourGradient fg(Colour(0xfffbfbff), cx - 100.f, 150.f, Colour(0xffc4cdea), cx + 200.f, 910.f, false);
    g.setGradientFill(fg);
    g.fillPath(face);
    ColourGradient shd(Colours::transparentBlack, cx, 460.f, Colour(0xff8a94c4).withAlpha(0.45f), cx + 340.f, 460.f, true);
    g.setGradientFill(shd);
    g.fillPath(face);
    g.setColour(Colour(0xff9aa4c4));
    g.strokePath(face, PathStrokeType(3.f));

    // high painted brows, lift when loud
    g.setColour(Colour(0xff2a1d26).withAlpha(0.8f));
    for (int s = -1; s <= 1; s += 2)
    {
        Path b;
        b.addEllipse(-34.f, -9.f, 68.f, 18.f);
        g.fillPath(b, AffineTransform::rotation((float) s * 0.2f).translated(cx + (float) s * 130.f, 190.f - sm * 14.f));
    }

    // eyes: serene closed arcs, turn into happy crescents when loud
    for (int s = -1; s <= 1; s += 2)
    {
        const float ex = cx + (float) s * 155.f;
        const float ctrl = lerpf(34.f, -40.f, jlimit(0.f, 1.f, sm * 2.f));
        g.setColour(kPinkP.withAlpha(0.28f));
        g.fillEllipse(ex - 95.f, 300.f, 190.f, 46.f);
        Path eye;
        eye.startNewSubPath(ex - 85.f, 352.f);
        eye.quadraticTo(ex, 352.f + ctrl, ex + 85.f, 352.f);
        g.setColour(Colour(0xff2a1d1a));
        g.strokePath(eye, PathStrokeType(10.f, PathStrokeType::curved, PathStrokeType::rounded));
        g.drawLine(ex + (float) s * 85.f, 352.f, ex + (float) s * 105.f, 336.f, 5.f);
    }

    // nose
    Path nose;
    nose.startNewSubPath(cx, 420.f);
    nose.quadraticTo(cx - 10.f, 520.f, cx + 14.f, 545.f);
    g.setColour(Colour(0xff6a74a8).withAlpha(0.28f));
    g.strokePath(nose, PathStrokeType(4.f, PathStrokeType::curved, PathStrokeType::rounded));

    // blush
    g.setColour(Colour(0xffff9aa6).withAlpha(0.35f + sm * 0.3f));
    g.fillEllipse(cx - 290.f, 560.f, 180.f, 110.f);
    g.fillEllipse(cx + 110.f, 560.f, 180.f, 110.f);

    // mouth: the louder, the wider the smile
    const float cyM = 770.f;
    const float halfW = 55.f + sm * 190.f;
    const float cornerY = cyM - sm * 55.f;
    const float yU = cyM - 14.f + (1.f - sm) * 6.f;
    const float yL = cyM + 26.f + sm * 110.f;
    Path lips;
    lips.startNewSubPath(cx - halfW, cornerY);
    lips.quadraticTo(cx, yU, cx + halfW, cornerY);
    lips.quadraticTo(cx, yL, cx - halfW, cornerY);
    lips.closeSubPath();
    g.setColour(kLipsP);
    g.fillPath(lips);
    if (sm > 0.12f)
    {
        const float k = 0.9f;
        Path in;
        in.startNewSubPath(cx - halfW * k, cornerY + 4.f);
        in.quadraticTo(cx, yU + 14.f, cx + halfW * k, cornerY + 4.f);
        in.quadraticTo(cx, yL - 14.f, cx - halfW * k, cornerY + 4.f);
        in.closeSubPath();
        g.setColour(Colour(0xff3a0012));
        g.fillPath(in);
        {
            Graphics::ScopedSaveState ss(g);
            g.reduceClipRegion(in, AffineTransform());
            g.setColour(Colour(0xfffbf6fa));
            const int nT = 12;
            const float tw = 2.f * halfW * k / (float) nT;
            for (int i = 0; i < nT; ++i)
                g.fillRoundedRectangle(cx - halfW * k + tw * (float) i + 2.f, yU + 4.f, tw - 4.f, 14.f + sm * 30.f, 4.f);
            g.setColour(Colour(0xffff8fb8).withAlpha(0.95f));
            g.fillEllipse(cx - 70.f * sm - 20.f, yL - 80.f, 140.f * sm + 40.f, 80.f);
        }
    }
    g.setColour(Colour(0xffffe08a).withAlpha(sm * 0.5f));
    g.strokePath(lips, PathStrokeType(14.f));
    g.setColour(Colour(0xff9a1230));
    g.strokePath(lips, PathStrokeType(4.f));
    g.setColour(Colours::white.withAlpha(0.3f));
    g.drawLine(cx - 20.f, yU + 3.f, cx + 20.f, yU + 3.f, 2.f);

    // soften the mask so the UI stays readable
    g.setColour(Colour(0xfffff5fb).withAlpha(0.22f));
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
    const float t = (float) tick;
    g.setFont(Font(FontOptions(40.f, Font::bold)));
    g.setColour(Colours::white.withAlpha(0.9f));
    g.drawText("SAKURAPARADISE", 32, 16, 600, 52, Justification::centredLeft);
    g.drawText("SAKURAPARADISE", 28, 20, 600, 52, Justification::centredLeft);
    g.setColour(kGold.withAlpha(0.8f));
    g.drawText("SAKURAPARADISE", 33, 19, 600, 52, Justification::centredLeft);
    g.setColour(kPlum);
    g.drawText("SAKURAPARADISE", 30, 18, 600, 52, Justification::centredLeft);

    // moving glint over the title
    {
        Graphics::ScopedSaveState ss(g);
        g.reduceClipRegion(Rectangle<int>(28, 18, 400, 52));
        const float gx = 20.f + std::fmod(t * 5.f, 700.f) - 120.f;
        ColourGradient g1(Colours::white.withAlpha(0.f), gx - 40.f, 0.f, Colours::white.withAlpha(0.55f), gx, 0.f, false);
        g.setGradientFill(g1);
        g.fillRect(R(gx - 40.f, 18.f, 40.f, 52.f));
        ColourGradient g2(Colours::white.withAlpha(0.55f), gx, 0.f, Colours::white.withAlpha(0.f), gx + 40.f, 0.f, false);
        g.setGradientFill(g2);
        g.fillRect(R(gx, 18.f, 40.f, 52.f));
    }

    g.setFont(Font(FontOptions(11.f)));
    g.setColour(kPlum.withAlpha(0.7f));
    g.drawText(String(L"\u685c\u5929\u56fd") + "  //  VOCAL HEAVEN PROCESSOR",
               32, 68, 400, 18, Justification::centredLeft);

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

void SakuraParadiseAudioProcessorEditor::drawIsland(Graphics& g, float x, float y, float s, float phase)
{
    const float t = (float) tick;
    y += std::sin(t * 0.03f + phase) * 5.f;
    Path rock;
    rock.startNewSubPath(x - 42.f * s, y);
    rock.lineTo(x + 42.f * s, y);
    rock.quadraticTo(x + 24.f * s, y + 40.f * s, x + 4.f * s, y + 66.f * s);
    rock.quadraticTo(x - 18.f * s, y + 36.f * s, x - 42.f * s, y);
    rock.closeSubPath();
    ColourGradient rg(Colour(0xffc9b0c8), x, y, Colour(0xff8d7a9e), x, y + 66.f * s, false);
    g.setGradientFill(rg);
    g.fillPath(rock);
    g.setColour(Colour(0xffd6f0c4));
    g.fillEllipse(x - 46.f * s, y - 9.f * s, 92.f * s, 20.f * s);
    g.setColour(Colour(0xffb9dca6));
    g.fillEllipse(x - 40.f * s, y - 4.f * s, 80.f * s, 12.f * s);
    g.setColour(Colour(0xfff2e6d8));
    g.fillRect(R(x - 9.f * s, y - 26.f * s, 18.f * s, 18.f * s));
    Path r1;
    r1.addTriangle(x - 17.f * s, y - 24.f * s, x + 17.f * s, y - 24.f * s, x, y - 36.f * s);
    g.setColour(Colour(0xffd8433a));
    g.fillPath(r1);
    g.setColour(Colour(0xfff2e6d8));
    g.fillRect(R(x - 6.f * s, y - 40.f * s, 12.f * s, 8.f * s));
    Path r2;
    r2.addTriangle(x - 13.f * s, y - 38.f * s, x + 13.f * s, y - 38.f * s, x, y - 48.f * s);
    g.setColour(Colour(0xffd8433a));
    g.fillPath(r2);
    g.setColour(kGold);
    g.fillRect(R(x - 1.f * s, y - 56.f * s, 2.f * s, 9.f * s));
    g.setColour(Colour(0xff5a3a3a));
    g.fillRect(R(x + 26.f * s, y - 20.f * s, 3.f * s, 14.f * s));
    g.setColour(Colour(0xffffb7d0));
    g.fillEllipse(x + 17.f * s, y - 34.f * s, 22.f * s, 18.f * s);
    g.setColour(Colour(0xffff93bb));
    g.fillEllipse(x + 22.f * s, y - 30.f * s, 12.f * s, 10.f * s);
    g.setColour(Colour(0xffffe9a8).withAlpha(0.25f));
    g.fillEllipse(x - 20.f * s, y + 50.f * s, 50.f * s, 14.f * s);
}

void SakuraParadiseAudioProcessorEditor::drawTorii(Graphics& g, float cx, float baseY, float h, float glowA)
{
    const float top = baseY - h;
    const float hw = h * 0.45f;
    const Colour red(0xffd8433a);

    ColourGradient gg(Colours::white.withAlpha(jlimit(0.f, 1.f, glowA)), cx, top + h * 0.5f,
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

void SakuraParadiseAudioProcessorEditor::drawHeart(Graphics& g, float x, float y, float s, Colour c, float alpha)
{
    Path heart;
    heart.startNewSubPath(0.f, s * 0.9f);
    heart.cubicTo(-s * 1.6f, -s * 0.2f, -s * 0.6f, -s * 1.3f, 0.f, -s * 0.4f);
    heart.cubicTo(s * 0.6f, -s * 1.3f, s * 1.6f, -s * 0.2f, 0.f, s * 0.9f);
    heart.closeSubPath();
    g.setColour(c.withAlpha(alpha));
    g.fillPath(heart, AffineTransform::translation(x, y));
}

void SakuraParadiseAudioProcessorEditor::drawNote(Graphics& g, float x, float y, float s, Colour c, float alpha)
{
    Path head;
    head.addEllipse(-3.2f, -2.2f, 6.4f, 4.4f);
    g.setColour(c.withAlpha(alpha));
    g.fillPath(head, AffineTransform::scale(s * 1.4f).rotated(-0.35f).translated(x, y));
    Path stem;
    stem.startNewSubPath(2.8f, -0.5f);
    stem.lineTo(2.8f, -12.f);
    stem.quadraticTo(8.f, -9.f, 6.f, -4.f);
    g.strokePath(stem, PathStrokeType(1.4f), AffineTransform::scale(s * 1.4f).translated(x, y));
}

void SakuraParadiseAudioProcessorEditor::drawSpark(Graphics& g, const Spark& p)
{
    const Colour c = kPal[p.kind];
    const float a = jlimit(0.f, 1.f, p.life * 1.4f);
    switch (p.shape)
    {
        case 0:
        {
            ColourGradient gg(c.withAlpha(a * 0.9f), p.x, p.y, c.withAlpha(0.f), p.x + p.size * 2.2f, p.y, true);
            g.setGradientFill(gg);
            g.fillEllipse(p.x - p.size * 2.2f, p.y - p.size * 2.2f, p.size * 4.4f, p.size * 4.4f);
            break;
        }
        case 1: drawStar(g, p.x, p.y, p.size, a, c); break;
        case 2: drawHeart(g, p.x, p.y, p.size, c, a); break;
        case 3:
        {
            Path f;
            f.addEllipse(-2.f, -7.f, 4.f, 14.f);
            g.setColour(Colours::white.interpolatedWith(c, 0.4f).withAlpha(a));
            g.fillPath(f, AffineTransform::scale(p.size * 0.25f).rotated(p.x * 0.05f + p.y * 0.03f).translated(p.x, p.y));
            break;
        }
        default: drawNote(g, p.x, p.y, p.size, c, a); break;
    }
}

void SakuraParadiseAudioProcessorEditor::drawAngel(Graphics& g, float x, float y, float s)
{
    {
        ColourGradient ag(Colour(0xffffe9a8).withAlpha(0.5f), x, y - 14.f * s, Colour(0xffffe9a8).withAlpha(0.f), x + 34.f * s, y - 14.f * s, true);
        g.setGradientFill(ag);
        g.fillEllipse(x - 34.f * s, y - 48.f * s, 68.f * s, 68.f * s);
    }
    Graphics::ScopedSaveState ss(g);
    g.addTransform(AffineTransform::scale(s).translated(x, y));
    const float flap = std::sin((float) tick * 0.2f) * 0.12f;
    for (int i = -1; i <= 1; i += 2)
    {
        Path w;
        w.addEllipse(-7.f, -30.f, 14.f, 34.f);
        g.setColour(Colours::white.withAlpha(0.95f));
        g.fillPath(w, AffineTransform::rotation((float) i * (0.9f + flap)).translated((float) i * 6.f, -12.f));
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
    const float mo = 1.8f + lvl * 3.f;
    g.fillEllipse(-2.f, -20.f, 4.f, mo);
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
    const float an = val(anime);
    g.setColour(Colour(0xff3d2a20).interpolatedWith(Colour(0xffff9fc4), an * 0.85f));
    g.fillEllipse(-24.f, -27.f, 48.f, 50.f);
    for (int i = 0; i < 7; ++i)
    {
        float a = -2.7f + (float) i * 0.45f;
        float sw = std::sin(t * 0.06f + (float) i) * (1.2f + kickMax() * 3.f);
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

    for (int s = -1; s <= 1; s += 2)
    {
        g.setColour(Colour(0xff2a1d1a));
        Path b;
        b.addRoundedRectangle(-3.f, -1.1f, 6.f, 2.2f, 1.1f);
        g.fillPath(b, AffineTransform::rotation((float) s * -0.15f).translated((float) s * 7.5f, -14.f));
        const float ex = (float) s * 6.5f;
        if (an < 0.12f)
        {
            Path e;
            e.startNewSubPath(ex - 4.5f, -5.f);
            e.quadraticTo(ex, -1.5f, ex + 4.5f, -5.f);
            g.strokePath(e, PathStrokeType(1.4f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        else
        {
            g.setColour(Colours::white);
            g.fillEllipse(ex - 4.5f, -9.5f, 9.f, 10.f);
            ColourGradient ig(Colour(0xffff9fd0), ex, -9.f, Colour(0xff7a4ad8), ex, 0.5f, false);
            g.setGradientFill(ig);
            g.fillEllipse(ex - 3.6f, -9.f, 7.2f, 9.2f);
            g.setColour(Colour(0xff2a1030));
            g.fillEllipse(ex - 1.8f, -6.8f, 3.6f, 4.6f);
            g.setColour(Colours::white);
            g.fillEllipse(ex - 2.9f, -8.4f, 2.6f, 2.6f);
            g.fillEllipse(ex + 0.8f, -3.6f, 1.4f, 1.4f);
            Path lash;
            lash.startNewSubPath(ex - 5.f, -6.f);
            lash.quadraticTo(ex, -11.5f, ex + 5.f, -6.f);
            g.setColour(Colour(0xff2a1d1a));
            g.strokePath(lash, PathStrokeType(1.5f, PathStrokeType::curved, PathStrokeType::rounded));
            g.drawLine(ex + (float) s * 5.f, -6.f, ex + (float) s * 7.f, -8.f, 1.2f);
        }
    }
    Path n;
    n.startNewSubPath(0.5f, -1.f);
    n.quadraticTo(-1.2f, 4.5f, 1.f, 5.2f);
    g.setColour(Colour(0xff7a82a8).withAlpha(0.6f));
    g.strokePath(n, PathStrokeType(0.7f));
    g.setColour(Colour(0xffff8f9a).withAlpha(0.5f + an * 0.3f));
    g.fillEllipse(-15.f, 3.f, 9.f, 6.f);
    g.fillEllipse(6.f, 3.f, 9.f, 6.f);
    if (an > 0.5f)
    {
        g.setColour(Colour(0xffff5f8a).withAlpha(0.8f));
        for (int i = 0; i < 3; ++i)
        {
            g.drawLine(-14.f + (float) i * 2.6f, 8.f, -13.f + (float) i * 2.6f, 5.f, 0.7f);
            g.drawLine(7.f + (float) i * 2.6f, 8.f, 8.f + (float) i * 2.6f, 5.f, 0.7f);
        }
    }

    if (lvl < 0.06f)
    {
        g.setColour(kLipsP);
        if (an > 0.3f)
        {
            Path w;
            w.startNewSubPath(-4.f, 11.5f);
            w.quadraticTo(-2.f, 15.f, 0.f, 11.5f);
            w.quadraticTo(2.f, 15.f, 4.f, 11.5f);
            g.strokePath(w, PathStrokeType(1.2f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        else
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

    if (an > 0.2f)
    {
        Path bow;
        bow.addTriangle(12.f, -24.f, 23.f, -31.f, 23.f, -17.f);
        bow.addTriangle(12.f, -24.f, 1.f, -31.f, 1.f, -17.f);
        g.setColour(Colour(0xffff6fa8));
        g.fillPath(bow);
        g.setColour(Colour(0xffd93a7a));
        g.fillEllipse(9.5f, -26.5f, 5.f, 5.f);
    }
}

void SakuraParadiseAudioProcessorEditor::drawSinger(Graphics& g, float cx, float baseY)
{
    const float t = (float) tick;
    const float vHeaven = val(heaven);
    const float wid = val(width);
    const float km = kickMax();
    const float bob = std::sin(t * 0.05f) * (4.f + lvl * 3.f) - vHeaven * 14.f;
    const float K = 1.3f * (1.f + 0.035f * km + 0.02f * lvl);

    g.setColour(Colour(0xff9a7ab8).withAlpha(0.25f));
    g.fillEllipse(cx - 55.f, baseY + 4.f, 110.f, 14.f);

    Graphics::ScopedSaveState ss(g);
    g.addTransform(AffineTransform::scale(K, K, cx, baseY).translated(0.f, bob));

    // soft glow behind the whole figure
    {
        ColourGradient sg(Colours::white.withAlpha(0.4f + lvl * 0.2f), cx, baseY - 110.f,
                          Colours::white.withAlpha(0.f), cx + 130.f, baseY - 110.f, true);
        g.setGradientFill(sg);
        g.fillEllipse(cx - 130.f, baseY - 240.f, 260.f, 260.f);
    }
    // halo glow
    {
        ColourGradient hg(Colour(0xffffe08a).withAlpha(0.35f + lvl * 0.4f), cx, baseY - 218.f,
                          Colour(0xffffe08a).withAlpha(0.f), cx + 70.f, baseY - 218.f, true);
        g.setGradientFill(hg);
        g.fillEllipse(cx - 70.f, baseY - 288.f, 140.f, 140.f);
    }

    // wings
    const float spread = 0.55f + wid * 0.75f + std::sin(t * 0.1f) * 0.03f * (1.f + lvl * 3.f)
                         + km * 0.1f * std::sin(t * 0.9f);
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
    g.setColour(Colours::white.withAlpha(0.35f));
    g.strokePath(robe, PathStrokeType(7.f));
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

    // arms: reach up to heaven as HEAVEN grows
    const Colour skin(0xffe8c4a8);
    for (int s = -1; s <= 1; s += 2)
    {
        const float fs = (float) s;
        Path arm;
        arm.startNewSubPath(cx + fs * 22.f, baseY - 150.f);
        arm.lineTo(cx + fs * 52.f, baseY - 134.f - vHeaven * 18.f);
        const float hx = cx + fs * 72.f;
        const float hy = baseY - 150.f - vHeaven * 34.f + std::sin(t * 0.07f + fs) * 4.f;
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
    const float vWid = val(width), vAnime = val(anime);
    const float floorY = sc.getBottom() - 70.f;
    const float singerY = floorY - 100.f;

    g.setColour(Colours::white.withAlpha(0.35f));
    g.fillRoundedRectangle(sc.expanded(5.f), 18.f);

    Graphics::ScopedSaveState ss(g);
    Path clip;
    clip.addRoundedRectangle(sc.getX(), sc.getY(), sc.getWidth(), sc.getHeight(), 14.f);
    g.reduceClipRegion(clip, AffineTransform());

    // sky: turns golden with WARMTH
    const Colour top = Colour(0xff9cc4ff).interpolatedWith(Colour(0xffffb0a0), vWarm * 0.8f);
    const Colour mid = Colour(0xffffd1e6).interpolatedWith(Colour(0xffffc77a), vWarm * 0.8f);
    const Colour bot = Colour(0xffffe7b8).interpolatedWith(Colour(0xffffe9a0), vWarm);
    ColourGradient sky(top, cx, sc.getY(), bot, cx, sc.getBottom(), false);
    sky.addColour(0.5, mid);
    g.setGradientFill(sky);
    g.fillRect(sc);

    // sun rises with WARMTH
    const float sy = lerpf(sc.getBottom() - 150.f, sc.getY() + 120.f, vWarm);
    const float sunR = 50.f + vWarm * 30.f + lvl * 8.f + kick[2] * 12.f;
    {
        ColourGradient glow(Colour(0xffffe9a8).withAlpha(0.4f + vWarm * 0.5f), cx, sy,
                            Colour(0xffffe9a8).withAlpha(0.f), cx, sy + sunR * 4.f, true);
        g.setGradientFill(glow);
        g.fillEllipse(cx - sunR * 4.f, sy - sunR * 4.f, sunR * 8.f, sunR * 8.f);

        const float rayA = 0.05f + vHeaven * 0.16f + lvl * 0.05f + kick[5] * 0.12f;
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

        // anamorphic streak
        Graphics::ScopedSaveState fs(g);
        g.addTransform(AffineTransform::scale(1.f, 0.07f, cx, sy));
        ColourGradient st(Colours::white.withAlpha(0.3f + vWarm * 0.4f), cx, sy, Colours::white.withAlpha(0.f), cx + 330.f, sy, true);
        g.setGradientFill(st);
        g.fillEllipse(cx - 330.f, sy - 330.f, 660.f, 660.f);
    }

    // rainbow over the gate (HEAVEN)
    {
        const float rbA = jlimit(0.f, 1.f, (vHeaven - 0.35f) * 1.3f) * 0.42f;
        if (rbA > 0.01f)
        {
            const Colour rb[6] = { Colour(0xffff6b6b), Colour(0xffffa94d), Colour(0xffffe066),
                                   Colour(0xff8ce99a), Colour(0xff74c0fc), Colour(0xffb197fc) };
            for (int i = 0; i < 6; ++i)
            {
                Path arc;
                const float r = 270.f + (float) i * 7.f;
                arc.addCentredArc(cx, floorY + 40.f, r, r, 0.f, -1.25f, 1.25f, true);
                g.setColour(rb[i].withAlpha(rbA));
                g.strokePath(arc, PathStrokeType(7.f));
            }
        }
    }

    // far clouds
    const float W = sc.getWidth() + 300.f;
    for (int i = 0; i < 4; ++i)
    {
        float x = std::fmod((float) i * 210.f + t * (0.15f + (float) i * 0.05f), W) - 150.f + sc.getX();
        drawCloud(g, x, sc.getY() + 50.f + (float) i * 38.f, 0.9f + (float) (i % 2) * 0.3f, 0.7f);
    }

    // mountains
    auto range = [&](float base, float amp, float f1, float f2, float ph, Colour col)
    {
        Path m;
        m.startNewSubPath(sc.getX(), sc.getBottom());
        for (float x = sc.getX(); x <= sc.getRight() + 12.f; x += 12.f)
        {
            float h = 0.5f + 0.3f * std::sin(x * f1 + ph) + 0.2f * std::sin(x * f2 + ph * 1.7f);
            m.lineTo(x, base - amp * h);
        }
        m.lineTo(sc.getRight() + 12.f, sc.getBottom());
        m.closeSubPath();
        g.setColour(col);
        g.fillPath(m);
    };
    range(floorY - 30.f, 150.f, 0.008f, 0.023f, 0.5f, Colour(0xffd9c6ee).withAlpha(0.85f));
    range(floorY - 10.f, 110.f, 0.011f, 0.03f, 2.0f, Colour(0xffc4aee4).withAlpha(0.92f));
    range(floorY + 10.f, 70.f, 0.015f, 0.04f, 4.0f, Colour(0xffae98d4));
    {
        ColourGradient haze(Colours::white.withAlpha(0.f), cx, floorY - 150.f, Colours::white.withAlpha(0.5f), cx, floorY + 10.f, false);
        g.setGradientFill(haze);
        g.fillRect(R(sc.getX(), floorY - 150.f, sc.getWidth(), 170.f));
    }

    // floating islands
    drawIsland(g, sc.getX() + 120.f, sc.getY() + 120.f, 1.f, 0.f);
    drawIsland(g, sc.getRight() - 120.f, sc.getY() + 85.f, 0.8f, 2.f);

    // stars + shooting star (SHIMMER)
    {
        const int count = (int) (vShim * 55.f);
        for (int i = 0; i < count; ++i)
        {
            float x = sc.getX() + std::fmod((float) i * 97.3f, sc.getWidth());
            float y = sc.getBottom() - std::fmod((float) i * 61.7f + t * (0.4f + (float) (i % 4) * 0.2f), sc.getHeight());
            float s = 2.5f + (float) (i % 3) * 1.5f + kick[6] * 3.f;
            float a = 0.4f + 0.6f * std::abs(std::sin(t * 0.1f + (float) i));
            drawStar(g, x, y, s, a, (i % 2 == 0) ? Colour(0xffffe08a) : Colours::white);
        }
        if (vShim > 0.15f)
        {
            const float sp = std::fmod(t, 110.f) / 110.f;
            if (sp < 0.3f)
            {
                const float u = sp / 0.3f;
                const float x0 = sc.getX() + sc.getWidth() * 0.15f + u * sc.getWidth() * 0.5f;
                const float y0 = sc.getY() + 20.f + u * 140.f;
                ColourGradient tail(Colours::white.withAlpha(0.f), x0 - 80.f, y0 - 45.f, Colours::white.withAlpha(0.9f * (1.f - u)), x0, y0, false);
                Path ln;
                ln.startNewSubPath(x0 - 80.f, y0 - 45.f);
                ln.lineTo(x0, y0);
                g.setGradientFill(tail);
                g.strokePath(ln, PathStrokeType(2.f));
                drawStar(g, x0, y0, 5.f, 1.f - u, Colours::white);
            }
        }
    }

    // gate of heaven + light columns
    drawTorii(g, cx, floorY - 6.f, 240.f, 0.1f + vHeaven * 0.75f + lvl * 0.15f + kick[5] * 0.3f);
    if (vHeaven > 0.05f)
        for (int i = 0; i < 5; ++i)
        {
            const float x = cx + (float) (i - 2) * 95.f + std::sin(t * 0.02f + (float) i) * 6.f;
            const float w = 26.f + (float) (i % 2) * 14.f;
            ColourGradient lc(Colours::white.withAlpha(vHeaven * 0.22f), x, sc.getY(), Colours::white.withAlpha(0.f), x, floorY, false);
            g.setGradientFill(lc);
            g.fillRect(R(x - w / 2.f, sc.getY(), w, floorY - sc.getY()));
        }

    // cherry trees
    drawBlossoms(g, sc.getX() - 10.f, sc.getBottom() - 20.f, 1.f, 210.f, 0.f);
    drawBlossoms(g, sc.getX() - 10.f, sc.getBottom() - 120.f, 1.f, 150.f, 2.f);
    drawBlossoms(g, sc.getRight() + 10.f, sc.getBottom() - 20.f, -1.f, 210.f, 1.f);
    drawBlossoms(g, sc.getRight() + 10.f, sc.getBottom() - 120.f, -1.f, 150.f, 3.f);

    // bokeh orbs
    {
        const float bk = 0.05f + vHeaven * 0.08f + vWarm * 0.07f;
        for (int i = 0; i < 12; ++i)
        {
            float x = sc.getX() + std::fmod((float) i * 157.f + t * (0.15f + (float) (i % 3) * 0.08f), sc.getWidth());
            float y = sc.getY() + 30.f + std::fmod((float) i * 89.f, sc.getHeight() - 80.f) + std::sin(t * 0.02f + (float) i) * 12.f;
            float r = 14.f + (float) ((i * 7) % 26);
            g.setColour(Colours::white.withAlpha(bk));
            g.fillEllipse(x - r, y - r, r * 2.f, r * 2.f);
            g.setColour(Colours::white.withAlpha(bk * 2.f));
            g.drawEllipse(x - r, y - r, r * 2.f, r * 2.f, 1.2f);
        }
    }
    // fireflies
    {
        const int count = 10 + (int) (vWarm * 20.f);
        for (int i = 0; i < count; ++i)
        {
            float x = cx + std::sin(t * 0.012f * (float) (1 + i % 3) + (float) i * 1.3f) * (120.f + (float) ((i * 37) % 200));
            float y = floorY - 60.f - (float) ((i * 29) % 200) + std::sin(t * 0.03f * (float) (1 + i % 2) + (float) i) * 14.f;
            float pulse = 0.4f + 0.6f * std::abs(std::sin(t * 0.08f + (float) i));
            ColourGradient fg(Colour(0xffffe08a).withAlpha(0.8f * pulse), x, y, Colour(0xffffe08a).withAlpha(0.f), x + 7.f, y, true);
            g.setGradientFill(fg);
            g.fillEllipse(x - 7.f, y - 7.f, 14.f, 14.f);
            g.setColour(Colours::white.withAlpha(pulse));
            g.fillEllipse(x - 1.2f, y - 1.2f, 2.4f, 2.4f);
        }
    }

    // silk ribbons: more of them, and they whip around while the knob moves
    {
        const int count = (int) (vSilk * 5.f + 0.99f);
        const Colour rc[5] = { Colour(0xffffb7d0), Colours::white, Colour(0xffffe08a), Colour(0xffc9b4ff), Colour(0xffff93bb) };
        const float amp = 14.f + kick[1] * 30.f + act[1] * 10.f;
        const float speed = 0.04f + kick[1] * 0.08f;
        for (int i = 0; i < count && vSilk > 0.001f; ++i)
        {
            Path rb;
            const float base = sc.getY() + 110.f + (float) i * 45.f;
            bool first = true;
            for (float x = sc.getX(); x <= sc.getRight(); x += 16.f)
            {
                float y = base + std::sin(x * 0.02f + t * speed + (float) i) * amp;
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

    // aura behind the singer grows with WIDTH
    {
        const float auraW = 90.f + vWid * 270.f + kick[7] * 60.f;
        Graphics::ScopedSaveState as(g);
        g.addTransform(AffineTransform::scale(1.f, 0.75f, cx, singerY - 40.f));
        ColourGradient au(Colour(0xffffd9ee).withAlpha(0.5f), cx, singerY - 40.f, Colour(0xffffd9ee).withAlpha(0.f), cx + auraW, singerY - 40.f, true);
        g.setGradientFill(au);
        g.fillEllipse(cx - auraW, singerY - 40.f - auraW, auraW * 2.f, auraW * 2.f);
    }

    // shockwave rings when any knob is moved
    for (auto& r : rings)
    {
        g.setColour(kPal[r.kind].withAlpha(r.life * 0.7f));
        g.drawEllipse(r.x - r.r, r.y - r.r, r.r * 2.f, r.r * 2.f, 2.5f);
    }

    // choir: angels orbit the singer
    const int angels = (int) (vChoir * 6.f + 0.5f);
    auto drawAngels = [&](bool front)
    {
        for (int i = 0; i < angels; ++i)
        {
            const float ang = t * 0.012f + (float) i * twoPi / (float) angels;
            const bool isFront = std::sin(ang) > 0.f;
            if (isFront != front) continue;
            const float x = cx + std::cos(ang) * (200.f + kick[4] * 20.f);
            const float y = sc.getY() + 170.f + std::sin(ang) * 55.f + std::sin(t * 0.07f + (float) i) * 4.f;
            const float depth = 0.75f + 0.25f * std::sin(ang);
            drawAngel(g, x, y, 0.95f * depth);
        }
    };
    drawAngels(false);

    drawSinger(g, cx, floorY + 18.f);

    // anime hearts around the singer
    {
        const int count = (int) (vAnime * 10.f);
        for (int i = 0; i < count; ++i)
        {
            const float prog = std::fmod((float) i * 37.f + t * (0.8f + (float) (i % 3) * 0.3f), 260.f);
            const float hx = cx + std::sin((float) i * 1.7f) * 95.f + std::sin(t * 0.05f + (float) i) * 8.f;
            drawHeart(g, hx, floorY - prog, 5.f + (float) (i % 3) * 2.f,
                      (i % 2 == 0) ? Colour(0xffff6fa8) : Colours::white, jlimit(0.f, 0.9f, 1.f - prog / 260.f + 0.2f));
        }
    }

    for (auto& p : sparks) drawSpark(g, p);
    drawAngels(true);

    // doves + wind streaks (AIR)
    {
        const int count = (int) (vAir * 6.f + 0.99f);
        const float spd = 1.2f + vAir * 2.5f + kick[3] * 4.f;
        for (int i = 0; i < count && vAir > 0.001f; ++i)
        {
            float x = sc.getX() + std::fmod(t * spd * (1.f + 0.25f * (float) i) + (float) i * 140.f, sc.getWidth() + 120.f) - 60.f;
            float y = sc.getY() + 70.f + (float) i * 38.f + std::sin(t * 0.07f + (float) i) * 10.f;
            drawDove(g, x, y, std::sin(t * (0.3f + kick[3]) + (float) i) * 7.f, 1.1f);
        }
        const int streaks = (int) (vAir * 10.f);
        for (int i = 0; i < streaks; ++i)
        {
            const float len = 40.f + (float) (i % 3) * 20.f;
            const float x = sc.getX() + std::fmod(t * (4.f + (float) (i % 3) * 2.f) + (float) i * 97.f, sc.getWidth() + 100.f) - 50.f;
            const float y = sc.getY() + 40.f + (float) ((i * 37) % 250);
            Path w;
            w.startNewSubPath(x, y);
            w.quadraticTo(x - len * 0.5f, y + std::sin(t * 0.1f + (float) i) * 5.f, x - len, y);
            g.setColour(Colours::white.withAlpha(0.35f));
            g.strokePath(w, PathStrokeType(1.6f, PathStrokeType::curved, PathStrokeType::rounded));
        }
    }

    // fog banks + dust (disappear with CLEAN)
    {
        const float fogA = 0.05f + (1.f - vClean) * 0.22f;
        for (int i = 0; i < 4; ++i)
        {
            float x = sc.getX() + std::fmod((float) i * 190.f + t * (0.2f + (float) i * 0.05f) - vClean * 200.f + 1000.f, sc.getWidth() + 300.f) - 150.f;
            float y = floorY - 20.f + (float) (i % 2) * 20.f + std::sin(t * 0.02f + (float) i) * 6.f;
            g.setColour(Colour(0xfff5eefc).withAlpha(fogA));
            g.fillEllipse(x, y, 320.f, 70.f);
            g.fillEllipse(x + 60.f, y - 20.f, 200.f, 50.f);
        }
        const int dust = (int) ((1.f - vClean) * 45.f);
        g.setColour(Colour(0xff8a8398).withAlpha(0.35f));
        for (int i = 0; i < dust; ++i)
        {
            float x = sc.getX() + std::fmod((float) i * 83.7f + t * (0.2f + (float) (i % 3) * 0.15f), sc.getWidth());
            float y = sc.getY() + std::fmod((float) i * 47.3f + t * 0.1f * (float) (1 + i % 2), sc.getHeight());
            float s = 1.5f + (float) (i % 3);
            g.fillEllipse(x, y, s, s);
        }
        g.setColour(Colour(0xffb8b2c4).withAlpha((1.f - vClean) * 0.14f));
        g.fillRect(sc);
    }

    // CLEAN: a bright sweep follows the knob
    {
        const float a = kick[0] * 0.45f + act[0] * 0.2f;
        if (a > 0.01f)
        {
            const float x = sc.getX() + vClean * sc.getWidth();
            ColourGradient s1(Colours::white.withAlpha(0.f), x - 50.f, 0.f, Colours::white.withAlpha(a), x, 0.f, false);
            g.setGradientFill(s1);
            g.fillRect(R(x - 50.f, sc.getY(), 50.f, sc.getHeight()));
            ColourGradient s2(Colours::white.withAlpha(a), x, 0.f, Colours::white.withAlpha(0.f), x + 50.f, 0.f, false);
            g.setGradientFill(s2);
            g.fillRect(R(x, sc.getY(), 50.f, sc.getHeight()));
        }
    }

    // colour grade + flashes
    g.setColour(Colour(0xffffb347).withAlpha(vWarm * 0.12f + kick[2] * 0.15f));
    g.fillRect(sc);
    g.setColour(Colours::white.withAlpha(kick[5] * 0.22f));
    g.fillRect(sc);
    {
        ColourGradient vg(Colours::transparentBlack, cx, sc.getCentreY(), Colour(0xff5a3a6a).withAlpha(0.3f),
                          sc.getX(), sc.getY(), true);
        g.setGradientFill(vg);
        g.fillRect(sc);
    }

    g.setColour(Colours::white.withAlpha(0.8f));
    g.drawRoundedRectangle(sc, 14.f, 2.f);
}

void SakuraParadiseAudioProcessorEditor::drawDeck(Graphics& g)
{
    const auto deck = R(20.f, 518.f, 720.f, 395.f);
    g.setColour(Colours::white.withAlpha(0.14f));
    g.fillRoundedRectangle(deck, 16.f);
    g.setColour(kGold.withAlpha(0.7f));
    g.drawRoundedRectangle(deck, 16.f, 2.f);

    auto plateGlow = [&](Rectangle<float> plate, int idx)
    {
        g.setColour(Colours::white.withAlpha(0.2f));
        g.fillRoundedRectangle(plate, 12.f);
        const float gl = jlimit(0.f, 1.f, kick[idx] + act[idx] * 0.6f);
        if (gl > 0.02f)
        {
            g.setColour(kPal[idx].withAlpha(gl * 0.18f));
            g.fillRoundedRectangle(plate, 12.f);
            g.setColour(kPal[idx].withAlpha(gl * 0.7f));
            g.drawRoundedRectangle(plate.expanded(2.f), 14.f, 3.5f);
        }
        g.setColour(Colour(0xffc9b4dc).withAlpha(0.8f));
        g.drawRoundedRectangle(plate, 12.f, 1.5f);
    };

    const float xs[4] = { 48.f, 224.f, 400.f, 576.f };
    for (int row = 0; row < 2; ++row)
        for (int i = 0; i < 4; ++i)
            plateGlow(R(xs[i] - 10.f, 530.f + (float) row * 184.f, 150.f, 184.f), row * 4 + i);

    plateGlow(R(525.f, 4.f, 110.f, 104.f), 8);
    auto op = R(645.f, 4.f, 110.f, 104.f);
    g.setColour(Colours::white.withAlpha(0.3f));
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
