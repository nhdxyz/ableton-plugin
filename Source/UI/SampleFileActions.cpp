#include "SampleFileActions.h"

namespace UI
{
SampleFileActions::SampleFileActions()
{
    setComponentID("SampleFileActions");

    loadButton.setTooltip("Load a WAV or AIFF sample into the sampler");
    loadButton.setWantsKeyboardFocus(false);
    loadButton.setMouseClickGrabsKeyboardFocus(false);
    loadButton.onClick = [this]
    {
        if (onLoadClicked != nullptr)
            onLoadClicked();
    };
    addAndMakeVisible(loadButton);

    clearButton.setTooltip("Clear the currently loaded sample");
    clearButton.setWantsKeyboardFocus(false);
    clearButton.setMouseClickGrabsKeyboardFocus(false);
    clearButton.onClick = [this]
    {
        if (onClearClicked != nullptr)
            onClearClicked();
    };
    addAndMakeVisible(clearButton);
}

void SampleFileActions::resized()
{
    auto area = getLocalBounds();
    loadButton.setBounds(area.removeFromLeft(area.getWidth() / 2).reduced(3, 4));
    clearButton.setBounds(area.reduced(3, 4));
}
}
