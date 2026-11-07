#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout SakuraHellAudioProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using R = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add(std::make_unique<P>(juce::ParameterID{ "pitch", 1 },  "Pitch",  R(-24.f, 12.f, 0.01f), 0.f));
    l.add(std::make_unique<P>(juce::ParameterID{ "crush", 1 },  "Crush",  R(0.f, 1.f, 0.001f), 0.f));
    l.add(std::make_unique<P>(juce::ParameterID{ "hell", 1 },   "Hell",   R(0.f, 1.f, 0.001f), 0.f));
    l.add(std::make_unique<P>(juce::ParameterID{ "doom", 1 },   "Doom",   R(0.f, 1.f, 0.001f), 0.f));
    l.add(std::make_unique<P>(juce::ParameterID{ "output", 1 }, "Output", R(-24.f, 6.f, 0.1f), 0.f));
    l.add(std::make_unique<P>(juce::ParameterID{ "tunespeed", 1 }, "Tune Speed", R(0.f, 1.f, 0.001f), 0.f));
    l.add(std::make_unique<P>(juce::ParameterID{ "tuneamt", 1 }, "Tune Amount", R(0.f, 1.f, 0.001f), 0.f));
    static const char* noteIds[12] = { "note0", "note1", "note2", "note3", "note4", "note5", "note6", "note7", "note8", "note9", "note10", "note11" };
    static const char* noteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const bool major[12] = { true, false, true, false, true, true, false, true, false, true, false, true };
    for (int i = 0; i < 12; ++i)
        l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ noteIds[i], 1 }, noteNames[i], major[i]));
    return l;
}

SakuraHellAudioProcessor::SakuraHellAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < 12; ++i)
        noteParams[i] = apvts.getRawParameterValue("note" + juce::String(i));
}

bool SakuraHellAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == l.getMainInputChannelSet()
        && (l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
         || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono());
}

void SakuraHellAudioProcessor::prepareToPlay(double sampleRate, int)
{
    sr = sampleRate;
    for (auto& s : shifter) { s.prepare(); s.grain = (float) (sr * 0.055); }
    reverb.reset();
    reverb.setSampleRate(sr);

    decFactor = juce::jmax(1, (int) std::lround(sr / 12000.0));
    for (auto& d : dec) d = 0.f;
    decW = decCount = hopCount = unvoicedCount = 0;
    decAcc = corrTarget = cs = 0.f;
    grainTarget = tuneGrain = (float) (sr * 0.02);
    for (auto& s : tuneShifter) { s.prepare(); s.grain = tuneGrain; }
    ratioBuf.assign(8192, 1.f);
    grainBuf.assign(8192, tuneGrain);
}

void SakuraHellAudioProcessor::pushDetect(float mono)
{
    decAcc += mono;
    if (++decCount >= decFactor)
    {
        dec[decW] = decAcc / (float) decFactor;
        decAcc = 0.f;
        decCount = 0;
        decW = (decW + 1) & (decN - 1);
        if (++hopCount >= 128) { hopCount = 0; detect(); }
    }
}

void SakuraHellAudioProcessor::detect()
{
    float x[decN];
    for (int i = 0; i < decN; ++i) x[i] = dec[(decW + i) & (decN - 1)];
    const auto r = yinDetect(x, decN, (float) (sr / (double) decFactor));
    if (r.voiced)
    {
        unvoicedCount = 0;
        const float midi = hzToMidi(r.f0);
        const int target = snapToScale(midi, noteMask);
        corrTarget = (float) target - midi;
        targetNote.store(target);
        detHz.store(r.f0);
        const float period = (float) sr / r.f0;
        const float g = juce::jlimit((float) (0.006 * sr), (float) (0.04 * sr), 2.f * period);
        if (std::abs(g - grainTarget) / grainTarget > 0.2f) grainTarget = g;
    }
    else if (++unvoicedCount > 3)
    {
        corrTarget = 0.f;
        targetNote.store(-1);
        detHz.store(0.f);
    }
}

void SakuraHellAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int nCh = juce::jmin(2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();

    const float pitchSt = apvts.getRawParameterValue("pitch")->load();
    const float crush   = apvts.getRawParameterValue("crush")->load();
    const float hell    = apvts.getRawParameterValue("hell")->load();
    const float doom    = apvts.getRawParameterValue("doom")->load();
    const float outDb   = apvts.getRawParameterValue("output")->load();

    const float ratio  = std::pow(2.f, pitchSt / 12.f);
    const float levels = std::pow(2.f, (16.f - crush * 14.f) - 1.f);
    const int   hold   = 1 + (int) (crush * 40.f);
    const float drive  = 1.f + hell * 40.f;
    const float ringMix = hell * 0.6f;
    const float ringInc = juce::MathConstants<float>::twoPi * (70.f + hell * 200.f) / (float) sr;
    const float outGain = juce::Decibels::decibelsToGain(outDb);
    const bool pitchOn = std::abs(pitchSt) > 0.01f;

    // ---- autotune: detect on the dry input, shared correction for both channels
    const float tuneSpeed = apvts.getRawParameterValue("tunespeed")->load();
    const float tuneAmt   = apvts.getRawParameterValue("tuneamt")->load();
    const bool tuneOn = tuneAmt > 0.001f;
    unsigned mask = 0;
    for (int i = 0; i < 12; ++i) if (noteParams[i] != nullptr && noteParams[i]->load() > 0.5f) mask |= (1u << i);
    noteMask = mask != 0u ? mask : 0xFFFu;
    if ((int) ratioBuf.size() < n) { ratioBuf.assign((size_t) n, 1.f); grainBuf.assign((size_t) n, tuneGrain); }
    {
        const float tau = 0.4f * std::exp(std::log(0.0125f) * tuneSpeed);
        const float aSmp = 1.f - std::exp(-1.f / (tau * (float) sr));
        const float* inL = buffer.getReadPointer(0);
        const float* inR = buffer.getReadPointer(nCh > 1 ? 1 : 0);
        for (int i = 0; i < n; ++i)
        {
            pushDetect(0.5f * (inL[i] + inR[i]));
            cs += (corrTarget * tuneAmt - cs) * aSmp;
            ratioBuf[(size_t) i] = std::pow(2.f, cs / 12.f);
            tuneGrain += (grainTarget - tuneGrain) * 0.00005f;
            grainBuf[(size_t) i] = tuneGrain;
        }
        tuneCorr.store(cs);
    }

    float phaseEnd = ringPhase;
    for (int ch = 0; ch < nCh; ++ch)
    {
        auto* d = buffer.getWritePointer(ch);
        float ph = ringPhase;
        for (int i = 0; i < n; ++i)
        {
            float x = d[i];
            tuneShifter[ch].grain = grainBuf[(size_t) i];
            const float tuned = tuneShifter[ch].process(x, tuneOn ? ratioBuf[(size_t) i] : 1.f);
            if (tuneOn) x = tuned;
            float shifted = shifter[ch].process(x, pitchOn ? ratio : 1.f);
            x = pitchOn ? shifted : x;

            if (holdCount[ch] <= 0) { held[ch] = std::round(x * levels) / levels; holdCount[ch] = hold; }
            --holdCount[ch];
            x = held[ch];

            float dist = std::tanh(x * drive);
            float rm = dist * std::sin(ph);
            x = (dist * (1.f - ringMix) + rm * ringMix) / (1.f + hell * 0.3f);
            ph += ringInc;
            if (ph > juce::MathConstants<float>::twoPi) ph -= juce::MathConstants<float>::twoPi;

            d[i] = x;
        }
        phaseEnd = ph;
    }
    ringPhase = phaseEnd;

    juce::Reverb::Parameters rp;
    rp.roomSize = 0.6f + doom * 0.4f;
    rp.damping = 0.25f;
    rp.wetLevel = doom * 0.55f;
    rp.dryLevel = 1.f - doom * 0.35f;
    rp.width = 1.f;
    reverb.setParameters(rp);
    if (nCh == 2) reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), n);
    else if (nCh == 1) reverb.processMono(buffer.getWritePointer(0), n);

    buffer.applyGain(outGain);

    float peak = 0.f;
    for (int ch = 0; ch < nCh; ++ch) peak = juce::jmax(peak, buffer.getMagnitude(ch, 0, n));
    level.store(juce::jmax(peak, level.load() * 0.9f));
}

juce::AudioProcessorEditor* SakuraHellAudioProcessor::createEditor()
{
    return new SakuraHellAudioProcessorEditor(*this);
}

void SakuraHellAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}

void SakuraHellAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SakuraHellAudioProcessor();
}
