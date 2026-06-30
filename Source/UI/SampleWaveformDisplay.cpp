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

void SampleWaveformDisplay::setSliceMarkers(const std::array<SliceMarker, 8>& newMarkers)
{
    auto markersChanged = false;

    for (size_t index = 0; index < sliceMarkers.size(); ++index)
    {
        const auto& current = sliceMarkers[index];
        const auto& next = newMarkers[index];
        if (std::abs(current.start - next.start) >= 0.001f
            || std::abs(current.end - next.end) >= 0.001f
            || std::abs(current.pitch - next.pitch) >= 0.01f
            || std::abs(current.gain - next.gain) >= 0.01f
            || std::abs(current.pan - next.pan) >= 0.01f
            || std::abs(current.probability - next.probability) >= 0.01f
            || std::abs(current.nudgePercent - next.nudgePercent) >= 0.05f
            || std::abs(current.fade - next.fade) >= 0.01f
            || current.repeats != next.repeats
            || current.custom != next.custom
            || current.selected != next.selected
            || current.reverse != next.reverse
            || current.stutter != next.stutter
            || current.choke != next.choke)
        {
            markersChanged = true;
            break;
        }
    }

    if (! markersChanged)
        return;

    sliceMarkers = newMarkers;
    repaint();
}

void SampleWaveformDisplay::setModulationState(float startAmount,
                                               float mixAmount,
                                               float pitchAmount,
                                               float rampAmount,
                                               float stutterAmount,
                                               int routeCount,
                                               juce::String sourceSummary)
{
    startAmount = juce::jlimit(-1.0f, 1.0f, startAmount);
    mixAmount = juce::jlimit(-1.0f, 1.0f, mixAmount);
    pitchAmount = juce::jlimit(-1.0f, 1.0f, pitchAmount);
    rampAmount = juce::jlimit(-1.0f, 1.0f, rampAmount);
    stutterAmount = juce::jlimit(-1.0f, 1.0f, stutterAmount);
    routeCount = juce::jmax(0, routeCount);
    sourceSummary = sourceSummary.trim();

    if (std::abs(startModAmount - startAmount) < 0.001f
        && std::abs(mixModAmount - mixAmount) < 0.001f
        && std::abs(pitchModAmount - pitchAmount) < 0.001f
        && std::abs(rampModAmount - rampAmount) < 0.001f
        && std::abs(stutterModAmount - stutterAmount) < 0.001f
        && modRouteCount == routeCount
        && modSourceSummary == sourceSummary)
    {
        return;
    }

    startModAmount = startAmount;
    mixModAmount = mixAmount;
    pitchModAmount = pitchAmount;
    rampModAmount = rampAmount;
    stutterModAmount = stutterAmount;
    modRouteCount = routeCount;
    modSourceSummary = std::move(sourceSummary);
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
    auto tooltip = overview.fileName + " | " + juce::String(seconds, 1) + "s | " + juce::String(selectedPercent) + "% selected";
    if (modRouteCount > 0)
        tooltip += "\nModulated: Start " + modulationText(startModAmount)
            + " Mix " + modulationText(mixModAmount)
            + " Pitch " + modulationText(pitchModAmount)
            + " Ramp " + modulationText(rampModAmount)
            + " Stutter " + modulationText(stutterModAmount)
            + (modSourceSummary.isNotEmpty() ? " | " + modSourceSummary : juce::String {});

    return tooltip;
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

    const auto sliceLane = sliceLaneBounds();
    g.setColour(juce::Colour(0xff0d1113).withAlpha(0.78f));
    g.fillRoundedRectangle(sliceLane, 4.0f);

    for (size_t index = 0; index < sliceMarkers.size(); ++index)
    {
        const auto& marker = sliceMarkers[index];
        const auto markerStart = juce::jlimit(0.0f, 1.0f, juce::jmin(marker.start, marker.end));
        const auto markerEnd = juce::jlimit(0.0f, 1.0f, juce::jmax(marker.start, marker.end));
        const auto markerStartX = juce::jmap(markerStart, 0.0f, 1.0f, plot.getX(), plot.getRight());
        const auto markerEndX = juce::jmap(markerEnd, 0.0f, 1.0f, plot.getX(), plot.getRight());
        auto markerBounds = juce::Rectangle<float> { markerStartX,
                                                     sliceLane.getY() + 2.0f,
                                                     juce::jmax(5.0f, markerEndX - markerStartX),
                                                     sliceLane.getHeight() - 4.0f };
        const auto markerColour = marker.selected ? regionColour : (marker.custom ? accent : juce::Colour(0xff39474d));
        g.setColour(markerColour.withAlpha(marker.selected ? 0.34f : marker.custom ? 0.2f : 0.12f));
        g.fillRoundedRectangle(markerBounds, 3.0f);
        g.setColour(markerColour.withAlpha(marker.selected ? 0.95f : 0.58f));
        g.drawRoundedRectangle(markerBounds, 3.0f, marker.selected ? 1.4f : 0.9f);

        auto labelBounds = markerBounds.reduced(2.0f, 0.0f).toNearestInt();
        const auto pitchText = std::abs(marker.pitch) >= 0.05f
            ? ((marker.pitch >= 0.0f ? "+" : "") + juce::String(juce::roundToInt(marker.pitch)))
            : juce::String("0");
        auto flagText = juce::String(static_cast<int>(index + 1)) + (marker.custom ? "*" : "");
        if (marker.reverse)
            flagText += " R";
        if (marker.stutter)
            flagText += " x" + juce::String(juce::jlimit(1, 8, marker.repeats));
        if (marker.choke)
            flagText += " C";
        if (marker.probability < 0.995f)
            flagText += " " + juce::String(juce::roundToInt(marker.probability * 100.0f)) + "%";
        if (std::abs(marker.pan) > 0.05f)
            flagText += marker.pan < 0.0f ? " L" : " R";
        if (std::abs(marker.nudgePercent) > 0.1f)
            flagText += marker.nudgePercent > 0.0f ? " >" : " <";
        if (marker.fade > 0.12f)
            flagText += marker.fade > 0.55f ? " F+" : " F";
        flagText += " " + pitchText;

        g.setFont(juce::FontOptions(markerBounds.getWidth() > 54.0f ? 8.0f : 7.0f, marker.selected ? juce::Font::bold : juce::Font::plain));
        g.setColour(marker.selected ? juce::Colour(0xffffffff) : juce::Colour(0xffb8c6c9));
        g.drawFittedText(flagText, labelBounds, juce::Justification::centred, 1, 0.55f);
    }

    if (std::abs(startModAmount) > 0.01f)
    {
        const auto modLow = juce::jlimit(0.0f, 1.0f, orderedStart + (juce::jmin(0.0f, startModAmount) * 0.36f));
        const auto modHigh = juce::jlimit(0.0f, 1.0f, orderedStart + (juce::jmax(0.0f, startModAmount) * 0.36f));
        const auto modLowX = juce::jmap(modLow, 0.0f, 1.0f, plot.getX(), plot.getRight());
        const auto modHighX = juce::jmap(modHigh, 0.0f, 1.0f, plot.getX(), plot.getRight());
        const auto rangeX = juce::jmin(modLowX, modHighX);
        const auto rangeWidth = juce::jmax(2.0f, std::abs(modHighX - modLowX));
        auto modRange = juce::Rectangle<float>(rangeX, plot.getY() + 2.0f, rangeWidth, plot.getHeight() - 4.0f);

        g.setColour(juce::Colour(0xff7bb7ff).withAlpha(0.14f));
        g.fillRoundedRectangle(modRange, 3.0f);
        g.setColour(juce::Colour(0xff7bb7ff).withAlpha(0.68f));
        g.drawVerticalLine(static_cast<int>(std::round(modLowX)), plot.getY(), plot.getBottom());
        g.drawVerticalLine(static_cast<int>(std::round(modHighX)), plot.getY(), plot.getBottom());
    }

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

    if (modRouteCount > 0)
    {
        auto modBadge = getLocalBounds().reduced(8, 4).removeFromTop(14).removeFromLeft(142);
        g.setColour(juce::Colour(0xff11191d).withAlpha(0.92f));
        g.fillRoundedRectangle(modBadge.toFloat(), 3.0f);
        g.setColour(juce::Colour(0xff7bb7ff).withAlpha(0.72f));
        g.drawRoundedRectangle(modBadge.toFloat(), 3.0f, 1.0f);
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.setColour(juce::Colour(0xffd7e3e6));
        g.drawFittedText("CHOP MOD " + modulationText(startModAmount)
                             + (modRouteCount > 1 ? " " + juce::String(modRouteCount) + "R" : ""),
                         modBadge.reduced(4, 0),
                         juce::Justification::centred,
                         1,
                         0.72f);

        auto detail = getLocalBounds().reduced(10, 4).removeFromBottom(11);
        const auto sourceText = modSourceSummary.isNotEmpty() ? modSourceSummary : "Routes";
        const auto detailText = "START " + modulationText(startModAmount)
            + "  MIX " + modulationText(mixModAmount)
            + "  PITCH " + modulationText(pitchModAmount)
            + "  RAMP " + modulationText(rampModAmount)
            + "  STUT " + modulationText(stutterModAmount)
            + "  " + sourceText;
        g.setFont(juce::FontOptions(8.0f));
        g.setColour(juce::Colour(0xff8a989e));
        g.drawFittedText(detailText, detail, juce::Justification::centredLeft, 1, 0.72f);
    }
}

void SampleWaveformDisplay::mouseDown(const juce::MouseEvent& event)
{
    if (! overview.isValid())
        return;

    const auto normalised = positionToNormalised(event.position.x);
    const auto startDistance = std::abs(normalised - startNormalised);
    const auto endDistance = std::abs(normalised - endNormalised);
    if (sliceLaneBounds().contains(event.position) && (onSliceSelected || onSliceBoundaryChange))
    {
        const auto boundaryIndex = sliceBoundaryIndexAtPosition(event.position.x);
        if (boundaryIndex >= 0 && onSliceBoundaryChange)
        {
            dragMode = DragMode::sliceBoundary;
            activeSliceBoundaryIndex = boundaryIndex;
            applySliceBoundary(normalised);
            return;
        }

        const auto sliceIndex = sliceIndexAtPosition(event.position.x);
        if (sliceIndex >= 0 && startDistance >= 0.025f && endDistance >= 0.025f)
        {
            dragMode = DragMode::none;
            onSliceSelected(static_cast<size_t>(sliceIndex));
            return;
        }
    }

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
        case DragMode::sliceBoundary: applySliceBoundary(normalised); break;
        case DragMode::none: break;
    }
}

void SampleWaveformDisplay::mouseUp(const juce::MouseEvent&)
{
    dragMode = DragMode::none;
    activeSliceBoundaryIndex = -1;
}

juce::Rectangle<float> SampleWaveformDisplay::plotBounds() const
{
    return getLocalBounds().toFloat().reduced(10.0f, 17.0f).withTrimmedTop(3.0f);
}

juce::Rectangle<float> SampleWaveformDisplay::sliceLaneBounds() const
{
    const auto plot = plotBounds();
    const auto laneHeight = juce::jlimit(16.0f, 30.0f, plot.getHeight() * 0.24f);
    return plot.withY(plot.getBottom() - laneHeight).withHeight(laneHeight);
}

float SampleWaveformDisplay::positionToNormalised(float xPosition) const
{
    const auto plot = plotBounds();
    return juce::jlimit(0.0f, 1.0f, (xPosition - plot.getX()) / juce::jmax(1.0f, plot.getWidth()));
}

int SampleWaveformDisplay::sliceIndexAtPosition(float xPosition) const
{
    const auto normalised = positionToNormalised(xPosition);
    auto bestIndex = -1;
    auto bestDistance = 2.0f;

    for (size_t index = 0; index < sliceMarkers.size(); ++index)
    {
        const auto& marker = sliceMarkers[index];
        const auto start = juce::jlimit(0.0f, 1.0f, juce::jmin(marker.start, marker.end));
        const auto end = juce::jlimit(0.0f, 1.0f, juce::jmax(marker.start, marker.end));
        if (normalised >= start && normalised <= end)
            return static_cast<int>(index);

        const auto centre = (start + end) * 0.5f;
        const auto distance = std::abs(normalised - centre);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            bestIndex = static_cast<int>(index);
        }
    }

    return bestIndex;
}

int SampleWaveformDisplay::sliceBoundaryIndexAtPosition(float xPosition) const
{
    const auto plot = plotBounds();
    const auto handleThreshold = juce::jmax(7.0f, plot.getWidth() * 0.012f);
    auto bestIndex = -1;
    auto bestDistance = handleThreshold;

    for (auto boundaryIndex = 0; boundaryIndex <= static_cast<int>(sliceMarkers.size()); ++boundaryIndex)
    {
        const auto boundaryX = juce::jmap(sliceBoundaryPosition(boundaryIndex), 0.0f, 1.0f, plot.getX(), plot.getRight());
        const auto distance = std::abs(xPosition - boundaryX);
        if (distance <= bestDistance)
        {
            bestDistance = distance;
            bestIndex = boundaryIndex;
        }
    }

    return bestIndex;
}

float SampleWaveformDisplay::sliceBoundaryPosition(int boundaryIndex) const
{
    boundaryIndex = juce::jlimit(0, static_cast<int>(sliceMarkers.size()), boundaryIndex);
    if (boundaryIndex == 0)
    {
        const auto& first = sliceMarkers.front();
        return juce::jlimit(0.0f, 1.0f, juce::jmin(first.start, first.end));
    }

    if (boundaryIndex == static_cast<int>(sliceMarkers.size()))
    {
        const auto& last = sliceMarkers.back();
        return juce::jlimit(0.0f, 1.0f, juce::jmax(last.start, last.end));
    }

    const auto& previous = sliceMarkers[static_cast<size_t>(boundaryIndex - 1)];
    const auto& current = sliceMarkers[static_cast<size_t>(boundaryIndex)];
    const auto previousEnd = juce::jlimit(0.0f, 1.0f, juce::jmax(previous.start, previous.end));
    const auto currentStart = juce::jlimit(0.0f, 1.0f, juce::jmin(current.start, current.end));
    return juce::jlimit(0.0f, 1.0f, (previousEnd + currentStart) * 0.5f);
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

void SampleWaveformDisplay::applySliceBoundary(float position)
{
    if (activeSliceBoundaryIndex < 0 || ! onSliceBoundaryChange)
        return;

    const auto minimumWidth = 0.004f;
    const auto boundaryCount = static_cast<int>(sliceMarkers.size());
    const auto lowerLimit = activeSliceBoundaryIndex <= 0
        ? 0.0f
        : sliceBoundaryPosition(activeSliceBoundaryIndex - 1) + minimumWidth;
    const auto upperLimit = activeSliceBoundaryIndex >= boundaryCount
        ? 1.0f
        : sliceBoundaryPosition(activeSliceBoundaryIndex + 1) - minimumWidth;
    const auto safeLower = juce::jlimit(0.0f, 1.0f, juce::jmin(lowerLimit, upperLimit));
    const auto safeUpper = juce::jlimit(safeLower, 1.0f, juce::jmax(lowerLimit, upperLimit));
    const auto normalised = juce::jlimit(safeLower, safeUpper, juce::jlimit(0.0f, 1.0f, position));

    onSliceBoundaryChange(static_cast<size_t>(activeSliceBoundaryIndex), normalised);
}

juce::String SampleWaveformDisplay::modulationText(float amount)
{
    amount = juce::jlimit(-1.0f, 1.0f, amount);
    return (amount >= 0.0f ? "+" : "") + juce::String(juce::roundToInt(amount * 100.0f)) + "%";
}
}
