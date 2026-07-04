#include "SequencerUtilityActions.h"

namespace UI
{
SequencerUtilityActions::SequencerUtilityActions()
{
    setComponentID("SequencerUtilityActions");

    configureButton(copyButton, Action::copy, "Copy the first half of the sequencer pattern to the second half");
    configureButton(rotateLeftButton, Action::rotateLeft, "Shift the whole sequencer pattern one step earlier");
    configureButton(rotateRightButton, Action::rotateRight, "Shift the whole sequencer pattern one step later");
    configureDragButton(exportMidiButton,
                        Action::exportMidi,
                        "Click to save the current sequencer pattern as MIDI, or drag into Ableton");
    configureDragButton(exportChainButton,
                        Action::exportChain,
                        "Click to save captured A/B/Fill/Drop scenes as one chained MIDI clip, or drag into Ableton");
}

void SequencerUtilityActions::resized()
{
    auto area = getLocalBounds();

    auto utilityRow = area.removeFromTop(area.getHeight() / 2).withTrimmedTop(2);
    copyButton.setBounds(utilityRow.removeFromLeft(utilityRow.getWidth() / 3).reduced(4));
    rotateLeftButton.setBounds(utilityRow.removeFromLeft(utilityRow.getWidth() / 2).reduced(4));
    rotateRightButton.setBounds(utilityRow.reduced(4));

    auto exportRow = area.withTrimmedTop(2);
    const auto exportWidth = juce::jmax(1, exportRow.getWidth() * 2 / 3);
    auto exportButtons = exportRow.removeFromLeft(exportWidth);
    exportMidiButton.setBounds(exportButtons.removeFromLeft(exportButtons.getWidth() / 2).reduced(4));
    exportChainButton.setBounds(exportButtons.reduced(4));
}

void SequencerUtilityActions::ExternalFileDragButton::mouseDown(const juce::MouseEvent& event)
{
    externalDragStarted = false;
    juce::TextButton::mouseDown(event);
}

void SequencerUtilityActions::ExternalFileDragButton::mouseDrag(const juce::MouseEvent& event)
{
    if (! externalDragStarted && event.getDistanceFromDragStart() >= 5)
    {
        externalDragStarted = true;
        if (onExternalDrag != nullptr && onExternalDrag(*this))
            return;
    }

    if (! externalDragStarted)
        juce::TextButton::mouseDrag(event);
}

void SequencerUtilityActions::ExternalFileDragButton::mouseUp(const juce::MouseEvent& event)
{
    const auto consumedByExternalDrag = externalDragStarted;
    externalDragStarted = false;
    if (! consumedByExternalDrag)
        juce::TextButton::mouseUp(event);
}

void SequencerUtilityActions::configureButton(juce::TextButton& button,
                                              Action action,
                                              const juce::String& tooltip)
{
    button.setTooltip(tooltip);
    button.setWantsKeyboardFocus(false);
    button.setMouseClickGrabsKeyboardFocus(false);
    button.onClick = [this, action]
    {
        if (onActionClicked != nullptr)
            onActionClicked(action);
    };
    addAndMakeVisible(button);
}

void SequencerUtilityActions::configureDragButton(ExternalFileDragButton& button,
                                                  Action action,
                                                  const juce::String& tooltip)
{
    configureButton(button, action, tooltip);
    button.onExternalDrag = [this, action] (juce::Component& sourceComponent)
    {
        return onExternalDrag != nullptr && onExternalDrag(action, sourceComponent);
    };
}
}
