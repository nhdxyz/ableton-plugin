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
}

int main()
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    if (! setPlainParameter(processor, Parameters::ID::fxPumpEnabled, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxPumpRate, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxPumpCurve, 2.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxPumpDepth, 0.72f)
        || ! setPlainParameter(processor, Parameters::ID::fxPumpShape, 0.68f)
        || ! setPlainParameter(processor, Parameters::ID::fxPumpPhase, 0.0f))
    {
        std::cerr << "Could not seed pump telemetry parameters\n";
        return 1;
    }

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);

    auto phase = 0.0f;
    auto gain = 1.0f;
    auto reduction = 0.0f;
    auto active = false;
    processor.getPumpMeterLevels(phase, gain, reduction, active);

    if (! active
        || phase <= 0.0f
        || phase >= 1.0f
        || gain >= 0.98f
        || reduction <= 0.02f)
    {
        std::cerr << "Pump telemetry did not report active ducking: active "
                  << active << " phase " << phase << " gain " << gain
                  << " reduction " << reduction << '\n';
        return 1;
    }

    if (! setPlainParameter(processor, Parameters::ID::fxPumpEnabled, 0.0f))
    {
        std::cerr << "Could not disable pump for telemetry reset\n";
        return 1;
    }

    processor.processBlock(buffer, midi);
    processor.getPumpMeterLevels(phase, gain, reduction, active);

    if (active || std::abs(gain - 1.0f) > 0.001f || reduction > 0.001f)
    {
        std::cerr << "Pump telemetry did not reset when disabled: active "
                  << active << " gain " << gain << " reduction " << reduction << '\n';
        return 1;
    }

    std::cout << "Pump telemetry audit passed for active ducking and reset.\n";
    return 0;
}
