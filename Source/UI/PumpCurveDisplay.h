#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class PumpCurveDisplay final : public juce::Component
{
public:
    void setState(int newCurveIndex,
                  float newDepth,
                  float newShape,
                  float newPhase,
                  int newRateIndex,
                  bool newEnabled);

    void paint(juce::Graphics& g) override;

private:
    int curveIndex = 0;
    int rateIndex = 0;
    float depth = 0.35f;
    float shape = 0.45f;
    float phase = 0.0f;
    bool enabled = false;

    float duckAmount(float recovery) const;
    juce::String curveName() const;
    juce::String rateName() const;
    juce::Rectangle<float> plotBounds() const;
};
}
