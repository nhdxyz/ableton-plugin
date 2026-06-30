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

    struct LayoutMetrics
    {
        juce::Rectangle<int> grid;
        juce::Rectangle<int> noteLabels;
        juce::Rectangle<int> lanes;
        juce::Rectangle<int> laneLabels;
        juce::Rectangle<int> header;
        float noteRowHeight = 0.0f;
        float laneRowHeight = 0.0f;
        float stepCellWidth = 0.0f;
    };

    LayoutMetrics getLayoutMetricsForAudit() const;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    static constexpr int numRows = Sequencer::PatternSequencer::maxNoteOffset
                                 - Sequencer::PatternSequencer::minNoteOffset
                                 + 1;
    static constexpr int laneCount = 8;
    static constexpr int laneLabelWidth = 44;
    static constexpr int minimumLaneAreaHeight = 64;
    static constexpr int maximumLaneAreaHeight = 112;
    static constexpr int targetMinimumNoteRowHeight = 6;
    enum class DragMode
    {
        none,
        paint,
        erase,
        velocity,
        probability,
        timing,
        length,
        lock,
        ratchet,
        condition,
        slide
    };

    StepGetter getStep;
    StepSetter setStep;
    int lastEditedStep = -1;
    int lastEditedRow = -1;
    int rootNote = 36;
    int scaleMode = 0;
    DragMode dragMode = DragMode::none;

    juce::Rectangle<int> gridBounds() const;
    juce::Rectangle<int> noteLabelBounds() const;
    juce::Rectangle<int> laneBounds() const;
    juce::Rectangle<int> laneLabelBounds() const;
    juce::Rectangle<int> stepHeaderBounds() const;
    int laneAreaHeightForCurrentBounds() const noexcept;
    int stepForPosition(juce::Point<int> position) const;
    int laneStepForPosition(juce::Point<int> position) const;
    int laneForPosition(juce::Point<int> position) const;
    int rowForPosition(juce::Point<int> position) const;
    int noteOffsetForRow(int row) const;
    int rowForNoteOffset(int noteOffset) const;
    bool isOffsetInScale(int noteOffset) const;
    void paintLaneRows(juce::Graphics& g) const;
    void beginEditAt(juce::Point<int> position);
    void editAt(juce::Point<int> position);
    void editLaneAt(juce::Point<int> position);
    void cycleTimingAt(juce::Point<int> position);
    void nudgeTimingAt(juce::Point<int> position, float delta);
    void nudgeLaneAt(juce::Point<int> position, float delta);
};
}
