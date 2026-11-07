#pragma once
#include <JuceHeader.h>
#include "Tuner.h"

// Simple two-tap granular pitch shifter
struct PitchShifter
{
    static constexpr int size = 16384;
    std::vector<float> buf;
    int w = 0;
    float phase = 0.f;
    float grain = 2400.f;

    void prepare() { buf.assign(size, 0.f); w = 0; phase = 0.f; }

    float read(float delay) const
    {
        float rp = (float) w - delay;
        while (rp < 0) rp += (float) size;
        int i0 = (int) rp;
        float fr = rp - (float) i0;
        int i1 = (i0 + 1) % size;
        return buf[(size_t) i0 % size] * (1.f - fr) + buf[(size_t) i1] * fr;
    }

    float process(float in, float ratio)
    {
        buf[(size_t) w] = in;
        phase += (1.f - ratio) / grain;
        phase -= std::floor(phase);
        float p2 = phase + 0.5f; p2 -= std::floor(p2);
        float w1 = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * phase);
        float w2 = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * p2);
        float out = read(phase * grain + 1.f) * w1 + read(p2 * grain + 1.f) * w2;
        w = (w + 1) % size;
        return out;
    }
};

class SakuraHellAudioProcessor : public juce::AudioProcessor
{
public:
    SakuraHellAudioProcessor();
    ~SakuraHellAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SakuraHell"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> level { 0.f };   // read by the editor for the mask smile
    std::atomic<float> detHz { 0.f };   // detected voice pitch (0 = unvoiced)
    std::atomic<int> targetNote { -1 }; // MIDI note the tuner snaps to (-1 = none)
    std::atomic<float> tuneCorr { 0.f };// current correction in semitones

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    PitchShifter shifter[2];
    float held[2] { 0.f, 0.f };
    int holdCount[2] { 0, 0 };
    float ringPhase = 0.f;
    double sr = 44100.0;
    juce::Reverb reverb;

    // autotune
    static constexpr int decN = 512;
    float dec[decN] {};
    int decW = 0, decCount = 0, hopCount = 0, decFactor = 4, unvoicedCount = 0;
    float decAcc = 0.f, corrTarget = 0.f, cs = 0.f, grainTarget = 960.f, tuneGrain = 960.f;
    unsigned noteMask = 0xFFFu;
    PitchShifter tuneShifter[2];
    std::vector<float> ratioBuf, grainBuf;
    std::atomic<float>* noteParams[12] {};
    void pushDetect(float mono);
    void detect();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SakuraHellAudioProcessor)
};
