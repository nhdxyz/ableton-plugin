#pragma once

#include "../Parameters.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

namespace UI
{
class SamplePlaybackControls final : public juce::Component
{
public:
    explicit SamplePlaybackControls(juce::AudioProcessorValueTreeState& valueTreeState);

    static constexpr int preferredHeight = 90;

    int getSliceStyleSelectedItemIndex() const;
    juce::String getSliceStyleText() const;

    void resized() override;

private:
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    juce::ComboBox modeBox;
    juce::ComboBox engineBox;
    juce::ComboBox sliceStyleBox;
    juce::ToggleButton stutterEnabledButton;
    juce::ComboBox stutterRateBox;

    std::unique_ptr<ComboBoxAttachment> modeAttachment;
    std::unique_ptr<ComboBoxAttachment> engineAttachment;
    std::unique_ptr<ComboBoxAttachment> sliceStyleAttachment;
    std::unique_ptr<ButtonAttachment> stutterEnabledAttachment;
    std::unique_ptr<ComboBoxAttachment> stutterRateAttachment;
};
}
