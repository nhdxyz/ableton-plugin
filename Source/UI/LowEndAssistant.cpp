#include "LowEndAssistant.h"

#include <cmath>

namespace UI
{
void LowEndAssistant::setState(const State& newState)
{
    state = newState;
    repaint();
}

void LowEndAssistant::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);

    g.setColour(juce::Colour(0xff101619));
    g.fillRoundedRectangle(bounds, 5.0f);
    g.setColour(juce::Colour(0xff2b363c));
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

    auto content = getLocalBounds().reduced(8, 4);
    auto header = content.removeFromTop(17);
    auto titleArea = header.removeFromLeft(38);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff8ee6c9));
    g.drawFittedText("CLUB", titleArea, juce::Justification::centredLeft, 1);

    auto rootArea = header.removeFromLeft(78);
    g.setColour(juce::Colour(0xffdce7e4));
    g.drawFittedText(state.rootText, rootArea, juce::Justification::centredLeft, 1);

    auto phaseArea = header.removeFromRight(76).reduced(2, 1).toFloat();
    const auto phaseColour = guidanceColour(state.guidanceLevel);
    g.setColour(phaseColour.withAlpha(0.18f));
    g.fillRoundedRectangle(phaseArea, 3.0f);
    g.setColour(phaseColour);
    g.drawRoundedRectangle(phaseArea, 3.0f, 1.0f);
    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.drawFittedText(state.phaseText, phaseArea.toNearestInt().reduced(2, 0), juce::Justification::centred, 1);

    const auto monoText = state.monoEnabled ? "MONO"
        : state.widthEnabled ? (juce::String("<") + juce::String(static_cast<int>(std::round(state.monoCrossoverHz))) + "Hz")
                             : "LOW WIDE";
    auto guideArea = content.removeFromTop(12);
    auto monoArea = guideArea.removeFromLeft(72);
    g.setColour(state.monoEnabled || state.widthEnabled ? juce::Colour(0xff8ee6c9) : juce::Colour(0xffffcf5a));
    g.drawFittedText(monoText, monoArea, juce::Justification::centredLeft, 1);
    g.setColour(guidanceColour(state.guidanceLevel));
    g.setFont(juce::FontOptions(8.8f, juce::Font::bold));
    g.drawFittedText(state.guidanceText, guideArea, juce::Justification::centredLeft, 1);

    const auto meterWidth = content.getWidth() / 3;
    drawMeter(g, content.removeFromLeft(meterWidth).reduced(3, 0), "SUB", state.subRms, false);
    drawMeter(g, content.removeFromLeft(meterWidth).reduced(3, 0), "SIDE", state.lowStereoRisk, true);
    drawMeter(g, content.reduced(3, 0), state.guardEnabled ? "SAFE" : "HEAD", state.outputPeak, false);
}

float LowEndAssistant::levelToProportion(float linearLevel) noexcept
{
    if (linearLevel <= 0.000001f)
        return 0.0f;

    const auto db = 20.0f * std::log10(linearLevel);
    return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
}

juce::Colour LowEndAssistant::levelColour(float linearLevel) noexcept
{
    const auto db = linearLevel <= 0.000001f ? -60.0f : 20.0f * std::log10(linearLevel);

    if (db >= -3.0f)
        return juce::Colour(0xffff6b5e);

    if (db >= -9.0f)
        return juce::Colour(0xffffcf5a);

    return juce::Colour(0xff8ee6c9);
}

juce::Colour LowEndAssistant::riskColour(float risk) noexcept
{
    if (risk >= 0.28f)
        return juce::Colour(0xffff6b5e);

    if (risk >= 0.16f)
        return juce::Colour(0xffffcf5a);

    return juce::Colour(0xff8ee6c9);
}

juce::Colour LowEndAssistant::guidanceColour(int guidanceLevel) noexcept
{
    if (guidanceLevel >= 2)
        return juce::Colour(0xffff6b5e);

    if (guidanceLevel == 1)
        return juce::Colour(0xffffcf5a);

    return juce::Colour(0xff8ee6c9);
}

void LowEndAssistant::drawMeter(juce::Graphics& g, juce::Rectangle<int> area, const juce::String& label, float value, bool riskMode) const
{
    auto labelArea = area.removeFromTop(11);
    auto track = area.removeFromBottom(juce::jmin(6, juce::jmax(3, area.getHeight()))).reduced(0, 1).toFloat();
    const auto proportion = riskMode ? juce::jlimit(0.0f, 1.0f, value)
                                     : levelToProportion(value);
    const auto colour = riskMode ? riskColour(value)
                                 : levelColour(value);

    g.setFont(juce::FontOptions(8.8f, juce::Font::bold));
    g.setColour(colour);
    g.drawFittedText(label, labelArea, juce::Justification::centred, 1);

    g.setColour(juce::Colour(0xff1d272b));
    g.fillRoundedRectangle(track, 2.0f);

    if (proportion > 0.0f)
    {
        g.setColour(colour.withAlpha(0.78f));
        g.fillRoundedRectangle(track.withWidth(track.getWidth() * proportion), 2.0f);
    }
}
}
