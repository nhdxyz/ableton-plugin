#include "ModSourceMeter.h"

#include <cmath>

namespace UI
{
void ModSourceMeter::setState(const juce::String& newSummary,
                              int newRouteCount,
                              float newDepth,
                              float newActivity,
                              const juce::String& newTooltip)
{
    newRouteCount = juce::jmax(0, newRouteCount);
    newDepth = juce::jlimit(-1.0f, 1.0f, newDepth);
    newActivity = juce::jlimit(0.0f, 1.0f, newActivity);

    if (summary == newSummary
        && routeCount == newRouteCount
        && std::abs(depth - newDepth) < 0.002f
        && std::abs(activity - newActivity) < 0.01f
        && tooltipText == newTooltip)
    {
        return;
    }

    summary = newSummary;
    routeCount = newRouteCount;
    depth = newDepth;
    activity = newActivity;
    tooltipText = newTooltip;
    repaint();
}

juce::String ModSourceMeter::getTooltip()
{
    return tooltipText;
}

void ModSourceMeter::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const auto configured = routeCount > 0;
    const auto positiveDepth = depth >= 0.0f;
    const auto accent = configured
        ? (positiveDepth ? juce::Colour(0xff8ee6c9) : juce::Colour(0xffffa36f))
        : juce::Colour(0xff5b6a70);

    g.setColour(configured ? accent.withAlpha(0.10f) : juce::Colour(0x00101619));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(configured ? accent.withAlpha(0.30f) : juce::Colour(0x22344047));
    g.drawRoundedRectangle(bounds, 4.0f, configured ? 1.2f : 0.8f);

    if (getHeight() < 16)
    {
        auto row = getLocalBounds().reduced(4, 0);
        auto meterArea = row.removeFromRight(30).withTrimmedTop(juce::jmax(1, row.getHeight() - 4));
        auto badgeArea = row.removeFromRight(12).reduced(1, 1);

        g.setFont(juce::FontOptions(8.0f, configured ? juce::Font::bold : juce::Font::plain));
        g.setColour(configured ? juce::Colour(0xffe1fff5) : juce::Colour(0xffbdcacb));
        g.drawFittedText(summary, row, juce::Justification::centredLeft, 1, 0.52f);

        if (badgeArea.getWidth() > 4 && badgeArea.getHeight() > 4)
        {
            g.setColour(configured ? accent.withAlpha(0.26f) : juce::Colour(0xff1a2327));
            g.fillRoundedRectangle(badgeArea.toFloat(), 2.0f);
            g.setColour(configured ? accent.withAlpha(0.76f) : juce::Colour(0xff39474d));
            g.drawRoundedRectangle(badgeArea.toFloat(), 2.0f, 0.8f);
            g.setColour(configured ? juce::Colour(0xffedf7f4) : juce::Colour(0xff879299));
            g.setFont(juce::FontOptions(7.4f, juce::Font::bold));
            g.drawFittedText(routeCount > 0 ? juce::String(routeCount) : "-", badgeArea, juce::Justification::centred, 1, 0.46f);
        }

        if (meterArea.getWidth() > 4 && meterArea.getHeight() > 1)
        {
            g.setColour(juce::Colour(0xff1d282d));
            g.fillRoundedRectangle(meterArea.toFloat(), 1.5f);

            auto fill = meterArea.toFloat();
            fill.setWidth(fill.getWidth() * activity);
            g.setColour(accent.withAlpha(configured ? 0.88f : 0.42f));
            g.fillRoundedRectangle(fill, 1.5f);
        }
        return;
    }

    auto textArea = getLocalBounds().reduced(5, 1);
    auto meterArea = textArea.removeFromRight(52);
    auto badgeArea = meterArea.removeFromRight(16).reduced(1, 2);
    meterArea = meterArea.withTrimmedTop(juce::jmax(2, meterArea.getHeight() / 2)).reduced(2, 2);

    g.setFont(juce::FontOptions(9.5f, configured ? juce::Font::bold : juce::Font::plain));
    g.setColour(configured ? juce::Colour(0xffd9fff1) : juce::Colour(0xffbdcacb));
    g.drawFittedText(summary, textArea, juce::Justification::centredLeft, 1);

    if (badgeArea.getWidth() > 4 && badgeArea.getHeight() > 4)
    {
        g.setColour(configured ? accent.withAlpha(0.24f) : juce::Colour(0xff1a2327));
        g.fillRoundedRectangle(badgeArea.toFloat(), 3.0f);
        g.setColour(configured ? accent.withAlpha(0.72f) : juce::Colour(0xff39474d));
        g.drawRoundedRectangle(badgeArea.toFloat(), 3.0f, 1.0f);
        g.setColour(configured ? juce::Colour(0xffedf7f4) : juce::Colour(0xff879299));
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.drawFittedText(routeCount > 0 ? juce::String(routeCount) : "-", badgeArea, juce::Justification::centred, 1);
    }

    if (meterArea.getWidth() > 4 && meterArea.getHeight() > 1)
    {
        g.setColour(juce::Colour(0xff1d282d));
        g.fillRoundedRectangle(meterArea.toFloat(), 2.0f);

        auto fill = meterArea.toFloat();
        fill.setWidth(fill.getWidth() * activity);
        g.setColour(accent.withAlpha(configured ? 0.88f : 0.42f));
        g.fillRoundedRectangle(fill, 2.0f);
    }
}
}
