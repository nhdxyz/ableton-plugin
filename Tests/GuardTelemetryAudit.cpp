#include "../Source/PluginProcessor.h"

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

void renderHotNote(NateVSTAudioProcessor& processor, juce::AudioBuffer<float>& buffer)
{
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 48, 1.0f), 0);
    processor.processBlock(buffer, midi);

    midi.clear();
    for (auto block = 0; block < 8; ++block)
        processor.processBlock(buffer, midi);
}
}

int main()
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    if (! setPlainParameter(processor, Parameters::ID::osc1Level, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::osc2Level, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::subLevel, 0.8f)
        || ! setPlainParameter(processor, Parameters::ID::filterCutoff, 12000.0f)
        || ! setPlainParameter(processor, Parameters::ID::ampAttack, 0.001f)
        || ! setPlainParameter(processor, Parameters::ID::ampSustain, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::outputGain, 6.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxGuardPush, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxGuardCeiling, 0.65f))
    {
        std::cerr << "Could not seed guard telemetry parameters\n";
        return 1;
    }

    juce::AudioBuffer<float> buffer(2, 512);
    renderHotNote(processor, buffer);

    auto drive = 0.0f;
    auto reduction = 0.0f;
    auto active = false;
    processor.getGuardMeterLevels(drive, reduction, active);

    if (! active || drive <= 1.0f || reduction <= 0.02f)
    {
        std::cerr << "Guard telemetry did not report active protection: active "
                  << active << " drive " << drive << " reduction " << reduction << '\n';
        return 1;
    }

    if (! setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 0.0f))
    {
        std::cerr << "Could not disable guard for telemetry reset\n";
        return 1;
    }

    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);
    processor.getGuardMeterLevels(drive, reduction, active);

    if (active || drive > 0.001f || reduction > 0.001f)
    {
        std::cerr << "Guard telemetry did not reset when disabled: active "
                  << active << " drive " << drive << " reduction " << reduction << '\n';
        return 1;
    }

    std::cout << "Guard telemetry audit passed for active protection and reset.\n";
    return 0;
}
