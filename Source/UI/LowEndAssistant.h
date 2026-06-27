#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class LowEndAssistant final : public juce::Component
{
public:
    struct State
    {
        juce::String rootText;
        float subRms = 0.0f;
        float lowStereoRisk = 0.0f;
        float outputPeak = 0.0f;
        float monoCrossoverHz = 120.0f;
        bool monoEnabled = false;
        bool widthEnabled = false;
        bool guardEnabled = false;
    };

    void setState(const State& newState);
    void paint(juce::Graphics& g) override;

private:
    State state;

    static float levelToProportion(float linearLevel) noexcept;
    static juce::Colour levelColour(float linearLevel) noexcept;
    static juce::Colour riskColour(float risk) noexcept;
    void drawMeter(juce::Graphics& g, juce::Rectangle<int> area, const juce::String& label, float value, bool riskMode) const;
};
}
