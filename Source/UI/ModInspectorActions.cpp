#include "ModInspectorActions.h"

namespace UI
{
ModInspectorActions::ModInspectorActions()
{
    setComponentID("ModInspectorActions");

    configureButton(addButton, &ModInspectorActions::onAddClicked, "Add the selected source to the inspected destination");
    configureButton(clearButton, &ModInspectorActions::onClearClicked, "Delete all active routes targeting the inspected destination");
    clearButton.setEnabled(false);
}

void ModInspectorActions::setClearEnabled(bool shouldBeEnabled)
{
    clearButton.setEnabled(shouldBeEnabled);
}

void ModInspectorActions::resized()
{
    auto area = getLocalBounds();
    addButton.setBounds(area.removeFromLeft(58).reduced(3, 4));
    clearButton.setBounds(area.removeFromLeft(66).reduced(3, 4));
}

void ModInspectorActions::configureButton(juce::TextButton& button,
                                          std::function<void()> ModInspectorActions::* callback,
                                          const juce::String& tooltip)
{
    button.setTooltip(tooltip);
    button.setWantsKeyboardFocus(false);
    button.setMouseClickGrabsKeyboardFocus(false);
    button.onClick = [this, callback]
    {
        if ((this->*callback) != nullptr)
            (this->*callback)();
    };
    addAndMakeVisible(button);
}
}
