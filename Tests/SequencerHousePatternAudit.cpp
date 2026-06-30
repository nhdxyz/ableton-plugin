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

    const auto grooveChoices = Parameters::sequencerGrooveModeChoices();
    if (grooveChoices.size() < 8
        || grooveChoices[0] != "Classic"
        || grooveChoices[3] != "Tight"
        || grooveChoices[4] != "House Shuf"
        || grooveChoices[7] != "Drive")
    {
        std::cerr << "Appended groove mode choices are not stable\n";
        return 1;
    }

    processor.applySequencerPatternPreset(0);
    if (! processor.applySequencerGrooveTransform(6)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerGrooveMode, 0.0f), 4.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerSwing, 0.0f), 0.28f)
        || processor.getSequencerStep(3).timing < 0.47f)
    {
        std::cerr << "House Shuffle template did not apply expected timing/global state\n";
        return 1;
    }

    processor.applySequencerPatternPreset(2);
    if (! processor.applySequencerGrooveTransform(7)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerGrooveMode, 0.0f), 2.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerSwing, 0.0f), 0.46f)
        || processor.getSequencerStep(7).timing < 0.81f
        || processor.getSequencerStep(7).lock < 0.53f)
    {
        std::cerr << "UKG 2-Step Push template did not apply expected timing/global state\n";
        return 1;
    }

    processor.applySequencerPatternPreset(0);
    if (! processor.applySequencerGrooveTransform(8)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerGrooveMode, 0.0f), 5.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerSwing, 0.0f), 0.12f)
        || processor.getSequencerStep(3).timing < 0.27f)
    {
        std::cerr << "Tech House Tight template did not apply expected timing/global state\n";
        return 1;
    }

    processor.applySequencerPatternPreset(0);
    if (! processor.applySequencerGrooveTransform(9)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerGrooveMode, 0.0f), 6.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerSwing, 0.0f), 0.2f)
        || processor.getSequencerStep(3).probability > 0.89f)
    {
        std::cerr << "Minimal Skip template did not apply expected timing/global state\n";
        return 1;
    }

    processor.applySequencerPatternPreset(10);
    if (! processor.applySequencerGrooveTransform(10)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerGrooveMode, 0.0f), 7.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerSwing, 0.0f), 0.1f)
        || processor.getSequencerStep(14).lock < 0.3f)
    {
        std::cerr << "Techno Drive template did not apply expected timing/global state\n";
        return 1;
    }

    processor.applySequencerPatternPreset(0);
    if (! processor.applySequencerGrooveTransform(11)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerGrooveMode, 0.0f), 4.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerSwing, 0.0f), 0.24f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerChordMode, 1.0f), 0.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerOctave, 0.0f), -1.0f)
        || processor.getSequencerStep(3).noteOffset != -7
        || processor.getSequencerStep(3).timing < 0.45f
        || processor.getSequencerStep(14).noteOffset != -2
        || processor.getSequencerStep(14).ratchet < 2)
    {
        std::cerr << "Bass Contour template did not apply expected bassline contour/global state\n";
        return 1;
    }

    processor.clearSequencerPattern();
    if (! processor.applySequencerGrooveTransform(13)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerChordMode, 0.0f), 9.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerChordVoicing, 0.0f), 4.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerGrooveMode, 0.0f), 4.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sequencerLockDestination, 0.0f), 5.0f)
        || processor.getSequencerStep(0).enabled
        || ! processor.getSequencerStep(2).enabled
        || processor.getSequencerStep(2).noteOffset != 0
        || ! processor.getSequencerStep(6).enabled
        || processor.getSequencerStep(6).noteOffset != 5
        || processor.getSequencerStep(14).ratchet < 2
        || processor.getSequencerStep(15).condition != 3)
    {
        std::cerr << "Chord Stab Paint template did not seed expected house-stab pattern/global state\n";
        return 1;
    }

    if (! processor.undoSequencerEdit()
        || processor.getSequencerStep(2).enabled
        || processor.getSequencerStep(6).enabled)
    {
        std::cerr << "Chord Stab Paint template did not participate in sequencer undo\n";
        return 1;
    }

    std::cout << "Sequencer house pattern audit passed.\n";
    return 0;
}
