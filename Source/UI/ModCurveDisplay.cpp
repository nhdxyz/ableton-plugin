#include "ModCurveDisplay.h"

#include <algorithm>
#include <cmath>

namespace UI
{
ModCurveDisplay::ModCurveDisplay()
{
    setInterceptsMouseClicks(true, true);
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

void ModCurveDisplay::setValues(const std::array<float, 8>& newValues, bool shouldHighlight)
{
    if (values == newValues && highlighted == shouldHighlight)
        return;

    values = newValues;
    highlighted = shouldHighlight;
    repaint();
}

void ModCurveDisplay::setPhase(float newPhase, bool shouldShowPhase)
{
    newPhase = juce::jlimit(0.0f, 1.0f, newPhase);
    if (std::abs(phase - newPhase) < 0.003f && showPhase == shouldShowPhase)
        return;

    phase = newPhase;
    showPhase = shouldShowPhase;
    repaint();
}

void ModCurveDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto plot = getPlotBounds();
    const auto accent = highlighted ? juce::Colour(0xff8ee6c9) : juce::Colour(0xff617078);

    g.setColour(juce::Colour(0xff101619));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour(0xff2a363c));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    g.setColour(juce::Colour(0xff1b2428));
    for (auto index = 1; index < 8; ++index)
    {
        const auto x = juce::jmap(static_cast<float>(index), 0.0f, 8.0f, plot.getX(), plot.getRight());
        g.drawVerticalLine(static_cast<int>(std::round(x)), plot.getY(), plot.getBottom());
    }

    g.setColour(juce::Colour(0xff263238));
    g.drawHorizontalLine(static_cast<int>(juce::jmap(-0.5f, -1.0f, 1.0f, plot.getBottom(), plot.getY())), plot.getX(), plot.getRight());
    g.drawHorizontalLine(static_cast<int>(juce::jmap(0.5f, -1.0f, 1.0f, plot.getBottom(), plot.getY())), plot.getX(), plot.getRight());

    g.setColour(juce::Colour(0xff405057));
    g.drawHorizontalLine(static_cast<int>(plot.getCentreY()), plot.getX(), plot.getRight());

    juce::Path fillPath;
    juce::Path linePath;
    const auto xForIndex = [&plot] (size_t index)
    {
        return juce::jmap(static_cast<float>(index), 0.0f, 7.0f, plot.getX(), plot.getRight());
    };
    const auto yForValue = [&plot] (float value)
    {
        return juce::jmap(juce::jlimit(-1.0f, 1.0f, value), -1.0f, 1.0f, plot.getBottom(), plot.getY());
    };

    fillPath.startNewSubPath(plot.getX(), plot.getCentreY());

    for (size_t index = 0; index < values.size(); ++index)
    {
        const auto x = xForIndex(index);
        const auto y = yForValue(values[index]);

        if (index == 0)
            linePath.startNewSubPath(x, y);
        else
            linePath.lineTo(x, y);

        fillPath.lineTo(x, y);
    }

    fillPath.lineTo(plot.getRight(), plot.getCentreY());
    fillPath.closeSubPath();

    for (size_t index = 0; index < values.size(); ++index)
    {
        const auto segmentWidth = plot.getWidth() / static_cast<float>(values.size());
        const auto segment = juce::Rectangle<float> {
            plot.getX() + (segmentWidth * static_cast<float>(index)),
            plot.getY(),
            segmentWidth,
            plot.getHeight()
        }.reduced(1.0f, 0.0f);
        const auto valueY = yForValue(values[index]);
        const auto barTop = std::min(valueY, plot.getCentreY());
        const auto barBottom = std::max(valueY, plot.getCentreY());
        g.setColour(accent.withAlpha(static_cast<int>(index) == hoveredIndex ? 0.20f : 0.10f));
        g.fillRect(juce::Rectangle<float> { segment.getX(), barTop, segment.getWidth(), barBottom - barTop });
    }

    g.setColour(accent.withAlpha(0.14f));
    g.fillPath(fillPath);
    g.setColour(accent.withAlpha(0.34f));
    g.strokePath(linePath, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(accent);
    g.strokePath(linePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (showPhase)
    {
        const auto cursorX = juce::jmap(phase, 0.0f, 1.0f, plot.getX(), plot.getRight());
        g.setColour(juce::Colour(0xffedf7f4).withAlpha(0.52f));
        g.drawVerticalLine(static_cast<int>(std::round(cursorX)), plot.getY(), plot.getBottom());

        const auto scaledPhase = juce::jlimit(0.0f, 0.999999f, phase) * 8.0f;
        const auto leftIndex = static_cast<int>(std::floor(scaledPhase)) % 8;
        const auto rightIndex = (leftIndex + 1) % 8;
        const auto fraction = scaledPhase - std::floor(scaledPhase);
        const auto cursorValue = values[static_cast<size_t>(leftIndex)]
            + ((values[static_cast<size_t>(rightIndex)] - values[static_cast<size_t>(leftIndex)]) * fraction);
        const auto cursorY = yForValue(cursorValue);

        g.setColour(juce::Colour(0xffedf7f4));
        g.fillEllipse(juce::Rectangle<float> { 6.0f, 6.0f }.withCentre({ cursorX, cursorY }));
    }

    for (size_t index = 0; index < values.size(); ++index)
    {
        const auto point = juce::Point<float> { xForIndex(index), yForValue(values[index]) };
        const auto isFocused = static_cast<int>(index) == draggedIndex || static_cast<int>(index) == hoveredIndex;
        const auto pointSize = isFocused ? 8.0f : 6.0f;
        g.setColour(juce::Colour(0xff0d1113));
        g.fillEllipse(juce::Rectangle<float> { pointSize, pointSize }.withCentre(point));
        g.setColour(isFocused ? juce::Colour(0xffedf7f4) : accent);
        g.drawEllipse(juce::Rectangle<float> { pointSize, pointSize }.withCentre(point), isFocused ? 1.8f : 1.4f);
    }

    const auto focusedIndex = draggedIndex >= 0 ? draggedIndex : hoveredIndex;
    if (focusedIndex >= 0 && focusedIndex < static_cast<int>(values.size()))
    {
        const auto point = juce::Point<float> {
            xForIndex(static_cast<size_t>(focusedIndex)),
            yForValue(values[static_cast<size_t>(focusedIndex)])
        };
        auto badge = juce::Rectangle<float> { 64.0f, 18.0f }.withCentre({ point.x, plot.getY() + 9.0f });
        badge.setX(juce::jlimit(plot.getX(), plot.getRight() - badge.getWidth(), badge.getX()));
        g.setColour(juce::Colour(0xee101619));
        g.fillRoundedRectangle(badge, 4.0f);
        g.setColour(juce::Colour(0xff344047));
        g.drawRoundedRectangle(badge, 4.0f, 1.0f);
        g.setColour(juce::Colour(0xffdce7e4));
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        const auto percent = juce::roundToInt(values[static_cast<size_t>(focusedIndex)] * 100.0f);
        g.drawFittedText("P" + juce::String(focusedIndex + 1) + " "
                             + (percent >= 0 ? "+" : "") + juce::String(percent),
                         badge.toNearestInt().reduced(4, 0),
                         juce::Justification::centred,
                         1);
    }
}

void ModCurveDisplay::mouseDown(const juce::MouseEvent& event)
{
    draggedIndex = static_cast<int>(pointIndexForX(event.position.x));
    lastEditedIndex = draggedIndex;
    updatePointFromPosition(event.position, event.mods);
}

void ModCurveDisplay::mouseDrag(const juce::MouseEvent& event)
{
    updatePointFromPosition(event.position, event.mods);
}

void ModCurveDisplay::mouseUp(const juce::MouseEvent&)
{
    draggedIndex = -1;
    lastEditedIndex = -1;
    repaint();
}

void ModCurveDisplay::mouseDoubleClick(const juce::MouseEvent& event)
{
    const auto index = pointIndexForX(event.position.x);
    hoveredIndex = static_cast<int>(index);
    applyPointValue(index, 0.0f);
    draggedIndex = -1;
    lastEditedIndex = -1;
    repaint();
}

void ModCurveDisplay::mouseMove(const juce::MouseEvent& event)
{
    const auto nextHoveredIndex = static_cast<int>(pointIndexForX(event.position.x));
    if (hoveredIndex == nextHoveredIndex)
        return;

    hoveredIndex = nextHoveredIndex;
    repaint();
}

void ModCurveDisplay::mouseExit(const juce::MouseEvent&)
{
    if (hoveredIndex < 0)
        return;

    hoveredIndex = -1;
    repaint();
}

juce::Rectangle<float> ModCurveDisplay::getPlotBounds() const
{
    return getLocalBounds().toFloat().reduced(11.0f, 8.0f);
}

size_t ModCurveDisplay::pointIndexForX(float xPosition) const
{
    const auto plot = getPlotBounds();
    if (plot.getWidth() <= 0.0f)
        return 0;

    const auto proportion = juce::jlimit(0.0f, 1.0f, (xPosition - plot.getX()) / plot.getWidth());
    return static_cast<size_t>(juce::jlimit(0, 7, static_cast<int>(std::round(proportion * 7.0f))));
}

float ModCurveDisplay::valueForPosition(juce::Point<float> position, juce::ModifierKeys modifiers) const
{
    const auto plot = getPlotBounds();
    auto nextValue = juce::jlimit(-1.0f, 1.0f, juce::jmap(position.y, plot.getBottom(), plot.getY(), -1.0f, 1.0f));

    if (modifiers.isShiftDown() || modifiers.isCommandDown())
        nextValue = std::round(nextValue * 12.0f) / 12.0f;

    return juce::jlimit(-1.0f, 1.0f, nextValue);
}

void ModCurveDisplay::applyPointValue(size_t index, float value)
{
    index = static_cast<size_t>(juce::jlimit(0, 7, static_cast<int>(index)));
    value = juce::jlimit(-1.0f, 1.0f, value);

    if (std::abs(values[index] - value) < 0.001f)
        return;

    values[index] = value;

    if (onPointChange)
        onPointChange(index, value);
}

void ModCurveDisplay::updatePointFromPosition(juce::Point<float> position, juce::ModifierKeys modifiers)
{
    const auto targetIndex = static_cast<int>(pointIndexForX(position.x));
    const auto targetValue = valueForPosition(position, modifiers);

    if (draggedIndex < 0)
        draggedIndex = targetIndex;

    auto changed = false;
    if (lastEditedIndex >= 0 && lastEditedIndex != targetIndex)
    {
        const auto start = std::min(lastEditedIndex, targetIndex);
        const auto end = std::max(lastEditedIndex, targetIndex);
        const auto previousValue = values[static_cast<size_t>(lastEditedIndex)];

        for (auto index = start; index <= end; ++index)
        {
            const auto proportion = (end == start)
                ? 1.0f
                : static_cast<float>(index - start) / static_cast<float>(end - start);
            const auto directedProportion = lastEditedIndex < targetIndex ? proportion : 1.0f - proportion;
            const auto interpolatedValue = previousValue + ((targetValue - previousValue) * directedProportion);
            const auto before = values[static_cast<size_t>(index)];
            applyPointValue(static_cast<size_t>(index), interpolatedValue);
            changed = changed || std::abs(before - values[static_cast<size_t>(index)]) >= 0.001f;
        }
    }
    else
    {
        const auto before = values[static_cast<size_t>(targetIndex)];
        applyPointValue(static_cast<size_t>(targetIndex), targetValue);
        changed = std::abs(before - values[static_cast<size_t>(targetIndex)]) >= 0.001f;
    }

    draggedIndex = targetIndex;
    hoveredIndex = targetIndex;
    lastEditedIndex = targetIndex;

    if (changed)
        repaint();
}
}
