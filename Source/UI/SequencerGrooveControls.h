#pragma once

#include "../Parameters.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

namespace UI
{
class SequencerGrooveControls final : public juce::Component
{
public:
    static constexpr int preferredHeight = 68;

    explicit SequencerGrooveControls(juce::AudioProcessorValueTreeState& valueTreeState);

    void resized() override;

private:
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    static void configureCombo(juce::ComboBox& box, const juce::StringArray& choices, const juce::String& emptyText);

    juce::ComboBox grooveBox;
    juce::ComboBox scaleBox;
    juce::ComboBox chordBox;
    juce::ComboBox voicingBox;

    std::unique_ptr<ComboBoxAttachment> grooveAttachment;
    std::unique_ptr<ComboBoxAttachment> scaleAttachment;
    std::unique_ptr<ComboBoxAttachment> chordAttachment;
    std::unique_ptr<ComboBoxAttachment> voicingAttachment;
};
}
