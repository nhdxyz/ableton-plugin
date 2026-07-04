#include "SequencerTransformControls.h"

namespace UI
{
SequencerTransformControls::SequencerTransformControls(juce::AudioProcessorValueTreeState& valueTreeState)
{
    setComponentID("SequencerTransformControls");

    lockDestinationBox.addItemList(Parameters::sequencerLockDestinationChoices(), 1);
    lockDestinationBox.setTextWhenNothingSelected("Lock");
    lockDestinationBox.setTooltip("Choose which safe synth or FX destination the per-step Lock lane moves");
    lockDestinationAttachment = std::make_unique<ComboBoxAttachment>(valueTreeState,
                                                                     Parameters::ID::sequencerLockDestination,
                                                                     lockDestinationBox);
    addAndMakeVisible(lockDestinationBox);

    grooveTransformBox.addItem("Tighten", 1);
    grooveTransformBox.addItem("Straight Anchors", 2);
    grooveTransformBox.addItem("Swung Ghosts", 3);
    grooveTransformBox.addItem("Late Stabs", 4);
    grooveTransformBox.addItem("Vocal Push", 5);
    grooveTransformBox.addItem("Humanize", 6);
    grooveTransformBox.addSectionHeading("House Templates");
    grooveTransformBox.addItem("House Shuffle", 7);
    grooveTransformBox.addItem("UKG 2-Step Push", 8);
    grooveTransformBox.addItem("Tech House Tight", 9);
    grooveTransformBox.addItem("Minimal Skip", 10);
    grooveTransformBox.addItem("Techno Drive", 11);
    grooveTransformBox.addItem("Bass Contour", 12);
    grooveTransformBox.addSectionHeading("Paint Tools");
    grooveTransformBox.addItem("Chord Stab Paint", 14);
    grooveTransformBox.addSectionHeading("Chain Tools");
    grooveTransformBox.addItem("Build 4-Bar Chain", 13);
    grooveTransformBox.setSelectedId(1, juce::dontSendNotification);
    grooveTransformBox.setTooltip("Choose a timing transform or genre groove template for the current sequence");
    addAndMakeVisible(grooveTransformBox);

    applyButton.setTooltip("Apply the selected groove transform to the current sequence");
    applyButton.onClick = [this]
    {
        if (onApplyTransform != nullptr)
            onApplyTransform(juce::jmax(1, grooveTransformBox.getSelectedId()) - 1,
                             grooveTransformBox.getText());
    };
    addAndMakeVisible(applyButton);
}

void SequencerTransformControls::resized()
{
    auto area = getLocalBounds();
    auto transformRow = area.removeFromTop(34).withTrimmedTop(2);
    lockDestinationBox.setBounds(transformRow.removeFromLeft(transformRow.getWidth() / 2).reduced(4));
    grooveTransformBox.setBounds(transformRow.reduced(4));
    applyButton.setBounds(area.reduced(4));
}
}
