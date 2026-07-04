#include "SequencerGrooveControls.h"

namespace UI
{
SequencerGrooveControls::SequencerGrooveControls(juce::AudioProcessorValueTreeState& valueTreeState)
{
    setComponentID("SequencerGrooveControls");

    configureCombo(grooveBox, Parameters::sequencerGrooveModeChoices(), "Groove");
    configureCombo(scaleBox, Parameters::sequencerScaleChoices(), "Scale");
    configureCombo(chordBox, Parameters::sequencerChordModeChoices(), "Chord");
    configureCombo(voicingBox, Parameters::sequencerChordVoicingChoices(), "Voice");

    grooveAttachment = std::make_unique<ComboBoxAttachment>(valueTreeState, Parameters::ID::sequencerGrooveMode, grooveBox);
    scaleAttachment = std::make_unique<ComboBoxAttachment>(valueTreeState, Parameters::ID::sequencerScale, scaleBox);
    chordAttachment = std::make_unique<ComboBoxAttachment>(valueTreeState, Parameters::ID::sequencerChordMode, chordBox);
    voicingAttachment = std::make_unique<ComboBoxAttachment>(valueTreeState, Parameters::ID::sequencerChordVoicing, voicingBox);

    addAndMakeVisible(grooveBox);
    addAndMakeVisible(scaleBox);
    addAndMakeVisible(chordBox);
    addAndMakeVisible(voicingBox);
}

void SequencerGrooveControls::configureCombo(juce::ComboBox& box,
                                             const juce::StringArray& choices,
                                             const juce::String& emptyText)
{
    box.addItemList(choices, 1);
    box.setTextWhenNothingSelected(emptyText);
}

void SequencerGrooveControls::resized()
{
    auto area = getLocalBounds();
    auto grooveRow = area.removeFromTop(area.getHeight() / 2);
    grooveBox.setBounds(grooveRow.removeFromLeft(grooveRow.getWidth() / 2).reduced(4));
    scaleBox.setBounds(grooveRow.reduced(4));

    chordBox.setBounds(area.removeFromLeft(area.getWidth() / 2).reduced(4));
    voicingBox.setBounds(area.reduced(4));
}
}
