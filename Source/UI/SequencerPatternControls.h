#pragma once

#include "../Parameters.h"
#include "SequencerPatternActions.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>

namespace UI
{
class SequencerPatternControls final : public juce::Component
{
public:
    using Action = SequencerPatternActions::Action;

    static constexpr int preferredHeight = 102;

    explicit SequencerPatternControls(juce::AudioProcessorValueTreeState& valueTreeState);

    std::function<void(Action)> onActionClicked;

    int selectedPatternIndex() const;
    void resized() override;

private:
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    juce::ToggleButton chordMemoryButton;
    juce::ComboBox patternBox;
    SequencerPatternActions patternActions;
    std::unique_ptr<ButtonAttachment> chordMemoryAttachment;
};
}
