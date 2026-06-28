#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace UI
{
class OutputSpectrumDisplay final : public juce::Component
{
public:
    static constexpr size_t bandCount = 12;
    using BandArray = std::array<float, bandCount>;

    void setLevels(const BandArray& newLevels, float newPeakLevel, bool newActive);
    void paint(juce::Graphics& g) override;

private:
    BandArray levels {};
    float peakLevel = 0.0f;
    bool active = false;

    static float clamp01(float value) noexcept;
    static juce::Colour bandColour(size_t index);
};
}
