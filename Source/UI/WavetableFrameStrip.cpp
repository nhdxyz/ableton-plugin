#include "WavetableFrameStrip.h"

#include <algorithm>
#include <cmath>

namespace UI
{
namespace
{
constexpr auto inactiveAlpha = 0.34f;

float clamp01(float value)
{
    return juce::jlimit(0.0f, 1.0f, std::isfinite(value) ? value : 0.0f);
}

juce::String positionText(float position)
{
    return juce::String(juce::roundToInt(clamp01(position) * 100.0f)) + "%";
}
}

WavetableFrameStrip::WavetableFrameStrip()
{
    setComponentID("WavetableFrameStrip");
    tooltipText = "Wavetable frame stacks: click or drag a lane to scan Osc 1 or Osc 2 custom frames";
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void WavetableFrameStrip::setTheme(const Theme& newTheme)
{
    if (theme.id == newTheme.id)
        return;

    theme = newTheme;
    repaint();
}

void WavetableFrameStrip::setState(State newState)
{
    const auto changed = state.summary != newState.summary
        || ! laneEqual(state.osc1, newState.osc1)
        || ! laneEqual(state.osc2, newState.osc2);

    if (! changed)
        return;

    state = std::move(newState);

    juce::StringArray lines;
    lines.add(state.summary.isNotEmpty() ? state.summary : juce::String("Wavetable frame stacks"));
    lines.add(state.osc1.label + ": " + state.osc1.detail + " at " + positionText(state.osc1.position));
    lines.add(state.osc2.label + ": " + state.osc2.detail + " at " + positionText(state.osc2.position));
    tooltipText = lines.joinIntoString("\n");

    repaint();
}

WavetableFrameStrip::LayoutMetrics WavetableFrameStrip::getLayoutMetricsForAudit() const
{
    LayoutMetrics metrics;
    const auto lanes = laneBoundsForArea(getLocalBounds().toFloat().reduced(1.0f));
    metrics.minLaneHeight = lanes.front().getHeight();

    for (size_t laneIndex = 0; laneIndex < lanes.size(); ++laneIndex)
    {
        const auto& lane = lanes[laneIndex];
        const auto& laneState = laneIndex == 0 ? state.osc1 : state.osc2;
        const auto selectedFrame = juce::jlimit<int>(
            0,
            static_cast<int>(frameCount - 1),
            juce::roundToInt(clamp01(laneState.position) * static_cast<float>(frameCount - 1)));

        metrics.minLaneHeight = std::min(metrics.minLaneHeight, lane.getHeight());
        const auto frames = frameBoundsForLane(lane);
        for (size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex)
        {
            const auto& frame = frames[frameIndex];
            if (frame.getWidth() <= 0.0f || frame.getHeight() <= 0.0f)
                continue;

            ++metrics.visibleFrameCards;
            if (laneState.active && static_cast<int>(frameIndex) == selectedFrame)
                ++metrics.selectedFrameCards;

            metrics.minFrameWidth = metrics.minFrameWidth <= 0.0f
                ? frame.getWidth()
                : std::min(metrics.minFrameWidth, frame.getWidth());
        }
    }

    metrics.readable = metrics.visibleFrameCards == 16
        && metrics.minFrameWidth >= 30.0f
        && metrics.minLaneHeight >= 42.0f;
    return metrics;
}

juce::String WavetableFrameStrip::getTooltip()
{
    return tooltipText;
}

void WavetableFrameStrip::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto lanes = laneBoundsForArea(bounds);

    g.setColour(theme.panelRaised);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(theme.outline);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    auto header = bounds.reduced(8.0f, 6.0f).removeFromTop(16.0f);
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.setColour(theme.accent);
    g.drawFittedText("WT FRAME STACKS", header.toNearestInt(), juce::Justification::centredLeft, 1, 0.72f);
    g.setColour(theme.textDim);
    g.drawFittedText(state.summary, header.toNearestInt(), juce::Justification::centredRight, 1, 0.55f);

    auto paintLane = [&] (const Lane& lane, size_t laneIndex, juce::Colour accent)
    {
        const auto laneBounds = lanes[laneIndex];
        const auto frameBounds = frameBoundsForLane(laneBounds);
        const auto laneActiveAlpha = lane.active ? 1.0f : inactiveAlpha;
        const auto isHoveredLane = hoveredLane == static_cast<int>(laneIndex);
        const auto selectedFrame = juce::jlimit<int>(
            0,
            static_cast<int>(frameCount - 1),
            juce::roundToInt(clamp01(lane.position) * static_cast<float>(frameCount - 1)));

        g.setColour((lane.active ? accent.withAlpha(0.12f) : theme.panelAlt).withMultipliedAlpha(laneActiveAlpha));
        g.fillRoundedRectangle(laneBounds, 5.0f);
        g.setColour(isHoveredLane ? accent : theme.outline);
        g.drawRoundedRectangle(laneBounds, 5.0f, isHoveredLane ? 1.4f : 1.0f);

        auto labelArea = laneBounds.reduced(6.0f, 5.0f).removeFromLeft(58.0f);
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.setColour((lane.active ? theme.text : theme.textDim).withMultipliedAlpha(laneActiveAlpha));
        g.drawFittedText(lane.label, labelArea.removeFromTop(13.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.65f);
        g.setFont(juce::FontOptions(7.6f));
        g.setColour(theme.textMuted.withMultipliedAlpha(laneActiveAlpha));
        g.drawFittedText(lane.detail, labelArea.toNearestInt(), juce::Justification::centredLeft, 2, 0.48f);

        for (size_t frameIndex = 0; frameIndex < frameBounds.size(); ++frameIndex)
        {
            const auto frame = frameBounds[frameIndex];
            const auto hoveredFrameCard = static_cast<int>(frameIndex) == hoveredFrame
                && hoveredLane == static_cast<int>(laneIndex);
            const auto selectedFrameCard = lane.active && static_cast<int>(frameIndex) == selectedFrame;
            const auto activeFrame = hoveredFrameCard || selectedFrameCard;
            const auto range = frameRange(lane.frames[frameIndex]);
            const auto cardAlpha = lane.active ? 1.0f : inactiveAlpha;

            g.setColour((selectedFrameCard ? accent.withAlpha(0.26f)
                                           : hoveredFrameCard ? accent.withAlpha(0.22f)
                                                              : theme.panelAlt)
                            .withMultipliedAlpha(cardAlpha));
            g.fillRoundedRectangle(frame, 4.0f);
            g.setColour((activeFrame ? accent : theme.outline.withAlpha(0.72f)).withMultipliedAlpha(cardAlpha));
            g.drawRoundedRectangle(frame, 4.0f, selectedFrameCard ? 1.7f : (hoveredFrameCard ? 1.3f : 1.0f));

            auto waveArea = frame.reduced(4.0f, 5.0f);
            waveArea.removeFromBottom(8.0f);
            const auto path = pathForFrame(lane.frames[frameIndex], waveArea);
            g.setColour(accent.withAlpha(0.24f * cardAlpha));
            g.fillRoundedRectangle(waveArea, 2.5f);
            g.setColour(accent.withAlpha((range > 0.08f ? 0.88f : 0.38f) * cardAlpha));
            g.strokePath(path, juce::PathStrokeType(1.25f));

            g.setFont(juce::FontOptions(7.2f, juce::Font::bold));
            g.setColour(theme.textDim.withMultipliedAlpha(cardAlpha));
            g.drawFittedText(juce::String(static_cast<int>(frameIndex + 1)),
                             frame.reduced(3.0f, 2.0f).removeFromBottom(8.0f).toNearestInt(),
                             juce::Justification::centred,
                             1,
                             0.58f);

            if (selectedFrameCard)
            {
                const auto marker = frame.reduced(5.0f, 3.0f).removeFromTop(2.0f);
                g.setColour(accent.withAlpha(0.88f));
                g.fillRoundedRectangle(marker, 1.0f);
            }
        }

        if (! frameBounds.empty())
        {
            const auto railStart = frameBounds.front().getX();
            const auto railEnd = frameBounds.back().getRight();
            const auto markerX = juce::jmap(clamp01(lane.position), 0.0f, 1.0f, railStart, railEnd);
            g.setColour(accent.withAlpha(lane.active ? 0.92f : 0.38f));
            g.drawLine(markerX, laneBounds.getY() + 3.0f, markerX, laneBounds.getBottom() - 3.0f, 1.4f);
            g.fillEllipse(markerX - 3.5f, laneBounds.getY() + 4.0f, 7.0f, 7.0f);
        }
    };

    paintLane(state.osc1, 0, theme.accent);
    paintLane(state.osc2, 1, theme.accentSecondary);
}

void WavetableFrameStrip::mouseMove(const juce::MouseEvent& event)
{
    const auto hit = hitTargetAt(event.position);
    const auto newLane = hit.valid ? (hit.osc2 ? 1 : 0) : -1;
    const auto newFrame = hit.valid ? hit.frameIndex : -1;
    if (newLane == hoveredLane && newFrame == hoveredFrame)
        return;

    hoveredLane = newLane;
    hoveredFrame = newFrame;
    repaint();
}

void WavetableFrameStrip::mouseExit(const juce::MouseEvent&)
{
    if (hoveredLane < 0 && hoveredFrame < 0)
        return;

    hoveredLane = -1;
    hoveredFrame = -1;
    repaint();
}

void WavetableFrameStrip::mouseDown(const juce::MouseEvent& event)
{
    const auto hit = hitTargetAt(event.position);
    if (! hit.valid)
        return;

    editingLane = hit.osc2 ? 1 : 0;
    hoveredLane = editingLane;
    hoveredFrame = hit.frameIndex;

    if (onPositionEditStart)
        onPositionEditStart(hit.osc2);

    if (hit.frameIndex >= 0)
        selectFrame(hit);
    else
        updatePositionAt(event.position);
}

void WavetableFrameStrip::mouseDrag(const juce::MouseEvent& event)
{
    updatePositionAt(event.position);
}

void WavetableFrameStrip::mouseUp(const juce::MouseEvent&)
{
    editingLane = -1;
}

std::array<juce::Rectangle<float>, 2> WavetableFrameStrip::laneBoundsForArea(juce::Rectangle<float> bounds) const
{
    auto inner = bounds.reduced(8.0f, 6.0f);
    inner.removeFromTop(18.0f);

    const auto gap = 7.0f;
    const auto laneHeight = (inner.getHeight() - gap) * 0.5f;
    std::array<juce::Rectangle<float>, 2> lanes {};
    lanes[0] = inner.removeFromTop(laneHeight);
    inner.removeFromTop(gap);
    lanes[1] = inner;
    return lanes;
}

std::array<juce::Rectangle<float>, WavetableFrameStrip::frameCount> WavetableFrameStrip::frameBoundsForLane(juce::Rectangle<float> laneBounds) const
{
    std::array<juce::Rectangle<float>, frameCount> frames {};
    auto frameArea = laneBounds.reduced(6.0f, 5.0f);
    frameArea.removeFromLeft(62.0f);

    const auto gap = 4.0f;
    const auto frameWidth = juce::jmax(22.0f, (frameArea.getWidth() - (gap * static_cast<float>(frameCount - 1))) / static_cast<float>(frameCount));

    for (size_t frameIndex = 0; frameIndex < frameCount; ++frameIndex)
    {
        frames[frameIndex] = frameArea.removeFromLeft(frameWidth);
        if (frameIndex + 1 < frameCount)
            frameArea.removeFromLeft(gap);
    }

    return frames;
}

WavetableFrameStrip::HitTarget WavetableFrameStrip::hitTargetAt(juce::Point<float> position) const
{
    HitTarget hit;
    const auto lanes = laneBoundsForArea(getLocalBounds().toFloat().reduced(1.0f));

    for (size_t laneIndex = 0; laneIndex < lanes.size(); ++laneIndex)
    {
        if (! lanes[laneIndex].contains(position))
            continue;

        const auto frames = frameBoundsForLane(lanes[laneIndex]);
        const auto railStart = frames.front().getX();
        const auto railEnd = frames.back().getRight();
        hit.valid = position.x >= railStart && position.x <= railEnd;
        hit.osc2 = laneIndex == 1;
        hit.position = juce::jlimit(0.0f, 1.0f, (position.x - railStart) / juce::jmax(1.0f, railEnd - railStart));

        for (size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex)
        {
            if (frames[frameIndex].contains(position))
            {
                hit.frameIndex = static_cast<int>(frameIndex);
                break;
            }
        }

        return hit;
    }

    return hit;
}

void WavetableFrameStrip::selectFrame(const HitTarget& hit)
{
    if (! hit.valid || hit.frameIndex < 0)
        return;

    const auto safeFrame = juce::jlimit(0, static_cast<int>(frameCount - 1), hit.frameIndex);
    const auto position = static_cast<float>(safeFrame) / static_cast<float>(frameCount - 1);
    auto& lane = hit.osc2 ? state.osc2 : state.osc1;
    if (std::abs(lane.position - position) < 0.001f)
        return;

    lane.position = position;

    if (onPositionChange)
        onPositionChange(hit.osc2, position);

    repaint();
}

void WavetableFrameStrip::updatePositionAt(juce::Point<float> position)
{
    if (editingLane < 0)
        return;

    const auto lanes = laneBoundsForArea(getLocalBounds().toFloat().reduced(1.0f));
    const auto frames = frameBoundsForLane(lanes[static_cast<size_t> (editingLane)]);
    const auto railStart = frames.front().getX();
    const auto railEnd = frames.back().getRight();
    const auto nextPosition = juce::jlimit(0.0f, 1.0f, (position.x - railStart) / juce::jmax(1.0f, railEnd - railStart));
    auto& lane = editingLane == 1 ? state.osc2 : state.osc1;
    if (std::abs(lane.position - nextPosition) < 0.001f)
        return;

    lane.position = nextPosition;

    if (onPositionChange)
        onPositionChange(editingLane == 1, nextPosition);

    repaint();
}

bool WavetableFrameStrip::laneEqual(const Lane& first, const Lane& second) noexcept
{
    if (first.label != second.label
        || first.detail != second.detail
        || std::abs(first.position - second.position) >= 0.001f
        || first.active != second.active)
    {
        return false;
    }

    for (size_t frameIndex = 0; frameIndex < frameCount; ++frameIndex)
        for (size_t pointIndex = 0; pointIndex < pointCount; ++pointIndex)
            if (std::abs(first.frames[frameIndex][pointIndex] - second.frames[frameIndex][pointIndex]) >= 0.001f)
                return false;

    return true;
}

float WavetableFrameStrip::frameRange(const CustomPointArray& frame) noexcept
{
    auto minValue = clamp01(frame.front());
    auto maxValue = minValue;

    for (const auto value : frame)
    {
        const auto clamped = clamp01(value);
        minValue = std::min(minValue, clamped);
        maxValue = std::max(maxValue, clamped);
    }

    return maxValue - minValue;
}

juce::Path WavetableFrameStrip::pathForFrame(const CustomPointArray& frame, juce::Rectangle<float> bounds)
{
    juce::Path path;
    const auto lastIndex = juce::jmax<size_t>(1, frame.size() - 1);

    for (size_t pointIndex = 0; pointIndex < frame.size(); ++pointIndex)
    {
        const auto phase = static_cast<float>(pointIndex) / static_cast<float>(lastIndex);
        const auto x = bounds.getX() + (phase * bounds.getWidth());
        const auto y = juce::jmap(clamp01(frame[pointIndex]), 0.0f, 1.0f, bounds.getBottom(), bounds.getY());

        if (pointIndex == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }

    return path;
}
}
