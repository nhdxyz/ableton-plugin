#include "SequencerPatternControls.h"

namespace UI
{
SequencerPatternControls::SequencerPatternControls(juce::AudioProcessorValueTreeState& valueTreeState)
{
    setComponentID("SequencerPatternControls");

    chordMemoryButton.setButtonText("Memory");
    chordMemoryButton.setTooltip("Expand live notes through the selected chord mode and voicing");
    chordMemoryAttachment = std::make_unique<ButtonAttachment>(valueTreeState,
                                                               Parameters::ID::sequencerChordMemory,
                                                               chordMemoryButton);
    addAndMakeVisible(chordMemoryButton);

    patternBox.addItem("Bass", 1);
    patternBox.addItem("Stab", 2);
    patternBox.addItem("UKG 2-Step", 3);
    patternBox.addItem("Shuffle Bass", 4);
    patternBox.addItem("Organ Skank", 5);
    patternBox.addItem("Vocal Chop", 6);
    patternBox.addItem("Late Stab", 7);
    patternBox.addItem("House Chord", 8);
    patternBox.addItem("Tech Bass", 9);
    patternBox.addItem("Minimal Pluck", 10);
    patternBox.addItem("Techno Pulse", 11);
    patternBox.addItem("Deep Chord", 12);
    patternBox.addItem("Dub Chord", 13);
    patternBox.addItem("Off Bass", 14);
    patternBox.addItem("Rolling Bass", 15);
    patternBox.setSelectedId(3, juce::dontSendNotification);
    addAndMakeVisible(patternBox);

    patternActions.onActionClicked = [this] (Action action)
    {
        if (onActionClicked != nullptr)
            onActionClicked(action);
    };
    addAndMakeVisible(patternActions);
}

int SequencerPatternControls::selectedPatternIndex() const
{
    return juce::jmax(1, patternBox.getSelectedId()) - 1;
}

void SequencerPatternControls::resized()
{
    auto area = getLocalBounds();
    auto patternRow = area.removeFromTop(34);
    chordMemoryButton.setBounds(patternRow.removeFromLeft(86).reduced(4));
    patternBox.setBounds(patternRow.reduced(4));
    patternActions.setBounds(area);
}
}
