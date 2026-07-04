#include "FxRackOrderControls.h"

namespace UI
{
FxRackOrderControls::FxRackOrderControls()
{
    setComponentID("FxRackOrderControls");

    moveUpButton.setTooltip("Move the selected FX module earlier in the rack");
    moveUpButton.setWantsKeyboardFocus(false);
    moveUpButton.setMouseClickGrabsKeyboardFocus(false);
    moveUpButton.onClick = [this]
    {
        if (onMoveRequested != nullptr)
            onMoveRequested(-1);
    };
    addAndMakeVisible(moveUpButton);

    moveDownButton.setTooltip("Move the selected FX module later in the rack");
    moveDownButton.setWantsKeyboardFocus(false);
    moveDownButton.setMouseClickGrabsKeyboardFocus(false);
    moveDownButton.onClick = [this]
    {
        if (onMoveRequested != nullptr)
            onMoveRequested(1);
    };
    addAndMakeVisible(moveDownButton);

    resetButton.setTooltip("Reset the FX rack order");
    resetButton.setWantsKeyboardFocus(false);
    resetButton.setMouseClickGrabsKeyboardFocus(false);
    resetButton.onClick = [this]
    {
        if (onResetRequested != nullptr)
            onResetRequested();
    };
    addAndMakeVisible(resetButton);

    removeButton.setTooltip("Remove the selected FX module from the visible rack");
    removeButton.setWantsKeyboardFocus(false);
    removeButton.setMouseClickGrabsKeyboardFocus(false);
    removeButton.onClick = [this]
    {
        if (onRemoveRequested != nullptr)
            onRemoveRequested();
    };
    addAndMakeVisible(removeButton);
}

void FxRackOrderControls::setButtonStates(bool canMoveUp, bool canMoveDown, bool canRemove)
{
    moveUpButton.setEnabled(canMoveUp);
    moveDownButton.setEnabled(canMoveDown);
    removeButton.setEnabled(canRemove);
}

void FxRackOrderControls::resized()
{
    auto area = getLocalBounds();
    moveUpButton.setBounds(area.removeFromLeft(52).reduced(4));
    moveDownButton.setBounds(area.removeFromLeft(58).reduced(4));
    resetButton.setBounds(area.removeFromLeft(72).reduced(4));
    removeButton.setBounds(area.removeFromLeft(86).reduced(4));
}
}
