#pragma once
// Pure C++ pitch detection (YIN) and scale snapping. No JUCE dependency.
#include <cmath>
#include <algorithm>

struct YinResult
{
    bool voiced = false;
    float f0 = 0.f;
    float conf = 0.f;
};

// x: N decimated samples (N <= 520), oldest first. srD: sample rate of x.
inline YinResult yinDetect(const float* x, int N, float srD)
{
    YinResult r;
    const int Wh = N / 2;
    float e = 0.f;
    for (int j = 0; j < N; ++j) e += x[j] * x[j];
    e /= (float) N;
    if (std::sqrt(e) < 0.004f) return r;

    const int minTau = std::max(2, (int) (srD / 1000.f));
    const int maxTau = std::min((int) (srD / 70.f), Wh - 2);
    if (maxTau <= minTau + 2) return r;

    float cm[264];
    cm[0] = 1.f;
    float run = 0.f;
    for (int tau = 1; tau <= maxTau + 1; ++tau)
    {
        float d = 0.f;
        for (int j = 0; j < Wh; ++j)
        {
            const float df = x[j] - x[j + tau];
            d += df * df;
        }
        run += d;
        cm[tau] = run > 1.0e-12f ? d * (float) tau / run : 1.f;
    }

    int tau = -1;
    for (int t = minTau; t <= maxTau; ++t)
    {
        if (cm[t] < 0.15f)
        {
            while (t + 1 <= maxTau && cm[t + 1] < cm[t]) ++t;
            tau = t;
            break;
        }
    }
    if (tau < 0)
    {
        int best = minTau;
        for (int t = minTau; t <= maxTau; ++t) if (cm[t] < cm[best]) best = t;
        if (cm[best] > 0.35f) return r;
        tau = best;
    }

    const float s0 = cm[tau - 1], s1 = cm[tau], s2 = cm[tau + 1];
    const float denom = s0 - 2.f * s1 + s2;
    float shift = (std::abs(denom) > 1.0e-9f) ? 0.5f * (s0 - s2) / denom : 0.f;
    shift = std::max(-1.f, std::min(1.f, shift));
    r.f0 = srD / ((float) tau + shift);
    r.conf = 1.f - cm[tau];
    r.voiced = r.conf > 0.6f;
    return r;
}

inline float hzToMidi(float hz) { return 69.f + 12.f * std::log2(hz / 440.f); }

// nearest allowed MIDI note; mask bit i = pitch class i (0 = C). Empty mask = chromatic.
inline int snapToScale(float midi, unsigned mask)
{
    if ((mask & 0xFFFu) == 0u) mask = 0xFFFu;
    const int base = (int) std::lround(midi);
    int best = base;
    float bestDiff = 1.0e9f;
    for (int n = base - 6; n <= base + 6; ++n)
    {
        const int pc = ((n % 12) + 12) % 12;
        if (!((mask >> pc) & 1u)) continue;
        const float df = std::abs((float) n - midi);
        if (df < bestDiff) { bestDiff = df; best = n; }
    }
    return best;
}
