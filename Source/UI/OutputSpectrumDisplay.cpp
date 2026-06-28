#include "OutputSpectrumDisplay.h"

namespace UI
{
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
    }

    if (changed)
        repaint();
}

void OutputSpectrumDisplay::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    auto content = bounds.reduced(10.0f, 7.0f);

    g.setColour(juce::Colour(0xff0c1114));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(active ? juce::Colour(0xff36545d) : juce::Colour(0xff243036));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    auto header = content.removeFromTop(14.0f);
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.setColour(active ? juce::Colour(0xff8ee6c9) : juce::Colour(0xff657178));
    g.drawText("SPECTRUM", header, juce::Justification::centredLeft);

    const auto peakText = peakLevel >= 0.82f ? "HOT" : (peakLevel >= 0.35f ? "LIVE" : "LOW");
    g.setColour(peakLevel >= 0.82f ? juce::Colour(0xffffa58f)
                                   : (active ? juce::Colour(0xffd7e3e0) : juce::Colour(0xff657178)));
    g.drawText(peakText, header, juce::Justification::centredRight);

    auto plot = content.withTrimmedTop(3.0f);
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

    auto labels = bounds.reduced(10.0f, 4.0f).removeFromBottom(10.0f);
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff657178));
    g.drawText("SUB", labels.removeFromLeft(36.0f), juce::Justification::centredLeft);
    g.drawText("LOW", labels.removeFromLeft(44.0f), juce::Justification::centred);
    g.drawText("MID", labels.removeFromLeft(54.0f), juce::Justification::centred);
    g.drawText("PRES", labels.removeFromLeft(56.0f), juce::Justification::centred);
    g.drawText("AIR", labels, juce::Justification::centredRight);
}
}
