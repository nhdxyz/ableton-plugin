#pragma once

#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class WavetableFrameStrip final : public juce::Component,
                                  public juce::TooltipClient
{
public:
    static constexpr size_t frameCount = 8;
    static constexpr size_t pointCount = 16;
    using CustomPointArray = std::array<float, pointCount>;
    using FrameSet = std::array<CustomPointArray, frameCount>;

    struct Lane
    {
        juce::String label;
        juce::String detail;
        FrameSet frames {};
        float position = 0.0f;
        bool active = false;
    };

    struct State
    {
        Lane osc1;
        Lane osc2;
        juce::String summary;
    };

    struct LayoutMetrics
    {
        int visibleFrameCards = 0;
        int selectedFrameCards = 0;
        float minFrameWidth = 0.0f;
        float minLaneHeight = 0.0f;
        bool readable = false;
    };

    std::function<void(bool)> onPositionEditStart;
    std::function<void(bool, float)> onPositionChange;

    WavetableFrameStrip();

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

private:
    struct HitTarget
    {
        bool valid = false;
        bool osc2 = false;
        float position = 0.0f;
        int frameIndex = -1;
    };

    Theme theme = themeFor(ThemeId::darkClub);
    State state;
    juce::String tooltipText;
    int hoveredLane = -1;
    int hoveredFrame = -1;
    int editingLane = -1;

    std::array<juce::Rectangle<float>, 2> laneBoundsForArea(juce::Rectangle<float> bounds) const;
    std::array<juce::Rectangle<float>, frameCount> frameBoundsForLane(juce::Rectangle<float> laneBounds) const;
    HitTarget hitTargetAt(juce::Point<float> position) const;
    void selectFrame(const HitTarget& hit);
    void updatePositionAt(juce::Point<float> position);
    static bool laneEqual(const Lane& first, const Lane& second) noexcept;
    static float frameRange(const CustomPointArray& frame) noexcept;
    static juce::Path pathForFrame(const CustomPointArray& frame, juce::Rectangle<float> bounds);
};
}
