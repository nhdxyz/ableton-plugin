#include "PresetCompareActions.h"

namespace UI
{
PresetCompareActions::PresetCompareActions()
{
    setComponentID("PresetCompareActions");

    configureButton(compareButton, &PresetCompareActions::onCompareClicked, "Load a preset to compare it against the sound that was active before loading");
    configureButton(revertButton, &PresetCompareActions::onRevertClicked, "Restore the sound from before the most recent Library preset load");
}

void PresetCompareActions::setState(bool hasSnapshots, bool showingLoaded, const juce::String& presetName)
{
    compareButton.setEnabled(hasSnapshots);
    revertButton.setEnabled(hasSnapshots);
    compareButton.setButtonText(showingLoaded ? "Before" : "Loaded");

    if (! hasSnapshots)
    {
        compareButton.setTooltip("Load a preset to compare it against the sound that was active before loading");
        revertButton.setTooltip("Restore becomes available after a Library preset load");
        return;
    }

    compareButton.setTooltip(showingLoaded
                                 ? "Hear the sound that was active before loading " + presetName
                                 : "Return to the loaded preset " + presetName);
    revertButton.setTooltip("Restore the sound from before loading " + presetName + " and clear this compare pair");
}

void PresetCompareActions::resized()
{
    auto area = getLocalBounds();
    compareButton.setBounds(area.removeFromLeft(82).reduced(2, 4));
    revertButton.setBounds(area.removeFromLeft(82).reduced(2, 4));
}

void PresetCompareActions::configureButton(juce::TextButton& button,
                                           std::function<void()> PresetCompareActions::* callback,
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
