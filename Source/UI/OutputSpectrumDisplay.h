#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace UI
{
class OutputSpectrumDisplay final : public juce::Component,
                                    public juce::TooltipClient
{
public:
    static constexpr size_t bandCount = 12;
    using BandArray = std::array<float, bandCount>;

    void setLevels(const BandArray& newLevels, float newPeakLevel, bool newActive);
    juce::String getTooltip() override;
    void paint(juce::Graphics& g) override;

private:
    BandArray levels {};
    BandArray heldLevels {};
    float peakLevel = 0.0f;
    bool active = false;
    juce::String tooltip { "Output spectrum: final rendered energy across club frequency ranges" };

    static float clamp01(float value) noexcept;
    static juce::Colour bandColour(size_t index);
    static float averageRange(const BandArray& values, size_t startIndex, size_t endIndex) noexcept;
    void updateTooltip();
};
}
