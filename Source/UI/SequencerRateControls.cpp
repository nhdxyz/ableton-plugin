#include "SequencerRateControls.h"

namespace UI
{
SequencerRateControls::SequencerRateControls()
{
    setComponentID("SequencerRateControls");

    const std::array<const char*, 3> tooltips {
        "Set sequencer rate to eighth notes",
        "Set sequencer rate to sixteenth notes",
        "Set sequencer rate to thirty-second notes"
    };

    for (size_t index = 0; index < rateButtons.size(); ++index)
    {
        auto& button = rateButtons[index];
        button.setTooltip(tooltips[index]);
        button.setWantsKeyboardFocus(false);
        button.setMouseClickGrabsKeyboardFocus(false);
        button.onClick = [this, index]
        {
            if (onRateSelected != nullptr)
                onRateSelected(static_cast<int>(index));
        };
        addAndMakeVisible(button);
    }
}

void SequencerRateControls::setSelectedIndex(int rateIndex)
{
    for (size_t index = 0; index < rateButtons.size(); ++index)
        rateButtons[index].setToggleState(static_cast<int>(index) == rateIndex, juce::dontSendNotification);
}

void SequencerRateControls::resized()
{
    auto area = getLocalBounds();
    const auto buttonWidth = area.getWidth() / static_cast<int>(rateButtons.size());
    for (auto& button : rateButtons)
        button.setBounds(area.removeFromLeft(buttonWidth).reduced(3, 4));
}
}
