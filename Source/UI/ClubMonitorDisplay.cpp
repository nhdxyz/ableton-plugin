#include "ClubMonitorDisplay.h"

#include <cmath>

namespace UI
{
float ClubMonitorDisplay::clamp01(float value) noexcept
{
    return juce::jlimit(0.0f, 1.0f, value);
}

float ClubMonitorDisplay::levelToProportion(float linearLevel) noexcept
{
    if (linearLevel <= 0.000001f)
        return 0.0f;

    const auto db = 20.0f * std::log10(linearLevel);
    return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
}

juce::Colour ClubMonitorDisplay::riskColour(float value, float warnAt, float hotAt) noexcept
{
    if (value >= hotAt)
        return juce::Colour(0xffff6b5e);

    if (value >= warnAt)
        return juce::Colour(0xffffcf5a);

    return juce::Colour(0xff8ee6c9);
}

juce::String ClubMonitorDisplay::statusText(const State& state) noexcept
{
    if (! state.active)
        return "IDLE";

    if (state.outputPeak >= 0.985f)
        return "CLIP";

    if (state.lowStereoRisk >= 0.28f)
        return "LOW SIDE";

    if (state.guardActive && state.guardReduction >= 0.08f)
        return "GUARD";

    if (state.outputPeak >= 0.9f)
        return "HOT";

    if (state.pumpActive && state.pumpReduction >= 0.08f)
        return "PUMP";

    return "SAFE";
}

juce::Colour ClubMonitorDisplay::statusColour(const State& state) noexcept
{
    const auto status = statusText(state);
    if (status == "CLIP" || status == "LOW SIDE")
        return juce::Colour(0xffff6b5e);

    if (status == "HOT" || status == "GUARD")
        return juce::Colour(0xffffcf5a);

    if (status == "PUMP")
        return juce::Colour(0xffd7e37b);

    return state.active ? juce::Colour(0xff8ee6c9) : juce::Colour(0xff657178);
}

juce::String ClubMonitorDisplay::percentText(float value)
{
    return juce::String(juce::roundToInt(clamp01(value) * 100.0f)) + "%";
}

void ClubMonitorDisplay::setState(const State& newState)
{
    state = newState;
    state.subRms = juce::jmax(0.0f, state.subRms);
    state.lowStereoRisk = clamp01(state.lowStereoRisk);
    state.outputPeak = clamp01(state.outputPeak);
    state.guardReduction = clamp01(state.guardReduction);
    state.pumpReduction = clamp01(state.pumpReduction);

    const auto sub = state.active ? levelToProportion(state.subRms) : 0.0f;
    const auto side = state.active ? state.lowStereoRisk : 0.0f;
    const auto guard = state.guardActive ? state.guardReduction : 0.0f;
    const auto pump = state.pumpActive ? state.pumpReduction : 0.0f;
    const auto peak = state.active ? state.outputPeak : 0.0f;
    pushHistory(sub, side, guard, pump, peak);
    updateTooltip();
    repaint();
}

juce::String ClubMonitorDisplay::getTooltip()
{
    return tooltip;
}

void ClubMonitorDisplay::pushHistory(float sub, float side, float guard, float pump, float peak)
{
    subHistory[writeIndex] = clamp01(sub);
    sideHistory[writeIndex] = clamp01(side);
    guardHistory[writeIndex] = clamp01(guard);
    pumpHistory[writeIndex] = clamp01(pump);
    peakHistory[writeIndex] = clamp01(peak);
    writeIndex = (writeIndex + 1) % historySize;
}

void ClubMonitorDisplay::drawTrace(juce::Graphics& g,
                                   juce::Rectangle<float> area,
                                   const juce::String& label,
                                   const History& history,
                                   juce::Colour colour,
                                   float currentValue) const
{
    if (area.getWidth() < 36.0f || area.getHeight() < 4.0f)
        return;

    const auto compact = area.getHeight() < 7.0f;
    auto labelArea = area.removeFromLeft(compact ? 32.0f : 38.0f);
    auto valueArea = area.removeFromRight(compact ? 28.0f : 34.0f);
    auto graph = area.reduced(3.0f, compact ? 1.0f : 2.0f);

    g.setFont(juce::FontOptions(compact ? 6.5f : 7.6f, juce::Font::bold));
    g.setColour(colour.withAlpha(state.active ? 0.96f : 0.38f));
    g.drawFittedText(label, labelArea.toNearestInt(), juce::Justification::centredLeft, 1, 0.62f);

    g.setColour(juce::Colour(0xff182126));
    g.fillRoundedRectangle(graph, 2.0f);

    juce::Path trace;
    for (size_t index = 0; index < historySize; ++index)
    {
        const auto historyIndex = (writeIndex + index) % historySize;
        const auto x = graph.getX() + (graph.getWidth() * static_cast<float>(index) / static_cast<float>(historySize - 1));
        const auto y = graph.getBottom() - (graph.getHeight() * clamp01(history[historyIndex]));
        if (index == 0)
            trace.startNewSubPath(x, y);
        else
            trace.lineTo(x, y);
    }

    g.setColour(colour.withAlpha(state.active ? 0.82f : 0.24f));
    g.strokePath(trace, juce::PathStrokeType(1.15f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(colour.withAlpha(state.active ? 0.22f : 0.08f));
    g.fillRoundedRectangle(graph.withWidth(graph.getWidth() * clamp01(currentValue)), 2.0f);

    g.setColour(juce::Colour(0xff9dafb3));
    g.drawFittedText(percentText(currentValue),
                     valueArea.toNearestInt(),
                     juce::Justification::centredRight,
                     1,
                     0.58f);
}

void ClubMonitorDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const auto compact = bounds.getHeight() < 62.0f;

    g.setColour(juce::Colour(0xff101619));
    g.fillRoundedRectangle(bounds, 5.0f);
    g.setColour(juce::Colour(0xff2b363c));
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

    auto content = bounds.reduced(8.0f, compact ? 4.0f : 5.0f);
    auto header = content.removeFromTop(compact ? 12.0f : 14.0f);
    const auto status = statusText(state);
    const auto statusAccent = statusColour(state);

    g.setFont(juce::FontOptions(compact ? 8.0f : 9.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff8ee6c9));
    g.drawFittedText("CLUB MON", header.removeFromLeft(compact ? 54.0f : 62.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.64f);

    auto statusArea = header.removeFromRight(compact ? 54.0f : 66.0f).reduced(0.0f, 1.0f);
    g.setColour(statusAccent.withAlpha(0.18f));
    g.fillRoundedRectangle(statusArea, 3.0f);
    g.setColour(statusAccent);
    g.drawRoundedRectangle(statusArea, 3.0f, 1.0f);
    g.drawFittedText(status, statusArea.toNearestInt().reduced(2, 0), juce::Justification::centred, 1, 0.62f);

    g.setColour(juce::Colour(0xff73848a));
    g.drawFittedText(state.active ? "short history" : "waiting",
                     header.toNearestInt(),
                     juce::Justification::centredRight,
                     1,
                     0.58f);

    content.removeFromTop(compact ? 1.0f : 2.0f);
    const auto rowHeight = juce::jmax(4.0f, content.getHeight() / 5.0f);
    drawTrace(g, content.removeFromTop(rowHeight), "SUB", subHistory, riskColour(levelToProportion(state.subRms), 0.52f, 0.78f), levelToProportion(state.subRms));
    drawTrace(g, content.removeFromTop(rowHeight), "SIDE", sideHistory, riskColour(state.lowStereoRisk, 0.16f, 0.28f), state.lowStereoRisk);
    drawTrace(g, content.removeFromTop(rowHeight), "PUMP", pumpHistory, juce::Colour(0xffd7e37b), state.pumpActive ? state.pumpReduction : 0.0f);
    drawTrace(g, content.removeFromTop(rowHeight), "GUARD", guardHistory, riskColour(state.guardReduction, 0.06f, 0.18f), state.guardActive ? state.guardReduction : 0.0f);
    drawTrace(g, content, "PEAK", peakHistory, riskColour(state.outputPeak, 0.9f, 0.985f), state.outputPeak);
}

void ClubMonitorDisplay::updateTooltip()
{
    tooltip = "Club monitor\nSub " + percentText(levelToProportion(state.subRms))
        + " | Low-side risk " + percentText(state.lowStereoRisk)
        + " | Pump " + percentText(state.pumpActive ? state.pumpReduction : 0.0f)
        + " | Guard " + percentText(state.guardActive ? state.guardReduction : 0.0f)
        + " | Peak " + percentText(state.outputPeak)
        + "\nUse it to spot short-term club risks while shaping house bass, stabs, pump, and width.";
}
}
