#include "ModCurveDisplay.h"

namespace UI
{
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
    const auto plot = bounds.reduced(10.0f, 7.0f);
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
        g.setColour(juce::Colour(0xff0d1113));
        g.fillEllipse(juce::Rectangle<float> { 6.0f, 6.0f }.withCentre(point));
        g.setColour(accent);
        g.drawEllipse(juce::Rectangle<float> { 6.0f, 6.0f }.withCentre(point), 1.4f);
    }
}
}
