#pragma once

#include "../Parameters.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>

namespace UI
{
class SequencerTransformControls final : public juce::Component
{
public:
    static constexpr int preferredHeight = 66;

    explicit SequencerTransformControls(juce::AudioProcessorValueTreeState& valueTreeState);

    std::function<void(int transformIndex, const juce::String& transformName)> onApplyTransform;

    void resized() override;

private:
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    juce::ComboBox lockDestinationBox;
    juce::ComboBox grooveTransformBox;
    juce::TextButton applyButton { "Shape" };
    std::unique_ptr<ComboBoxAttachment> lockDestinationAttachment;
};
}
