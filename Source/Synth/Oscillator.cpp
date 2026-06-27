#include "Oscillator.h"

#include <cmath>

namespace Synth
{
namespace
{
float wrapUnitPhase(float value)
{
    return value - std::floor(value);
}

float triangleFromPhase(float phase)
{
    return 1.0f - (4.0f * std::abs(phase - 0.5f));
}

float polyBlep(float phase, float phaseDelta)
{
    if (phaseDelta <= 0.0f || phaseDelta >= 0.5f)
        return 0.0f;

    if (phase < phaseDelta)
    {
        const auto t = phase / phaseDelta;
        return (t + t) - (t * t) - 1.0f;
    }

    if (phase > 1.0f - phaseDelta)
    {
        const auto t = (phase - 1.0f) / phaseDelta;
        return (t * t) + (t + t) + 1.0f;
    }

    return 0.0f;
}

float bandlimitedSaw(float phase, float phaseDelta)
{
    return ((2.0f * phase) - 1.0f) - polyBlep(phase, phaseDelta);
}

float bandlimitedSquare(float phase, float phaseDelta)
{
    auto sample = phase < 0.5f ? 1.0f : -1.0f;
    sample += polyBlep(phase, phaseDelta);
    sample -= polyBlep(wrapUnitPhase(phase + 0.5f), phaseDelta);
    return sample;
}
}

void Oscillator::prepare(double newSampleRate)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    updatePhaseDelta();
}

void Oscillator::reset()
{
    phase = 0.0f;
    triangleState = triangleFromPhase(phase);
}

void Oscillator::setFrequency(float newFrequencyHz)
{
    frequencyHz = juce::jlimit(8.0f, 24000.0f, newFrequencyHz);
    updatePhaseDelta();
}

void Oscillator::setWaveform(Waveform newWaveform)
{
    if (waveform == newWaveform)
        return;

    waveform = newWaveform;

    if (waveform == Waveform::triangle)
        triangleState = triangleFromPhase(phase);
}

float Oscillator::process()
{
    float sample = 0.0f;

    switch (waveform)
    {
        case Waveform::sine:
            sample = std::sin(juce::MathConstants<float>::twoPi * phase);
            break;

        case Waveform::saw:
            sample = bandlimitedSaw(phase, phaseDelta);
            break;

        case Waveform::square:
            sample = bandlimitedSquare(phase, phaseDelta);
            break;

        case Waveform::triangle:
            sample = triangleState;
            break;
    }

    if (waveform == Waveform::triangle)
    {
        triangleState += bandlimitedSquare(phase, phaseDelta) * phaseDelta * 4.0f;
        triangleState = juce::jlimit(-1.1f, 1.1f, triangleState);
    }

    phase += phaseDelta;
    if (phase >= 1.0f)
        phase -= std::floor(phase);

    return juce::jlimit(-1.0f, 1.0f, sample);
}

void Oscillator::updatePhaseDelta()
{
    phaseDelta = juce::jlimit(0.0f, 0.49f, static_cast<float>(frequencyHz / sampleRate));
}
}
