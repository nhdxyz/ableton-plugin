#pragma once

#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class HomeSoundStage final : public juce::Component,
                             public juce::TooltipClient
{
public:
    static constexpr size_t frameCount = 8;
    static constexpr size_t pointCount = 16;
    using WaveFrame = std::array<float, pointCount>;
    using WaveStack = std::array<WaveFrame, frameCount>;

    struct OscillatorState
    {
        juce::String name = "O1";
        juce::String waveName = "Saw";
        WaveStack frames {};
        float level = 0.0f;
        float position = 0.0f;
        float warp = 0.0f;
        int waveType = 1;
        bool active = false;
    };

    struct State
    {
        std::array<OscillatorState, 2> oscillators;
        std::array<float, 5> sourceLevels {};
        std::array<float, 8> macros {};
        float cutoff = 0.0f;
        float drive = 0.0f;
        float outputPeak = 0.0f;
        float animationPhase = 0.0f;
        juce::String presetName = "Init";
        juce::String safetyName = "SAFE";
        bool perspective = true;
        bool animate = true;
        bool osc2Selected = false;
    };

    struct LayoutMetrics
    {
        float stageWidth = 0.0f;
        float stageHeight = 0.0f;
        bool readable = false;
    };

    std::function<void(bool)> onOscillatorSelected;
    std::function<void(bool)> onPositionEditStart;
    std::function<void(bool, float)> onPositionChange;
    std::function<void(bool)> onOpenEditor;

    HomeSoundStage();

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
    Theme theme = themeFor(ThemeId::darkClub);
    State state;
    int hoveredOscillator = -1;
    int editingOscillator = -1;
    bool positionEditStarted = false;

    std::array<juce::Rectangle<float>, 2> oscillatorAreas() const;
    int oscillatorAt(juce::Point<float> point) const;
    void updatePositionAt(juce::Point<float> point);
    juce::Path wavePath(const OscillatorState& oscillator,
                        size_t frameIndex,
                        juce::Rectangle<float> bounds) const;
    float waveValue(const OscillatorState& oscillator, size_t frameIndex, float phase) const;
    static float clamp01(float value) noexcept;
};
}
