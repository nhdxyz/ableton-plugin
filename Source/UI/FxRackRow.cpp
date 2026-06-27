#include "FxRackRow.h"

namespace UI
{
FxRackRow::FxRackRow(const juce::String& name)
    : juce::Button(name),
      moduleName(name)
{
    setWantsKeyboardFocus(false);
    setClickingTogglesState(false);
}

void FxRackRow::setState(const juce::String& name,
                         const juce::String& summary,
                         bool enabled,
                         bool selected,
                         bool pinned)
{
    if (moduleName == name
        && moduleSummary == summary
        && moduleEnabled == enabled
        && moduleSelected == selected
        && modulePinned == pinned)
    {
        return;
    }

    moduleName = name;
    moduleSummary = summary;
    moduleEnabled = enabled;
    moduleSelected = selected;
    modulePinned = pinned;

    setButtonText(moduleName);
    setTooltip(moduleSummary);
    repaint();
}

void FxRackRow::paintButton(juce::Graphics& g,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const auto rowRadius = 5.0f;

    auto fill = juce::Colour(0xff151d20);
    if (moduleSelected)
        fill = juce::Colour(0xff20312f);
    else if (shouldDrawButtonAsDown)
        fill = juce::Colour(0xff26343a);
    else if (shouldDrawButtonAsHighlighted)
        fill = juce::Colour(0xff1d282c);

    g.setColour(fill);
    g.fillRoundedRectangle(bounds, rowRadius);

    const auto outline = moduleSelected ? juce::Colour(0xff8ee6c9)
                                        : juce::Colour(0xff344047);
    g.setColour(outline);
    g.drawRoundedRectangle(bounds, rowRadius, moduleSelected ? 1.4f : 1.0f);

    auto content = getLocalBounds().reduced(8, 4);
    const auto accent = modulePinned ? juce::Colour(0xffffc36b)
                                     : (moduleEnabled ? juce::Colour(0xff8ee6c9)
                                                      : juce::Colour(0xff5f6b70));

    g.setColour(accent);
    g.fillRoundedRectangle(content.removeFromLeft(4).toFloat(), 2.0f);
    content.removeFromLeft(7);

    auto badgeArea = content.removeFromRight(modulePinned ? 42 : 34).reduced(0, 3);
    const auto badgeFill = moduleEnabled ? juce::Colour(0xff315b52)
                                         : juce::Colour(0xff263035);

    g.setColour(modulePinned ? juce::Colour(0xff5b4722) : badgeFill);
    g.fillRoundedRectangle(badgeArea.toFloat(), 4.0f);
    g.setColour(modulePinned ? juce::Colour(0xffffc36b)
                             : (moduleEnabled ? juce::Colour(0xff8ee6c9)
                                              : juce::Colour(0xff6d7b82)));
    g.drawRoundedRectangle(badgeArea.toFloat(), 4.0f, 1.0f);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawFittedText(modulePinned ? "SAFE" : (moduleEnabled ? "ON" : "OFF"),
                     badgeArea,
                     juce::Justification::centred,
                     1);

    content.removeFromRight(6);
    auto nameArea = content;
    auto summaryArea = content;
    nameArea.removeFromBottom(getHeight() > 32 ? 12 : 0);
    summaryArea.removeFromTop(getHeight() > 32 ? 15 : getHeight());

    g.setColour(moduleSelected ? juce::Colour(0xffedf7f4) : juce::Colour(0xffd3e1df));
    g.setFont(juce::FontOptions(12.0f, moduleSelected ? juce::Font::bold : juce::Font::plain));
    g.drawFittedText(moduleName, nameArea, juce::Justification::centredLeft, 1);

    if (summaryArea.getHeight() > 0)
    {
        g.setColour(juce::Colour(0xff879299));
        g.setFont(juce::FontOptions(9.5f));
        g.drawFittedText(moduleSummary, summaryArea, juce::Justification::centredLeft, 1);
    }
}
}
