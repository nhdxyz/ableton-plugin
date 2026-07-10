#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include <functional>

namespace UI
{
class PerformanceKeyboard final : public juce::MidiKeyboardComponent
{
public:
    PerformanceKeyboard(juce::MidiKeyboardState& state, Orientation orientation);

    std::function<void(int)> onManualNoteStart;

    bool mouseDownOnKey(int midiNoteNumber, const juce::MouseEvent& event) override;
    bool mouseDraggedToKey(int midiNoteNumber, const juce::MouseEvent& event) override;
};
}
