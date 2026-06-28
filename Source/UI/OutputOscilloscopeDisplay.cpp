#include "OutputOscilloscopeDisplay.h"

#include <cmath>

namespace UI
{
float OutputOscilloscopeDisplay::clamp01(float value) noexcept
{
    return juce::jlimit(0.0f, 1.0f, value);
}

void OutputOscilloscopeDisplay::setSamples(const SampleArray& newSamples,
                                           float newPeakLevel,
                                           float newTransientLevel,
                                           bool newActive)
{
    auto changed = active != newActive
        || std::abs(peakLevel - newPeakLevel) > 0.001f
        || std::abs(transientLevel - newTransientLevel) > 0.001f;

    active = newActive;
    peakLevel = clamp01(newPeakLevel);
    transientLevel = clamp01(newTransientLevel);

    for (size_t index = 0; index < samples.size(); ++index)
    {
        const auto next = juce::jlimit(-1.0f, 1.0f, newSamples[index]);
        changed = changed || std::abs(samples[index] - next) > 0.001f;
        samples[index] = next;
    }

    if (changed)
    {
        updateTooltip();
        repaint();
    }
}

juce::String OutputOscilloscopeDisplay::getTooltip()
{
    return tooltip;
}

void OutputOscilloscopeDisplay::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    auto content = bounds.reduced(10.0f, 5.0f);

    g.setColour(juce::Colour(0xff0c1114));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(active ? juce::Colour(0xff385058) : juce::Colour(0xff243036));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    auto header = content.removeFromTop(12.0f);
    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.setColour(active ? juce::Colour(0xff8ee6c9) : juce::Colour(0xff657178));
    g.drawText("SCOPE", header, juce::Justification::centredLeft);

    const auto status = peakLevel >= 0.92f ? "CLIP"
                      : (transientLevel >= 0.62f ? "PUNCH"
                      : (active ? "LIVE" : "LOW"));
    g.setColour(peakLevel >= 0.92f ? juce::Colour(0xffff9a78)
                                   : (transientLevel >= 0.62f ? juce::Colour(0xffffd47a)
                                                              : juce::Colour(0xffd7e3e0)));
    g.drawText(status, header, juce::Justification::centredRight);

    auto plot = content.withTrimmedTop(2.0f);
    const auto centreY = plot.getCentreY();
    const auto plotHeight = plot.getHeight();

    g.setColour(juce::Colour(0x223d4a50));
    g.drawHorizontalLine(juce::roundToInt(centreY), plot.getX(), plot.getRight());
    g.drawVerticalLine(juce::roundToInt(plot.getX() + plot.getWidth() * 0.25f), plot.getY(), plot.getBottom());
    g.drawVerticalLine(juce::roundToInt(plot.getX() + plot.getWidth() * 0.50f), plot.getY(), plot.getBottom());
    g.drawVerticalLine(juce::roundToInt(plot.getX() + plot.getWidth() * 0.75f), plot.getY(), plot.getBottom());

    juce::Path waveform;
    for (size_t index = 0; index < samples.size(); ++index)
    {
        const auto normalisedX = static_cast<float>(index) / static_cast<float>(samples.size() - 1);
        const auto x = plot.getX() + normalisedX * plot.getWidth();
        const auto sample = active ? juce::jlimit(-1.0f, 1.0f, samples[index]) : 0.0f;
        const auto y = centreY - (sample * plotHeight * 0.44f);

        if (index == 0)
            waveform.startNewSubPath(x, y);
        else
            waveform.lineTo(x, y);
    }

    g.setColour(juce::Colour(0xff8ee6c9).withAlpha(active ? 0.92f : 0.20f));
    g.strokePath(waveform, juce::PathStrokeType(active ? 1.45f : 1.0f,
                                                juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    const auto transientWidth = juce::jlimit(3.0f, plot.getWidth(), plot.getWidth() * transientLevel);
    auto transientBar = plot.removeFromBottom(3.0f).withWidth(transientWidth);
    g.setColour((peakLevel >= 0.92f ? juce::Colour(0xffff9a78) : juce::Colour(0xffffd47a)).withAlpha(active ? 0.82f : 0.18f));
    g.fillRoundedRectangle(transientBar, 1.5f);
}

void OutputOscilloscopeDisplay::updateTooltip()
{
    const auto peakPercent = juce::roundToInt(peakLevel * 100.0f);
    const auto transientPercent = juce::roundToInt(transientLevel * 100.0f);
    tooltip = "Output oscilloscope\nPeak " + juce::String(peakPercent)
        + "% | Transient " + juce::String(transientPercent)
        + "% | " + (active ? "live final output" : "waiting for audio");
}
}
