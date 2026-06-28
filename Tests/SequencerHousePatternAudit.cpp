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
}

int main()
{
    NateVSTAudioProcessor processor;
    Sequencer::PatternSequencer sequencer(processor.getValueTreeState());

    if (! setPlainParameter(processor, Parameters::ID::sequencerChordMode, 9.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerChordVoicing, 0.0f))
    {
        std::cerr << "Could not seed House 9 chord mode\n";
        return 1;
    }

    auto noteCount = 0;
    auto notes = sequencer.getChordNotes(60, 0, 0, noteCount);
    if (noteCount != 5
        || notes[0] != 60
        || notes[1] != 64
        || notes[2] != 67
        || notes[3] != 70
        || notes[4] != 74)
    {
        std::cerr << "House 9 chord mode produced unexpected notes\n";
        return 1;
    }

    if (! setPlainParameter(processor, Parameters::ID::sequencerChordMode, 10.0f))
    {
        std::cerr << "Could not seed Dub chord mode\n";
        return 1;
    }

    notes = sequencer.getChordNotes(60, 0, 0, noteCount);
    if (noteCount != 5
        || notes[0] != 60
        || notes[1] != 67
        || notes[2] != 70
        || notes[3] != 74
        || notes[4] != 77)
    {
        std::cerr << "Dub chord mode produced unexpected notes\n";
        return 1;
    }

    processor.applySequencerPatternPreset(11); // Deep Chord
    if (! near(readPlainParameter(processor, Parameters::ID::sequencerChordMode, 0.0f), 9.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerChordVoicing, 0.0f), 4.0f)
        || ! processor.getSequencerStep(0).enabled
        || ! processor.getSequencerStep(10).enabled)
    {
        std::cerr << "Deep Chord preset did not seed House 9 chord state\n";
        return 1;
    }

    processor.applySequencerPatternPreset(12); // Dub Chord
    if (! near(readPlainParameter(processor, Parameters::ID::sequencerChordMode, 0.0f), 10.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerLockDestination, 0.0f), 6.0f)
        || ! processor.getSequencerStep(2).enabled
        || ! near(processor.getSequencerStep(14).lock, 0.5f))
    {
        std::cerr << "Dub Chord preset did not seed dub chord/throw state\n";
        return 1;
    }

    processor.applySequencerPatternPreset(13); // Off Bass
    if (! near(readPlainParameter(processor, Parameters::ID::sequencerChordMode, 1.0f), 0.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerOctave, 0.0f), -1.0f)
        || ! processor.getSequencerStep(2).enabled
        || ! processor.getSequencerStep(6).enabled)
    {
        std::cerr << "Off Bass preset did not seed offbeat bass state\n";
        return 1;
    }

    processor.applySequencerPatternPreset(14); // Rolling Bass
    if (! near(readPlainParameter(processor, Parameters::ID::sequencerLockDestination, 0.0f), 1.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerGrooveMode, 0.0f), 3.0f)
        || ! processor.getSequencerStep(5).enabled
        || ! processor.getSequencerStep(15).enabled
        || processor.getSequencerStep(15).lock <= 0.4f)
    {
        std::cerr << "Rolling Bass preset did not seed rolling contour state\n";
        return 1;
    }

    std::cout << "Sequencer house pattern audit passed.\n";
    return 0;
}
