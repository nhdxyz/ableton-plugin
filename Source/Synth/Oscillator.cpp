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

float harmonicWarp(float sample, float amount)
{
    amount = juce::jlimit(0.0f, 1.0f, amount);

    if (amount <= 0.0001f)
        return sample;

    const auto drive = 1.0f + (amount * 3.4f);
    const auto saturated = std::tanh(sample * drive) / std::tanh(drive);
    const auto folded = std::sin(sample * juce::MathConstants<float>::halfPi * (1.0f + (amount * 1.65f)));
    const auto foldedMix = amount * 0.42f;

    return juce::jlimit(-1.0f, 1.0f, saturated + ((folded - saturated) * foldedMix));
}

float sineHarmonic(float phase, float harmonic, float phaseOffset = 0.0f)
{
    return std::sin(juce::MathConstants<float>::twoPi * ((phase * harmonic) + phaseOffset));
}

float wavetableFrameSample(int frameIndex, float phase, float phaseDelta)
{
    auto add = [phase, phaseDelta] (float harmonic, float amount, float phaseOffset = 0.0f)
    {
        if (harmonic * phaseDelta >= 0.45f)
            return 0.0f;

        return sineHarmonic(phase, harmonic, phaseOffset) * amount;
    };

    switch (frameIndex)
    {
        case 0:
            return (add(1.0f, 1.0f) + add(2.0f, 0.46f) + add(3.0f, 0.28f) + add(6.0f, 0.16f)) * 0.58f;

        case 1:
        {
            auto sample = 0.0f;
            for (auto harmonic = 1; harmonic <= 12; ++harmonic)
                sample += add(static_cast<float>(harmonic), (harmonic % 2 == 0 ? -0.72f : 1.0f) / static_cast<float>(harmonic));
            return sample * 0.62f;
        }

        case 2:
        {
            auto sample = 0.0f;
            for (auto harmonic = 1; harmonic <= 13; harmonic += 2)
                sample += add(static_cast<float>(harmonic), 1.0f / static_cast<float>(harmonic));
            return sample * 0.82f;
        }

        case 3:
            return (add(1.0f, 0.82f)
                    + add(2.0f, 0.48f, 0.08f)
                    + add(5.0f, 0.33f, 0.19f)
                    + add(8.0f, 0.18f, 0.31f)) * 0.72f;

        default:
        {
            const auto edge = bandlimitedSaw(phase, phaseDelta);
            const auto fold = std::sin(edge * juce::MathConstants<float>::pi * 1.85f);
            return (edge * 0.55f) + (fold * 0.45f);
        }
    }
}

float wavetableSample(float phase, float phaseDelta, float position)
{
    constexpr auto lastFrame = 4.0f;
    const auto framePosition = juce::jlimit(0.0f, 1.0f, position) * lastFrame;
    const auto lowerFrame = juce::jlimit(0, static_cast<int>(lastFrame), static_cast<int>(std::floor(framePosition)));
    const auto upperFrame = juce::jlimit(0, static_cast<int>(lastFrame), lowerFrame + 1);
    const auto mix = juce::jlimit(0.0f, 1.0f, framePosition - static_cast<float>(lowerFrame));
    const auto smoothMix = mix * mix * (3.0f - (2.0f * mix));

    return wavetableFrameSample(lowerFrame, phase, phaseDelta)
        + ((wavetableFrameSample(upperFrame, phase, phaseDelta) - wavetableFrameSample(lowerFrame, phase, phaseDelta)) * smoothMix);
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

void Oscillator::setWarp(float newWarpAmount)
{
    warpAmount = juce::jlimit(0.0f, 1.0f, newWarpAmount);
}

void Oscillator::setWavetablePosition(float newPosition)
{
    wavetablePosition = juce::jlimit(0.0f, 1.0f, newPosition);
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

        case Waveform::wavetable:
            sample = wavetableSample(phase, phaseDelta, wavetablePosition);
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

    return harmonicWarp(juce::jlimit(-1.0f, 1.0f, sample), warpAmount);
}

void Oscillator::updatePhaseDelta()
{
    phaseDelta = juce::jlimit(0.0f, 0.49f, static_cast<float>(frequencyHz / sampleRate));
}
}
