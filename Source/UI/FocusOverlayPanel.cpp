#include "FocusOverlayPanel.h"

namespace UI
{
FocusOverlayPanel::FocusOverlayPanel()
{
    setInterceptsMouseClicks(true, true);
}

juce::Rectangle<int> FocusOverlayPanel::getPanelBounds() const
{
    auto available = getLocalBounds().reduced(24, 18);
    const auto panelWidth = juce::jlimit(620, 1040, available.getWidth());
    const auto panelHeight = juce::jlimit(310, 720, available.getHeight());
    return available.withSizeKeepingCentre(panelWidth, panelHeight);
}

juce::Rectangle<int> FocusOverlayPanel::getContentBounds() const
{
    auto panel = getPanelBounds().reduced(18, 14);
    panel.removeFromTop(34);
    return panel;
}

void FocusOverlayPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xd8070a0c));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);

    const auto panel = getPanelBounds().toFloat();
    g.setColour(juce::Colour(0xff0d1417));
    g.fillRoundedRectangle(panel, 8.0f);
    g.setColour(juce::Colour(0xff344148));
    g.drawRoundedRectangle(panel, 8.0f, 1.2f);

    g.setColour(juce::Colour(0xff8ee6c9).withAlpha(0.42f));
    g.fillRoundedRectangle(panel.reduced(16.0f, 12.0f).removeFromTop(3.0f), 2.0f);
}
}
