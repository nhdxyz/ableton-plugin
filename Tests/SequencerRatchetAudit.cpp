#include "../Source/PluginProcessor.h"

#include <cmath>
#include <iostream>
#include <vector>

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

bool configureSequencer(NateVSTAudioProcessor& processor)
{
    return setPlainParameter(processor, Parameters::ID::sequencerEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerRate, 1.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerRoot, 60.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerGate, 0.42f)
        && setPlainParameter(processor, Parameters::ID::sequencerSwing, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerGrooveMode, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerChordMode, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerChordVoicing, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerChordStrum, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerAccent, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerOctave, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerProbability, 1.0f);
}

Sequencer::Step ratchetStep(int ratchet, int condition = 0, bool slide = false)
{
    Sequencer::Step step;
    step.enabled = true;
    step.noteOffset = 0;
    step.velocity = 0.9f;
    step.probability = 1.0f;
    step.timing = 0.0f;
    step.length = 1.0f;
    step.lock = 0.0f;
    step.ratchet = ratchet;
    step.condition = condition;
    step.slide = slide;
    return step;
}

std::vector<int> noteOnPositions(const juce::MidiBuffer& midi)
{
    std::vector<int> positions;
    for (const auto metadata : midi)
        if (metadata.getMessage().isNoteOn())
            positions.push_back(metadata.samplePosition);

    return positions;
}

struct NoteOnEvent
{
    int sample = 0;
    int note = -1;
};

struct NoteEvent
{
    int sample = 0;
    int note = -1;
    bool noteOn = false;
};

std::vector<NoteOnEvent> noteOnEvents(const juce::MidiBuffer& midi)
{
    std::vector<NoteOnEvent> events;
    for (const auto metadata : midi)
        if (metadata.getMessage().isNoteOn())
            events.push_back({ metadata.samplePosition, metadata.getMessage().getNoteNumber() });

    return events;
}

std::vector<NoteEvent> noteEvents(const juce::MidiBuffer& midi)
{
    std::vector<NoteEvent> events;
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (message.isNoteOnOrOff())
            events.push_back({ metadata.samplePosition, message.getNoteNumber(), message.isNoteOn() });
    }

    return events;
}

bool near(int actual, int expected, int tolerance)
{
    return std::abs(actual - expected) <= tolerance;
}
}

int main()
{
    NateVSTAudioProcessor processor;
    if (! configureSequencer(processor))
    {
        std::cerr << "Could not configure sequencer parameters for ratchet audit\n";
        return 1;
    }

    processor.setSequencerStep(0, ratchetStep(9));
    if (processor.getSequencerStep(0).ratchet != Sequencer::PatternSequencer::maxRatchet)
    {
        std::cerr << "Sequencer step did not clamp high ratchet values\n";
        return 1;
    }

    processor.setSequencerStep(0, ratchetStep(0));
    if (processor.getSequencerStep(0).ratchet != Sequencer::PatternSequencer::minRatchet)
    {
        std::cerr << "Sequencer step did not clamp low ratchet values\n";
        return 1;
    }

    processor.setSequencerStep(0, ratchetStep(1, 9));
    if (processor.getSequencerStep(0).condition != Sequencer::PatternSequencer::maxCondition)
    {
        std::cerr << "Sequencer step did not clamp high condition values\n";
        return 1;
    }

    processor.setSequencerStep(0, ratchetStep(1, -3));
    if (processor.getSequencerStep(0).condition != Sequencer::PatternSequencer::minCondition)
    {
        std::cerr << "Sequencer step did not clamp low condition values\n";
        return 1;
    }

    processor.setSequencerStep(0, ratchetStep(2, 0, true));
    if (processor.getSequencerStep(0).slide)
    {
        std::cerr << "Sequencer step did not clear slide on ratcheted steps\n";
        return 1;
    }

    processor.setSequencerStep(0, ratchetStep(3, 3));
    processor.setSequencerStep(1, ratchetStep(1, 0, true));
    processor.captureSequencerPatternScene(0);

    juce::MemoryBlock savedState;
    processor.getStateInformation(savedState);

    NateVSTAudioProcessor restored;
    restored.setStateInformation(savedState.getData(), static_cast<int>(savedState.getSize()));
    if (restored.getSequencerStep(0).ratchet != 3
        || restored.getSequencerStep(0).condition != 3
        || ! restored.getSequencerStep(1).slide
        || ! restored.hasSequencerPatternScene(0)
        || ! restored.getSequencerPatternSceneSummary(0).containsIgnoreCase("rats")
        || ! restored.getSequencerPatternSceneSummary(0).containsIgnoreCase("conds")
        || ! restored.getSequencerPatternSceneSummary(0).containsIgnoreCase("slides"))
    {
        std::cerr << "Ratchet/condition/slide step state did not survive preset/scene save and summary\n";
        return 1;
    }

    restored.setSequencerStep(0, ratchetStep(1));
    restored.setSequencerStep(1, ratchetStep(1, 0, false));
    if (! restored.recallSequencerPatternScene(0)
        || restored.getSequencerStep(0).ratchet != 3
        || restored.getSequencerStep(0).condition != 3
        || ! restored.getSequencerStep(1).slide)
    {
        std::cerr << "Ratchet/condition/slide step state did not restore from a pattern scene\n";
        return 1;
    }

    Sequencer::PatternSequencer sequencer(processor.getValueTreeState());
    sequencer.prepare(44100.0);
    sequencer.clear();
    sequencer.setStep(0, ratchetStep(3));

    juce::MidiBuffer midi;
    sequencer.process(midi, 6000, 120.0, {});
    const auto liveNoteOns = noteOnPositions(midi);
    if (liveNoteOns.size() != 3
        || ! near(liveNoteOns[0], 0, 1)
        || ! near(liveNoteOns[1], 1837, 16)
        || ! near(liveNoteOns[2], 3675, 16))
    {
        std::cerr << "Live sequencer ratchets did not emit three evenly spaced note-ons:";
        for (const auto position : liveNoteOns)
            std::cerr << ' ' << position;
        std::cerr << '\n';
        return 1;
    }

    const auto oneStepSamples = static_cast<int>((44100.0 * 60.0 / 120.0) / 4.0);
    const auto cycleSamples = oneStepSamples * Sequencer::PatternSequencer::numSteps;

    Sequencer::PatternSequencer slideSequencer(processor.getValueTreeState());
    slideSequencer.prepare(44100.0);
    slideSequencer.clear();
    auto slideOutStep = ratchetStep(1, 0, true);
    auto slideTargetStep = ratchetStep(1);
    slideTargetStep.noteOffset = 7;
    slideSequencer.setStep(0, slideOutStep);
    slideSequencer.setStep(1, slideTargetStep);
    juce::MidiBuffer slideMidi;
    slideSequencer.process(slideMidi, oneStepSamples * 2 + 32, 120.0, {});
    const auto slideEvents = noteEvents(slideMidi);
    auto firstNoteOff = -1;
    auto secondNoteOn = -1;
    for (const auto& event : slideEvents)
    {
        if (event.note == 60 && ! event.noteOn && firstNoteOff < 0)
            firstNoteOff = event.sample;
        if (event.note == 67 && event.noteOn && secondNoteOn < 0)
            secondNoteOn = event.sample;
    }
    if (secondNoteOn < 0 || firstNoteOff <= secondNoteOn)
    {
        std::cerr << "Live slide did not overlap the previous note into the target step:";
        for (const auto& event : slideEvents)
            std::cerr << " (" << event.sample << ',' << event.note << ',' << (event.noteOn ? "on" : "off") << ')';
        std::cerr << '\n';
        return 1;
    }

    Sequencer::PatternSequencer oddConditionSequencer(processor.getValueTreeState());
    oddConditionSequencer.prepare(44100.0);
    oddConditionSequencer.clear();
    oddConditionSequencer.setStep(0, ratchetStep(1, 1));
    juce::MidiBuffer oddConditionMidi;
    oddConditionSequencer.process(oddConditionMidi, cycleSamples * 2 - 128, 120.0, {});
    const auto oddConditionOns = noteOnPositions(oddConditionMidi);
    if (oddConditionOns.size() != 1 || ! near(oddConditionOns[0], 0, 1))
    {
        std::cerr << "Odd-cycle condition did not suppress the second cycle:";
        for (const auto position : oddConditionOns)
            std::cerr << ' ' << position;
        std::cerr << '\n';
        return 1;
    }

    Sequencer::PatternSequencer evenConditionSequencer(processor.getValueTreeState());
    evenConditionSequencer.prepare(44100.0);
    evenConditionSequencer.clear();
    evenConditionSequencer.setStep(0, ratchetStep(1, 2));
    juce::MidiBuffer evenConditionMidi;
    evenConditionSequencer.process(evenConditionMidi, cycleSamples + oneStepSamples, 120.0, {});
    const auto evenConditionOns = noteOnPositions(evenConditionMidi);
    if (evenConditionOns.size() != 1 || ! near(evenConditionOns[0], cycleSamples, 32))
    {
        std::cerr << "Even-cycle condition did not wait for the second cycle:";
        for (const auto position : evenConditionOns)
            std::cerr << ' ' << position;
        std::cerr << '\n';
        return 1;
    }

    Sequencer::PatternSequencer fillConditionSequencer(processor.getValueTreeState());
    fillConditionSequencer.prepare(44100.0);
    fillConditionSequencer.clear();
    fillConditionSequencer.setStep(0, ratchetStep(1, 3));
    juce::MidiBuffer fillConditionMidi;
    fillConditionSequencer.process(fillConditionMidi, cycleSamples * 3 + oneStepSamples, 120.0, {});
    const auto fillConditionOns = noteOnPositions(fillConditionMidi);
    if (fillConditionOns.size() != 1 || ! near(fillConditionOns[0], cycleSamples * 3, 48))
    {
        std::cerr << "Fill condition did not trigger on the fourth cycle:";
        for (const auto position : fillConditionOns)
            std::cerr << ' ' << position;
        std::cerr << '\n';
        return 1;
    }

    Sequencer::PatternSequencer chainSequencer(processor.getValueTreeState());
    chainSequencer.prepare(44100.0);
    chainSequencer.clear();
    Sequencer::PatternSequencer::SceneChain chainScenes {};
    chainScenes[0][0] = ratchetStep(1);
    chainScenes[1][0] = ratchetStep(1);
    chainScenes[1][0].noteOffset = 12;
    chainSequencer.setSceneChain(chainScenes, 2);
    juce::MidiBuffer chainMidi;
    chainSequencer.process(chainMidi, cycleSamples + oneStepSamples, 120.0, {});
    const auto chainNoteOns = noteOnEvents(chainMidi);
    if (chainNoteOns.size() != 2
        || ! near(chainNoteOns[0].sample, 0, 1)
        || chainNoteOns[0].note != 60
        || ! near(chainNoteOns[1].sample, cycleSamples, 32)
        || chainNoteOns[1].note != 72)
    {
        std::cerr << "Live scene-chain playback did not alternate cached scene steps:";
        for (const auto& event : chainNoteOns)
            std::cerr << " (" << event.sample << ',' << event.note << ')';
        std::cerr << '\n';
        return 1;
    }

    Sequencer::PatternSequencer chainControlSequencer(processor.getValueTreeState());
    chainControlSequencer.prepare(44100.0);
    chainControlSequencer.clear();
    Sequencer::PatternSequencer::SceneChain controlScenes {};
    Sequencer::PatternSequencer::SceneControlChain controlChain {};
    controlScenes[0][0] = ratchetStep(1);
    controlScenes[1][0] = ratchetStep(1);
    controlChain[0].root = 48.0f;
    controlChain[0].gate = 0.42f;
    controlChain[0].chordMode = 0.0f;
    controlChain[0].probability = 1.0f;
    controlChain[1].root = 60.0f;
    controlChain[1].gate = 0.42f;
    controlChain[1].chordMode = 1.0f;
    controlChain[1].probability = 1.0f;
    chainControlSequencer.setSceneChain(controlScenes, controlChain, 2);
    juce::MidiBuffer chainControlMidi;
    chainControlSequencer.process(chainControlMidi, cycleSamples + oneStepSamples, 120.0, {});
    const auto chainControlNoteOns = noteOnEvents(chainControlMidi);
    if (chainControlNoteOns.size() != 3
        || ! near(chainControlNoteOns[0].sample, 0, 1)
        || chainControlNoteOns[0].note != 48
        || ! near(chainControlNoteOns[1].sample, cycleSamples, 32)
        || chainControlNoteOns[1].note != 60
        || ! near(chainControlNoteOns[2].sample, cycleSamples, 32)
        || chainControlNoteOns[2].note != 67)
    {
        std::cerr << "Live scene-chain playback did not apply cached scene controls:";
        for (const auto& event : chainControlNoteOns)
            std::cerr << " (" << event.sample << ',' << event.note << ')';
        std::cerr << '\n';
        return 1;
    }

    processor.clearSequencerPattern();
    processor.setSequencerStep(0, ratchetStep(3, 3));

    const auto outputFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getNonexistentChildFile("nate-vst-ratchet-audit", ".mid", false);
    if (! processor.exportSequencerMidiFile(outputFile))
    {
        std::cerr << "Ratchet MIDI export failed\n";
        return 1;
    }

    juce::FileInputStream input(outputFile);
    juce::MidiFile midiFile;
    if (! input.openedOk() || ! midiFile.readFrom(input))
    {
        std::cerr << "Could not read exported ratchet MIDI file\n";
        return 1;
    }

    std::vector<int> exportedNoteOns;
    for (auto trackIndex = 0; trackIndex < midiFile.getNumTracks(); ++trackIndex)
    {
        if (auto* track = midiFile.getTrack(trackIndex))
        {
            for (auto eventIndex = 0; eventIndex < track->getNumEvents(); ++eventIndex)
            {
                if (auto* event = track->getEventPointer(eventIndex))
                    if (event->message.isNoteOn())
                        exportedNoteOns.push_back(static_cast<int>(std::round(event->message.getTimeStamp())));
            }
        }
    }

    outputFile.deleteFile();

    if (exportedNoteOns.size() != 3
        || exportedNoteOns[0] != 0
        || exportedNoteOns[1] != 80
        || exportedNoteOns[2] != 160)
    {
        std::cerr << "Exported ratchet MIDI did not contain three 1/16-step subdivisions:";
        for (const auto tick : exportedNoteOns)
            std::cerr << ' ' << tick;
        std::cerr << '\n';
        return 1;
    }

    NateVSTAudioProcessor slideExportProcessor;
    if (! configureSequencer(slideExportProcessor))
    {
        std::cerr << "Could not configure sequencer parameters for slide export audit\n";
        return 1;
    }

    slideExportProcessor.clearSequencerPattern();
    slideExportProcessor.setSequencerStep(0, slideOutStep);
    slideExportProcessor.setSequencerStep(1, slideTargetStep);

    const auto slideExportFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getNonexistentChildFile("nate-vst-slide-audit", ".mid", false);
    if (! slideExportProcessor.exportSequencerMidiFile(slideExportFile))
    {
        std::cerr << "Slide MIDI export failed\n";
        return 1;
    }

    juce::FileInputStream slideInput(slideExportFile);
    juce::MidiFile slideMidiFile;
    if (! slideInput.openedOk() || ! slideMidiFile.readFrom(slideInput))
    {
        std::cerr << "Could not read exported slide MIDI file\n";
        return 1;
    }

    auto exportedFirstOff = -1;
    auto exportedSecondOn = -1;
    for (auto trackIndex = 0; trackIndex < slideMidiFile.getNumTracks(); ++trackIndex)
    {
        if (auto* track = slideMidiFile.getTrack(trackIndex))
        {
            for (auto eventIndex = 0; eventIndex < track->getNumEvents(); ++eventIndex)
            {
                if (auto* event = track->getEventPointer(eventIndex))
                {
                    if (event->message.getNoteNumber() == 60 && event->message.isNoteOff())
                        exportedFirstOff = static_cast<int>(std::round(event->message.getTimeStamp()));
                    if (event->message.getNoteNumber() == 67 && event->message.isNoteOn())
                        exportedSecondOn = static_cast<int>(std::round(event->message.getTimeStamp()));
                }
            }
        }
    }
    slideExportFile.deleteFile();

    if (exportedSecondOn < 0 || exportedFirstOff <= exportedSecondOn)
    {
        std::cerr << "Exported slide MIDI did not overlap the first note into the second note: off="
                  << exportedFirstOff << " secondOn=" << exportedSecondOn << '\n';
        return 1;
    }

    Sequencer::PatternSequencer fillChainSequencer(processor.getValueTreeState());
    fillChainSequencer.prepare(44100.0);
    fillChainSequencer.clear();
    Sequencer::PatternSequencer::SceneChain fillChainScenes {};
    fillChainScenes[0][0] = ratchetStep(1);
    fillChainScenes[1][0] = ratchetStep(1, 3);
    fillChainSequencer.setSceneChain(fillChainScenes, 2);
    juce::MidiBuffer fillChainMidi;
    fillChainSequencer.process(fillChainMidi, cycleSamples * 2 + oneStepSamples, 120.0, {});
    const auto fillChainOns = noteOnPositions(fillChainMidi);
    if (fillChainOns.size() != 3
        || ! near(fillChainOns[0], 0, 1)
        || ! near(fillChainOns[1], cycleSamples, 32)
        || ! near(fillChainOns[2], cycleSamples * 2, 48))
    {
        std::cerr << "Live two-scene chain fill condition did not match export-style final-scene behavior:";
        for (const auto position : fillChainOns)
            std::cerr << ' ' << position;
        std::cerr << '\n';
        return 1;
    }

    std::cout << "Sequencer ratchet/condition/slide audit passed for live playback, state restore, scenes, and MIDI export.\n";
    return 0;
}
