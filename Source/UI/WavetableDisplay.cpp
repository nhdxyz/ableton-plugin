#include "WavetableDisplay.h"

#include <cmath>

namespace UI
{
namespace
{
float sineHarmonic(float phase, float harmonic, float phaseOffset = 0.0f)
{
    return std::sin(juce::MathConstants<float>::twoPi * ((phase * harmonic) + phaseOffset));
}

float drawFrame(int frameIndex, float phase)
{
    switch (frameIndex)
    {
        case 0:
            return (sineHarmonic(phase, 1.0f)
                    + (sineHarmonic(phase, 2.0f) * 0.46f)
                    + (sineHarmonic(phase, 3.0f) * 0.28f)
                    + (sineHarmonic(phase, 6.0f) * 0.16f)) * 0.58f;

        case 1:
        {
            auto sample = 0.0f;
            for (auto harmonic = 1; harmonic <= 12; ++harmonic)
                sample += sineHarmonic(phase,
                                       static_cast<float>(harmonic),
                                       0.0f) * ((harmonic % 2 == 0 ? -0.72f : 1.0f) / static_cast<float>(harmonic));
            return sample * 0.62f;
        }

        case 2:
        {
            auto sample = 0.0f;
            for (auto harmonic = 1; harmonic <= 13; harmonic += 2)
                sample += sineHarmonic(phase, static_cast<float>(harmonic)) / static_cast<float>(harmonic);
            return sample * 0.82f;
        }

        case 3:
            return ((sineHarmonic(phase, 1.0f) * 0.82f)
                    + (sineHarmonic(phase, 2.0f, 0.08f) * 0.48f)
                    + (sineHarmonic(phase, 5.0f, 0.19f) * 0.33f)
                    + (sineHarmonic(phase, 8.0f, 0.31f) * 0.18f)) * 0.72f;

        default:
        {
            const auto edge = (phase * 2.0f) - 1.0f;
            const auto fold = std::sin(edge * juce::MathConstants<float>::pi * 1.85f);
            return (edge * 0.55f) + (fold * 0.45f);
        }
    }
}
}

void WavetableDisplay::setState(float newOsc1Position, float newOsc2Position, bool newOsc1Active, bool newOsc2Active)
{
    newOsc1Position = juce::jlimit(0.0f, 1.0f, newOsc1Position);
    newOsc2Position = juce::jlimit(0.0f, 1.0f, newOsc2Position);

    if (std::abs(osc1Position - newOsc1Position) < 0.001f
        && std::abs(osc2Position - newOsc2Position) < 0.001f
        && osc1Active == newOsc1Active
        && osc2Active == newOsc2Active)
    {
        return;
    }

    osc1Position = newOsc1Position;
    osc2Position = newOsc2Position;
    osc1Active = newOsc1Active;
    osc2Active = newOsc2Active;
    repaint();
}

void WavetableDisplay::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto plot = bounds.reduced(8.0f, 8.0f);
    const auto inactive = ! osc1Active && ! osc2Active;

    g.setColour(juce::Colour(0xff101619));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(inactive ? juce::Colour(0xff27343a) : juce::Colour(0xff40535a));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    g.setColour(juce::Colour(0x223d4a50));
    for (auto line = 1; line < 4; ++line)
    {
        const auto y = plot.getY() + (plot.getHeight() * static_cast<float>(line) / 4.0f);
        g.drawHorizontalLine(juce::roundToInt(y), plot.getX(), plot.getRight());
    }

    g.setColour(juce::Colour(0x335b6e75));
    g.drawLine(plot.getX(), plot.getCentreY(), plot.getRight(), plot.getCentreY(), 1.0f);

    const auto drawPath = [&] (float position, juce::Colour colour, float thickness)
    {
        g.setColour(colour);
        g.strokePath(makePath(plot, position), juce::PathStrokeType(thickness));
        const auto markerX = plot.getX() + (plot.getWidth() * juce::jlimit(0.0f, 1.0f, position));
        g.drawLine(markerX, plot.getY(), markerX, plot.getBottom(), 1.0f);
    };

    if (osc2Active)
        drawPath(osc2Position, juce::Colour(0x99c4a2ff), 1.2f);

    if (osc1Active)
        drawPath(osc1Position, juce::Colour(0xff8ee6c9), 1.6f);

    if (inactive)
    {
        g.setColour(juce::Colour(0xff617078));
        g.setFont(juce::FontOptions(10.0f));
        g.drawFittedText("WT IDLE", getLocalBounds().reduced(8), juce::Justification::centred, 1);
    }
    else
    {
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        const auto labelBounds = getLocalBounds().removeFromTop(16).reduced(8, 1);
        g.setColour(juce::Colour(0xff8ee6c9));
        if (osc1Active)
            g.drawText("O1 " + juce::String(juce::roundToInt(osc1Position * 100.0f)) + "%",
                       labelBounds,
                       juce::Justification::centredLeft);

        if (osc2Active)
        {
            g.setColour(juce::Colour(0xffc4a2ff));
            g.drawText("O2 " + juce::String(juce::roundToInt(osc2Position * 100.0f)) + "%",
                       labelBounds,
                       juce::Justification::centredRight);
        }
    }
}

float WavetableDisplay::sampleFrame(float phase, float position)
{
    constexpr auto lastFrame = 4.0f;
    const auto framePosition = juce::jlimit(0.0f, 1.0f, position) * lastFrame;
    const auto lowerFrame = juce::jlimit(0, static_cast<int>(lastFrame), static_cast<int>(std::floor(framePosition)));
    const auto upperFrame = juce::jlimit(0, static_cast<int>(lastFrame), lowerFrame + 1);
    const auto mix = juce::jlimit(0.0f, 1.0f, framePosition - static_cast<float>(lowerFrame));
    const auto smoothMix = mix * mix * (3.0f - (2.0f * mix));
    const auto lowerSample = drawFrame(lowerFrame, phase);
    const auto upperSample = drawFrame(upperFrame, phase);

    return lowerSample + ((upperSample - lowerSample) * smoothMix);
}

juce::Path WavetableDisplay::makePath(juce::Rectangle<float> bounds, float position)
{
    juce::Path path;
    constexpr auto points = 96;

    for (auto index = 0; index < points; ++index)
    {
        const auto phase = static_cast<float>(index) / static_cast<float>(points - 1);
        const auto sample = juce::jlimit(-1.0f, 1.0f, sampleFrame(phase, position));
        const auto x = bounds.getX() + (bounds.getWidth() * phase);
        const auto y = bounds.getCentreY() - (sample * bounds.getHeight() * 0.42f);

        if (index == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }

    return path;
}
}
