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

bool finiteInRange(float value, float min, float max)
{
    return std::isfinite(value) && value >= min && value <= max;
}
}

int main()
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    auto correlation = 1.0f;
    auto width = 1.0f;
    auto balance = 1.0f;
    auto lowStereoRisk = 1.0f;
    processor.getStereoFieldLevels(correlation, width, balance, lowStereoRisk);
    if (std::abs(correlation) > 0.000001f
        || std::abs(width) > 0.000001f
        || std::abs(balance) > 0.000001f
        || std::abs(lowStereoRisk) > 0.000001f)
    {
        std::cerr << "Stereo field telemetry should reset to zero after prepare\n";
        return 1;
    }

    if (! setPlainParameter(processor, Parameters::ID::osc1Level, 0.9f)
        || ! setPlainParameter(processor, Parameters::ID::osc2Level, 0.35f)
        || ! setPlainParameter(processor, Parameters::ID::osc2Tune, 4.0f)
        || ! setPlainParameter(processor, Parameters::ID::unisonVoices, 4.0f)
        || ! setPlainParameter(processor, Parameters::ID::unisonDetune, 0.12f)
        || ! setPlainParameter(processor, Parameters::ID::unisonSpread, 0.75f)
        || ! setPlainParameter(processor, Parameters::ID::filterCutoff, 9000.0f)
        || ! setPlainParameter(processor, Parameters::ID::ampAttack, 0.001f)
        || ! setPlainParameter(processor, Parameters::ID::ampSustain, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxWidthEnabled, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxWidthAmount, 1.35f)
        || ! setPlainParameter(processor, Parameters::ID::outputGain, -5.0f))
    {
        std::cerr << "Could not seed stereo field telemetry parameters\n";
        return 1;
    }

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 48, 0.9f), 0);
    processor.processBlock(buffer, midi);

    midi.clear();
    for (auto block = 0; block < 8; ++block)
        processor.processBlock(buffer, midi);

    auto peakLeft = 0.0f;
    auto peakRight = 0.0f;
    auto rmsLeft = 0.0f;
    auto rmsRight = 0.0f;
    processor.getOutputMeterLevels(peakLeft, peakRight, rmsLeft, rmsRight);
    if (juce::jmax(peakLeft, peakRight) <= 0.0001f)
    {
        std::cerr << "Stereo field telemetry audit did not render output\n";
        return 1;
    }

    processor.getStereoFieldLevels(correlation, width, balance, lowStereoRisk);
    if (! finiteInRange(correlation, -1.0f, 1.0f)
        || ! finiteInRange(width, 0.0f, 1.0f)
        || ! finiteInRange(balance, -1.0f, 1.0f)
        || ! finiteInRange(lowStereoRisk, 0.0f, 1.0f))
    {
        std::cerr << "Stereo field telemetry out of range: corr=" << correlation
                  << " width=" << width
                  << " balance=" << balance
                  << " lowRisk=" << lowStereoRisk << '\n';
        return 1;
    }

    processor.prepareToPlay(44100.0, 512);
    processor.getStereoFieldLevels(correlation, width, balance, lowStereoRisk);
    if (std::abs(correlation) > 0.000001f
        || std::abs(width) > 0.000001f
        || std::abs(balance) > 0.000001f
        || std::abs(lowStereoRisk) > 0.000001f)
    {
        std::cerr << "Stereo field telemetry did not reset after prepare\n";
        return 1;
    }

    std::cout << "Stereo field telemetry audit passed for reset and finite rendered output.\n";
    return 0;
}
