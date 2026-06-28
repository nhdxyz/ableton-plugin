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

float maxAbsSample(const std::array<float, NateVSTAudioProcessor::outputSpectrumSnapshotSize>& snapshot)
{
    auto peak = 0.0f;
    for (const auto sample : snapshot)
    {
        if (! std::isfinite(sample))
            return std::numeric_limits<float>::infinity();
        peak = juce::jmax(peak, std::abs(sample));
    }

    return peak;
}
}

int main()
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    std::array<float, NateVSTAudioProcessor::outputSpectrumSnapshotSize> snapshot {};
    processor.getOutputSpectrumSnapshot(snapshot);
    if (maxAbsSample(snapshot) > 0.000001f)
    {
        std::cerr << "Spectrum snapshot should start silent after prepare\n";
        return 1;
    }

    if (! setPlainParameter(processor, Parameters::ID::osc1Level, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::subLevel, 0.5f)
        || ! setPlainParameter(processor, Parameters::ID::filterCutoff, 8000.0f)
        || ! setPlainParameter(processor, Parameters::ID::ampAttack, 0.001f)
        || ! setPlainParameter(processor, Parameters::ID::ampSustain, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::outputGain, -4.0f))
    {
        std::cerr << "Could not seed spectrum snapshot parameters\n";
        return 1;
    }

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 45, 0.9f), 0);
    processor.processBlock(buffer, midi);

    midi.clear();
    for (auto block = 0; block < 8; ++block)
        processor.processBlock(buffer, midi);

    processor.getOutputSpectrumSnapshot(snapshot);
    const auto activePeak = maxAbsSample(snapshot);
    if (! std::isfinite(activePeak) || activePeak <= 0.0001f)
    {
        std::cerr << "Spectrum snapshot did not capture rendered output: peak " << activePeak << '\n';
        return 1;
    }

    processor.prepareToPlay(44100.0, 512);
    processor.getOutputSpectrumSnapshot(snapshot);
    if (maxAbsSample(snapshot) > 0.000001f)
    {
        std::cerr << "Spectrum snapshot did not reset after prepare\n";
        return 1;
    }

    std::cout << "Spectrum snapshot audit passed for silent reset and rendered output capture.\n";
    return 0;
}
