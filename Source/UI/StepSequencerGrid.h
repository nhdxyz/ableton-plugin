#pragma once

#include "../Sequencer/PatternSequencer.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class StepSequencerGrid final : public juce::Component
{
public:
    using StepGetter = std::function<Sequencer::Step(int)>;
    using StepSetter = std::function<void(int, Sequencer::Step)>;

    void setCallbacks(StepGetter getter, StepSetter setter);
    void setRootNote(int newRootNote);
    void setScaleMode(int newScaleMode);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    static constexpr int numRows = 13;
    StepGetter getStep;
    StepSetter setStep;
    int lastEditedStep = -1;
    int lastEditedRow = -1;
    int rootNote = 36;
    int scaleMode = 0;

    juce::Rectangle<int> gridBounds() const;
    juce::Rectangle<int> noteLabelBounds() const;
    juce::Rectangle<int> stepHeaderBounds() const;
    int stepForPosition(juce::Point<int> position) const;
    int rowForPosition(juce::Point<int> position) const;
    int noteOffsetForRow(int row) const;
    int rowForNoteOffset(int noteOffset) const;
    bool isOffsetInScale(int noteOffset) const;
    void editAt(juce::Point<int> position);
    void cycleTimingAt(juce::Point<int> position);
    void nudgeTimingAt(juce::Point<int> position, float delta);
};
}
