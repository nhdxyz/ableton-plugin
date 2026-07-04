#include "SampleChopHeader.h"

namespace UI
{
SampleChopHeader::SampleChopHeader()
{
    setComponentID("SampleChopHeader");

    titleLabel.setText("CHOP", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    expandButton.setTooltip("Open a larger sample chop editor");
    expandButton.setWantsKeyboardFocus(false);
    expandButton.setMouseClickGrabsKeyboardFocus(false);
    expandButton.onClick = [this]
    {
        if (onExpandClicked != nullptr)
            onExpandClicked();
    };
    addAndMakeVisible(expandButton);

    applyTheme(themeFor(ThemeId::darkClub));
}

void SampleChopHeader::applyTheme(const Theme& theme)
{
    titleLabel.setColour(juce::Label::textColourId, theme.accent);
}

void SampleChopHeader::resized()
{
    auto area = getLocalBounds();
    expandButton.setBounds(area.removeFromRight(30).reduced(3, 1));
    titleLabel.setBounds(area.withTrimmedLeft(4));
}
}
