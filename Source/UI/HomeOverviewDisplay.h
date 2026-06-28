#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace UI
{
class HomeOverviewDisplay : public juce::Component
{
public:
    struct State
    {
        std::array<float, 4> sources { 1.0f, 0.0f, 0.0f, 0.0f };
        std::array<float, 8> macros {};
        float cutoff = 0.0f;
        float drive = 0.0f;
        float pumpReduction = 0.0f;
        float guardReduction = 0.0f;
        float outputPeak = 0.0f;
        float delaySend = 0.0f;
        float reverbSend = 0.0f;
        juce::String sourceName = "Synth";
        juce::String recipeName = "House";
        juce::String safetyName = "SAFE";
    };

    void setState(const State& newState);
    void paint(juce::Graphics& g) override;

private:
    State state;

    static float clamp01(float value) noexcept;
    static juce::Colour accentForIndex(size_t index);
};
}
