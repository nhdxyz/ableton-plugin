#include "PumpCurveDisplay.h"

#include <cmath>

namespace UI
{
void PumpCurveDisplay::setState(int newCurveIndex,
                                float newDepth,
                                float newShape,
                                float newPhase,
                                int newRateIndex,
                                bool newEnabled)
{
    newCurveIndex = juce::jlimit(0, 4, newCurveIndex);
    newRateIndex = juce::jlimit(0, 3, newRateIndex);
    newDepth = juce::jlimit(0.0f, 1.0f, newDepth);
    newShape = juce::jlimit(0.0f, 1.0f, newShape);
    newPhase = juce::jlimit(0.0f, 1.0f, newPhase);

    if (curveIndex == newCurveIndex
        && rateIndex == newRateIndex
        && std::abs(depth - newDepth) < 0.001f
        && std::abs(shape - newShape) < 0.001f
        && std::abs(phase - newPhase) < 0.001f
        && enabled == newEnabled)
    {
        return;
    }

    curveIndex = newCurveIndex;
    rateIndex = newRateIndex;
    depth = newDepth;
    shape = newShape;
    phase = newPhase;
    enabled = newEnabled;
    repaint();
}

void PumpCurveDisplay::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto plot = plotBounds();
    const auto accent = enabled ? juce::Colour(0xff8ee6c9) : juce::Colour(0xff617078);

    g.setColour(juce::Colour(0xff101619));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(enabled ? juce::Colour(0xff36514c) : juce::Colour(0xff2a363c));
    g.drawRoundedRectangle(bounds, 6.0f, enabled ? 1.4f : 1.0f);

    g.setColour(juce::Colour(0xff263238));
    for (auto line = 1; line < 4; ++line)
    {
        const auto x = juce::jmap(static_cast<float>(line), 0.0f, 4.0f, plot.getX(), plot.getRight());
        g.drawVerticalLine(static_cast<int>(std::round(x)), plot.getY(), plot.getBottom());
    }

    g.setColour(juce::Colour(0xff2f3b40));
    g.drawHorizontalLine(static_cast<int>(std::round(plot.getBottom() - (plot.getHeight() * 0.5f))), plot.getX(), plot.getRight());

    juce::Path fillPath;
    juce::Path linePath;
    constexpr auto pointCount = 80;

    fillPath.startNewSubPath(plot.getX(), plot.getBottom());

    for (auto index = 0; index < pointCount; ++index)
    {
        const auto xNormalised = static_cast<float>(index) / static_cast<float>(pointCount - 1);
        auto curvePhase = std::fmod(xNormalised + phase, 1.0f);
        if (curvePhase < 0.0f)
            curvePhase += 1.0f;

        const auto gain = juce::jlimit(0.0f, 1.0f, 1.0f - (depth * duckAmount(curvePhase)));
        const auto x = juce::jmap(xNormalised, 0.0f, 1.0f, plot.getX(), plot.getRight());
        const auto y = juce::jmap(gain, 0.0f, 1.0f, plot.getBottom(), plot.getY());

        if (index == 0)
            linePath.startNewSubPath(x, y);
        else
            linePath.lineTo(x, y);

        fillPath.lineTo(x, y);
    }

    fillPath.lineTo(plot.getRight(), plot.getBottom());
    fillPath.closeSubPath();

    g.setColour(accent.withAlpha(enabled ? 0.18f : 0.10f));
    g.fillPath(fillPath);
    g.setColour(accent);
    g.strokePath(linePath, juce::PathStrokeType(2.0f));

    const auto header = bounds.withHeight(18.0f).reduced(10.0f, 0.0f);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(enabled ? juce::Colour(0xffedf7f4) : juce::Colour(0xff879299));
    g.drawFittedText(curveName().toUpperCase(), header.toNearestInt(), juce::Justification::centredLeft, 1);

    g.setFont(juce::FontOptions(9.0f));
    const auto detail = rateName() + " | " + juce::String(juce::roundToInt(depth * 100.0f)) + "%";
    g.setColour(juce::Colour(0xff879299));
    g.drawFittedText(detail, header.toNearestInt(), juce::Justification::centredRight, 1);
}

float PumpCurveDisplay::duckAmount(float recovery) const
{
    const auto safeRecovery = juce::jlimit(0.0f, 1.0f, recovery);
    const auto safeShape = juce::jlimit(0.0f, 1.0f, shape);
    const auto inverseRecovery = 1.0f - safeRecovery;

    switch (curveIndex)
    {
        case 1:
            return std::pow(inverseRecovery, juce::jmap(safeShape, 2.2f, 8.0f));

        case 2:
        {
            const auto hold = juce::jmap(safeShape, 0.10f, 0.28f);
            if (safeRecovery <= hold)
                return 1.0f;

            const auto release = juce::jlimit(0.0f, 1.0f, (safeRecovery - hold) / juce::jmax(0.001f, 1.0f - hold));
            return std::pow(1.0f - release, juce::jmap(safeShape, 1.1f, 4.0f));
        }

        case 3:
        {
            const auto pulsePhase = std::fmod(safeRecovery * 4.0f, 1.0f);
            const auto pulseDuck = pulsePhase < 0.32f
                ? 1.0f
                : std::pow(1.0f - ((pulsePhase - 0.32f) / 0.68f), juce::jmap(safeShape, 1.0f, 6.0f));
            const auto baseDuck = std::pow(inverseRecovery, juce::jmap(safeShape, 0.6f, 2.0f));
            return juce::jlimit(0.0f, 1.0f, juce::jmax(baseDuck * 0.45f, pulseDuck * 0.7f));
        }

        case 4:
        {
            const auto gateLength = juce::jmap(safeShape, 0.22f, 0.58f);
            if (safeRecovery <= gateLength)
                return 1.0f;

            const auto tail = juce::jlimit(0.0f, 1.0f, (safeRecovery - gateLength) / juce::jmax(0.001f, 1.0f - gateLength));
            return std::pow(1.0f - tail, 8.0f);
        }

        case 0:
        default:
            return std::pow(inverseRecovery, juce::jmap(safeShape, 0.35f, 4.5f));
    }
}

juce::String PumpCurveDisplay::curveName() const
{
    switch (curveIndex)
    {
        case 1: return "Tight";
        case 2: return "Garage";
        case 3: return "Stutter";
        case 4: return "Gate";
        case 0:
        default: return "Smooth";
    }
}

juce::String PumpCurveDisplay::rateName() const
{
    switch (rateIndex)
    {
        case 1: return "1/8";
        case 2: return "1/8T";
        case 3: return "1/16";
        case 0:
        default: return "1/4";
    }
}

juce::Rectangle<float> PumpCurveDisplay::plotBounds() const
{
    return getLocalBounds().toFloat().reduced(10.0f, 10.0f).withTrimmedTop(14.0f);
}
}
