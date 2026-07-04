#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class SequencerRateControls final : public juce::Component
{
public:
    SequencerRateControls();

    std::function<void(int rateIndex)> onRateSelected;

    void setSelectedIndex(int rateIndex);
    void resized() override;

private:
    std::array<juce::TextButton, 3> rateButtons {
        juce::TextButton { "1/8" },
        juce::TextButton { "1/16" },
        juce::TextButton { "1/32" }
    };
};
}
