#include "../Source/PluginProcessor.h"

#include <cmath>
#include <iostream>

namespace
{
bool setPlainParameter(NateVSTAudioProcessor& processor, const juce::String& parameterID, float value)
{
    if (auto* parameter = processor.getValueTreeState().getParameter(parameterID))
    {
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        return true;
    }

    return false;
}

float rmsForBuffer(const juce::AudioBuffer<float>& buffer)
{
    double sumSquares = 0.0;
    auto sampleCount = 0;

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const auto* samples = buffer.getReadPointer(channel);

        for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
        {
            const auto sample = static_cast<double>(samples[sampleIndex]);
            sumSquares += sample * sample;
            ++sampleCount;
        }
    }

    return sampleCount > 0 ? static_cast<float>(std::sqrt(sumSquares / static_cast<double>(sampleCount))) : 0.0f;
}

struct NoiseRenderStats
{
    float earlyRms = 0.0f;
    float lateRms = 0.0f;
};

bool configureNoiseOnlyPatch(NateVSTAudioProcessor& processor, int noiseType, float noiseDecay)
{
    return setPlainParameter(processor, Parameters::ID::osc1Level, 0.0f)
        && setPlainParameter(processor, Parameters::ID::osc2Level, 0.0f)
        && setPlainParameter(processor, Parameters::ID::subLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::noiseLevel, 1.0f)
        && setPlainParameter(processor, Parameters::ID::noiseType, static_cast<float>(noiseType))
        && setPlainParameter(processor, Parameters::ID::noiseDecay, noiseDecay)
        && setPlainParameter(processor, Parameters::ID::ampAttack, 0.001f)
        && setPlainParameter(processor, Parameters::ID::ampDecay, 1.0f)
        && setPlainParameter(processor, Parameters::ID::ampSustain, 1.0f)
        && setPlainParameter(processor, Parameters::ID::ampRelease, 0.2f)
        && setPlainParameter(processor, Parameters::ID::filterCutoff, 20000.0f)
        && setPlainParameter(processor, Parameters::ID::filterResonance, 0.1f)
        && setPlainParameter(processor, Parameters::ID::filterEnvAmount, 0.0f)
        && setPlainParameter(processor, Parameters::ID::driveAmount, 0.0f)
        && setPlainParameter(processor, Parameters::ID::outputGain, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sampleEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 0.0f);
}

NoiseRenderStats renderNoiseOnlyNote(int noiseType, float noiseDecay)
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    NoiseRenderStats stats;
    if (! configureNoiseOnlyPatch(processor, noiseType, noiseDecay))
        return stats;

    juce::AudioBuffer<float> buffer(2, 512);

    for (auto blockIndex = 0; blockIndex < 96; ++blockIndex)
    {
        juce::MidiBuffer midi;
        if (blockIndex == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(100)), 0);

        processor.processBlock(buffer, midi);

        const auto blockRms = rmsForBuffer(buffer);
        if (blockIndex < 4)
            stats.earlyRms = juce::jmax(stats.earlyRms, blockRms);
        if (blockIndex >= 88)
            stats.lateRms = juce::jmax(stats.lateRms, blockRms);
    }

    return stats;
}
}

int main()
{
    const auto choices = Parameters::noiseTypeChoices();
    if (choices.size() != 7
        || choices[0] != "White"
        || choices[4] != "Tick"
        || choices[6] != "Digital")
    {
        std::cerr << "Noise source choices changed unexpectedly\n";
        return 1;
    }

    const auto tickStats = renderNoiseOnlyNote(4, 0.01f);
    if (tickStats.earlyRms <= 0.001f || tickStats.lateRms >= tickStats.earlyRms * 0.25f)
    {
        std::cerr << "Tick noise did not produce a short decaying transient: early "
                  << tickStats.earlyRms << " late " << tickStats.lateRms << '\n';
        return 1;
    }

    const auto whiteStats = renderNoiseOnlyNote(0, 0.18f);
    if (whiteStats.earlyRms <= 0.001f || whiteStats.lateRms <= 0.001f)
    {
        std::cerr << "White noise did not sustain through the noise-only patch: early "
                  << whiteStats.earlyRms << " late " << whiteStats.lateRms << '\n';
        return 1;
    }

    const auto digitalStats = renderNoiseOnlyNote(6, 0.18f);
    if (digitalStats.earlyRms <= 0.001f || digitalStats.lateRms <= 0.001f)
    {
        std::cerr << "Digital noise did not render sustained grit: early "
                  << digitalStats.earlyRms << " late " << digitalStats.lateRms << '\n';
        return 1;
    }

    std::cout << "Noise source audit passed for choices, tick decay, and sustained noise modes.\n";
    return 0;
}
