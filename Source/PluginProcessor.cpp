#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout SakuraParadiseAudioProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using R = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    auto add = [&](const char* id, const char* name, float def)
    {
        l.add(std::make_unique<P>(juce::ParameterID{ id, 1 }, name, R(0.f, 1.f, 0.001f), def));
    };
    add("clean", "Clean", 0.f);
    add("silk", "Silk", 0.f);
    add("warmth", "Warmth", 0.f);
    add("air", "Air", 0.f);
    add("choir", "Choir", 0.f);
    add("heaven", "Heaven", 0.3f);
    add("shimmer", "Shimmer", 0.f);
    add("width", "Width", 0.5f);
    add("anime", "Anime", 0.f);
    l.add(std::make_unique<P>(juce::ParameterID{ "output", 1 }, "Output", R(-24.f, 6.f, 0.1f), 0.f));
    return l;
}

SakuraParadiseAudioProcessor::SakuraParadiseAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
}

bool SakuraParadiseAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void SakuraParadiseAudioProcessor::prepareToPlay(double sampleRate, int)
{
    sr = sampleRate;
    chBuf.assign((size_t) bufSize, 0.f);
    pdBuf.assign((size_t) bufSize, 0.f);
    chW = pdW = 0;
    pdSamples = juce::jlimit(1, bufSize - 2, (int) (0.028 * sr));
    shifter.prepare();
    shifter.grain = (float) (sr * 0.06);
    shFb = 0.f;
    for (int c = 0; c < 2; ++c) { voice[c].prepare(); voice[c].grain = (float) (sr * 0.035); animeShelf[c] = Biquad(); }
    vibPh = 0.f;
    vibInc = juce::MathConstants<float>::twoPi * 5.5f / (float) sr;
    env = 0.f; gate = 1.f; sibEnv = 0.f;

    auto coef = [this](double seconds) { return 1.f - std::exp(-1.f / (float) (seconds * sr)); };
    envAtk = coef(0.003);  envRel = coef(0.12);
    gateAtk = coef(0.01);  gateRel = coef(0.08);
    sibAtk = coef(0.001);  sibRel = coef(0.03);

    const float rates[3] = { 0.23f, 0.31f, 0.41f };
    for (int i = 0; i < 3; ++i)
        lfoInc[i] = juce::MathConstants<float>::twoPi * rates[i] / (float) sr;

    for (int c = 0; c < 2; ++c) { hpf[c] = Biquad(); lowS[c] = Biquad(); highS[c] = Biquad(); sibHp[c] = Biquad(); }
    reverb.reset();
    reverb.setSampleRate(sr);
}

float SakuraParadiseAudioProcessor::readChorus(float d) const
{
    int di = (int) d;
    float fr = d - (float) di;
    float s0 = chBuf[(size_t) ((chW - di) & bufMask)];
    float s1 = chBuf[(size_t) ((chW - di - 1) & bufMask)];
    return s0 * (1.f - fr) + s1 * fr;
}

void SakuraParadiseAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() < 2) return;
    const int n = buffer.getNumSamples();

    auto P = [this](const char* id) { return apvts.getRawParameterValue(id)->load(); };
    const float clean = P("clean"), silk = P("silk"), warmth = P("warmth"), air = P("air");
    const float choir = P("choir"), heaven = P("heaven"), shimmer = P("shimmer"), width = P("width");
    const float anime = P("anime");
    const float animeRatio = std::pow(2.f, anime * 7.f / 12.f);
    const float outGain = juce::Decibels::decibelsToGain(P("output"));

    for (int c = 0; c < 2; ++c)
    {
        hpf[c].highPass(sr, 40.f + clean * 100.f, 0.707f);
        lowS[c].lowShelf(sr, 180.f, warmth * 4.f);
        highS[c].highShelf(sr, 9500.f, air * 9.f);
        sibHp[c].highPass(sr, 6500.f, 0.707f);
        animeShelf[c].highShelf(sr, 3200.f, anime * 5.f);
    }

    juce::Reverb::Parameters rp;
    rp.roomSize = 0.82f + 0.17f * heaven;
    rp.damping = 0.3f;
    rp.wetLevel = 0.33f;
    rp.dryLevel = 0.f;
    rp.width = 1.f;
    reverb.setParameters(rp);

    const float thrDb = -78.f + clean * 42.f;
    const float maxRed = 6.f + clean * 30.f;
    const float twoPi = juce::MathConstants<float>::twoPi;
    const float baseDelay[3] = { 0.014f, 0.019f, 0.024f };

    float* L = buffer.getWritePointer(0);
    float* R = buffer.getWritePointer(1);
    float peak = 0.f;

    for (int i = 0; i < n; ++i)
    {
        float l = hpf[0].process(L[i]);
        float r = hpf[1].process(R[i]);

        // noise reduction (downward expander)
        const float det = juce::jmax(std::abs(l), std::abs(r));
        env += (det - env) * (det > env ? envAtk : envRel);
        float targetDb = 0.f;
        if (clean > 0.001f)
        {
            const float envDb = juce::Decibels::gainToDecibels(env + 1.0e-6f);
            if (envDb < thrDb) targetDb = juce::jmax(-maxRed, (envDb - thrDb) * 2.f);
        }
        const float tg = juce::Decibels::decibelsToGain(targetDb);
        gate += (tg - gate) * (tg < gate ? gateAtk : gateRel);
        l *= gate; r *= gate;

        // de-esser
        const float sl = sibHp[0].process(l);
        const float sr2 = sibHp[1].process(r);
        const float sd = juce::jmax(std::abs(sl), std::abs(sr2));
        sibEnv += (sd - sibEnv) * (sd > sibEnv ? sibAtk : sibRel);
        float gs = 1.f;
        if (silk > 0.001f && sibEnv > 0.04f) gs = std::pow(0.04f / sibEnv, silk * 0.9f);
        l -= sl * (1.f - gs);
        r -= sr2 * (1.f - gs);

        // anime voice: pitch/formant lift + light vibrato + brightness
        vibPh += vibInc;
        if (vibPh > twoPi) vibPh -= twoPi;
        {
            const float ratio = animeRatio * (1.f + 0.005f * anime * std::sin(vibPh));
            float al = animeShelf[0].process(voice[0].process(l, ratio));
            float ar = animeShelf[1].process(voice[1].process(r, ratio));
            if (anime > 0.001f)
            {
                const float mixA = juce::jmin(1.f, anime * 20.f);
                l = l * (1.f - mixA) + al * mixA;
                r = r * (1.f - mixA) + ar * mixA;
            }
        }

        // warmth + air
        l = highS[0].process(lowS[0].process(l));
        r = highS[1].process(lowS[1].process(r));
        if (warmth > 0.001f)
        {
            const float k = warmth * 0.5f;
            l = l * (1.f - k) + std::tanh(l * 2.f) * 0.5f * k;
            r = r * (1.f - k) + std::tanh(r * 2.f) * 0.5f * k;
        }

        // choir (3-voice ensemble)
        const float mid = 0.5f * (l + r);
        chBuf[(size_t) chW] = mid;
        float tap[3] = { 0.f, 0.f, 0.f };
        for (int v = 0; v < 3; ++v)
        {
            lfoPh[v] += lfoInc[v];
            if (lfoPh[v] > twoPi) lfoPh[v] -= twoPi;
            if (choir > 0.001f)
            {
                float d = (baseDelay[v] + std::sin(lfoPh[v]) * 0.003f * (0.4f + choir)) * (float) sr;
                tap[v] = readChorus(d);
            }
        }
        const float cl = tap[0] * 0.7f + tap[2] * 0.3f;
        const float cr = tap[1] * 0.7f + tap[2] * 0.3f;
        chW = (chW + 1) & bufMask;
        l += cl * choir * 0.6f;
        r += cr * choir * 0.6f;

        // heaven reverb with shimmer
        pdBuf[(size_t) pdW] = 0.5f * (l + r);
        const float delayed = pdBuf[(size_t) ((pdW - pdSamples) & bufMask)];
        pdW = (pdW + 1) & bufMask;
        float wl = delayed + shFb, wr = delayed + shFb;
        reverb.processStereo(&wl, &wr, 1);
        const float sh = shifter.process(0.5f * (wl + wr), 2.f);
        shFb = std::tanh(sh * shimmer * 0.6f);
        l += (wl + sh * shimmer * 0.3f) * heaven * 1.1f;
        r += (wr + sh * shimmer * 0.3f) * heaven * 1.1f;

        // stereo width
        const float m = 0.5f * (l + r);
        const float s = 0.5f * (l - r) * (width * 2.f);
        l = m + s;
        r = m - s;

        l *= outGain; r *= outGain;
        L[i] = l; R[i] = r;
        peak = juce::jmax(peak, juce::jmax(std::abs(l), std::abs(r)));
    }
    level.store(juce::jmax(peak, level.load() * 0.9f));
}

juce::AudioProcessorEditor* SakuraParadiseAudioProcessor::createEditor()
{
    return new SakuraParadiseAudioProcessorEditor(*this);
}

void SakuraParadiseAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}

void SakuraParadiseAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SakuraParadiseAudioProcessor();
}
