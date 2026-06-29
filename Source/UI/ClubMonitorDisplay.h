#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace UI
{
class ClubMonitorDisplay final : public juce::Component,
                                 public juce::TooltipClient
{
public:
    struct State
    {
        float subRms = 0.0f;
        float lowStereoRisk = 0.0f;
        float outputPeak = 0.0f;
        float guardReduction = 0.0f;
        float pumpReduction = 0.0f;
        bool guardActive = false;
        bool pumpActive = false;
        bool active = false;
    };

    void setState(const State& newState);
    juce::String getTooltip() override;
    void paint(juce::Graphics& g) override;

private:
    static constexpr size_t historySize = 40;
    using History = std::array<float, historySize>;

    State state;
    History subHistory {};
    History sideHistory {};
    History guardHistory {};
    History pumpHistory {};
    History peakHistory {};
    size_t writeIndex = 0;
    juce::String tooltip { "Club monitor: short history for sub, stereo risk, pump, Guard, and peak level" };

    static float clamp01(float value) noexcept;
    static float levelToProportion(float linearLevel) noexcept;
    static juce::Colour riskColour(float value, float warnAt, float hotAt) noexcept;
    static juce::String statusText(const State& state) noexcept;
    static juce::Colour statusColour(const State& state) noexcept;
    static juce::String percentText(float value);

    void pushHistory(float sub, float side, float guard, float pump, float peak);
    void drawTrace(juce::Graphics& g,
                   juce::Rectangle<float> area,
                   const juce::String& label,
                   const History& history,
                   juce::Colour colour,
                   float currentValue) const;
    void updateTooltip();
};
}
