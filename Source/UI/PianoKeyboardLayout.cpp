#include "PianoKeyboardLayout.h"

namespace UI::PianoKeyboardLayout
{
static_assert(lowestNote <= minLowestVisibleNote);
static_assert(minLowestVisibleNote <= initialLowestNote);
static_assert(initialLowestNote <= maxLowestVisibleNote);
static_assert(maxLowestVisibleNote + typingKeySpanSemitones <= highestNote);
static_assert(lowestNote <= visualLowestNote);
static_assert(visualLowestNote <= visualHighestNote);
static_assert(visualHighestNote <= highestNote);
static_assert(visualLowestNote == 36);
static_assert(visualHighestNote == 84);
static_assert(initialLowestNote == 72);
static_assert(minLowestVisibleNote == 36);
static_assert(maxLowestVisibleNote == 72);

int clampLowestVisibleNote(int note) noexcept
{
    return juce::jlimit(minLowestVisibleNote, maxLowestVisibleNote, note);
}

int typingBaseNoteForLowestNote(int note) noexcept
{
    return clampLowestVisibleNote(note);
}

juce::String noteName(int midiNote)
{
    return juce::MidiMessage::getMidiNoteName(midiNote, true, true, middleCOctave);
}

int whiteKeyCountInRange(int rangeLowestNote, int rangeHighestNote) noexcept
{
    auto count = 0;
    for (auto note = rangeLowestNote; note <= rangeHighestNote; ++note)
        if (! juce::MidiMessage::isMidiNoteBlack(note % 12))
            ++count;

    return juce::jmax(1, count);
}

float responsiveKeyWidthForBounds(juce::Rectangle<int> keyboardBounds) noexcept
{
    const auto whiteKeyCount = static_cast<float>(whiteKeyCountInRange(visualLowestNote,
                                                                       visualHighestNote));
    const auto fillWidth = static_cast<float>(juce::jmax(1, keyboardBounds.getWidth())) / whiteKeyCount;
    return juce::jlimit(minimumWhiteKeyWidth, maximumWhiteKeyWidth, fillWidth);
}
}
