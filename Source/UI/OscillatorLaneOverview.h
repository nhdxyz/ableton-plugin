#pragma once

#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

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
        bool osc2Selected = false;
    };

    struct LayoutMetrics
    {
        int visibleLanes = 0;
        int selectedLanes = 0;
        float minLaneWidth = 0.0f;
        float minLaneHeight = 0.0f;
        bool readable = false;
    };

    std::function<void(bool)> onLaneSelected;
    std::function<void(bool)> onPositionEditStart;
    std::function<void(bool, float)> onPositionChange;
    std::function<void(bool)> onOpenLaneEditor;

    OscillatorLaneOverview();

    void setTheme(const Theme& newTheme);
    void setState(State newState);
    LayoutMetrics getLayoutMetricsForAudit() const;
    juce::String getTooltip() override;
    void paint(juce::Graphics& g) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

private:
    struct HitTarget
    {
        bool valid = false;
        bool osc2 = false;
        float position = 0.0f;
    };

    Theme theme = themeFor(ThemeId::darkClub);
    State state;
    juce::String tooltipText;
    int hoveredLane = -1;
    int editingLane = -1;

    std::array<juce::Rectangle<float>, laneCount> laneBoundsForArea(juce::Rectangle<float> bounds) const;
    HitTarget hitTargetAt(juce::Point<float> position) const;
    void updatePositionAt(juce::Point<float> position);
    static bool lanesEqual(const Lane& left, const Lane& right) noexcept;
    static juce::String percentText(float value);
};
}
