#include "SequencerPatternActions.h"

namespace UI
{
SequencerPatternActions::SequencerPatternActions()
{
    setComponentID("SequencerPatternActions");

    configureButton(applyButton, Action::apply, "Load the selected pattern preset into the sequencer");
    configureButton(randomButton, Action::random, "Generate a new sequencer pattern");
    configureButton(mutateButton, Action::mutate, "Create a small variation of the current sequencer pattern");
    configureButton(undoButton, Action::undo, "Undo the last sequencer utility edit");
    configureButton(clearButton, Action::clear, "Clear the current sequencer pattern");
}

void SequencerPatternActions::resized()
{
    auto area = getLocalBounds();

    auto generateRow = area.removeFromTop(area.getHeight() / 2);
    applyButton.setBounds(generateRow.removeFromLeft(generateRow.getWidth() / 2).reduced(4));
    randomButton.setBounds(generateRow.reduced(4));

    auto editRow = area;
    mutateButton.setBounds(editRow.removeFromLeft(editRow.getWidth() / 3).reduced(4));
    undoButton.setBounds(editRow.removeFromLeft(editRow.getWidth() / 2).reduced(4));
    clearButton.setBounds(editRow.reduced(4));
}

void SequencerPatternActions::configureButton(juce::TextButton& button,
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
}
