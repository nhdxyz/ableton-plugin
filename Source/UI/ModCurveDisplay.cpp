#include "ModCurveDisplay.h"

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

void ModCurveDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto plot = getPlotBounds();
    const auto accent = highlighted ? juce::Colour(0xff8ee6c9) : juce::Colour(0xff617078);

    g.setColour(juce::Colour(0xff101619));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour(0xff2a363c));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    g.setColour(juce::Colour(0xff2f3b40));
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

    g.setColour(accent.withAlpha(0.14f));
    g.fillPath(fillPath);
    g.setColour(accent);
    g.strokePath(linePath, juce::PathStrokeType(2.0f));

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
}

void ModCurveDisplay::mouseDown(const juce::MouseEvent& event)
{
    draggedIndex = static_cast<int>(pointIndexForX(event.position.x));
    updatePointFromPosition(event.position);
}

void ModCurveDisplay::mouseDrag(const juce::MouseEvent& event)
{
    updatePointFromPosition(event.position);
}

void ModCurveDisplay::mouseUp(const juce::MouseEvent&)
{
    draggedIndex = -1;
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

void ModCurveDisplay::updatePointFromPosition(juce::Point<float> position)
{
    if (draggedIndex < 0)
        draggedIndex = static_cast<int>(pointIndexForX(position.x));

    const auto plot = getPlotBounds();
    const auto nextValue = juce::jlimit(-1.0f, 1.0f, juce::jmap(position.y, plot.getBottom(), plot.getY(), -1.0f, 1.0f));
    const auto index = static_cast<size_t>(juce::jlimit(0, 7, draggedIndex));

    if (std::abs(values[index] - nextValue) < 0.001f)
        return;

    values[index] = nextValue;
    hoveredIndex = static_cast<int>(index);
    repaint();

    if (onPointChange)
        onPointChange(index, nextValue);
}
}
