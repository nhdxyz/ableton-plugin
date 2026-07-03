#include "OscillatorLaneOverview.h"

#include <algorithm>
#include <cmath>

namespace UI
{
namespace
{
constexpr auto activeThreshold = 0.025f;

juce::Colour laneColour(const Theme& theme, size_t index, bool active)
{
    auto colour = index == 0 ? theme.accent : theme.accentSecondary;
    if (! active)
        colour = theme.textDim;

    return colour;
}

juce::Path makeMiniWavePath(juce::Rectangle<float> bounds, float morph, float warpA, float warpB, bool custom)
{
    juce::Path path;
    const auto samples = 48;
    const auto centreY = bounds.getCentreY();
    const auto amplitude = bounds.getHeight() * (custom ? 0.38f : 0.31f);
    const auto harmonic = 1.0f + (juce::jlimit(0.0f, 1.0f, morph) * 2.0f);
    const auto warpDrive = 1.0f + (juce::jlimit(0.0f, 1.0f, warpA + warpB) * 1.2f);

    for (auto index = 0; index < samples; ++index)
    {
        const auto xNorm = static_cast<float>(index) / static_cast<float>(samples - 1);
        const auto phase = xNorm * juce::MathConstants<float>::twoPi;
        auto value = std::sin(phase * harmonic);
        value = std::tanh(value * warpDrive);
        value += std::sin(phase * (harmonic + 1.0f)) * (custom ? 0.18f : 0.08f);
        value = juce::jlimit(-1.0f, 1.0f, value);

        const auto x = bounds.getX() + (bounds.getWidth() * xNorm);
        const auto y = centreY - (value * amplitude);
        if (index == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }

    return path;
}

void drawBar(juce::Graphics& g,
             juce::Rectangle<float> bounds,
             const Theme& theme,
             juce::Colour accent,
             const juce::String& label,
             float amount)
{
    amount = juce::jlimit(0.0f, 1.0f, amount);
    g.setColour(theme.panelAlt);
    g.fillRoundedRectangle(bounds, 2.5f);
    g.setColour(accent.withAlpha(0.55f));
    g.fillRoundedRectangle(bounds.withWidth(bounds.getWidth() * amount), 2.5f);
    g.setColour(theme.outline);
    g.drawRoundedRectangle(bounds, 2.5f, 1.0f);

    g.setFont(juce::FontOptions(7.4f, juce::Font::bold));
    g.setColour(theme.textMuted);
    g.drawFittedText(label, bounds.toNearestInt(), juce::Justification::centred, 1, 0.55f);
}
}

OscillatorLaneOverview::OscillatorLaneOverview()
{
    setComponentID("OscillatorLaneOverview");
    tooltipText = "Oscillator lanes: source level, wavetable position, and A/B warp stack overview";
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void OscillatorLaneOverview::setTheme(const Theme& newTheme)
{
    if (theme.id == newTheme.id)
        return;

    theme = newTheme;
    repaint();
}

void OscillatorLaneOverview::setState(State newState)
{
    auto changed = state.summary != newState.summary
        || state.osc2Selected != newState.osc2Selected;
    for (size_t index = 0; index < laneCount; ++index)
        changed = changed || ! lanesEqual(state.lanes[index], newState.lanes[index]);

    if (! changed)
        return;

    state = std::move(newState);

    juce::StringArray tooltipLines;
    tooltipLines.add(state.summary.isNotEmpty() ? state.summary : juce::String("Oscillator lanes"));
    for (const auto& lane : state.lanes)
    {
        tooltipLines.add(lane.label + ": " + lane.waveName
                         + " | level " + percentText(lane.level)
                         + " | WT " + percentText(lane.wavetablePosition)
                         + " | A " + lane.warpAMode + " " + percentText(lane.warpA)
                         + " | B " + lane.warpBMode + " " + percentText(lane.warpB));
    }

    tooltipText = tooltipLines.joinIntoString("\n");
    repaint();
}

OscillatorLaneOverview::LayoutMetrics OscillatorLaneOverview::getLayoutMetricsForAudit() const
{
    LayoutMetrics metrics;
    const auto lanes = laneBoundsForArea(getLocalBounds().toFloat().reduced(1.0f));
    metrics.visibleLanes = static_cast<int>(std::count_if(lanes.begin(),
                                                          lanes.end(),
                                                          [] (const auto& lane)
                                                          {
                                                              return lane.getWidth() > 0.0f && lane.getHeight() > 0.0f;
                                                          }));
    if (metrics.visibleLanes <= 0)
        return metrics;

    metrics.selectedLanes = 1;
    metrics.minLaneWidth = lanes.front().getWidth();
    metrics.minLaneHeight = lanes.front().getHeight();
    for (const auto& lane : lanes)
    {
        metrics.minLaneWidth = std::min(metrics.minLaneWidth, lane.getWidth());
        metrics.minLaneHeight = std::min(metrics.minLaneHeight, lane.getHeight());
    }

    metrics.readable = metrics.visibleLanes == static_cast<int>(laneCount)
        && metrics.minLaneWidth >= 112.0f
        && metrics.minLaneHeight >= 44.0f;
    return metrics;
}

juce::String OscillatorLaneOverview::getTooltip()
{
    return tooltipText;
}

void OscillatorLaneOverview::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    auto inner = bounds.reduced(7.0f, 6.0f);
    const auto compact = bounds.getHeight() < 84.0f;

    g.setColour(theme.panelRaised);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(theme.outline);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    if (! compact)
    {
        auto header = inner.removeFromTop(14.0f);
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.setColour(theme.accent);
        g.drawFittedText("OSC LANES", header.toNearestInt(), juce::Justification::centredLeft, 1, 0.72f);

        g.setColour(theme.textDim);
        g.drawFittedText(state.summary, header.toNearestInt(), juce::Justification::centredRight, 1, 0.60f);
    }

    const auto lanes = laneBoundsForArea(bounds);
    for (size_t index = 0; index < laneCount; ++index)
    {
        auto laneBounds = lanes[index];
        const auto& lane = state.lanes[index];
        const auto level = juce::jlimit(0.0f, 1.0f, lane.level);
        const auto active = lane.active && level > activeThreshold;
        const auto hovered = hoveredLane == static_cast<int>(index);
        const auto selected = state.osc2Selected == (index == 1);
        const auto accent = laneColour(theme, index, active);

        g.setColour(selected ? accent.withAlpha(active ? 0.24f : 0.14f)
                             : (active ? accent.withAlpha(0.16f) : theme.panelAlt));
        g.fillRoundedRectangle(laneBounds, 4.5f);
        g.setColour(selected || hovered ? accent : (active ? accent.withAlpha(0.72f) : theme.outline));
        g.drawRoundedRectangle(laneBounds, 4.5f, selected ? 1.8f : (hovered ? 1.4f : (active ? 1.3f : 1.0f)));

        auto textArea = laneBounds.reduced(5.0f, compact ? 3.0f : 4.0f);
        auto top = textArea.removeFromTop(compact ? 12.0f : 14.0f);
        g.setFont(juce::FontOptions(compact ? 8.0f : 8.8f, juce::Font::bold));
        g.setColour(active ? theme.text : theme.textDim);
        g.drawFittedText(lane.label, top.removeFromLeft(38.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.62f);

        g.setColour(active ? accent : theme.textDim);
        g.drawFittedText(lane.waveName, top.toNearestInt(), juce::Justification::centredRight, 1, 0.58f);

        auto waveArea = textArea.removeFromTop(compact ? 15.0f : 18.0f).reduced(0.0f, compact ? 1.0f : 0.0f);
        g.setColour(theme.outline.withAlpha(0.45f));
        g.drawHorizontalLine(juce::roundToInt(waveArea.getCentreY()), waveArea.getX(), waveArea.getRight());
        g.setColour(accent.withAlpha(active ? 0.85f : 0.35f));
        g.strokePath(makeMiniWavePath(waveArea, lane.wavetablePosition, lane.warpA, lane.warpB, lane.customActive),
                     juce::PathStrokeType(active ? 1.45f : 1.0f));

        auto meterArea = textArea.removeFromTop(compact ? 8.0f : 10.0f);
        const auto levelWidth = meterArea.getWidth() * level;
        g.setColour(theme.panelAlt);
        g.fillRoundedRectangle(meterArea, 2.5f);
        g.setColour(accent.withAlpha(active ? 0.62f : 0.28f));
        g.fillRoundedRectangle(meterArea.withWidth(levelWidth), 2.5f);
        g.setColour(theme.outline.withAlpha(0.75f));
        g.drawRoundedRectangle(meterArea, 2.5f, 1.0f);

        if (textArea.getHeight() >= 13.0f)
        {
            const auto bars = warpBarBoundsForLane(laneBounds);
            drawBar(g, bars[0], theme, accent, "A " + lane.warpAMode, lane.warpA);
            drawBar(g, bars[1], theme, accent, "B " + lane.warpBMode, lane.warpB);
            textArea.removeFromTop(10.0f);
        }

        if (! compact && textArea.getHeight() >= 9.0f)
        {
            g.setFont(juce::FontOptions(7.2f));
            g.setColour(lane.wavetableActive || lane.customActive ? theme.textMuted : theme.textDim);
            g.drawFittedText("WT " + percentText(lane.wavetablePosition),
                             textArea.toNearestInt(),
                             juce::Justification::centred,
                             1,
                             0.55f);
        }
    }
}

void OscillatorLaneOverview::mouseMove(const juce::MouseEvent& event)
{
    const auto hit = hitTargetAt(event.position);
    const auto nextHoveredLane = hit.valid ? (hit.osc2 ? 1 : 0) : -1;
    if (nextHoveredLane == hoveredLane)
        return;

    hoveredLane = nextHoveredLane;
    repaint();
}

void OscillatorLaneOverview::mouseExit(const juce::MouseEvent&)
{
    if (hoveredLane < 0)
        return;

    hoveredLane = -1;
    repaint();
}

void OscillatorLaneOverview::mouseDown(const juce::MouseEvent& event)
{
    const auto hit = hitTargetAt(event.position);
    if (! hit.valid)
        return;

    editingLane = hit.osc2 ? 1 : 0;
    editingInteraction = hit.interaction;
    editingWarpB = hit.warpB;
    hoveredLane = editingLane;

    if (onLaneSelected)
        onLaneSelected(hit.osc2);

    if (hit.interaction == Interaction::warp)
    {
        if (onWarpEditStart)
            onWarpEditStart(hit.osc2, hit.warpB);
        updateWarpAt(event.position);
    }
    else
    {
        if (onPositionEditStart)
            onPositionEditStart(hit.osc2);
        updatePositionAt(event.position);
    }
}

void OscillatorLaneOverview::mouseDrag(const juce::MouseEvent& event)
{
    if (editingInteraction == Interaction::warp)
        updateWarpAt(event.position);
    else
        updatePositionAt(event.position);
}

void OscillatorLaneOverview::mouseUp(const juce::MouseEvent&)
{
    editingLane = -1;
    editingInteraction = Interaction::position;
    editingWarpB = false;
}

void OscillatorLaneOverview::mouseDoubleClick(const juce::MouseEvent& event)
{
    const auto hit = hitTargetAt(event.position);
    if (! hit.valid)
        return;

    if (onLaneSelected)
        onLaneSelected(hit.osc2);
    if (onOpenLaneEditor)
        onOpenLaneEditor(hit.osc2);
}

std::array<juce::Rectangle<float>, OscillatorLaneOverview::laneCount>
OscillatorLaneOverview::laneBoundsForArea(juce::Rectangle<float> bounds) const
{
    std::array<juce::Rectangle<float>, laneCount> lanes {};
    auto laneArea = bounds.reduced(6.0f, bounds.getHeight() < 84.0f ? 5.0f : 22.0f);
    const auto gap = 6.0f;
    const auto laneWidth = (laneArea.getWidth() - gap) * 0.5f;
    lanes[0] = laneArea.removeFromLeft(laneWidth);
    laneArea.removeFromLeft(gap);
    lanes[1] = laneArea;
    return lanes;
}

std::array<juce::Rectangle<float>, 2>
OscillatorLaneOverview::warpBarBoundsForLane(juce::Rectangle<float> laneBounds) const
{
    const auto compact = getLocalBounds().getHeight() < 84;
    auto textArea = laneBounds.reduced(5.0f, compact ? 3.0f : 4.0f);
    textArea.removeFromTop(compact ? 12.0f : 14.0f);
    textArea.removeFromTop(compact ? 15.0f : 18.0f);
    textArea.removeFromTop(compact ? 8.0f : 10.0f);

    auto barRow = textArea.removeFromTop(10.0f);
    const auto gap = 4.0f;
    std::array<juce::Rectangle<float>, 2> bars {};
    bars[0] = barRow.removeFromLeft((barRow.getWidth() - gap) * 0.5f);
    barRow.removeFromLeft(gap);
    bars[1] = barRow;
    return bars;
}

OscillatorLaneOverview::HitTarget OscillatorLaneOverview::hitTargetAt(juce::Point<float> position) const
{
    HitTarget hit;
    const auto lanes = laneBoundsForArea(getLocalBounds().toFloat().reduced(1.0f));

    for (size_t index = 0; index < lanes.size(); ++index)
    {
        if (! lanes[index].contains(position))
            continue;

        hit.valid = true;
        hit.osc2 = index == 1;
        const auto bars = warpBarBoundsForLane(lanes[index]);
        for (size_t barIndex = 0; barIndex < bars.size(); ++barIndex)
        {
            if (! bars[barIndex].contains(position))
                continue;

            hit.interaction = Interaction::warp;
            hit.warpB = barIndex == 1;
            hit.position = juce::jlimit(0.0f,
                                        1.0f,
                                        (position.x - bars[barIndex].getX()) / juce::jmax(1.0f, bars[barIndex].getWidth()));
            return hit;
        }

        hit.position = juce::jlimit(0.0f,
                                    1.0f,
                                    (position.x - lanes[index].getX()) / juce::jmax(1.0f, lanes[index].getWidth()));
        return hit;
    }

    return hit;
}

void OscillatorLaneOverview::updatePositionAt(juce::Point<float> position)
{
    if (editingLane < 0)
        return;

    const auto lanes = laneBoundsForArea(getLocalBounds().toFloat().reduced(1.0f));
    if (! lanes[static_cast<size_t> (editingLane)].contains(position))
        return;

    const auto nextPosition = juce::jlimit(0.0f,
                                           1.0f,
                                           (position.x - lanes[static_cast<size_t> (editingLane)].getX())
                                               / juce::jmax(1.0f, lanes[static_cast<size_t> (editingLane)].getWidth()));
    auto& lane = state.lanes[static_cast<size_t> (editingLane)];
    if (std::abs(lane.wavetablePosition - nextPosition) < 0.001f)
        return;

    lane.wavetablePosition = nextPosition;

    if (onPositionChange)
        onPositionChange(editingLane == 1, nextPosition);

    repaint();
}

void OscillatorLaneOverview::updateWarpAt(juce::Point<float> position)
{
    if (editingLane < 0)
        return;

    const auto lanes = laneBoundsForArea(getLocalBounds().toFloat().reduced(1.0f));
    const auto bars = warpBarBoundsForLane(lanes[static_cast<size_t> (editingLane)]);
    const auto& activeBar = bars[editingWarpB ? 1 : 0];
    const auto nextAmount = juce::jlimit(0.0f,
                                         1.0f,
                                         (position.x - activeBar.getX()) / juce::jmax(1.0f, activeBar.getWidth()));

    auto& lane = state.lanes[static_cast<size_t> (editingLane)];
    auto& amount = editingWarpB ? lane.warpB : lane.warpA;
    if (std::abs(amount - nextAmount) < 0.001f)
        return;

    amount = nextAmount;

    if (onWarpChange)
        onWarpChange(editingLane == 1, editingWarpB, nextAmount);

    repaint();
}

bool OscillatorLaneOverview::lanesEqual(const Lane& left, const Lane& right) noexcept
{
    return left.label == right.label
        && left.waveName == right.waveName
        && left.warpAMode == right.warpAMode
        && left.warpBMode == right.warpBMode
        && std::abs(left.level - right.level) < 0.001f
        && std::abs(left.wavetablePosition - right.wavetablePosition) < 0.001f
        && std::abs(left.warpA - right.warpA) < 0.001f
        && std::abs(left.warpB - right.warpB) < 0.001f
        && left.active == right.active
        && left.wavetableActive == right.wavetableActive
        && left.customActive == right.customActive;
}

juce::String OscillatorLaneOverview::percentText(float value)
{
    return juce::String(juce::roundToInt(juce::jlimit(0.0f, 1.0f, value) * 100.0f)) + "%";
}
}
