#include "MacroPerformanceMap.h"

#include <cmath>

namespace UI
{
namespace
{
constexpr auto macroCount = 8;
constexpr auto mapHeaderHeight = 18.0f;
constexpr auto radarInset = 10.0f;
constexpr auto minimumChangeDelta = 0.001f;
}

MacroPerformanceMap::MacroPerformanceMap()
{
    setInterceptsMouseClicks(true, true);
    setRepaintsOnMouseActivity(true);
}

void MacroPerformanceMap::setValues(ValueArray newValues)
{
    auto changed = false;
    for (size_t index = 0; index < newValues.size(); ++index)
    {
        newValues[index] = clamp01(newValues[index]);
        changed = changed || std::abs(values[index] - newValues[index]) > minimumChangeDelta;
    }

    values = newValues;

    if (changed)
        repaint();
}

juce::String MacroPerformanceMap::getTooltip()
{
    juce::StringArray lines;
    lines.add("Macro performance map");
    lines.add("Drag a spoke to edit that macro.");

    for (size_t index = 0; index < values.size(); ++index)
        lines.add(labelForIndex(index) + ": " + juce::String(juce::roundToInt(values[index] * 100.0f)) + "%");

    return lines.joinIntoString("\n");
}

void MacroPerformanceMap::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const auto activeIndex = juce::isPositiveAndBelow(activeMacroIndex, macroCount)
        ? static_cast<size_t>(activeMacroIndex)
        : size_t { 0 };
    const auto activeColour = colourForIndex(activeIndex);

    g.setColour(juce::Colour(0xff0d1215));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour(0xff2d393f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    auto header = bounds.reduced(9.0f, 5.0f).removeFromTop(mapHeaderHeight);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff8ee6c9));
    g.drawFittedText("MACRO SHAPE", header.removeFromLeft(86.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.72f);

    const auto activeText = labelForIndex(activeIndex) + " " + juce::String(juce::roundToInt(values[activeIndex] * 100.0f));
    g.setColour(activeColour);
    g.drawFittedText(activeText, header.toNearestInt(), juce::Justification::centredRight, 1, 0.72f);

    const auto mapBounds = getMapBounds();
    const auto centre = mapBounds.getCentre();
    const auto radius = juce::jmax(1.0f, juce::jmin(mapBounds.getWidth(), mapBounds.getHeight()) * 0.36f);

    g.setColour(juce::Colour(0xff151e22));
    g.fillRoundedRectangle(mapBounds, 5.0f);
    g.setColour(juce::Colour(0xff2a363c));
    g.drawRoundedRectangle(mapBounds, 5.0f, 1.0f);

    for (auto ring = 1; ring <= 3; ++ring)
    {
        const auto ringRadius = radius * static_cast<float>(ring) / 3.0f;
        g.setColour(juce::Colour(0xff263138).withAlpha(0.65f));
        g.drawEllipse(juce::Rectangle<float> { ringRadius * 2.0f, ringRadius * 2.0f }.withCentre(centre), 0.8f);
    }

    juce::Path shape;
    for (size_t index = 0; index < values.size(); ++index)
    {
        const auto fullPoint = pointForIndex(index, 1.0f, mapBounds);
        const auto valuePoint = pointForIndex(index, values[index], mapBounds);

        g.setColour(colourForIndex(index).withAlpha(index == activeIndex ? 0.68f : 0.34f));
        g.drawLine({ centre, fullPoint }, index == activeIndex ? 1.5f : 1.0f);

        if (index == 0)
            shape.startNewSubPath(valuePoint);
        else
            shape.lineTo(valuePoint);
    }
    shape.closeSubPath();

    juce::ColourGradient fill(colourForIndex(0).withAlpha(0.26f),
                              mapBounds.getX(),
                              mapBounds.getY(),
                              colourForIndex(3).withAlpha(0.18f),
                              mapBounds.getRight(),
                              mapBounds.getBottom(),
                              false);
    fill.addColour(0.62, colourForIndex(6).withAlpha(0.22f));
    g.setGradientFill(fill);
    g.fillPath(shape);
    g.setColour(activeColour.withAlpha(0.84f));
    g.strokePath(shape, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto showLabels = mapBounds.getWidth() >= 96.0f && mapBounds.getHeight() >= 72.0f;
    for (size_t index = 0; index < values.size(); ++index)
    {
        const auto handle = pointForIndex(index, values[index], mapBounds);
        const auto colour = colourForIndex(index);
        const auto active = index == activeIndex;
        const auto handleSize = active ? 7.0f : 5.2f;

        g.setColour(juce::Colour(0xff070a0c));
        g.fillEllipse(juce::Rectangle<float> { handleSize + 2.0f, handleSize + 2.0f }.withCentre(handle));
        g.setColour(colour);
        g.fillEllipse(juce::Rectangle<float> { handleSize, handleSize }.withCentre(handle));

        if (! showLabels)
            continue;

        const auto labelPoint = pointForIndex(index, 1.15f, mapBounds);
        auto labelArea = juce::Rectangle<float> { 25.0f, 11.0f }.withCentre(labelPoint).toNearestInt();
        labelArea = labelArea.getIntersection(getLocalBounds().reduced(7, 22));

        g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
        g.setColour(active ? colour : juce::Colour(0xff8d9ba0));
        g.drawFittedText(shortLabelForIndex(index), labelArea, juce::Justification::centred, 1, 0.58f);
    }
}

void MacroPerformanceMap::mouseDown(const juce::MouseEvent& event)
{
    applyMousePosition(event.position);
}

void MacroPerformanceMap::mouseDrag(const juce::MouseEvent& event)
{
    applyMousePosition(event.position);
}

juce::Rectangle<float> MacroPerformanceMap::getMapBounds() const
{
    auto bounds = getLocalBounds().toFloat().reduced(8.0f, 6.0f);
    bounds.removeFromTop(mapHeaderHeight + 2.0f);
    return bounds.reduced(2.0f, 1.0f);
}

juce::Point<float> MacroPerformanceMap::pointForIndex(size_t index, float amount, juce::Rectangle<float> mapBounds) const noexcept
{
    const auto angle = (-juce::MathConstants<float>::halfPi)
                     + (static_cast<float>(index) * juce::MathConstants<float>::twoPi / static_cast<float>(macroCount));
    const auto radius = juce::jmax(1.0f, (juce::jmin(mapBounds.getWidth(), mapBounds.getHeight()) * 0.36f) - radarInset);
    const auto centre = mapBounds.getCentre();

    return {
        centre.x + (std::cos(angle) * radius * amount),
        centre.y + (std::sin(angle) * radius * amount)
    };
}

size_t MacroPerformanceMap::indexForPosition(juce::Point<float> position, juce::Rectangle<float> mapBounds) const noexcept
{
    const auto centre = mapBounds.getCentre();
    auto angle = std::atan2(position.y - centre.y, position.x - centre.x) + juce::MathConstants<float>::halfPi;

    while (angle < 0.0f)
        angle += juce::MathConstants<float>::twoPi;

    const auto sector = juce::MathConstants<float>::twoPi / static_cast<float>(macroCount);
    return static_cast<size_t>(juce::roundToInt(angle / sector)) % macroCount;
}

void MacroPerformanceMap::applyMousePosition(juce::Point<float> position)
{
    const auto mapBounds = getMapBounds();
    if (mapBounds.isEmpty())
        return;

    const auto index = indexForPosition(position, mapBounds);
    const auto centre = mapBounds.getCentre();
    const auto fullPoint = pointForIndex(index, 1.0f, mapBounds);
    const auto axis = fullPoint - centre;
    const auto length = juce::jmax(1.0f, axis.getDistanceFromOrigin());
    const auto projection = ((position.x - centre.x) * axis.x + (position.y - centre.y) * axis.y) / (length * length);
    const auto nextValue = clamp01(projection);

    activeMacroIndex = static_cast<int>(index);

    if (std::abs(values[index] - nextValue) <= minimumChangeDelta)
    {
        repaint();
        return;
    }

    values[index] = nextValue;
    repaint();
    notifyMacroChange(index);
}

void MacroPerformanceMap::notifyMacroChange(size_t index)
{
    if (onChange)
        onChange(index, values[index]);
}

float MacroPerformanceMap::clamp01(float value) noexcept
{
    return juce::jlimit(0.0f, 1.0f, value);
}

juce::Colour MacroPerformanceMap::colourForIndex(size_t index) noexcept
{
    static constexpr std::array<juce::uint32, macroCount> colours {
        0xff8ee6c9,
        0xffffc36b,
        0xff7bb7ff,
        0xffb7a4ff,
        0xffd7e37b,
        0xff7fd0e6,
        0xfff0a86e,
        0xffff8fb4
    };

    return juce::Colour(colours[index % colours.size()]);
}

juce::String MacroPerformanceMap::labelForIndex(size_t index)
{
    static const std::array<juce::String, macroCount> labels {
        "Tone",
        "Dirt",
        "Motion",
        "Space",
        "Weight",
        "Bounce",
        "Warp",
        "Throw"
    };

    return labels[index % labels.size()];
}

juce::String MacroPerformanceMap::shortLabelForIndex(size_t index)
{
    static const std::array<juce::String, macroCount> labels {
        "TN",
        "DR",
        "MO",
        "SP",
        "WT",
        "BN",
        "WP",
        "TH"
    };

    return labels[index % labels.size()];
}
}
