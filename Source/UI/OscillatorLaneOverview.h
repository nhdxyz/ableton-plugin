#pragma once

#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace UI
{
class OscillatorLaneOverview final : public juce::Component,
                                     public juce::TooltipClient
{
public:
    static constexpr size_t laneCount = 2;

    struct Lane
    {
        juce::String label;
        juce::String waveName;
        juce::String warpAMode;
        juce::String warpBMode;
        float level = 0.0f;
        float wavetablePosition = 0.0f;
        float warpA = 0.0f;
        float warpB = 0.0f;
        bool active = false;
        bool wavetableActive = false;
        bool customActive = false;
    };

    struct State
    {
        std::array<Lane, laneCount> lanes;
        juce::String summary;
    };

    struct LayoutMetrics
    {
        int visibleLanes = 0;
        float minLaneWidth = 0.0f;
        float minLaneHeight = 0.0f;
        bool readable = false;
    };

    OscillatorLaneOverview();

    void setTheme(const Theme& newTheme);
    void setState(State newState);
    LayoutMetrics getLayoutMetricsForAudit() const;
    juce::String getTooltip() override;
    void paint(juce::Graphics& g) override;

private:
    Theme theme = themeFor(ThemeId::darkClub);
    State state;
    juce::String tooltipText;

    std::array<juce::Rectangle<float>, laneCount> laneBoundsForArea(juce::Rectangle<float> bounds) const;
    static bool lanesEqual(const Lane& left, const Lane& right) noexcept;
    static juce::String percentText(float value);
};
}
