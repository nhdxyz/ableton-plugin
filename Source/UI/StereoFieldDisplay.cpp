#include "StereoFieldDisplay.h"

#include <cmath>

namespace UI
{
float StereoFieldDisplay::clamp01(float value) noexcept
{
    return juce::jlimit(0.0f, 1.0f, value);
}

float StereoFieldDisplay::clampBipolar(float value) noexcept
{
    return juce::jlimit(-1.0f, 1.0f, value);
}

juce::Colour StereoFieldDisplay::colourForCorrelation(float correlation) noexcept
{
    if (correlation < 0.0f)
        return juce::Colour(0xffff6b5e);

    if (correlation < 0.35f)
        return juce::Colour(0xffffcf5a);

    return juce::Colour(0xff8ee6c9);
}

juce::Colour StereoFieldDisplay::colourForRisk(float risk) noexcept
{
    if (risk >= 0.28f)
        return juce::Colour(0xffff6b5e);

    if (risk >= 0.16f)
        return juce::Colour(0xffffcf5a);

    return juce::Colour(0xff8ee6c9);
}

juce::String StereoFieldDisplay::statusTextForState(const State& state)
{
    if (! state.active)
        return "IDLE";

    if (state.correlation < 0.0f)
        return "PHASE";

    if (state.lowStereoRisk >= 0.28f)
        return "LOW SIDE";

    if (state.width >= 0.54f)
        return "WIDE";

    if (state.width <= 0.14f)
        return "MONO";

    return "FOCUS";
}

void StereoFieldDisplay::setState(const State& newState)
{
    const auto nextCorrelation = clampBipolar(newState.correlation);
    const auto nextWidth = clamp01(newState.width);
    const auto nextBalance = clampBipolar(newState.balance);
    const auto nextRisk = clamp01(newState.lowStereoRisk);

    const auto changed = state.active != newState.active
        || std::abs(state.correlation - nextCorrelation) > 0.002f
        || std::abs(state.width - nextWidth) > 0.002f
        || std::abs(state.balance - nextBalance) > 0.002f
        || std::abs(state.lowStereoRisk - nextRisk) > 0.002f;

    state.active = newState.active;
    state.correlation = nextCorrelation;
    state.width = nextWidth;
    state.balance = nextBalance;
    state.lowStereoRisk = nextRisk;

    if (changed)
        repaint();
}

void StereoFieldDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    auto content = bounds.reduced(9.0f, 7.0f);

    g.setColour(juce::Colour(0xff0c1114));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(state.active ? juce::Colour(0xff33464b) : juce::Colour(0xff243036));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    auto header = content.removeFromTop(14.0f);
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.setColour(state.active ? juce::Colour(0xff8ee6c9) : juce::Colour(0xff657178));
    g.drawFittedText("STEREO", header.toNearestInt(), juce::Justification::centredLeft, 1);

    const auto status = statusTextForState(state);
    g.setColour(status == "PHASE" || status == "LOW SIDE" ? juce::Colour(0xffff6b5e)
                                                           : (status == "WIDE" ? juce::Colour(0xffffcf5a)
                                                                               : juce::Colour(0xffd7e3e0)));
    g.drawFittedText(status, header.toNearestInt(), juce::Justification::centredRight, 1, 0.7f);

    auto scope = content.removeFromLeft(juce::jlimit(54.0f, 72.0f, content.getWidth() * 0.38f)).reduced(0.0f, 3.0f);
    content.removeFromLeft(8.0f);

    const auto scopeSize = juce::jmin(scope.getWidth(), scope.getHeight());
    auto scopeBox = juce::Rectangle<float> { scope.getX(), scope.getY(), scopeSize, scopeSize };
    scopeBox.setCentre(scope.getCentre());
    const auto centre = scopeBox.getCentre();
    const auto radius = scopeBox.getWidth() * 0.5f;

    g.setColour(juce::Colour(0xff151f23));
    g.fillEllipse(scopeBox);
    g.setColour(juce::Colour(0xff2f3f45));
    g.drawEllipse(scopeBox, 1.0f);
    g.drawLine(centre.x - radius, centre.y, centre.x + radius, centre.y, 0.8f);
    g.drawLine(centre.x, centre.y - radius, centre.x, centre.y + radius, 0.8f);

    const auto activeAlpha = state.active ? 0.88f : 0.18f;
    const auto widthRadius = radius * juce::jlimit(0.08f, 0.92f, 0.16f + (state.width * 0.76f));
    const auto balanceOffset = radius * 0.55f * state.balance;
    const auto corrHeight = radius * juce::jlimit(0.12f, 0.92f, 0.18f + ((state.correlation + 1.0f) * 0.37f));

    juce::Path field;
    field.addEllipse(centre.x - widthRadius + balanceOffset,
                     centre.y - corrHeight,
                     widthRadius * 2.0f,
                     corrHeight * 2.0f);
    g.setColour(colourForCorrelation(state.correlation).withAlpha(activeAlpha * 0.18f));
    g.fillPath(field);
    g.setColour(colourForCorrelation(state.correlation).withAlpha(activeAlpha));
    g.strokePath(field, juce::PathStrokeType(1.4f));

    g.setColour(colourForRisk(state.lowStereoRisk).withAlpha(activeAlpha));
    g.fillEllipse(centre.x + balanceOffset - 2.5f, centre.y - 2.5f, 5.0f, 5.0f);

    auto bars = content.withTrimmedTop(1.0f);
    drawBar(g, bars.removeFromTop(13.0f), "CORR", state.correlation, colourForCorrelation(state.correlation), true);
    bars.removeFromTop(4.0f);
    drawBar(g, bars.removeFromTop(13.0f), "WIDTH", state.width, juce::Colour(0xff7bb7ff), false);
    bars.removeFromTop(4.0f);
    drawBar(g, bars.removeFromTop(13.0f), "BASS SIDE", state.lowStereoRisk, colourForRisk(state.lowStereoRisk), false);
}

void StereoFieldDisplay::drawBar(juce::Graphics& g,
                                 juce::Rectangle<float> area,
                                 const juce::String& label,
                                 float value,
                                 juce::Colour colour,
                                 bool bipolar) const
{
    auto labelArea = area.removeFromLeft(48.0f);
    auto track = area.reduced(0.0f, 3.0f);

    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.setColour(colour.withAlpha(state.active ? 0.96f : 0.28f));
    g.drawFittedText(label, labelArea.toNearestInt(), juce::Justification::centredLeft, 1, 0.62f);

    g.setColour(juce::Colour(0xff182126));
    g.fillRoundedRectangle(track, 2.0f);

    if (bipolar)
    {
        const auto centreX = track.getCentreX();
        const auto amount = (track.getWidth() * 0.5f) * std::abs(clampBipolar(value));
        auto fill = value >= 0.0f ? track.withX(centreX).withWidth(amount)
                                  : track.withX(centreX - amount).withWidth(amount);
        g.setColour(colour.withAlpha(state.active ? 0.76f : 0.18f));
        g.fillRoundedRectangle(fill, 2.0f);
        g.setColour(juce::Colour(0xff506169));
        g.drawVerticalLine(juce::roundToInt(centreX), track.getY(), track.getBottom());
    }
    else
    {
        const auto proportion = clamp01(value);
        g.setColour(colour.withAlpha(state.active ? 0.76f : 0.18f));
        g.fillRoundedRectangle(track.withWidth(track.getWidth() * proportion), 2.0f);
    }
}
}
