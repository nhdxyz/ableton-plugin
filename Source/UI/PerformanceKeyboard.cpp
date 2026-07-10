#include "PerformanceKeyboard.h"

namespace UI
{
PerformanceKeyboard::PerformanceKeyboard(juce::MidiKeyboardState& state, Orientation orientation)
    : juce::MidiKeyboardComponent(state, orientation)
{
}

bool PerformanceKeyboard::mouseDownOnKey(int midiNoteNumber, const juce::MouseEvent&)
{
    if (onManualNoteStart != nullptr)
        onManualNoteStart(midiNoteNumber);

    return true;
}

bool PerformanceKeyboard::mouseDraggedToKey(int midiNoteNumber, const juce::MouseEvent&)
{
    if (onManualNoteStart != nullptr)
        onManualNoteStart(midiNoteNumber);

    return true;
}

void PerformanceKeyboard::mouseUp(const juce::MouseEvent& event)
{
    juce::MidiKeyboardComponent::mouseUp(event);
    if (onManualNoteEnd != nullptr)
        onManualNoteEnd();
}

void PerformanceKeyboard::focusLost(juce::Component::FocusChangeType cause)
{
    juce::MidiKeyboardComponent::focusLost(cause);
    if (onManualNoteEnd != nullptr)
        onManualNoteEnd();
}
}
