#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class WavetableDisplay final : public juce::Component
{
public:
    void setState(float newOsc1Position, float newOsc2Position, bool newOsc1Active, bool newOsc2Active);
    void paint(juce::Graphics& g) override;

private:
    float osc1Position = 0.0f;
    float osc2Position = 0.35f;
    bool osc1Active = false;
    bool osc2Active = false;

    static float sampleFrame(float phase, float position);
    static juce::Path makePath(juce::Rectangle<float> bounds, float position);
};
}
