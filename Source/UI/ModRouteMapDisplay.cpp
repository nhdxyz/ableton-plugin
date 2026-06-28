#include "ModRouteMapDisplay.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace UI
{
juce::Colour ModRouteMapDisplay::colourForAmount(float amount, bool enabled) noexcept
{
    const auto base = amount < 0.0f ? juce::Colour(0xffffa36f)
                                    : juce::Colour(0xff8ee6c9);
    return enabled ? base : juce::Colour(0xff68777d);
}

juce::String ModRouteMapDisplay::compactName(const juce::String& name)
{
    if (name.equalsIgnoreCase("Filter Cutoff"))
        return "Cutoff";
    if (name.equalsIgnoreCase("Filter Res"))
        return "Res";
    if (name.equalsIgnoreCase("Filter Env"))
        return "Env";
    if (name.equalsIgnoreCase("Osc 2 Tune"))
        return "O2 Tune";
    if (name.equalsIgnoreCase("Osc 2 Level"))
        return "O2 Lev";
    if (name.equalsIgnoreCase("Osc Warp"))
        return "Warp";
    if (name.startsWithIgnoreCase("FX "))
        return name.fromFirstOccurrenceOf("FX ", false, true);
    if (name.startsWithIgnoreCase("Sample "))
        return name.fromFirstOccurrenceOf("Sample ", false, true);
    if (name.containsIgnoreCase("Wavetable"))
        return name.replace("Wavetable", "WT");

    return name;
}

void ModRouteMapDisplay::setRoutes(std::vector<Route> newRoutes)
{
    auto changed = routes.size() != newRoutes.size();
    for (size_t index = 0; index < routes.size() && index < newRoutes.size(); ++index)
    {
        const auto& current = routes[index];
        const auto& next = newRoutes[index];
        changed = changed
            || current.source != next.source
            || current.destination != next.destination
            || std::abs(current.amount - next.amount) > 0.001f
            || current.enabled != next.enabled;
    }

    routes = std::move(newRoutes);
    rebuildTooltip();

    if (changed)
        repaint();
}

juce::String ModRouteMapDisplay::getTooltip()
{
    return tooltip;
}

void ModRouteMapDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const auto compact = bounds.getHeight() < 54.0f;
    auto content = bounds.reduced(10.0f, compact ? 4.0f : 7.0f);

    g.setColour(juce::Colour(0xff0c1114));
    g.fillRoundedRectangle(bounds, 5.0f);
    g.setColour(routes.empty() ? juce::Colour(0xff253037) : juce::Colour(0xff385058));
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

    auto header = content.removeFromTop(compact ? 10.0f : 14.0f);
    g.setFont(juce::FontOptions(compact ? 8.0f : 9.0f, juce::Font::bold));
    g.setColour(routes.empty() ? juce::Colour(0xff657178) : juce::Colour(0xff8ee6c9));
    g.drawFittedText("ROUTE MAP", header.toNearestInt(), juce::Justification::centredLeft, 1);
    g.setColour(juce::Colour(0xff9dafb2));
    g.drawFittedText(routes.empty() ? "EMPTY" : juce::String(static_cast<int>(routes.size())) + " ROUTES",
                     header.toNearestInt(),
                     juce::Justification::centredRight,
                     1,
                     0.7f);

    auto map = content.withTrimmedTop(compact ? 1.0f : 3.0f);
    if (routes.empty())
    {
        g.setFont(juce::FontOptions(compact ? 8.0f : 10.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff657178));
        g.drawFittedText("Assign a source to see modulation flow",
                         map.toNearestInt(),
                         juce::Justification::centred,
                         2,
                         0.72f);
        return;
    }

    const auto displayRouteCount = compact ? std::min<size_t>(routes.size(), 4) : routes.size();
    const auto laneHeight = map.getHeight() / static_cast<float>(juce::jmax(1, static_cast<int>(displayRouteCount)));
    for (size_t index = 0; index < displayRouteCount; ++index)
    {
        const auto& route = routes[index];
        auto lane = map.removeFromTop(laneHeight).reduced(0.0f, 1.0f);
        const auto active = route.enabled && std::abs(route.amount) > 0.001f;
        const auto colour = colourForAmount(route.amount, route.enabled);
        const auto amount = juce::jlimit(-1.0f, 1.0f, route.amount);
        const auto sourceText = compactName(route.source);
        const auto destinationText = compactName(route.destination);

        if (compact)
        {
            auto pathArea = lane.reduced(3.0f, 0.0f);
            const auto centreY = pathArea.getCentreY();
            const auto startX = pathArea.getX();
            const auto endX = pathArea.getRight();
            juce::Path path;
            path.startNewSubPath(startX, centreY);
            path.cubicTo(startX + (pathArea.getWidth() * 0.32f),
                         centreY - 3.0f,
                         endX - (pathArea.getWidth() * 0.32f),
                         centreY + 3.0f,
                         endX,
                         centreY);

            g.setColour(colour.withAlpha(active ? 0.84f : 0.28f));
            g.strokePath(path, juce::PathStrokeType(1.0f + (std::abs(amount) * 1.8f),
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
            g.fillEllipse(startX - 1.7f, centreY - 1.7f, 3.4f, 3.4f);
            g.fillEllipse(endX - 1.7f, centreY - 1.7f, 3.4f, 3.4f);
            continue;
        }

        auto sourceBox = lane.removeFromLeft(58.0f).reduced(0.0f, 1.0f);
        auto destBox = lane.removeFromRight(70.0f).reduced(0.0f, 1.0f);
        auto pathArea = lane.reduced(5.0f, 0.0f);

        g.setColour(active ? colour.withAlpha(0.18f) : juce::Colour(0xff151d21));
        g.fillRoundedRectangle(sourceBox, 3.0f);
        g.fillRoundedRectangle(destBox, 3.0f);
        g.setColour(active ? colour.withAlpha(0.74f) : juce::Colour(0xff3b474d));
        g.drawRoundedRectangle(sourceBox, 3.0f, 0.8f);
        g.drawRoundedRectangle(destBox, 3.0f, 0.8f);

        g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
        g.setColour(active ? juce::Colour(0xffedf7f4) : juce::Colour(0xff87969a));
        g.drawFittedText(sourceText, sourceBox.toNearestInt().reduced(3, 0), juce::Justification::centred, 1, 0.52f);
        g.drawFittedText(destinationText, destBox.toNearestInt().reduced(3, 0), juce::Justification::centred, 1, 0.52f);

        const auto centreY = pathArea.getCentreY();
        const auto startX = pathArea.getX();
        const auto endX = pathArea.getRight();
        const auto bend = (pathArea.getHeight() * 0.42f) * (index % 2 == 0 ? -1.0f : 1.0f);
        juce::Path path;
        path.startNewSubPath(startX, centreY);
        path.cubicTo(startX + (pathArea.getWidth() * 0.32f),
                     centreY + bend,
                     endX - (pathArea.getWidth() * 0.32f),
                     centreY - bend,
                     endX,
                     centreY);

        g.setColour(colour.withAlpha(active ? 0.86f : 0.28f));
        g.strokePath(path, juce::PathStrokeType(1.1f + (std::abs(amount) * 2.2f),
                                                juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

        const auto markerX = juce::jmap(std::abs(amount), 0.0f, 1.0f, startX + 8.0f, endX - 8.0f);
        g.fillEllipse(markerX - 2.2f, centreY - 2.2f, 4.4f, 4.4f);
    }
}

void ModRouteMapDisplay::rebuildTooltip()
{
    if (routes.empty())
    {
        tooltip = "No modulation routes";
        return;
    }

    juce::StringArray lines;
    lines.add("Modulation route map");
    for (const auto& route : routes)
    {
        const auto percent = juce::roundToInt(route.amount * 100.0f);
        lines.add(route.source + " -> " + route.destination
                  + " " + (percent >= 0 ? "+" : "") + juce::String(percent) + "%"
                  + (route.enabled ? "" : " bypassed"));
    }

    tooltip = lines.joinIntoString("\n");
}
}
