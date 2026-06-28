#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class StereoFieldDisplay final : public juce::Component
{
public:
    struct State
    {
        float correlation = 0.0f;
        float width = 0.0f;
        float balance = 0.0f;
        float lowStereoRisk = 0.0f;
        bool active = false;
    };

    void setState(const State& newState);
    void paint(juce::Graphics& g) override;

private:
    State state;

    static float clamp01(float value) noexcept;
    static float clampBipolar(float value) noexcept;
    static juce::Colour colourForCorrelation(float correlation) noexcept;
    static juce::Colour colourForRisk(float risk) noexcept;
    static juce::String statusTextForState(const State& state);
    void drawBar(juce::Graphics& g,
                 juce::Rectangle<float> area,
                 const juce::String& label,
                 float value,
                 juce::Colour colour,
                 bool bipolar) const;
};
}
