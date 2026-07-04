#include "SamplePlaybackControls.h"

namespace UI
{
SamplePlaybackControls::SamplePlaybackControls(juce::AudioProcessorValueTreeState& valueTreeState)
{
    setComponentID("SamplePlaybackControls");

    modeBox.addItem("Gate", 1);
    modeBox.addItem("One Shot", 2);
    modeBox.addItem("Slice Keys", 3);
    modeBox.setTextWhenNothingSelected("Mode");
    modeBox.setTooltip("Gate follows note-off, One Shot plays the selected range, Slice Keys maps MIDI notes C3-G3 across the eight stored slice pads and repeats every octave");
    addAndMakeVisible(modeBox);
    modeAttachment = std::make_unique<ComboBoxAttachment>(valueTreeState, Parameters::ID::samplePlaybackMode, modeBox);

    engineBox.addItemList(Parameters::sampleEngineModeChoices(), 1);
    engineBox.setTextWhenNothingSelected("Engine");
    engineBox.setTooltip("Classic playback, Granular spray, Spectral freeze/smear, or Cloud grain playback for recorded/imported snippets");
    addAndMakeVisible(engineBox);
    engineAttachment = std::make_unique<ComboBoxAttachment>(valueTreeState, Parameters::ID::sampleEngineMode, engineBox);

    sliceStyleBox.addItemList(Parameters::sampleSliceStyleChoices(), 1);
    sliceStyleBox.setTextWhenNothingSelected("Slice Style");
    sliceStyleBox.setTooltip("Choose how default slice pads set pitch, reverse, choke, and stutter behavior");
    addAndMakeVisible(sliceStyleBox);
    sliceStyleAttachment = std::make_unique<ComboBoxAttachment>(valueTreeState, Parameters::ID::sampleSliceStyle, sliceStyleBox);

    stutterEnabledButton.setButtonText("Stutter");
    addAndMakeVisible(stutterEnabledButton);
    stutterEnabledAttachment = std::make_unique<ButtonAttachment>(valueTreeState,
                                                                 Parameters::ID::sampleStutterEnabled,
                                                                 stutterEnabledButton);

    stutterRateBox.addItem("1/8", 1);
    stutterRateBox.addItem("1/16", 2);
    stutterRateBox.addItem("1/32", 3);
    stutterRateBox.setTextWhenNothingSelected("Rate");
    addAndMakeVisible(stutterRateBox);
    stutterRateAttachment = std::make_unique<ComboBoxAttachment>(valueTreeState,
                                                                Parameters::ID::sampleStutterRate,
                                                                stutterRateBox);
}

int SamplePlaybackControls::getSliceStyleSelectedItemIndex() const
{
    return sliceStyleBox.getSelectedItemIndex();
}

juce::String SamplePlaybackControls::getSliceStyleText() const
{
    return sliceStyleBox.getText();
}

void SamplePlaybackControls::resized()
{
    auto area = getLocalBounds();

    auto modeRow = area.removeFromTop(30);
    modeBox.setBounds(modeRow.removeFromLeft(modeRow.getWidth() / 2).reduced(4));
    engineBox.setBounds(modeRow.reduced(4));

    sliceStyleBox.setBounds(area.removeFromTop(30).reduced(4));

    auto stutterRow = area.removeFromTop(30);
    stutterEnabledButton.setBounds(stutterRow.removeFromLeft(stutterRow.getWidth() / 2).reduced(3, 4));
    stutterRateBox.setBounds(stutterRow.reduced(3, 4));
}
}
