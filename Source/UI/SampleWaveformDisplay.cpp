#include "SampleWaveformDisplay.h"

#include <cmath>
#include <utility>

namespace UI
{
SampleWaveformDisplay::SampleWaveformDisplay()
{
    setInterceptsMouseClicks(true, true);
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
}

void SampleWaveformDisplay::setOverview(Sampler::SamplePeakOverview newOverview)
{
    if (overview.fileName == newOverview.fileName
        && overview.totalSamples == newOverview.totalSamples
        && overview.minimums.size() == newOverview.minimums.size())
    {
        overview = std::move(newOverview);
        repaint();
        return;
    }

    overview = std::move(newOverview);
    repaint();
}

void SampleWaveformDisplay::setRange(float newStart, float newEnd)
{
    newStart = juce::jlimit(0.0f, 1.0f, newStart);
    newEnd = juce::jlimit(0.0f, 1.0f, newEnd);

    if (std::abs(startNormalised - newStart) < 0.001f
        && std::abs(endNormalised - newEnd) < 0.001f)
    {
        return;
    }

    startNormalised = newStart;
    endNormalised = newEnd;
    repaint();
}

juce::String SampleWaveformDisplay::getTooltip()
{
    if (! overview.isValid())
        return "No sample loaded";

    const auto seconds = overview.sourceSampleRate > 0.0
        ? static_cast<double>(overview.totalSamples) / overview.sourceSampleRate
        : 0.0;
    const auto selectedPercent = juce::roundToInt(std::abs(endNormalised - startNormalised) * 100.0f);
    return overview.fileName + " | " + juce::String(seconds, 1) + "s | " + juce::String(selectedPercent) + "% selected";
}

void SampleWaveformDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const auto plot = plotBounds();
    const auto accent = juce::Colour(0xff8ee6c9);
    const auto regionColour = juce::Colour(0xffb7a4ff);

    g.setColour(juce::Colour(0xff101619));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour(0xff2a363c));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    g.setColour(juce::Colour(0xff243036));
    g.drawHorizontalLine(static_cast<int>(std::round(plot.getCentreY())), plot.getX(), plot.getRight());

    if (! overview.isValid())
    {
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff617078));
        g.drawFittedText("NO SAMPLE", getLocalBounds().reduced(8), juce::Justification::centred, 1);
        return;
    }

    juce::Path waveformFill;
    const auto pointCount = static_cast<int>(overview.minimums.size());
    const auto xForIndex = [plot, pointCount] (int index)
    {
        return juce::jmap(static_cast<float>(index),
                          0.0f,
                          static_cast<float>(juce::jmax(1, pointCount - 1)),
                          plot.getX(),
                          plot.getRight());
    };
    const auto yForValue = [plot] (float value)
    {
        return juce::jmap(juce::jlimit(-1.0f, 1.0f, value), -1.0f, 1.0f, plot.getBottom(), plot.getY());
    };

    waveformFill.startNewSubPath(plot.getX(), yForValue(overview.maximums.front()));
    for (auto index = 1; index < pointCount; ++index)
        waveformFill.lineTo(xForIndex(index), yForValue(overview.maximums[static_cast<size_t>(index)]));
    for (auto index = pointCount - 1; index >= 0; --index)
        waveformFill.lineTo(xForIndex(index), yForValue(overview.minimums[static_cast<size_t>(index)]));
    waveformFill.closeSubPath();

    g.setColour(accent.withAlpha(0.28f));
    g.fillPath(waveformFill);
    g.setColour(accent.withAlpha(0.8f));
    g.strokePath(waveformFill, juce::PathStrokeType(1.1f));

    const auto orderedStart = juce::jmin(startNormalised, endNormalised);
    const auto orderedEnd = juce::jmax(startNormalised, endNormalised);
    const auto startX = juce::jmap(orderedStart, 0.0f, 1.0f, plot.getX(), plot.getRight());
    const auto endX = juce::jmap(orderedEnd, 0.0f, 1.0f, plot.getX(), plot.getRight());
    auto selection = juce::Rectangle<float> { startX, plot.getY(), juce::jmax(1.0f, endX - startX), plot.getHeight() };

    g.setColour(regionColour.withAlpha(0.18f));
    g.fillRoundedRectangle(selection, 3.0f);
    g.setColour(regionColour.withAlpha(0.8f));
    g.drawRoundedRectangle(selection, 3.0f, 1.2f);

    for (auto sliceIndex = 1; sliceIndex < 8; ++sliceIndex)
    {
        const auto sliceX = juce::jmap(static_cast<float>(sliceIndex), 0.0f, 8.0f, plot.getX(), plot.getRight());
        const auto isInsideSelection = sliceX >= selection.getX() && sliceX <= selection.getRight();
        g.setColour((isInsideSelection ? regionColour : juce::Colour(0xff39474d)).withAlpha(isInsideSelection ? 0.7f : 0.5f));
        g.drawVerticalLine(static_cast<int>(std::round(sliceX)), plot.getY(), plot.getBottom());
    }

    auto drawHandle = [&g, &plot, regionColour] (float x, const juce::String& text)
    {
        auto handle = juce::Rectangle<float> { x - 4.0f, plot.getY(), 8.0f, plot.getHeight() };
        g.setColour(juce::Colour(0xff0d1113));
        g.fillRoundedRectangle(handle, 3.0f);
        g.setColour(regionColour);
        g.drawRoundedRectangle(handle, 3.0f, 1.4f);
        g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
        g.drawFittedText(text, handle.withHeight(12.0f).toNearestInt(), juce::Justification::centred, 1);
    };

    drawHandle(startX, "S");
    drawHandle(endX, "E");

    const auto seconds = overview.sourceSampleRate > 0.0
        ? static_cast<double>(overview.totalSamples) / overview.sourceSampleRate
        : 0.0;
    const auto selectedPercent = juce::roundToInt((orderedEnd - orderedStart) * 100.0f);
    g.setFont(juce::FontOptions(9.5f));
    g.setColour(juce::Colour(0xffa8b6b8));
    g.drawFittedText(juce::String(seconds, 1) + "s  " + juce::String(selectedPercent) + "%",
                     getLocalBounds().reduced(8, 5).removeFromTop(14),
                     juce::Justification::centredRight,
                     1);
}

void SampleWaveformDisplay::mouseDown(const juce::MouseEvent& event)
{
    if (! overview.isValid())
        return;

    const auto normalised = positionToNormalised(event.position.x);
    const auto startDistance = std::abs(normalised - startNormalised);
    const auto endDistance = std::abs(normalised - endNormalised);
    dragAnchor = normalised;

    if (startDistance < 0.025f || endDistance < 0.025f)
        dragMode = startDistance <= endDistance ? DragMode::start : DragMode::end;
    else
        dragMode = DragMode::range;

    if (dragMode == DragMode::range)
    {
        const auto quickWindowStart = juce::jlimit(0.0f, 1.0f, normalised < 0.98f ? normalised : normalised - 0.02f);
        applyRange(quickWindowStart, quickWindowStart + 0.02f);
        dragAnchor = quickWindowStart;
    }
}

void SampleWaveformDisplay::mouseDrag(const juce::MouseEvent& event)
{
    if (! overview.isValid() || dragMode == DragMode::none)
        return;

    const auto normalised = positionToNormalised(event.position.x);

    switch (dragMode)
    {
        case DragMode::start: applyRange(normalised, endNormalised); break;
        case DragMode::end: applyRange(startNormalised, normalised); break;
        case DragMode::range: applyRange(dragAnchor, normalised); break;
        case DragMode::none: break;
    }
}

void SampleWaveformDisplay::mouseUp(const juce::MouseEvent&)
{
    dragMode = DragMode::none;
}

juce::Rectangle<float> SampleWaveformDisplay::plotBounds() const
{
    return getLocalBounds().toFloat().reduced(10.0f, 17.0f).withTrimmedTop(3.0f);
}

float SampleWaveformDisplay::positionToNormalised(float xPosition) const
{
    const auto plot = plotBounds();
    return juce::jlimit(0.0f, 1.0f, (xPosition - plot.getX()) / juce::jmax(1.0f, plot.getWidth()));
}

void SampleWaveformDisplay::applyRange(float start, float end)
{
    auto orderedStart = juce::jlimit(0.0f, 1.0f, juce::jmin(start, end));
    auto orderedEnd = juce::jlimit(0.0f, 1.0f, juce::jmax(start, end));

    if (orderedEnd - orderedStart < 0.002f)
    {
        if (orderedEnd >= 1.0f)
            orderedStart = juce::jlimit(0.0f, 1.0f, orderedEnd - 0.002f);

        orderedEnd = juce::jlimit(0.0f, 1.0f, orderedStart + 0.002f);
    }

    setRange(orderedStart, orderedEnd);

    if (onRangeChange)
        onRangeChange(startNormalised, endNormalised);
}
}
