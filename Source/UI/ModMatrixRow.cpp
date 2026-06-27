#include "ModMatrixRow.h"

#include <cmath>

namespace UI
{
ModMatrixRow::ModMatrixRow()
{
    setInterceptsMouseClicks(false, false);
}

void ModMatrixRow::setState(int newSlotNumber,
                            const juce::String& newSourceText,
                            const juce::String& newDestinationText,
                            float newAmount,
                            bool newEnabled)
{
    newAmount = juce::jlimit(-1.0f, 1.0f, newAmount);

    if (slotNumber == newSlotNumber
        && sourceText == newSourceText
        && destinationText == newDestinationText
        && std::abs(amount - newAmount) < 0.001f
        && enabled == newEnabled)
    {
        return;
    }

    slotNumber = newSlotNumber;
    sourceText = newSourceText;
    destinationText = newDestinationText;
    amount = newAmount;
    enabled = newEnabled;
    repaint();
}

juce::String ModMatrixRow::getTooltip()
{
    if (! isConfigured())
        return "Mod slot " + juce::String(slotNumber) + ": off";

    const auto percent = juce::roundToInt(amount * 100.0f);
    return "Mod slot " + juce::String(slotNumber)
        + ": " + sourceText
        + " -> " + destinationText
        + " (" + (percent >= 0 ? "+" : "") + juce::String(percent) + "%)"
        + (enabled ? "" : " bypassed");
}

void ModMatrixRow::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const auto active = isActive();
    const auto configured = isConfigured();
    const auto accent = accentColour();

    g.setColour(active ? accent.withAlpha(0.09f) : (configured ? juce::Colour(0xff151b1e) : juce::Colour(0xff101619)));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(active ? accent.withAlpha(0.55f) : (configured ? juce::Colour(0xff46525a) : juce::Colour(0xff2a363c)));
    g.drawRoundedRectangle(bounds, 4.0f, active ? 1.2f : 0.8f);

    auto slotBadge = bounds.removeFromLeft(28.0f).reduced(4.0f, 3.0f);
    g.setColour(active ? accent.withAlpha(0.22f) : (configured ? juce::Colour(0xff222b30) : juce::Colour(0xff172024)));
    g.fillRoundedRectangle(slotBadge, 3.0f);
    g.setColour(active ? accent : (configured ? juce::Colour(0xff8b989d) : juce::Colour(0xff617078)));
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.drawFittedText(juce::String(slotNumber), slotBadge.toNearestInt(), juce::Justification::centred, 1);

    const auto barBounds = getLocalBounds().toFloat().reduced(35.0f, 2.5f).removeFromBottom(2.0f);
    g.setColour(juce::Colour(0xff202a2f));
    g.fillRoundedRectangle(barBounds, 1.0f);

    if (active)
    {
        const auto centreX = barBounds.getCentreX();
        const auto width = (barBounds.getWidth() * 0.5f) * std::abs(amount);
        auto amountBar = amount >= 0.0f ? juce::Rectangle<float> { centreX, barBounds.getY(), width, barBounds.getHeight() }
                                       : juce::Rectangle<float> { centreX - width, barBounds.getY(), width, barBounds.getHeight() };
        g.setColour(accent);
        g.fillRoundedRectangle(amountBar, 1.0f);
    }
    else if (configured)
    {
        g.setColour(juce::Colour(0xff8b989d));
        g.fillRoundedRectangle(barBounds.withSizeKeepingCentre(barBounds.getWidth() * 0.34f, barBounds.getHeight()), 1.0f);
    }
}

bool ModMatrixRow::isActive() const noexcept
{
    return enabled
        && ! sourceText.equalsIgnoreCase("Off")
        && ! destinationText.equalsIgnoreCase("Off")
        && std::abs(amount) > 0.001f;
}

bool ModMatrixRow::isConfigured() const noexcept
{
    return ! sourceText.equalsIgnoreCase("Off")
        && ! destinationText.equalsIgnoreCase("Off")
        && std::abs(amount) > 0.001f;
}

juce::Colour ModMatrixRow::accentColour() const noexcept
{
    if (amount < 0.0f)
        return juce::Colour(0xffffa36f);

    return juce::Colour(0xff8ee6c9);
}
}
