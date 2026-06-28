#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class WavetableDisplay final : public juce::Component
{
public:
    void setState(float newOsc1Position,
                  float newOsc2Position,
                  bool newOsc1Active,
                  bool newOsc2Active,
                  float newOsc1ModAmount = 0.0f,
                  float newOsc2ModAmount = 0.0f,
                  int newModRouteCount = 0,
                  juce::String newModSourceSummary = {});
    void paint(juce::Graphics& g) override;

private:
    float osc1Position = 0.0f;
    float osc2Position = 0.35f;
    float osc1ModAmount = 0.0f;
    float osc2ModAmount = 0.0f;
    int modRouteCount = 0;
    juce::String modSourceSummary;
    bool osc1Active = false;
    bool osc2Active = false;

    static float sampleFrame(float phase, float position);
    static juce::Path makePath(juce::Rectangle<float> bounds, float position);
    static juce::String modulationText(float amount);
};
}
