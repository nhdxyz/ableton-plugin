#include "OutputSpectrumDisplay.h"

#include <cmath>

namespace UI
{
namespace
{
struct FrequencyRange
{
    const char* label;
    size_t startIndex;
    size_t endIndex;
    juce::uint32 colour;
};

constexpr std::array<FrequencyRange, 5> frequencyRanges {
    FrequencyRange { "SUB", 0, 2, 0xff7fd0e6 },
    FrequencyRange { "LOW", 2, 5, 0xffd7e37b },
    FrequencyRange { "MID", 5, 8, 0xffff8f78 },
    FrequencyRange { "PRES", 8, 11, 0xff9cb8ff },
    FrequencyRange { "AIR", 11, 12, 0xffd6f4ff }
};
}

float OutputSpectrumDisplay::clamp01(float value) noexcept
{
    return juce::jlimit(0.0f, 1.0f, value);
}

juce::Colour OutputSpectrumDisplay::bandColour(size_t index)
{
    static constexpr std::array<juce::uint32, bandCount> colours {
        0xff7fd0e6,
        0xff8ee6c9,
        0xffd7e37b,
        0xffffc36b,
        0xffffaa78,
        0xffff8f78,
        0xfff0a86e,
        0xffc4a7ff,
        0xff9cb8ff,
        0xff7bb7ff,
        0xff9dd6ff,
        0xffd6f4ff
    };

    return juce::Colour(colours[index % colours.size()]);
}

float OutputSpectrumDisplay::averageRange(const BandArray& values, size_t startIndex, size_t endIndex) noexcept
{
    if (startIndex >= endIndex || startIndex >= values.size())
        return 0.0f;

    endIndex = juce::jmin(endIndex, values.size());
    auto sum = 0.0f;
    auto count = 0.0f;
    for (auto index = startIndex; index < endIndex; ++index)
    {
        sum += clamp01(values[index]);
        count += 1.0f;
    }

    return count > 0.0f ? clamp01(sum / count) : 0.0f;
}

void OutputSpectrumDisplay::setLevels(const BandArray& newLevels, float newPeakLevel, bool newActive)
{
    auto changed = active != newActive || std::abs(peakLevel - newPeakLevel) > 0.001f;
    active = newActive;
    peakLevel = clamp01(newPeakLevel);

    for (size_t index = 0; index < levels.size(); ++index)
    {
        const auto nextLevel = clamp01(newLevels[index]);
        changed = changed || std::abs(levels[index] - nextLevel) > 0.001f;
        levels[index] = nextLevel;

        const auto decayedHold = active ? juce::jmax(0.0f, heldLevels[index] - 0.018f)
                                        : heldLevels[index] * 0.82f;
        const auto nextHold = juce::jmax(nextLevel, decayedHold);
        const auto cleanedHold = nextHold < 0.006f ? 0.0f : clamp01(nextHold);
        changed = changed || std::abs(heldLevels[index] - cleanedHold) > 0.001f;
        heldLevels[index] = cleanedHold;
    }

    if (changed)
    {
        updateTooltip();
        repaint();
    }
}

juce::String OutputSpectrumDisplay::getTooltip()
{
    return tooltip;
}

void OutputSpectrumDisplay::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto compact = bounds.getHeight() < 48.0f;
    auto content = bounds.reduced(8.0f, compact ? 4.0f : 7.0f);

    g.setColour(juce::Colour(0xff0c1114));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(active ? juce::Colour(0xff36545d) : juce::Colour(0xff243036));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    auto header = content.removeFromTop(compact ? 11.0f : 14.0f);
    g.setFont(juce::FontOptions(compact ? 8.5f : 9.5f, juce::Font::bold));
    g.setColour(active ? juce::Colour(0xff8ee6c9) : juce::Colour(0xff657178));
    g.drawText("SPECTRUM", header, juce::Justification::centredLeft);

    const auto peakText = peakLevel >= 0.82f ? "HOT" : (peakLevel >= 0.35f ? "LIVE" : "LOW");
    g.setColour(peakLevel >= 0.82f ? juce::Colour(0xffffa58f)
                                   : (active ? juce::Colour(0xffd7e3e0) : juce::Colour(0xff657178)));
    g.drawText(peakText, header, juce::Justification::centredRight);

    auto labels = content.removeFromBottom(compact ? 8.0f : 10.0f);
    auto plot = content.withTrimmedTop(compact ? 1.0f : 3.0f);
    const auto bandGap = 3.0f;
    const auto bandWidth = (plot.getWidth() - (bandGap * static_cast<float>(levels.size() - 1)))
        / static_cast<float>(levels.size());

    g.setColour(juce::Colour(0x243d4a50));
    for (auto line = 1; line < 3; ++line)
    {
        const auto y = plot.getY() + (plot.getHeight() * static_cast<float>(line) / 3.0f);
        g.drawHorizontalLine(juce::roundToInt(y), plot.getX(), plot.getRight());
    }

    juce::Path trace;
    auto bandArea = plot;
    for (size_t index = 0; index < levels.size(); ++index)
    {
        auto band = bandArea.removeFromLeft(bandWidth);
        bandArea.removeFromLeft(bandGap);

        const auto level = active ? clamp01(levels[index]) : 0.0f;
        const auto fillHeight = juce::jmax(2.0f, band.getHeight() * level);
        const auto fill = band.withTop(band.getBottom() - fillHeight);

        g.setColour(juce::Colour(0xff182025));
        g.fillRoundedRectangle(band, 2.0f);
        g.setColour(bandColour(index).withAlpha(active ? 0.84f : 0.18f));
        g.fillRoundedRectangle(fill, 2.0f);

        const auto holdLevel = clamp01(heldLevels[index]);
        if (holdLevel > 0.012f)
        {
            const auto heldY = band.getBottom() - (band.getHeight() * holdLevel);
            g.setColour(bandColour(index).brighter(0.28f).withAlpha(active ? 0.88f : 0.24f));
            g.drawLine(band.getX() + 1.0f, heldY, band.getRight() - 1.0f, heldY, 1.2f);
        }

        const auto x = band.getCentreX();
        const auto y = fill.getY();
        if (index == 0)
            trace.startNewSubPath(x, y);
        else
            trace.lineTo(x, y);
    }

    if (active)
    {
        g.setColour(juce::Colour(0xffd6f4ff).withAlpha(0.86f));
        g.strokePath(trace, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setFont(juce::FontOptions(compact ? 7.0f : 8.0f, juce::Font::bold));

    const auto segmentWidth = labels.getWidth() / static_cast<float>(frequencyRanges.size());
    for (size_t index = 0; index < frequencyRanges.size(); ++index)
    {
        auto labelArea = index + 1 == frequencyRanges.size() ? labels : labels.removeFromLeft(segmentWidth);
        const auto rangeEnergy = averageRange(levels, frequencyRanges[index].startIndex, frequencyRanges[index].endIndex);
        const auto accent = juce::Colour(frequencyRanges[index].colour);

        auto strip = labelArea.removeFromTop(compact ? 1.5f : 2.0f).reduced(1.0f, 0.0f);
        const auto stripWidth = juce::jmax(2.0f, strip.getWidth() * (active ? rangeEnergy : 0.0f));
        g.setColour(juce::Colour(0xff243036));
        g.fillRoundedRectangle(strip, 1.0f);
        g.setColour(accent.withAlpha(active ? 0.34f + (rangeEnergy * 0.54f) : 0.16f));
        g.fillRoundedRectangle(strip.withWidth(stripWidth), 1.0f);

        g.setColour(active && rangeEnergy > 0.34f ? accent.brighter(0.08f) : juce::Colour(0xff657178));
        const auto justification = index == 0 ? juce::Justification::centredLeft
                                 : (index + 1 == frequencyRanges.size() ? juce::Justification::centredRight
                                                                         : juce::Justification::centred);
        g.drawText(frequencyRanges[index].label, labelArea, justification);
    }
}

void OutputSpectrumDisplay::updateTooltip()
{
    auto dominantIndex = size_t {};
    auto dominantEnergy = 0.0f;
    for (size_t index = 0; index < frequencyRanges.size(); ++index)
    {
        const auto rangeEnergy = averageRange(levels, frequencyRanges[index].startIndex, frequencyRanges[index].endIndex);
        if (rangeEnergy > dominantEnergy)
        {
            dominantEnergy = rangeEnergy;
            dominantIndex = index;
        }
    }

    const auto peakPercent = juce::roundToInt(peakLevel * 100.0f);
    const auto holdPercent = juce::roundToInt(averageRange(heldLevels, 0, heldLevels.size()) * 100.0f);
    tooltip = "Output spectrum\nPeak " + juce::String(peakPercent)
        + "% | Held energy " + juce::String(holdPercent)
        + "% | Focus " + juce::String(active ? frequencyRanges[dominantIndex].label : "waiting for audio")
        + "\nSub/low/mid/presence/air balance is measured from the final output.";
}
}
