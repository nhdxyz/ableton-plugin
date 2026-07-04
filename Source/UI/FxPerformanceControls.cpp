#include "FxPerformanceControls.h"

namespace UI
{
FxPerformanceControls::FxPerformanceControls()
{
    setComponentID("FxPerformanceControls");

    configureClickButton(delayThrowButton, Action::delayThrow, "Apply a tempo delay throw");
    configureClickButton(spaceThrowButton, Action::spaceThrow, "Apply a delay and reverb wash");
    configureClickButton(pumpDropButton, Action::pumpDrop, "Apply a temporary pump/drop gesture");
    configureClickButton(dryResetButton, Action::dryReset, "Clear active FX throws and return to dry movement");

    configureHoldButton(holdDelayButton, Action::holdDelay, "Hold for a temporary delay throw, release to restore");
    configureHoldButton(holdSpaceButton, Action::holdSpace, "Hold for a temporary delay and reverb wash, release to restore");
    configureHoldButton(holdPumpButton, Action::holdPump, "Hold for a temporary pump/duck move, release to restore");
    configureHoldButton(muteDropButton, Action::muteDrop, "Hold for a temporary mute drop, release to restore");
}

void FxPerformanceControls::resized()
{
    auto area = getLocalBounds();
    delayThrowButton.setBounds(area.removeFromLeft(102).reduced(4));
    spaceThrowButton.setBounds(area.removeFromLeft(106).reduced(4));
    pumpDropButton.setBounds(area.removeFromLeft(96).reduced(4));
    dryResetButton.setBounds(area.removeFromLeft(88).reduced(4));
    holdDelayButton.setBounds(area.removeFromLeft(84).reduced(4));
    holdSpaceButton.setBounds(area.removeFromLeft(84).reduced(4));
    holdPumpButton.setBounds(area.removeFromLeft(92).reduced(4));
    muteDropButton.setBounds(area.removeFromLeft(90).reduced(4));
}

void FxPerformanceControls::configureClickButton(juce::TextButton& button, Action action, const juce::String& tooltip)
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

void FxPerformanceControls::configureHoldButton(juce::TextButton& button, Action action, const juce::String& tooltip)
{
    button.setTooltip(tooltip);
    button.setWantsKeyboardFocus(false);
    button.setMouseClickGrabsKeyboardFocus(false);
    button.onStateChange = [this, buttonPtr = &button, action]
    {
        if (onMomentaryActionChanged != nullptr)
            onMomentaryActionChanged(action, buttonPtr->isDown());
    };
    addAndMakeVisible(button);
}
}
