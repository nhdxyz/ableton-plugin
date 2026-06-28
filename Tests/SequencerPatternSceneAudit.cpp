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

float readPlainParameter(NateVSTAudioProcessor& processor, const juce::String& parameterID, float fallback)
{
    if (auto* value = processor.getValueTreeState().getRawParameterValue(parameterID))
        return value->load();

    return fallback;
}

bool near(float actual, float expected, float tolerance = 0.001f)
{
    return std::abs(actual - expected) <= tolerance;
}

Sequencer::Step sceneStep(int note, float velocity, float probability, float timing, float length, float lock)
{
    Sequencer::Step step;
    step.enabled = true;
    step.noteOffset = note;
    step.velocity = velocity;
    step.probability = probability;
    step.timing = timing;
    step.length = length;
    step.lock = lock;
    return step;
}
}

int main()
{
    NateVSTAudioProcessor processor;

    if (! setPlainParameter(processor, Parameters::ID::sequencerEnabled, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerGrooveMode, 2.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerChordMode, 3.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerChordVoicing, 4.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerChordStrum, 0.18f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerLockDestination, 5.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerLockDepth, 0.42f))
    {
        std::cerr << "Could not seed scene A parameters\n";
        return 1;
    }

    processor.setSequencerStep(0, sceneStep(0, 0.92f, 1.0f, 0.0f, 0.72f, 0.0f));
    processor.setSequencerStep(6, sceneStep(7, 0.68f, 0.78f, 0.62f, 0.44f, 0.5f));
    processor.captureSequencerPatternScene(0);

    if (! processor.hasSequencerPatternScene(0)
        || processor.hasSequencerPatternScene(2)
        || ! processor.getSequencerPatternSceneSummary(0).containsIgnoreCase("chord")
        || ! processor.getSequencerPatternSceneSummary(0).containsIgnoreCase("groove"))
    {
        std::cerr << "Scene A did not capture a descriptive summary: "
                  << processor.getSequencerPatternSceneSummary(0) << '\n';
        return 1;
    }

    if (! setPlainParameter(processor, Parameters::ID::sequencerGrooveMode, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerChordMode, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerChordVoicing, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerChordStrum, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerLockDestination, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerLockDepth, 0.24f))
    {
        std::cerr << "Could not seed scene B parameters\n";
        return 1;
    }

    processor.setSequencerStep(0, sceneStep(-5, 0.55f, 1.0f, 0.0f, 0.36f, 0.1f));
    processor.setSequencerStep(3, sceneStep(2, 0.44f, 0.62f, 0.72f, 0.28f, 0.64f));
    processor.setSequencerStep(11, sceneStep(10, 0.74f, 0.84f, 0.48f, 0.58f, 0.32f));
    processor.captureSequencerPatternScene(1);

    if (! processor.recallSequencerPatternScene(0))
    {
        std::cerr << "Scene A recall failed\n";
        return 1;
    }

    const auto recalledA0 = processor.getSequencerStep(0);
    const auto recalledA6 = processor.getSequencerStep(6);
    if (recalledA0.noteOffset != 0
        || recalledA6.noteOffset != 7
        || ! near(recalledA6.timing, 0.62f)
        || ! near(recalledA6.lock, 0.5f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerChordMode, 0.0f), 3.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerGrooveMode, 0.0f), 2.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerLockDepth, 0.0f), 0.42f))
    {
        std::cerr << "Scene A recall did not restore steps and sequencer controls\n";
        return 1;
    }

    juce::MemoryBlock savedState;
    processor.getStateInformation(savedState);

    NateVSTAudioProcessor restored;
    restored.setStateInformation(savedState.getData(), static_cast<int>(savedState.getSize()));

    if (! restored.hasSequencerPatternScene(0) || ! restored.hasSequencerPatternScene(1))
    {
        std::cerr << "Saved plugin state did not restore captured pattern scenes\n";
        return 1;
    }

    if (! restored.recallSequencerPatternScene(1))
    {
        std::cerr << "Restored scene B recall failed\n";
        return 1;
    }

    const auto restoredB3 = restored.getSequencerStep(3);
    const auto restoredB11 = restored.getSequencerStep(11);
    if (restoredB3.noteOffset != 2
        || restoredB11.noteOffset != 10
        || ! near(restoredB3.timing, 0.72f)
        || ! near(restoredB3.lock, 0.64f)
        || ! near(readPlainParameter(restored, Parameters::ID::sequencerChordMode, 0.0f), 0.0f)
        || ! near(readPlainParameter(restored, Parameters::ID::sequencerLockDestination, 0.0f), 1.0f))
    {
        std::cerr << "Restored scene B did not restore expected sequence data\n";
        return 1;
    }

    std::cout << "Sequencer pattern scene audit passed.\n";
    return 0;
}
