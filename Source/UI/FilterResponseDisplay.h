#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class FilterResponseDisplay final : public juce::Component
{
public:
    struct State
    {
        float cutoffHz = 1800.0f;
        float resonance = 0.45f;
        float envAmount = 0.15f;
        float drive = 0.18f;
        int mode = 0;
        int character = 0;
        int slope = 0;
    };

    void setState(const State& newState);
    void paint(juce::Graphics& g) override;

private:
    State state;

    static float normaliseFrequency(float frequencyHz);
    static float frequencyForX(float xNormalised);
    static float responseAt(const State& state, float frequencyHz);
    static juce::Path makeResponsePath(juce::Rectangle<float> plotBounds, const State& state);
    static juce::String modeText(int mode);
    static juce::String characterText(int character);
    static juce::String slopeText(int slope);
    static juce::Colour characterColour(int character);
};
}
