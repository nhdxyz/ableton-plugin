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

struct ExportedNoteOn
{
    int tick = 0;
    int note = -1;
};

Sequencer::Step sceneStep(int note, float velocity, float probability, float timing, float length, float lock, int ratchet = 1, int condition = 0)
{
    Sequencer::Step step;
    step.enabled = true;
    step.noteOffset = note;
    step.velocity = velocity;
    step.probability = probability;
    step.timing = timing;
    step.length = length;
    step.lock = lock;
    step.ratchet = ratchet;
    step.condition = condition;
    return step;
}

std::vector<ExportedNoteOn> exportedNoteOns(const juce::File& file)
{
    std::vector<ExportedNoteOn> notes;
    juce::FileInputStream input(file);
    juce::MidiFile midiFile;
    if (! input.openedOk() || ! midiFile.readFrom(input))
        return notes;

    for (auto trackIndex = 0; trackIndex < midiFile.getNumTracks(); ++trackIndex)
    {
        if (auto* track = midiFile.getTrack(trackIndex))
        {
            for (auto eventIndex = 0; eventIndex < track->getNumEvents(); ++eventIndex)
            {
                if (auto* event = track->getEventPointer(eventIndex))
                    if (event->message.isNoteOn())
                        notes.push_back({ static_cast<int>(std::round(event->message.getTimeStamp())),
                                          event->message.getNoteNumber() });
            }
        }
    }

    return notes;
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
    processor.setSequencerStep(6, sceneStep(7, 0.68f, 0.78f, 0.62f, 0.44f, 0.5f, 2, 1));
    processor.captureSequencerPatternScene(0);

    if (! processor.hasSequencerPatternScene(0)
        || processor.hasSequencerPatternScene(2)
        || ! processor.getSequencerPatternSceneSummary(0).containsIgnoreCase("chord")
        || ! processor.getSequencerPatternSceneSummary(0).containsIgnoreCase("groove")
        || ! processor.getSequencerPatternSceneSummary(0).containsIgnoreCase("rats")
        || ! processor.getSequencerPatternSceneSummary(0).containsIgnoreCase("conds"))
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
    processor.setSequencerStep(3, sceneStep(2, 0.44f, 0.62f, 0.72f, 0.28f, 0.64f, 3, 3));
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
        || recalledA6.ratchet != 2
        || recalledA6.condition != 1
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
        || restoredB3.ratchet != 3
        || restoredB3.condition != 3
        || ! near(readPlainParameter(restored, Parameters::ID::sequencerChordMode, 0.0f), 0.0f)
        || ! near(readPlainParameter(restored, Parameters::ID::sequencerLockDestination, 0.0f), 1.0f))
    {
        std::cerr << "Restored scene B did not restore expected sequence data\n";
        return 1;
    }

    NateVSTAudioProcessor chainProcessor;
    if (! setPlainParameter(chainProcessor, Parameters::ID::sequencerEnabled, 1.0f)
        || ! setPlainParameter(chainProcessor, Parameters::ID::sequencerRate, 1.0f)
        || ! setPlainParameter(chainProcessor, Parameters::ID::sequencerRoot, 60.0f)
        || ! setPlainParameter(chainProcessor, Parameters::ID::sequencerGate, 0.5f)
        || ! setPlainParameter(chainProcessor, Parameters::ID::sequencerSwing, 0.0f)
        || ! setPlainParameter(chainProcessor, Parameters::ID::sequencerGrooveMode, 0.0f)
        || ! setPlainParameter(chainProcessor, Parameters::ID::sequencerScale, 0.0f)
        || ! setPlainParameter(chainProcessor, Parameters::ID::sequencerChordMode, 0.0f)
        || ! setPlainParameter(chainProcessor, Parameters::ID::sequencerChordVoicing, 0.0f)
        || ! setPlainParameter(chainProcessor, Parameters::ID::sequencerChordStrum, 0.0f)
        || ! setPlainParameter(chainProcessor, Parameters::ID::sequencerAccent, 0.0f)
        || ! setPlainParameter(chainProcessor, Parameters::ID::sequencerOctave, 0.0f)
        || ! setPlainParameter(chainProcessor, Parameters::ID::sequencerProbability, 1.0f))
    {
        std::cerr << "Could not configure chain export processor\n";
        return 1;
    }

    chainProcessor.clearSequencerPattern();
    chainProcessor.setSequencerStep(0, sceneStep(0, 0.9f, 1.0f, 0.0f, 0.8f, 0.0f, 1, 1));
    chainProcessor.captureSequencerPatternScene(0);
    chainProcessor.clearSequencerPattern();
    chainProcessor.setSequencerStep(0, sceneStep(12, 0.8f, 1.0f, 0.0f, 0.8f, 0.0f, 1, 2));
    chainProcessor.captureSequencerPatternScene(1);
    chainProcessor.setSequencerSceneChainPlaybackEnabled(true);

    if (! chainProcessor.isSequencerSceneChainPlaybackEnabled()
        || chainProcessor.getSequencerSceneChainPlaybackLength() != 2)
    {
        std::cerr << "Live scene-chain playback did not enable with two captured scenes\n";
        return 1;
    }

    juce::MemoryBlock chainState;
    chainProcessor.getStateInformation(chainState);
    NateVSTAudioProcessor restoredChainProcessor;
    restoredChainProcessor.setStateInformation(chainState.getData(), static_cast<int>(chainState.getSize()));
    if (! restoredChainProcessor.isSequencerSceneChainPlaybackEnabled()
        || restoredChainProcessor.getSequencerSceneChainPlaybackLength() != 2)
    {
        std::cerr << "Live scene-chain playback state did not restore\n";
        return 1;
    }

    const auto chainFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getNonexistentChildFile("nate-vst-scene-chain-audit", ".mid", false);
    if (! chainProcessor.exportSequencerSceneChainMidiFile(chainFile))
    {
        std::cerr << "Scene chain MIDI export failed\n";
        return 1;
    }

    const auto chainNotes = exportedNoteOns(chainFile);
    chainFile.deleteFile();
    if (chainNotes.size() != 2
        || chainNotes[0].tick != 0
        || chainNotes[0].note != 60
        || chainNotes[1].tick != 3840
        || chainNotes[1].note != 72)
    {
        std::cerr << "Scene chain MIDI export did not place A/B scenes at expected bar offsets:";
        for (const auto& note : chainNotes)
            std::cerr << " (" << note.tick << ',' << note.note << ')';
        std::cerr << '\n';
        return 1;
    }

    chainProcessor.setSequencerSceneChainClipBars(4);
    if (chainProcessor.getSequencerSceneChainClipBars() != 4
        || chainProcessor.getSequencerSceneChainPlaybackLength() != 4)
    {
        std::cerr << "Forced 4-bar scene-chain mode did not report four effective bars\n";
        return 1;
    }

    juce::MemoryBlock forcedChainState;
    chainProcessor.getStateInformation(forcedChainState);
    NateVSTAudioProcessor restoredForcedChain;
    restoredForcedChain.setStateInformation(forcedChainState.getData(), static_cast<int>(forcedChainState.getSize()));
    if (! restoredForcedChain.isSequencerSceneChainPlaybackEnabled()
        || restoredForcedChain.getSequencerSceneChainClipBars() != 4
        || restoredForcedChain.getSequencerSceneChainPlaybackLength() != 4)
    {
        std::cerr << "Forced 4-bar scene-chain mode did not restore\n";
        return 1;
    }

    const auto forcedFourFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getNonexistentChildFile("nate-vst-forced-four-chain-audit", ".mid", false);
    if (! restoredForcedChain.exportSequencerSceneChainMidiFile(forcedFourFile))
    {
        std::cerr << "Forced 4-bar scene-chain MIDI export failed\n";
        return 1;
    }

    const auto forcedFourNotes = exportedNoteOns(forcedFourFile);
    forcedFourFile.deleteFile();
    if (forcedFourNotes.size() != 4
        || forcedFourNotes[0].tick != 0
        || forcedFourNotes[0].note != 60
        || forcedFourNotes[1].tick != 3840
        || forcedFourNotes[1].note != 72
        || forcedFourNotes[2].tick != 7680
        || forcedFourNotes[2].note != 60
        || forcedFourNotes[3].tick != 11520
        || forcedFourNotes[3].note != 72)
    {
        std::cerr << "Forced 4-bar scene-chain MIDI export did not repeat A/B as A/B/A/B:";
        for (const auto& note : forcedFourNotes)
            std::cerr << " (" << note.tick << ',' << note.note << ')';
        std::cerr << '\n';
        return 1;
    }

    chainProcessor.setSequencerSceneChainClipBars(2);
    const auto forcedTwoFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getNonexistentChildFile("nate-vst-forced-two-chain-audit", ".mid", false);
    if (! chainProcessor.exportSequencerSceneChainMidiFile(forcedTwoFile))
    {
        std::cerr << "Forced 2-bar scene-chain MIDI export failed\n";
        return 1;
    }

    const auto forcedTwoNotes = exportedNoteOns(forcedTwoFile);
    forcedTwoFile.deleteFile();
    if (forcedTwoNotes.size() != 2
        || forcedTwoNotes[0].tick != 0
        || forcedTwoNotes[0].note != 60
        || forcedTwoNotes[1].tick != 3840
        || forcedTwoNotes[1].note != 72)
    {
        std::cerr << "Forced 2-bar scene-chain MIDI export did not keep A/B only:";
        for (const auto& note : forcedTwoNotes)
            std::cerr << " (" << note.tick << ',' << note.note << ')';
        std::cerr << '\n';
        return 1;
    }

    chainProcessor.setSequencerSceneChainClipBars(0);

    NateVSTAudioProcessor chainBuilder;
    if (! setPlainParameter(chainBuilder, Parameters::ID::sequencerEnabled, 1.0f)
        || ! setPlainParameter(chainBuilder, Parameters::ID::sequencerRate, 1.0f)
        || ! setPlainParameter(chainBuilder, Parameters::ID::sequencerRoot, 60.0f)
        || ! setPlainParameter(chainBuilder, Parameters::ID::sequencerGate, 0.52f)
        || ! setPlainParameter(chainBuilder, Parameters::ID::sequencerSwing, 0.18f)
        || ! setPlainParameter(chainBuilder, Parameters::ID::sequencerGrooveMode, 4.0f)
        || ! setPlainParameter(chainBuilder, Parameters::ID::sequencerScale, 0.0f)
        || ! setPlainParameter(chainBuilder, Parameters::ID::sequencerChordMode, 0.0f)
        || ! setPlainParameter(chainBuilder, Parameters::ID::sequencerChordVoicing, 0.0f)
        || ! setPlainParameter(chainBuilder, Parameters::ID::sequencerChordStrum, 0.0f)
        || ! setPlainParameter(chainBuilder, Parameters::ID::sequencerAccent, 0.54f)
        || ! setPlainParameter(chainBuilder, Parameters::ID::sequencerOctave, 0.0f)
        || ! setPlainParameter(chainBuilder, Parameters::ID::sequencerProbability, 1.0f)
        || ! setPlainParameter(chainBuilder, Parameters::ID::sequencerLockDepth, 0.3f))
    {
        std::cerr << "Could not configure chain builder processor\n";
        return 1;
    }

    chainBuilder.clearSequencerPattern();
    chainBuilder.setSequencerStep(0, sceneStep(0, 0.9f, 1.0f, 0.0f, 0.7f, 0.0f));
    chainBuilder.setSequencerStep(6, sceneStep(-5, 0.72f, 0.92f, 0.22f, 0.46f, 0.24f));
    chainBuilder.setSequencerStep(11, sceneStep(3, 0.66f, 0.84f, 0.48f, 0.42f, 0.3f));

    if (! chainBuilder.applySequencerGrooveTransform(12))
    {
        std::cerr << "4-bar scene-chain builder transform failed\n";
        return 1;
    }

    if (! chainBuilder.isSequencerSceneChainPlaybackEnabled()
        || chainBuilder.getSequencerSceneChainPlaybackLength() != 4)
    {
        std::cerr << "4-bar scene-chain builder did not enable a four-scene live chain\n";
        return 1;
    }

    for (auto slot = 0; slot < 4; ++slot)
    {
        if (! chainBuilder.hasSequencerPatternScene(slot))
        {
            std::cerr << "4-bar scene-chain builder missed scene slot " << slot << '\n';
            return 1;
        }
    }

    NateVSTAudioProcessor undoBuilder;
    undoBuilder.clearSequencerPattern();
    undoBuilder.setSequencerStep(0, sceneStep(0, 0.9f, 1.0f, 0.0f, 0.7f, 0.0f));
    undoBuilder.setSequencerStep(6, sceneStep(-5, 0.72f, 0.92f, 0.22f, 0.46f, 0.24f));
    if (! undoBuilder.applySequencerGrooveTransform(12) || ! undoBuilder.undoSequencerEdit())
    {
        std::cerr << "4-bar scene-chain builder did not participate in sequencer undo\n";
        return 1;
    }

    if (undoBuilder.isSequencerSceneChainPlaybackEnabled()
        || undoBuilder.getSequencerSceneChainPlaybackLength() != 0
        || undoBuilder.hasSequencerPatternScene(0)
        || undoBuilder.hasSequencerPatternScene(1)
        || undoBuilder.hasSequencerPatternScene(2)
        || undoBuilder.hasSequencerPatternScene(3))
    {
        std::cerr << "Sequencer undo did not restore pre-chain scene/live state\n";
        return 1;
    }

    if (! chainBuilder.recallSequencerPatternScene(1)
        || ! chainBuilder.getSequencerStep(7).enabled
        || chainBuilder.getSequencerStep(7).timing < 0.55f
        || chainBuilder.getSequencerStep(7).noteOffset != 7)
    {
        std::cerr << "4-bar scene-chain builder did not create the B pickup variation\n";
        return 1;
    }

    if (! chainBuilder.recallSequencerPatternScene(2)
        || chainBuilder.getSequencerStep(14).ratchet < 3
        || chainBuilder.getSequencerStep(15).ratchet < 2)
    {
        std::cerr << "4-bar scene-chain builder did not create the fill ratchets\n";
        return 1;
    }

    if (! chainBuilder.recallSequencerPatternScene(3)
        || chainBuilder.getSequencerStep(15).condition != 3
        || chainBuilder.getSequencerStep(12).velocity < 0.9f)
    {
        std::cerr << "4-bar scene-chain builder did not create the drop/fill-cycle gestures\n";
        return 1;
    }

    juce::MemoryBlock builtChainState;
    chainBuilder.getStateInformation(builtChainState);
    NateVSTAudioProcessor restoredBuilder;
    restoredBuilder.setStateInformation(builtChainState.getData(), static_cast<int>(builtChainState.getSize()));
    if (! restoredBuilder.isSequencerSceneChainPlaybackEnabled()
        || restoredBuilder.getSequencerSceneChainPlaybackLength() != 4)
    {
        std::cerr << "4-bar built chain did not restore live-chain state\n";
        return 1;
    }

    const auto builtChainFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getNonexistentChildFile("nate-vst-built-scene-chain-audit", ".mid", false);
    if (! restoredBuilder.exportSequencerSceneChainMidiFile(builtChainFile))
    {
        std::cerr << "Built 4-bar scene-chain MIDI export failed\n";
        return 1;
    }

    const auto builtChainNotes = exportedNoteOns(builtChainFile);
    builtChainFile.deleteFile();
    auto hasNoteAt = [&builtChainNotes] (int tick, int note)
    {
        for (const auto& exportedNote : builtChainNotes)
            if (exportedNote.tick == tick && exportedNote.note == note)
                return true;

        return false;
    };

    if (! hasNoteAt(0, 60)
        || ! hasNoteAt(3840, 60)
        || ! hasNoteAt(7680, 60)
        || ! hasNoteAt(11520, 60))
    {
        std::cerr << "Built 4-bar scene-chain MIDI export did not preserve bar-start anchors:";
        for (const auto& note : builtChainNotes)
            std::cerr << " (" << note.tick << ',' << note.note << ')';
        std::cerr << '\n';
        return 1;
    }

    std::cout << "Sequencer pattern scene audit passed.\n";
    return 0;
}
