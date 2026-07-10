#include "PanelTabBar.h"

namespace UI
{
PanelTabBar::PanelTabBar()
{
    for (size_t index = 0; index < tabButtons.size(); ++index)
    {
        auto& button = tabButtons[index];
        const auto& spec = tabSpecs[index];

        button.setButtonText(spec.label);
        button.setTooltip(spec.tooltip);
        button.setWantsKeyboardFocus(false);
        button.setMouseClickGrabsKeyboardFocus(false);
        button.onClick = [this, tab = spec.tab]
        {
            if (onTabSelected != nullptr)
                onTabSelected(tab);
        };

        addAndMakeVisible(button);
    }
}

void PanelTabBar::resized()
{
    auto bounds = getLocalBounds();

    for (size_t index = 0; index < tabButtons.size(); ++index)
        tabButtons[index].setBounds(bounds.removeFromLeft(tabSpecs[index].width).reduced(3, 4));
}

void PanelTabBar::setActiveTab(Tab tab)
{
    activeTab = tab;

    for (size_t index = 0; index < tabButtons.size(); ++index)
        tabButtons[index].setToggleState(activeTab == tabSpecs[index].tab, juce::dontSendNotification);
}
}
