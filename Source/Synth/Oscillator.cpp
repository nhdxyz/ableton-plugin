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

float foldWarp(float sample, float amount)
{
    amount = juce::jlimit(0.0f, 1.0f, amount);

    if (amount <= 0.0001f)
        return sample;

    const auto drive = 1.0f + (amount * 5.2f);
    const auto driven = sample * drive;
    const auto folded = std::sin(driven * juce::MathConstants<float>::halfPi);
    const auto clipped = std::tanh(driven) / std::tanh(drive);
    const auto foldMix = 0.36f + (amount * 0.44f);

    return juce::jlimit(-1.0f, 1.0f, clipped + ((folded - clipped) * foldMix));
}

float bendWarp(float sample, float amount)
{
    amount = juce::jlimit(0.0f, 1.0f, amount);

    if (amount <= 0.0001f)
        return sample;

    const auto drive = 1.0f + (amount * 3.0f);
    const auto bias = amount * 0.36f;
    const auto positive = std::tanh((sample + bias) * drive);
    const auto negative = std::tanh((sample - bias) * (1.0f + (amount * 1.4f)));
    const auto shaped = sample >= 0.0f ? positive : negative;
    const auto centre = std::tanh((sample >= 0.0f ? bias : -bias) * drive);
    const auto normaliser = juce::jmax(0.35f, std::tanh(drive));

    return juce::jlimit(-1.0f, 1.0f, (shaped - centre) / normaliser);
}

float syncWarp(float sample, float phase, float phaseDelta, float amount)
{
    amount = juce::jlimit(0.0f, 1.0f, amount);

    if (amount <= 0.0001f)
        return sample;

    const auto ratio = 1.0f + (amount * 6.0f);
    const auto syncPhase = wrapUnitPhase(phase * ratio);
    const auto syncEdge = bandlimitedSaw(syncPhase, juce::jlimit(0.0f, 0.49f, phaseDelta * ratio));
    const auto contour = std::sin(juce::MathConstants<float>::twoPi * syncPhase);
    const auto syncSample = (syncEdge * 0.72f) + (contour * 0.28f);
    const auto mix = 0.18f + (amount * 0.58f);

    return juce::jlimit(-1.0f, 1.0f, sample + ((syncSample - sample) * mix));
}

WarpMode warpModeFromIndex(int mode)
{
    switch (juce::jlimit(0, 3, mode))
    {
        case 1:
            return WarpMode::fold;

        case 2:
            return WarpMode::bend;

        case 3:
            return WarpMode::sync;

        default:
            break;
    }

    return WarpMode::harmonic;
}

float applyWarp(float sample, float phase, float phaseDelta, float amount, WarpMode mode)
{
    switch (mode)
    {
        case WarpMode::fold:
            return foldWarp(sample, amount);

        case WarpMode::bend:
            return bendWarp(sample, amount);

        case WarpMode::sync:
            return syncWarp(sample, phase, phaseDelta, amount);

        case WarpMode::harmonic:
            break;
    }

    return harmonicWarp(sample, amount);
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

float additiveHarmonic(float phase, float phaseDelta, float harmonic, float amount, float phaseOffset = 0.0f)
{
    if (harmonic * phaseDelta >= 0.45f)
        return 0.0f;

    return sineHarmonic(phase, harmonic, phaseOffset) * amount;
}

float organSample(float phase, float phaseDelta)
{
    const auto fundamental = additiveHarmonic(phase, phaseDelta, 1.0f, 0.92f);
    const auto octave = additiveHarmonic(phase, phaseDelta, 2.0f, 0.72f);
    const auto fifth = additiveHarmonic(phase, phaseDelta, 3.0f, 0.46f);
    const auto secondOctave = additiveHarmonic(phase, phaseDelta, 4.0f, 0.34f);
    const auto mutation = additiveHarmonic(phase, phaseDelta, 6.0f, 0.18f, 0.04f)
        + additiveHarmonic(phase, phaseDelta, 8.0f, 0.12f, 0.11f);

    return (fundamental + octave + fifth + secondOctave + mutation) * 0.42f;
}

float housePianoSample(float phase, float phaseDelta)
{
    auto sample = additiveHarmonic(phase, phaseDelta, 1.0f, 1.0f);
    sample += additiveHarmonic(phase, phaseDelta, 2.0f, 0.62f, 0.02f);
    sample += additiveHarmonic(phase, phaseDelta, 3.0f, 0.38f, 0.18f);
    sample += additiveHarmonic(phase, phaseDelta, 5.0f, 0.26f, 0.08f);
    sample += additiveHarmonic(phase, phaseDelta, 7.0f, 0.18f, 0.27f);
    sample += additiveHarmonic(phase, phaseDelta, 10.0f, 0.1f, 0.36f);
    sample += additiveHarmonic(phase, phaseDelta, 14.0f, 0.07f, 0.12f);

    const auto tine = std::sin(sample * juce::MathConstants<float>::halfPi * 1.22f);
    return ((sample * 0.72f) + (tine * 0.28f)) * 0.48f;
}

}

Oscillator::Oscillator()
{
    for (auto& frame : customWaveFrames)
        frame = customWavePoints;

    customWavetable = WavetableModel::fromNormalisedControlFrames(customWaveFrames);
    customFramesInitialised = true;
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

void Oscillator::setWarpMode(int newWarpMode)
{
    warpMode = warpModeFromIndex(newWarpMode);
}

void Oscillator::setWarpB(float newWarpAmount)
{
    warpBAmount = juce::jlimit(0.0f, 1.0f, newWarpAmount);
}

void Oscillator::setWarpBMode(int newWarpMode)
{
    warpBMode = warpModeFromIndex(newWarpMode);
}

void Oscillator::setWavetablePosition(float newPosition)
{
    wavetablePosition = juce::jlimit(0.0f, 1.0f, newPosition);
}

void Oscillator::setCustomWaveform(const CustomWavePoints& points)
{
    for (size_t index = 0; index < customWavePoints.size(); ++index)
        customWavePoints[index] = juce::jlimit(0.0f, 1.0f, points[index]);

    customWaveFrames[0] = customWavePoints;
    if (! customFramesInitialised)
    {
        for (auto& frame : customWaveFrames)
            frame = customWavePoints;

        customFramesInitialised = true;
    }

    customWavetable = WavetableModel::fromNormalisedControlFrames(customWaveFrames);
}

void Oscillator::setCustomWavetableFrames(const CustomWaveFrames& frames)
{
    for (size_t frameIndex = 0; frameIndex < customWaveFrames.size(); ++frameIndex)
        for (size_t pointIndex = 0; pointIndex < customWaveFrames[frameIndex].size(); ++pointIndex)
            customWaveFrames[frameIndex][pointIndex] = juce::jlimit(0.0f, 1.0f, frames[frameIndex][pointIndex]);

    customWavePoints = customWaveFrames[0];
    customWavetable = WavetableModel::fromNormalisedControlFrames(customWaveFrames);
    customFramesInitialised = true;
}

float Oscillator::process(float phaseOffset)
{
    float sample = 0.0f;
    auto currentPhase = phase + juce::jlimit(-0.49f, 0.49f, phaseOffset);
    currentPhase -= std::floor(currentPhase);

    switch (waveform)
    {
        case Waveform::sine:
            sample = std::sin(juce::MathConstants<float>::twoPi * currentPhase);
            break;

        case Waveform::saw:
            sample = bandlimitedSaw(currentPhase, phaseDelta);
            break;

        case Waveform::square:
            sample = bandlimitedSquare(currentPhase, phaseDelta);
            break;

        case Waveform::triangle:
            sample = std::abs(phaseOffset) > 0.000001f ? triangleFromPhase(currentPhase) : triangleState;
            break;

        case Waveform::wavetable:
            sample = wavetableSample(currentPhase, phaseDelta, wavetablePosition);
            break;

        case Waveform::organ:
            sample = organSample(currentPhase, phaseDelta);
            break;

        case Waveform::housePiano:
            sample = housePianoSample(currentPhase, phaseDelta);
            break;

        case Waveform::custom:
            sample = customWavetable.sample(currentPhase, wavetablePosition);
            break;
    }

    if (waveform == Waveform::triangle)
    {
        triangleState += bandlimitedSquare(currentPhase, phaseDelta) * phaseDelta * 4.0f;
        triangleState = juce::jlimit(-1.1f, 1.1f, triangleState);
    }

    phase += phaseDelta;
    if (phase >= 1.0f)
        phase -= std::floor(phase);

    auto warped = applyWarp(juce::jlimit(-1.0f, 1.0f, sample), currentPhase, phaseDelta, warpAmount, warpMode);
    warped = applyWarp(warped, currentPhase, phaseDelta, warpBAmount, warpBMode);
    return juce::jlimit(-1.0f, 1.0f, warped);
}

void Oscillator::updatePhaseDelta()
{
    phaseDelta = juce::jlimit(0.0f, 0.49f, static_cast<float>(frequencyHz / sampleRate));
}
}
