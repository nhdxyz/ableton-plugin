#include "PresetQuickFilterBar.h"

namespace UI
{
PresetQuickFilterBar::PresetQuickFilterBar()
{
    setComponentID("PresetQuickFilterBar");

    for (size_t index = 0; index < buttons.size(); ++index)
    {
        auto& button = buttons[index];
        const auto label = labelForIndex(index);
        button.setButtonText(label);
        button.setTooltip("Quick Library filter: " + label);
        button.setClickingTogglesState(false);
        button.setWantsKeyboardFocus(false);
        button.setMouseClickGrabsKeyboardFocus(false);
        button.onClick = [this, index]
        {
            if (onFilterSelected != nullptr)
                onFilterSelected(index);
        };
        addAndMakeVisible(button);
    }
}

void PresetQuickFilterBar::setFilterActive(size_t index, bool active)
{
    if (juce::isPositiveAndBelow(index, buttons.size()))
        buttons[index].setToggleState(active, juce::dontSendNotification);
}

void PresetQuickFilterBar::resized()
{
    auto area = getLocalBounds();
    placeFilterRow(0, 5, area.removeFromTop(area.getHeight() / 2).withTrimmedTop(2));
    placeFilterRow(5, buttons.size(), area.withTrimmedTop(2));
}

juce::String PresetQuickFilterBar::labelForIndex(size_t index)
{
    switch (index)
    {
        case 0: return "All";
        case 1: return "Fav";
        case 2: return "Recent";
        case 3: return "Similar";
        case 4: return "Bass";
        case 5: return "Lead";
        case 6: return "Chord";
        case 7: return "Pad";
        case 8: return "FX";
        case 9: return "Seq";
        default: return {};
    }
}

void PresetQuickFilterBar::placeFilterRow(size_t firstIndex, size_t endIndex, juce::Rectangle<int> row)
{
    endIndex = juce::jmin(endIndex, buttons.size());
    if (firstIndex >= endIndex)
        return;

    for (auto index = firstIndex; index < endIndex; ++index)
    {
        const auto remaining = static_cast<int>(endIndex - index);
        buttons[index].setBounds(row.removeFromLeft(row.getWidth() / remaining).reduced(2, 3));
    }
}
}
