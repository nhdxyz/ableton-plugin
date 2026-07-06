#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace UI::PianoKeyboardLayout
{
inline constexpr auto minimumWhiteKeyWidth = 20.0f;
inline constexpr auto maximumWhiteKeyWidth = 96.0f;
inline constexpr auto lowestNote = 36;
inline constexpr auto highestNote = 96;
inline constexpr auto visualLowestNote = 36;
inline constexpr auto visualHighestNote = 84;
inline constexpr auto initialLowestNote = 72;
inline constexpr auto minLowestVisibleNote = 36;
inline constexpr auto maxLowestVisibleNote = 72;
inline constexpr auto typingKeySpanSemitones = 16;
inline constexpr auto middleCOctave = 3;

int clampLowestVisibleNote(int note) noexcept;
int typingBaseNoteForLowestNote(int note) noexcept;
juce::String noteName(int midiNote);
int whiteKeyCountInRange(int lowestNote, int highestNote) noexcept;
float responsiveKeyWidthForBounds(juce::Rectangle<int> keyboardBounds) noexcept;
}
