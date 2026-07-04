#pragma once

#include "Theme.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class SequencerRootControls final : public juce::Component
{
public:
    static constexpr int minRoot = 24;
    static constexpr int maxRoot = 84;

    SequencerRootControls();

    std::function<void(int semitones)> onStepRoot;

    void applyTheme(const Theme& theme);
    void setRootState(int root, int octave);
    void resized() override;

private:
    juce::TextButton rootDownButton { "-" };
    juce::TextButton rootUpButton { "+" };
    juce::Label rootValueLabel;
};
}
