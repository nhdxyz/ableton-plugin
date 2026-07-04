#include "SampleRecipeActions.h"

namespace UI
{
SampleRecipeActions::SampleRecipeActions()
{
    setComponentID("SampleRecipeActions");

    randomCutButton.setTooltip("Randomize the active sample start and end points");
    randomCutButton.setWantsKeyboardFocus(false);
    randomCutButton.setMouseClickGrabsKeyboardFocus(false);
    randomCutButton.onClick = [this]
    {
        if (onRandomCutClicked != nullptr)
            onRandomCutClicked();
    };
    addAndMakeVisible(randomCutButton);

    ukgChopButton.setTooltip("Build a UK garage-style sample chop and sequencer pattern");
    ukgChopButton.setWantsKeyboardFocus(false);
    ukgChopButton.setMouseClickGrabsKeyboardFocus(false);
    ukgChopButton.onClick = [this]
    {
        if (onUkgChopClicked != nullptr)
            onUkgChopClicked();
    };
    addAndMakeVisible(ukgChopButton);
}

void SampleRecipeActions::resized()
{
    auto area = getLocalBounds();
    randomCutButton.setBounds(area.removeFromLeft(area.getWidth() / 2).reduced(3, 4));
    ukgChopButton.setBounds(area.reduced(3, 4));
}
}
