#include "OutputMeter.h"

#include <cmath>

namespace UI
{
void OutputMeter::setLevels(float newPeakLeft, float newPeakRight, float newRmsLeft, float newRmsRight)
{
    peakLeft = juce::jlimit(0.0f, 2.0f, newPeakLeft);
    peakRight = juce::jlimit(0.0f, 2.0f, newPeakRight);
    rmsLeft = juce::jlimit(0.0f, 2.0f, newRmsLeft);
    rmsRight = juce::jlimit(0.0f, 2.0f, newRmsRight);
    const auto maxPeak = juce::jmax(peakLeft, peakRight);
    const auto maxRms = juce::jmax(rmsLeft, rmsRight);
    safetyText = safetyTextForLevels(maxPeak, maxRms);
    safetyColour = safetyColourForText(safetyText);
    repaint();
}

void OutputMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);

    g.setColour(juce::Colour(0xff101619));
    g.fillRoundedRectangle(bounds, 5.0f);
    g.setColour(juce::Colour(0xff2b363c));
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

    auto content = getLocalBounds().reduced(8, 5);
    auto labelArea = content.removeFromLeft(38);

    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff8ee6c9));
    g.drawFittedText("OUT", labelArea.removeFromTop(15), juce::Justification::centredLeft, 1);
    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.setColour(safetyColour);
    g.drawFittedText(safetyText, labelArea, juce::Justification::centredLeft, 1);

    auto meterArea = content.toFloat().withTrimmedLeft(2.0f);
    const auto channelHeight = (meterArea.getHeight() - 4.0f) * 0.5f;
    drawChannel(g, meterArea.removeFromTop(channelHeight), peakLeft, rmsLeft);
    meterArea.removeFromTop(4.0f);
    drawChannel(g, meterArea.withHeight(channelHeight), peakRight, rmsRight);
}

float OutputMeter::levelToProportion(float linearLevel) noexcept
{
    if (linearLevel <= 0.000001f)
        return 0.0f;

    const auto db = 20.0f * std::log10(linearLevel);
    return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
}

juce::Colour OutputMeter::colourForLevel(float linearLevel) noexcept
{
    const auto db = linearLevel <= 0.000001f ? -60.0f : 20.0f * std::log10(linearLevel);

    if (db >= -3.0f)
        return juce::Colour(0xffff6b5e);

    if (db >= -9.0f)
        return juce::Colour(0xffffcf5a);

    return juce::Colour(0xff8ee6c9);
}

juce::String OutputMeter::safetyTextForLevels(float peak, float rms)
{
    const auto peakDb = peak <= 0.000001f ? -60.0f : 20.0f * std::log10(peak);
    const auto rmsDb = rms <= 0.000001f ? -60.0f : 20.0f * std::log10(rms);

    if (peak >= 0.995f)
        return "CLIP";

    if (peakDb >= -3.0f || rmsDb >= -9.0f)
        return "HOT";

    if (peakDb <= -30.0f && rmsDb <= -42.0f)
        return "LOW";

    return "SAFE";
}

juce::Colour OutputMeter::safetyColourForText(const juce::String& text) noexcept
{
    if (text == "CLIP")
        return juce::Colour(0xffff6b5e);

    if (text == "HOT")
        return juce::Colour(0xffffcf5a);

    if (text == "LOW")
        return juce::Colour(0xff7d8b90);

    return juce::Colour(0xff8ee6c9);
}

void OutputMeter::drawChannel(juce::Graphics& g, juce::Rectangle<float> area, float peak, float rms) const
{
    const auto track = area.reduced(0.0f, 1.0f);

    g.setColour(juce::Colour(0xff1d272b));
    g.fillRoundedRectangle(track, 2.0f);

    g.setColour(juce::Colour(0xff2f3a40));
    for (auto tick = 1; tick < 4; ++tick)
    {
        const auto x = track.getX() + (track.getWidth() * static_cast<float>(tick) * 0.25f);
        g.drawVerticalLine(static_cast<int>(std::round(x)), track.getY() + 1.0f, track.getBottom() - 1.0f);
    }

    const auto rmsWidth = track.getWidth() * levelToProportion(rms);
    if (rmsWidth > 0.0f)
    {
        g.setColour(colourForLevel(rms).withAlpha(0.72f));
        g.fillRoundedRectangle(track.withWidth(rmsWidth), 2.0f);
    }

    const auto peakPosition = levelToProportion(peak);
    if (peakPosition > 0.0f)
    {
        const auto peakX = track.getX() + (track.getWidth() * peakPosition);
        g.setColour(colourForLevel(peak));
        g.drawLine(peakX, track.getY(), peakX, track.getBottom(), 1.4f);
    }
}
}
