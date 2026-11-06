#pragma once
#include <JuceHeader.h>

// RBJ biquad
struct Biquad
{
    float b0 = 1.f, b1 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f, z1 = 0.f, z2 = 0.f;

    float process(float x)
    {
        float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void set(float nb0, float nb1, float nb2, float a0, float na1, float na2)
    {
        b0 = nb0 / a0; b1 = nb1 / a0; b2 = nb2 / a0; a1 = na1 / a0; a2 = na2 / a0;
    }
    void highPass(double sr, float f, float q)
    {
        float w0 = juce::MathConstants<float>::twoPi * f / (float) sr;
        float cs = std::cos(w0), sn = std::sin(w0), al = sn / (2.f * q);
        set((1.f + cs) * 0.5f, -(1.f + cs), (1.f + cs) * 0.5f, 1.f + al, -2.f * cs, 1.f - al);
    }
    void lowShelf(double sr, float f, float gainDb)
    {
        float A = std::pow(10.f, gainDb / 40.f);
        float w0 = juce::MathConstants<float>::twoPi * f / (float) sr;
        float cs = std::cos(w0), sn = std::sin(w0);
        float al = sn * 0.5f * std::sqrt(2.f);
        float sA = 2.f * std::sqrt(A) * al;
        set(A * ((A + 1.f) - (A - 1.f) * cs + sA),
            2.f * A * ((A - 1.f) - (A + 1.f) * cs),
            A * ((A + 1.f) - (A - 1.f) * cs - sA),
            (A + 1.f) + (A - 1.f) * cs + sA,
            -2.f * ((A - 1.f) + (A + 1.f) * cs),
            (A + 1.f) + (A - 1.f) * cs - sA);
    }
    void highShelf(double sr, float f, float gainDb)
    {
        float A = std::pow(10.f, gainDb / 40.f);
        float w0 = juce::MathConstants<float>::twoPi * f / (float) sr;
        float cs = std::cos(w0), sn = std::sin(w0);
        float al = sn * 0.5f * std::sqrt(2.f);
        float sA = 2.f * std::sqrt(A) * al;
        set(A * ((A + 1.f) + (A - 1.f) * cs + sA),
            -2.f * A * ((A - 1.f) + (A + 1.f) * cs),
            A * ((A + 1.f) + (A - 1.f) * cs - sA),
            (A + 1.f) - (A - 1.f) * cs + sA,
            2.f * ((A - 1.f) - (A + 1.f) * cs),
            (A + 1.f) - (A - 1.f) * cs - sA);
    }
};

// Two-tap granular pitch shifter (used for the shimmer octave)
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
        return buf[(size_t) (i0 % size)] * (1.f - fr) + buf[(size_t) i1] * fr;
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

class SakuraParadiseAudioProcessor : public juce::AudioProcessor
{
public:
    SakuraParadiseAudioProcessor();
    ~SakuraParadiseAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SakuraParadise"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> level { 0.f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float readChorus(float delaySamples) const;

    static constexpr int bufSize = 16384;
    static constexpr int bufMask = bufSize - 1;

    double sr = 44100.0;
    Biquad hpf[2], lowS[2], highS[2], sibHp[2];
    float env = 0.f, gate = 1.f, sibEnv = 0.f;
    float envAtk = 0.f, envRel = 0.f, gateAtk = 0.f, gateRel = 0.f, sibAtk = 0.f, sibRel = 0.f;

    std::vector<float> chBuf, pdBuf;
    int chW = 0, pdW = 0, pdSamples = 1000;
    float lfoPh[3] { 0.f, 2.1f, 4.2f };
    float lfoInc[3] { 0.f, 0.f, 0.f };

    juce::Reverb reverb;
    PitchShifter shifter;
    float shFb = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SakuraParadiseAudioProcessor)
};
