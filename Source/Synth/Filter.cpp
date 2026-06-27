#include "Filter.h"

#include <cmath>

namespace Synth
{
namespace
{
float softClip(float sample, float drive)
{
    const auto gain = 1.0f + (drive * 4.0f);
    return std::tanh(sample * gain) / std::tanh(gain);
}

float asymmetricClip(float sample, float drive)
{
    const auto shaped = sample + (sample * sample * drive * 0.18f);
    return softClip(shaped, drive);
}

float hardLimit(float sample)
{
    return juce::jlimit(-1.6f, 1.6f, sample);
}
}

void Filter::prepare(double sampleRate, int maximumBlockSize)
{
    juce::dsp::ProcessSpec spec {
        sampleRate,
        static_cast<juce::uint32>(juce::jmax(1, maximumBlockSize)),
        1
    };

    filter.prepare(spec);
    secondFilter.prepare(spec);
    setMode(Mode::lowpass);
    reset();
}

void Filter::reset()
{
    filter.reset();
    secondFilter.reset();
}

void Filter::setMode(Mode mode)
{
    auto type = juce::dsp::StateVariableTPTFilterType::lowpass;

    switch (mode)
    {
        case Mode::lowpass:
            type = juce::dsp::StateVariableTPTFilterType::lowpass;
            break;
        case Mode::bandpass:
            type = juce::dsp::StateVariableTPTFilterType::bandpass;
            break;
        case Mode::highpass:
            type = juce::dsp::StateVariableTPTFilterType::highpass;
            break;
    }

    filter.setType(type);
    secondFilter.setType(type);
}

void Filter::setCharacter(Character newCharacter, float drive)
{
    character = newCharacter;
    characterDrive = juce::jlimit(0.0f, 1.0f, drive);
}

void Filter::setSlope(Slope newSlope)
{
    slope = newSlope;
}

void Filter::setCutoffAndResonance(float cutoffHz, float resonance)
{
    const auto safeCutoff = juce::jlimit(20.0f, 20000.0f, cutoffHz);
    const auto safeResonance = juce::jlimit(0.1f, 1.6f, resonance);

    filter.setCutoffFrequency(safeCutoff);
    filter.setResonance(safeResonance);
    secondFilter.setCutoffFrequency(safeCutoff);
    secondFilter.setResonance(juce::jlimit(0.1f, 1.15f, safeResonance * 0.62f));
}

float Filter::process(float sample)
{
    switch (character)
    {
        case Character::clean:
            return processFilterStack(sample);

        case Character::warm:
        {
            const auto input = softClip(sample, characterDrive * 0.45f);
            const auto filtered = processFilterStack(input);
            return softClip(filtered, characterDrive * 0.25f);
        }

        case Character::acid:
        {
            const auto input = asymmetricClip(sample, 0.18f + (characterDrive * 0.55f));
            const auto filtered = processFilterStack(input);
            return hardLimit(filtered * (1.0f + (characterDrive * 0.38f)));
        }

        case Character::dirty:
        {
            const auto input = softClip(sample, 0.28f + (characterDrive * 0.72f));
            const auto filtered = processFilterStack(input);
            return softClip(filtered, 0.32f + (characterDrive * 0.7f));
        }
    }

    return processFilterStack(sample);
}

float Filter::processFilterStack(float sample)
{
    auto filtered = filter.processSample(0, sample);

    if (slope == Slope::twentyFourDb)
        filtered = secondFilter.processSample(0, filtered);

    return filtered;
}
}
