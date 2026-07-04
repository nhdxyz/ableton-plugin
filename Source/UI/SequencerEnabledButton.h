#pragma once

#include "../Parameters.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class SequencerEnabledButton final : public juce::ToggleButton
{
public:
    explicit SequencerEnabledButton(juce::AudioProcessorValueTreeState& valueTreeState);

private:
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    ButtonAttachment attachment;
};
}
