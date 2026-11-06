#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class PearlLook : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float a0, float a1, juce::Slider&) override;
};

class SakuraParadiseAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SakuraParadiseAudioProcessorEditor(SakuraParadiseAudioProcessor&);
    ~SakuraParadiseAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider s;
        juce::Label l;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    struct Petal { float x, y, vx, vy, rot, vr, size; bool white; };
    struct Spark { float x, y, vx, vy, life, size; int kind, shape; };
    struct Ring { float x, y, r, life; int kind; };

    void setupKnob(Knob&, const juce::String& id, const juce::String& text, const juce::String& suffix);
    void timerCallback() override;
    Knob& knobAt(int i);
    float val(const Knob& k) const { return (float) k.s.getValue(); }
    float kickMax() const;
    void spawnSpark(int kind, int shape, float x, float y, float vx, float vy, float jitter);
    juce::Rectangle<float> sceneRect() const { return { 30.f, 112.f, 700.f, 390.f }; }

    void drawBackground(juce::Graphics&);
    void drawPetals(juce::Graphics&);
    void drawHeader(juce::Graphics&);
    void drawScene(juce::Graphics&);
    void drawDeck(juce::Graphics&);
    void drawCloud(juce::Graphics&, float x, float y, float s, float a);
    void drawTorii(juce::Graphics&, float cx, float baseY, float h, float glowA);
    void drawBlossoms(juce::Graphics&, float x0, float y0, float dir, float len, float phase);
    void drawDove(juce::Graphics&, float x, float y, float flap, float s);
    void drawStar(juce::Graphics&, float x, float y, float s, float alpha, juce::Colour c);
    void drawHeart(juce::Graphics&, float x, float y, float s, juce::Colour c, float alpha);
    void drawNote(juce::Graphics&, float x, float y, float s, juce::Colour c, float alpha);
    void drawSpark(juce::Graphics&, const Spark&);
    void drawIsland(juce::Graphics&, float x, float y, float s, float phase);
    void drawAngel(juce::Graphics&, float x, float y, float s);
    void drawSinger(juce::Graphics&, float cx, float baseY);
    void drawSingerFace(juce::Graphics&);

    SakuraParadiseAudioProcessor& proc;
    PearlLook look;
    Knob clean, silk, warmth, air, choir, heaven, shimmer, width, anime, output;
    std::vector<Petal> petals;
    std::vector<Spark> sparks;
    std::vector<Ring> rings;
    juce::Random rnd;
    int tick = 0;
    float lvl = 0.f;
    float kick[9] = {};
    float prevVal[9] = {};
    float act[9] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SakuraParadiseAudioProcessorEditor)
};
