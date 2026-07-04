#include "PageButtonStrip.h"

namespace UI
{
void PageButtonStrip::setItems(std::initializer_list<Item> newItems)
{
    for (auto& button : buttons)
        removeChildComponent(button.get());

    buttons.clear();
    buttons.reserve(newItems.size());

    size_t index = 0;
    for (const auto& item : newItems)
    {
        auto button = std::make_unique<juce::TextButton>(item.label);
        button->setTooltip(item.tooltip);
        button->setWantsKeyboardFocus(false);
        button->setMouseClickGrabsKeyboardFocus(false);
        button->onClick = [this, index]
        {
            if (onItemSelected != nullptr)
                onItemSelected(index);
        };

        addAndMakeVisible(*button);
        buttons.push_back(std::move(button));
        ++index;
    }

    if (activeIndex >= buttons.size())
        activeIndex = buttons.empty() ? 0 : buttons.size() - 1;

    setActiveIndex(activeIndex);
    resized();
}

void PageButtonStrip::setOrientation(Orientation newOrientation)
{
    if (orientation == newOrientation)
        return;

    orientation = newOrientation;
    resized();
}

void PageButtonStrip::setActiveIndex(size_t index)
{
    activeIndex = index;

    for (size_t buttonIndex = 0; buttonIndex < buttons.size(); ++buttonIndex)
        buttons[buttonIndex]->setToggleState(buttonIndex == activeIndex, juce::dontSendNotification);
}

void PageButtonStrip::resized()
{
    auto bounds = getLocalBounds();

    if (buttons.empty())
        return;

    if (orientation == Orientation::vertical)
    {
        const auto buttonHeight = juce::jlimit(32, 42, bounds.getHeight() / static_cast<int>(buttons.size()));

        for (auto& button : buttons)
            button->setBounds(bounds.removeFromTop(buttonHeight).reduced(4));

        return;
    }

    for (size_t index = 0; index < buttons.size(); ++index)
    {
        const auto remaining = static_cast<int>(buttons.size() - index);
        buttons[index]->setBounds(bounds.removeFromLeft(bounds.getWidth() / juce::jmax(1, remaining)).reduced(4));
    }
}
}
