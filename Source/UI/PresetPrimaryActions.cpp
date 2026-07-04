#include "PresetPrimaryActions.h"

namespace UI
{
PresetPrimaryActions::PresetPrimaryActions()
{
    setComponentID("PresetPrimaryActions");

    configureButton(loadButton, &PresetPrimaryActions::onLoadClicked, "Load the selected preset");
    configureButton(auditionButton, &PresetPrimaryActions::onAuditionClicked, "Play the selected preset preview without loading the patch");
    configureButton(warmPreviewsButton, &PresetPrimaryActions::onWarmPreviewsClicked, "Render previews for the selected and visible preset rows without loading patches");
    configureButton(favoriteButton, &PresetPrimaryActions::onFavoriteClicked, "Favorite the selected preset");
}

int PresetPrimaryActions::preferredWidth(LayoutMode layoutMode) noexcept
{
    return layoutMode == LayoutMode::home ? 252 : 310;
}

void PresetPrimaryActions::setLayoutMode(LayoutMode newMode)
{
    if (mode == newMode)
        return;

    mode = newMode;
    resized();
}

void PresetPrimaryActions::setPresetState(bool hasPreset, bool isFavorite)
{
    auditionButton.setEnabled(hasPreset);
    favoriteButton.setEnabled(hasPreset);
    favoriteButton.setToggleState(hasPreset && isFavorite, juce::dontSendNotification);
}

void PresetPrimaryActions::setWarmPreviewsEnabled(bool shouldBeEnabled)
{
    warmPreviewsButton.setEnabled(shouldBeEnabled);
}

void PresetPrimaryActions::setAuditionTooltip(const juce::String& tooltipText)
{
    auditionButton.setTooltip(tooltipText);
}

void PresetPrimaryActions::setWarmPreviewsTooltip(const juce::String& tooltipText)
{
    warmPreviewsButton.setTooltip(tooltipText);
}

void PresetPrimaryActions::resized()
{
    auto area = getLocalBounds();

    if (mode == LayoutMode::home)
    {
        warmPreviewsButton.setVisible(false);
        loadButton.setBounds(area.removeFromLeft(88).reduced(3, 4));
        auditionButton.setBounds(area.removeFromLeft(96).reduced(3, 4));
        favoriteButton.setBounds(area.removeFromLeft(68).reduced(3, 4));
        return;
    }

    warmPreviewsButton.setVisible(true);
    loadButton.setBounds(area.removeFromLeft(76).reduced(2, 4));
    auditionButton.setBounds(area.removeFromLeft(92).reduced(2, 4));
    warmPreviewsButton.setBounds(area.removeFromLeft(72).reduced(2, 4));
    favoriteButton.setBounds(area.removeFromLeft(70).reduced(2, 4));
}

void PresetPrimaryActions::configureButton(juce::TextButton& button,
                                           std::function<void()> PresetPrimaryActions::* callback,
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
