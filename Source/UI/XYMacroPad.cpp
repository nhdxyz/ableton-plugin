#include "XYMacroPad.h"

#include <cmath>

namespace UI
{
XYMacroPad::XYMacroPad()
{
    setInterceptsMouseClicks(true, true);
    setRepaintsOnMouseActivity(true);
}

void XYMacroPad::setValues(float newX, float newY)
{
    newX = juce::jlimit(0.0f, 1.0f, newX);
    newY = juce::jlimit(0.0f, 1.0f, newY);

    if (std::abs(x - newX) < 0.001f && std::abs(y - newY) < 0.001f)
        return;

    x = newX;
    y = newY;
    repaint();
}

juce::String XYMacroPad::getTooltip()
{
    return "XY performance control for Motion and Space macros";
}

void XYMacroPad::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const auto pad = getPadBounds();
    const auto accent = juce::Colour(0xff8ee6c9);
    const auto spaceColour = juce::Colour(0xffb7a4ff);

    g.setColour(juce::Colour(0xff101619));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour(0xff2b363c));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(accent);
    g.drawFittedText("MOTION", getLocalBounds().reduced(9, 5).removeFromTop(13), juce::Justification::centredLeft, 1);
    g.setColour(spaceColour);
    g.drawFittedText("SPACE", getLocalBounds().reduced(9, 5).removeFromTop(13), juce::Justification::centredRight, 1);

    g.setColour(juce::Colour(0xff192226));
    g.fillRoundedRectangle(pad, 4.0f);
    g.setColour(juce::Colour(0xff2f3b40));
    g.drawRoundedRectangle(pad, 4.0f, 1.0f);

    g.setColour(juce::Colour(0xff263238));
    for (auto index = 1; index < 4; ++index)
    {
        const auto verticalX = juce::jmap(static_cast<float>(index), 0.0f, 4.0f, pad.getX(), pad.getRight());
        const auto horizontalY = juce::jmap(static_cast<float>(index), 0.0f, 4.0f, pad.getY(), pad.getBottom());
        g.drawVerticalLine(static_cast<int>(verticalX), pad.getY(), pad.getBottom());
        g.drawHorizontalLine(static_cast<int>(horizontalY), pad.getX(), pad.getRight());
    }

    const auto handleRange = pad.reduced(7.0f);
    const auto handle = juce::Point<float> {
        juce::jmap(x, 0.0f, 1.0f, handleRange.getX(), handleRange.getRight()),
        juce::jmap(y, 0.0f, 1.0f, handleRange.getBottom(), handleRange.getY())
    };

    g.setColour(accent.withAlpha(0.26f));
    g.drawLine(pad.getX(), handle.y, pad.getRight(), handle.y, 1.5f);
    g.setColour(spaceColour.withAlpha(0.26f));
    g.drawLine(handle.x, pad.getY(), handle.x, pad.getBottom(), 1.5f);

    juce::ColourGradient glow(accent.withAlpha(0.35f), handle.x, handle.y,
                              spaceColour.withAlpha(0.18f), handle.x + 20.0f, handle.y - 18.0f,
                              true);
    g.setGradientFill(glow);
    g.fillEllipse(juce::Rectangle<float> { 24.0f, 24.0f }.withCentre(handle));

    g.setColour(juce::Colour(0xff0d1113));
    g.fillEllipse(juce::Rectangle<float> { 12.0f, 12.0f }.withCentre(handle));
    g.setColour(accent);
    g.drawEllipse(juce::Rectangle<float> { 12.0f, 12.0f }.withCentre(handle), 1.6f);

    g.setFont(juce::FontOptions(9.0f, juce::Font::plain));
    g.setColour(juce::Colour(0xffa8b6b8));
    const auto valueText = juce::String(juce::roundToInt(x * 100.0f)) + " / " + juce::String(juce::roundToInt(y * 100.0f));
    g.drawFittedText(valueText, getLocalBounds().reduced(9, 5).removeFromBottom(12), juce::Justification::centredRight, 1);
}

void XYMacroPad::mouseDown(const juce::MouseEvent& event)
{
    applyMousePosition(event.position);
}

void XYMacroPad::mouseDrag(const juce::MouseEvent& event)
{
    applyMousePosition(event.position);
}

juce::Rectangle<float> XYMacroPad::getPadBounds() const
{
    return getLocalBounds().toFloat().reduced(9.0f, 20.0f).withTrimmedBottom(2.0f);
}

void XYMacroPad::applyMousePosition(juce::Point<float> position)
{
    const auto pad = getPadBounds();
    const auto nextX = juce::jlimit(0.0f, 1.0f, (position.x - pad.getX()) / juce::jmax(1.0f, pad.getWidth()));
    const auto nextY = juce::jlimit(0.0f, 1.0f, (pad.getBottom() - position.y) / juce::jmax(1.0f, pad.getHeight()));

    setValues(nextX, nextY);

    if (onChange)
        onChange(x, y);
}
}
