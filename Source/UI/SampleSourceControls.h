#pragma once

#include "../Parameters.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

namespace UI
{
class SampleSourceControls final : public juce::Component
{
public:
    explicit SampleSourceControls(juce::AudioProcessorValueTreeState& valueTreeState);

    static constexpr int preferredHeight = 30;

    void resized() override;

private:
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    juce::ToggleButton enabledButton;
    juce::ToggleButton reverseButton;

    std::unique_ptr<ButtonAttachment> enabledAttachment;
    std::unique_ptr<ButtonAttachment> reverseAttachment;
};
}
