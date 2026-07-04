#include "MacroAssignmentActions.h"

namespace UI
{
MacroAssignmentActions::MacroAssignmentActions()
{
    setComponentID("MacroAssignmentActions");

    configureButton(addButton, &MacroAssignmentActions::onAddClicked, "Add or update this macro assignment");
    configureButton(replaceButton, &MacroAssignmentActions::onReplaceClicked, "Replace all assignments for the selected macro with this one destination");
    configureButton(clearButton, &MacroAssignmentActions::onClearClicked, "Delete all routes owned by the selected macro");
    clearButton.setEnabled(false);
}

void MacroAssignmentActions::setState(bool selectedRouteExists, bool hasRoutesForSelectedMacro)
{
    addButton.setButtonText(selectedRouteExists ? "Update" : "Add");
    clearButton.setEnabled(hasRoutesForSelectedMacro);
}

void MacroAssignmentActions::resized()
{
    auto area = getLocalBounds();
    addButton.setBounds(area.removeFromLeft(64).reduced(3, 4));
    replaceButton.setBounds(area.removeFromLeft(78).reduced(3, 4));
    clearButton.setBounds(area.removeFromLeft(66).reduced(3, 4));
}

void MacroAssignmentActions::configureButton(juce::TextButton& button,
                                             std::function<void()> MacroAssignmentActions::* callback,
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
