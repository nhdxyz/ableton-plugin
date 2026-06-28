#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class OutputMeter final : public juce::Component
{
public:
    void setLevels(float newPeakLeft, float newPeakRight, float newRmsLeft, float newRmsRight);
    void paint(juce::Graphics& g) override;

private:
    float peakLeft = 0.0f;
    float peakRight = 0.0f;
    float rmsLeft = 0.0f;
    float rmsRight = 0.0f;
    juce::String safetyText { "SAFE" };
    juce::Colour safetyColour { 0xff8ee6c9 };

    static float levelToProportion(float linearLevel) noexcept;
    static juce::Colour colourForLevel(float linearLevel) noexcept;
    static juce::String safetyTextForLevels(float peak, float rms);
    static juce::Colour safetyColourForText(const juce::String& text) noexcept;
    void drawChannel(juce::Graphics& g, juce::Rectangle<float> area, float peak, float rms) const;
};
}
