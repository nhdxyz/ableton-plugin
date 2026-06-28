#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace UI
{
class OutputOscilloscopeDisplay final : public juce::Component,
                                       public juce::TooltipClient
{
public:
    static constexpr size_t sampleCount = 192;
    using SampleArray = std::array<float, sampleCount>;

    void setSamples(const SampleArray& newSamples, float newPeakLevel, float newTransientLevel, bool newActive);
    juce::String getTooltip() override;
    void paint(juce::Graphics& g) override;

private:
    SampleArray samples {};
    float peakLevel = 0.0f;
    float transientLevel = 0.0f;
    bool active = false;
    juce::String tooltip { "Output oscilloscope: final rendered waveform and transient shape" };

    static float clamp01(float value) noexcept;
    void updateTooltip();
};
}
