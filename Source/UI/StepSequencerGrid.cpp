#include "StepSequencerGrid.h"

#include <cmath>
#include <initializer_list>

namespace UI
{
namespace
{
juce::String conditionLabel(int condition)
{
    switch (juce::jlimit(Sequencer::PatternSequencer::minCondition,
                         Sequencer::PatternSequencer::maxCondition,
                         condition))
    {
        case 1: return "O";
        case 2: return "E";
        case 3: return "F";
        case 0:
        default:
            return "A";
    }
}
}

void StepSequencerGrid::setCallbacks(StepGetter getter, StepSetter setter)
{
    getStep = std::move(getter);
    setStep = std::move(setter);
}

void StepSequencerGrid::setRootNote(int newRootNote)
{
    newRootNote = juce::jlimit(0, 127, newRootNote);
    if (rootNote == newRootNote)
        return;

    rootNote = newRootNote;
    repaint();
}

void StepSequencerGrid::setScaleMode(int newScaleMode)
{
    newScaleMode = juce::jlimit(0, 4, newScaleMode);
    if (scaleMode == newScaleMode)
        return;

    scaleMode = newScaleMode;
    repaint();
}

void StepSequencerGrid::paint(juce::Graphics& g)
{
    const auto bounds = gridBounds();
    const auto noteLabels = noteLabelBounds();
    const auto header = stepHeaderBounds();
    const auto cellWidth = static_cast<float>(bounds.getWidth()) / static_cast<float>(Sequencer::PatternSequencer::numSteps);
    const auto cellHeight = static_cast<float>(bounds.getHeight()) / static_cast<float>(numRows);

    g.setColour(juce::Colour(0xff101619));
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    g.fillRoundedRectangle(noteLabels.toFloat(), 5.0f);
    g.fillRoundedRectangle(header.toFloat(), 5.0f);
    paintLaneRows(g);

    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    for (auto step = 0; step < Sequencer::PatternSequencer::numSteps; ++step)
    {
        const auto x = bounds.getX() + (static_cast<float>(step) * cellWidth);
        const auto isBeatStart = (step % 4) == 0;
        auto stepArea = juce::Rectangle<float>(x, static_cast<float>(header.getY()), cellWidth, static_cast<float>(header.getHeight())).reduced(1.0f);
        g.setColour(isBeatStart ? juce::Colour(0xff8ee6c9) : juce::Colour(0xff617078));
        const auto text = isBeatStart ? juce::String((step / 4) + 1) : juce::String((step % 4) + 1);
        g.drawFittedText(text, stepArea.toNearestInt(), juce::Justification::centred, 1);
    }

    for (auto row = 0; row < numRows; ++row)
    {
        const auto y = bounds.getY() + (static_cast<float>(row) * cellHeight);
        const auto noteOffset = noteOffsetForRow(row);
        const auto inScale = isOffsetInScale(noteOffset);
        g.setColour(noteOffset == 0 ? juce::Colour(0xff223038)
                                    : inScale ? (row % 2 == 0 ? juce::Colour(0xff182126) : juce::Colour(0xff151e22))
                                              : juce::Colour(0xff11171a));
        g.fillRect(juce::Rectangle<float>(static_cast<float>(bounds.getX()), y, static_cast<float>(bounds.getWidth()), cellHeight - 1.0f));

        auto labelArea = juce::Rectangle<float>(static_cast<float>(noteLabels.getX()),
                                                y,
                                                static_cast<float>(noteLabels.getWidth()),
                                                cellHeight - 1.0f).reduced(4.0f, 0.0f);
        const auto noteName = juce::MidiMessage::getMidiNoteName(juce::jlimit(0, 127, rootNote + noteOffset), true, true, 3);
        const auto labelFontSize = juce::jlimit(6.0f, 9.5f, cellHeight - 2.0f);
        g.setColour(noteOffset == 0 ? juce::Colour(0xff8ee6c9) : inScale ? juce::Colour(0xffa8b6b8) : juce::Colour(0xff536066));
        g.setFont(juce::FontOptions(noteOffset == 0 ? labelFontSize : juce::jmin(9.0f, labelFontSize),
                                    noteOffset == 0 ? juce::Font::bold : juce::Font::plain));
        g.drawFittedText(noteName, labelArea.toNearestInt(), juce::Justification::centredRight, 1);
    }

    for (auto step = 0; step <= Sequencer::PatternSequencer::numSteps; ++step)
    {
        const auto x = bounds.getX() + (static_cast<float>(step) * cellWidth);
        const auto isBeatLine = (step % 4) == 0;
        g.setColour(isBeatLine ? juce::Colour(0xff4b5c62) : juce::Colour(0xff293339));
        g.drawVerticalLine(static_cast<int>(std::round(x)), static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));
        if (isBeatLine && step < Sequencer::PatternSequencer::numSteps)
            g.drawVerticalLine(static_cast<int>(std::round(x + 1.0f)), static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));
    }

    g.setColour(juce::Colour(0xff293339));
    for (auto row = 0; row <= numRows; ++row)
    {
        const auto y = bounds.getY() + (static_cast<float>(row) * cellHeight);
        g.drawHorizontalLine(static_cast<int>(std::round(y)), static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));
    }

    if (getStep)
    {
        for (auto stepIndex = 0; stepIndex < Sequencer::PatternSequencer::numSteps; ++stepIndex)
        {
            const auto step = getStep(stepIndex);
            if (! step.enabled)
                continue;

            const auto row = rowForNoteOffset(step.noteOffset);
            auto cell = juce::Rectangle<float>(
                bounds.getX() + (static_cast<float>(stepIndex) * cellWidth),
                bounds.getY() + (static_cast<float>(row) * cellHeight),
                cellWidth,
                cellHeight).reduced(cellWidth > 24.0f ? 3.0f : 2.0f,
                                    cellHeight > 12.0f ? 2.0f : 1.0f);
            const auto fullCell = cell;
            const auto noteLength = juce::jlimit(0.1f, 1.0f, step.length);
            cell.setWidth(juce::jmax(4.0f, fullCell.getWidth() * noteLength));

            const auto alpha = juce::jlimit(0.35f, 1.0f, step.velocity);
            g.setColour(juce::Colour(0xff8ee6c9).withAlpha(alpha));
            g.fillRoundedRectangle(cell, 3.0f);
            if (noteLength < 0.995f)
            {
                auto remainder = juce::Rectangle<float>(
                    cell.getRight(),
                    fullCell.getY(),
                    fullCell.getRight() - cell.getRight(),
                    fullCell.getHeight());
                g.setColour(juce::Colour(0xff273238).withAlpha(0.42f));
                g.fillRoundedRectangle(remainder, 3.0f);
            }
            if (step.timing > 0.0f)
            {
                const auto timingWidth = juce::jmax(3.0f, cell.getWidth() * juce::jlimit(0.0f, 1.0f, step.timing));
                auto timingMarker = cell.withY(cell.getBottom() - 2.0f).withHeight(2.0f).withWidth(timingWidth);
                g.setColour(juce::Colour(0xffffc857).withAlpha(0.9f));
                g.fillRoundedRectangle(timingMarker, 1.0f);
            }
            if (step.probability < 0.995f)
            {
                const auto probabilityWidth = juce::jmax(3.0f, cell.getWidth() * juce::jlimit(0.0f, 1.0f, step.probability));
                auto probabilityMarker = cell.withHeight(2.0f).withWidth(probabilityWidth);
                g.setColour(juce::Colour(0xffb7a4ff).withAlpha(0.9f));
                g.fillRoundedRectangle(probabilityMarker, 1.0f);
            }
            if (step.slide)
            {
                g.setColour(juce::Colour(0xff69d2ff).withAlpha(0.92f));
                juce::Path slidePath;
                slidePath.startNewSubPath(cell.getX() + 3.0f, cell.getBottom() - 3.0f);
                slidePath.lineTo(cell.getRight() - 3.0f, cell.getY() + 3.0f);
                g.strokePath(slidePath, juce::PathStrokeType(1.8f));
            }
            g.setColour(juce::Colour(0xff0d1113));
            g.drawRoundedRectangle(cell, 3.0f, 1.0f);
        }
    }

    g.setColour(juce::Colour(0xff344047));
    g.drawRoundedRectangle(bounds.toFloat(), 6.0f, 1.0f);
    g.drawRoundedRectangle(noteLabels.toFloat(), 5.0f, 1.0f);
    g.drawRoundedRectangle(header.toFloat(), 5.0f, 1.0f);
}

StepSequencerGrid::LayoutMetrics StepSequencerGrid::getLayoutMetricsForAudit() const
{
    LayoutMetrics metrics;
    metrics.grid = gridBounds();
    metrics.noteLabels = noteLabelBounds();
    metrics.lanes = laneBounds();
    metrics.laneLabels = laneLabelBounds();
    metrics.header = stepHeaderBounds();
    metrics.noteRowHeight = static_cast<float>(metrics.grid.getHeight()) / static_cast<float>(numRows);
    metrics.laneRowHeight = static_cast<float>(metrics.lanes.getHeight()) / static_cast<float>(laneCount);
    metrics.stepCellWidth = static_cast<float>(metrics.grid.getWidth()) / static_cast<float>(Sequencer::PatternSequencer::numSteps);
    return metrics;
}

void StepSequencerGrid::mouseDown(const juce::MouseEvent& event)
{
    lastEditedStep = -1;
    lastEditedRow = -1;
    dragMode = DragMode::none;

    if (event.mods.isPopupMenu() || event.mods.isCommandDown())
    {
        cycleTimingAt(event.getPosition());
        return;
    }

    beginEditAt(event.getPosition());
}

void StepSequencerGrid::mouseDrag(const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu() || event.mods.isCommandDown())
        return;

    editAt(event.getPosition());
}

void StepSequencerGrid::mouseUp(const juce::MouseEvent&)
{
    dragMode = DragMode::none;
    lastEditedStep = -1;
    lastEditedRow = -1;
}

void StepSequencerGrid::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (laneForPosition(event.getPosition()) >= 0)
    {
        nudgeLaneAt(event.getPosition(), wheel.deltaY >= 0.0f ? 0.05f : -0.05f);
        return;
    }

    nudgeTimingAt(event.getPosition(), wheel.deltaY >= 0.0f ? 0.1f : -0.1f);
}

juce::Rectangle<int> StepSequencerGrid::gridBounds() const
{
    auto bounds = getLocalBounds().reduced(2);
    bounds.removeFromTop(18);
    bounds.removeFromLeft(laneLabelWidth + 4);
    bounds.removeFromBottom(laneAreaHeightForCurrentBounds() + 4);
    return bounds;
}

juce::Rectangle<int> StepSequencerGrid::noteLabelBounds() const
{
    auto bounds = getLocalBounds().reduced(2);
    bounds.removeFromTop(18);
    auto labels = bounds.removeFromLeft(laneLabelWidth);
    labels.removeFromBottom(laneAreaHeightForCurrentBounds() + 4);
    return labels;
}

juce::Rectangle<int> StepSequencerGrid::laneBounds() const
{
    auto bounds = getLocalBounds().reduced(2);
    bounds.removeFromTop(18);
    bounds.removeFromLeft(laneLabelWidth + 4);
    return bounds.removeFromBottom(laneAreaHeightForCurrentBounds());
}

juce::Rectangle<int> StepSequencerGrid::laneLabelBounds() const
{
    auto bounds = getLocalBounds().reduced(2);
    bounds.removeFromTop(18);
    auto labels = bounds.removeFromLeft(laneLabelWidth);
    return labels.removeFromBottom(laneAreaHeightForCurrentBounds());
}

juce::Rectangle<int> StepSequencerGrid::stepHeaderBounds() const
{
    auto bounds = getLocalBounds().reduced(2);
    bounds.removeFromLeft(laneLabelWidth + 4);
    return bounds.removeFromTop(16);
}

int StepSequencerGrid::laneAreaHeightForCurrentBounds() const noexcept
{
    const auto availableHeight = juce::jmax(0, getHeight() - 26);
    const auto maxLaneHeightForReadableNotes = juce::jmax(minimumLaneAreaHeight,
                                                          availableHeight - (numRows * targetMinimumNoteRowHeight));
    const auto upperLimit = juce::jmin(maximumLaneAreaHeight, maxLaneHeightForReadableNotes);
    const auto target = juce::roundToInt(static_cast<float>(availableHeight) * 0.34f);

    return juce::jlimit(minimumLaneAreaHeight, upperLimit, target);
}

int StepSequencerGrid::stepForPosition(juce::Point<int> position) const
{
    const auto bounds = gridBounds();
    if (! bounds.contains(position))
        return -1;

    const auto normalised = static_cast<float>(position.x - bounds.getX()) / static_cast<float>(bounds.getWidth());
    return juce::jlimit(0, Sequencer::PatternSequencer::numSteps - 1,
                        static_cast<int>(normalised * static_cast<float>(Sequencer::PatternSequencer::numSteps)));
}

int StepSequencerGrid::laneStepForPosition(juce::Point<int> position) const
{
    const auto bounds = laneBounds();
    if (! bounds.contains(position))
        return -1;

    const auto normalised = static_cast<float>(position.x - bounds.getX()) / static_cast<float>(juce::jmax(1, bounds.getWidth()));
    return juce::jlimit(0, Sequencer::PatternSequencer::numSteps - 1,
                        static_cast<int>(normalised * static_cast<float>(Sequencer::PatternSequencer::numSteps)));
}

int StepSequencerGrid::laneForPosition(juce::Point<int> position) const
{
    const auto bounds = laneBounds();
    if (! bounds.contains(position))
        return -1;

    const auto laneHeight = static_cast<float>(bounds.getHeight()) / static_cast<float>(laneCount);
    const auto normalised = static_cast<float>(position.y - bounds.getY()) / juce::jmax(1.0f, laneHeight);
    return juce::jlimit(0, laneCount - 1, static_cast<int>(normalised));
}

int StepSequencerGrid::rowForPosition(juce::Point<int> position) const
{
    const auto bounds = gridBounds();
    if (! bounds.contains(position))
        return -1;

    const auto normalised = static_cast<float>(position.y - bounds.getY()) / static_cast<float>(bounds.getHeight());
    return juce::jlimit(0, numRows - 1, static_cast<int>(normalised * static_cast<float>(numRows)));
}

int StepSequencerGrid::noteOffsetForRow(int row) const
{
    return Sequencer::PatternSequencer::maxNoteOffset - juce::jlimit(0, numRows - 1, row);
}

int StepSequencerGrid::rowForNoteOffset(int noteOffset) const
{
    return juce::jlimit(0,
                        numRows - 1,
                        Sequencer::PatternSequencer::maxNoteOffset
                            - juce::jlimit(Sequencer::PatternSequencer::minNoteOffset,
                                           Sequencer::PatternSequencer::maxNoteOffset,
                                           noteOffset));
}

bool StepSequencerGrid::isOffsetInScale(int noteOffset) const
{
    if (scaleMode <= 0)
        return true;

    const auto pitchClass = ((noteOffset % 12) + 12) % 12;

    auto contains = [pitchClass] (std::initializer_list<int> scale)
    {
        for (const auto degree : scale)
            if (pitchClass == degree)
                return true;

        return false;
    };

    switch (scaleMode)
    {
        case 1: return contains({ 0, 2, 4, 5, 7, 9, 11 });
        case 2: return contains({ 0, 2, 3, 5, 7, 8, 10 });
        case 3: return contains({ 0, 2, 3, 5, 7, 9, 10 });
        case 4: return contains({ 0, 3, 5, 7, 10 });
        default: return true;
    }
}

void StepSequencerGrid::paintLaneRows(juce::Graphics& g) const
{
    const auto lanes = laneBounds();
    const auto labels = laneLabelBounds();
    if (lanes.isEmpty() || labels.isEmpty())
        return;

    const auto laneHeight = static_cast<float>(lanes.getHeight()) / static_cast<float>(laneCount);
    const auto cellWidth = static_cast<float>(lanes.getWidth()) / static_cast<float>(Sequencer::PatternSequencer::numSteps);
    const std::array<const char*, laneCount> laneNames { "Vel", "Prob", "Late", "Len", "Lock", "Rat", "Cond", "Slide" };
    const std::array<juce::Colour, laneCount> laneColours {
        juce::Colour(0xff8ee6c9),
        juce::Colour(0xffb7a4ff),
        juce::Colour(0xffffc857),
        juce::Colour(0xff8fb7ff),
        juce::Colour(0xffff8ba7),
        juce::Colour(0xfff7a85d),
        juce::Colour(0xff69d2ff),
        juce::Colour(0xff8ee6c9)
    };

    g.setColour(juce::Colour(0xff101619));
    g.fillRoundedRectangle(lanes.toFloat(), 5.0f);
    g.fillRoundedRectangle(labels.toFloat(), 5.0f);

    for (auto lane = 0; lane < laneCount; ++lane)
    {
        auto row = juce::Rectangle<float>(
            static_cast<float>(lanes.getX()),
            static_cast<float>(lanes.getY()) + (static_cast<float>(lane) * laneHeight),
            static_cast<float>(lanes.getWidth()),
            laneHeight).reduced(1.0f, 1.0f);

        auto labelRow = juce::Rectangle<float>(
            static_cast<float>(labels.getX()),
            row.getY(),
            static_cast<float>(labels.getWidth()),
            row.getHeight()).reduced(4.0f, 0.0f);

        g.setColour((lane % 2) != 0 ? juce::Colour(0xff121920) : juce::Colour(0xff151e22));
        g.fillRoundedRectangle(row, 3.0f);
        g.setColour(laneColours[static_cast<size_t>(lane)].withAlpha(0.72f));
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.drawFittedText(laneNames[static_cast<size_t>(lane)], labelRow.toNearestInt(), juce::Justification::centredRight, 1);

        if (! getStep)
            continue;

        for (auto stepIndex = 0; stepIndex < Sequencer::PatternSequencer::numSteps; ++stepIndex)
        {
            const auto step = getStep(stepIndex);
            auto value = 0.0f;
            switch (lane)
            {
                case 0: value = step.velocity; break;
                case 1: value = step.probability; break;
                case 2: value = step.timing; break;
                case 3: value = step.length; break;
                case 4: value = step.lock; break;
                case 5:
                    value = static_cast<float>(juce::jlimit(Sequencer::PatternSequencer::minRatchet,
                                                            Sequencer::PatternSequencer::maxRatchet,
                                                            step.ratchet)
                                               - Sequencer::PatternSequencer::minRatchet)
                          / static_cast<float>(Sequencer::PatternSequencer::maxRatchet
                                               - Sequencer::PatternSequencer::minRatchet);
                    break;
                case 6:
                    value = static_cast<float>(juce::jlimit(Sequencer::PatternSequencer::minCondition,
                                                            Sequencer::PatternSequencer::maxCondition,
                                                            step.condition))
                          / static_cast<float>(Sequencer::PatternSequencer::maxCondition);
                    break;
                case 7:
                    value = step.slide ? 1.0f : 0.0f;
                    break;
                default: break;
            }

            value = juce::jlimit(0.0f, 1.0f, value);
            auto cell = juce::Rectangle<float>(
                static_cast<float>(lanes.getX()) + (static_cast<float>(stepIndex) * cellWidth),
                row.getY(),
                cellWidth,
                row.getHeight()).reduced(2.0f, 2.0f);

            g.setColour(juce::Colour(0xff273238));
            g.fillRoundedRectangle(cell, 2.0f);

            const auto barHeight = juce::jmax(1.0f, cell.getHeight() * value);
            auto bar = cell.withY(cell.getBottom() - barHeight).withHeight(barHeight);
            g.setColour(laneColours[static_cast<size_t>(lane)].withAlpha(step.enabled ? 0.86f : 0.24f));
            g.fillRoundedRectangle(bar, 2.0f);
            if (lane == 5 && step.ratchet > 1)
            {
                g.setColour(juce::Colour(0xff101619).withAlpha(0.86f));
                g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
                g.drawFittedText(juce::String(step.ratchet) + "x",
                                 cell.toNearestInt().reduced(1),
                                 juce::Justification::centred,
                                 1);
            }
            else if (lane == 6 && step.condition > Sequencer::PatternSequencer::minCondition)
            {
                g.setColour(juce::Colour(0xff101619).withAlpha(0.86f));
                g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
                g.drawFittedText(conditionLabel(step.condition),
                                 cell.toNearestInt().reduced(1),
                                 juce::Justification::centred,
                                 1);
            }
            else if (lane == 7 && step.slide)
            {
                g.setColour(juce::Colour(0xff101619).withAlpha(0.86f));
                g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
                g.drawFittedText("SL",
                                 cell.toNearestInt().reduced(1),
                                 juce::Justification::centred,
                                 1);
            }
        }
    }

    g.setColour(juce::Colour(0xff344047));
    for (auto step = 0; step <= Sequencer::PatternSequencer::numSteps; ++step)
    {
        const auto x = lanes.getX() + (static_cast<float>(step) * cellWidth);
        g.drawVerticalLine(static_cast<int>(std::round(x)),
                           static_cast<float>(lanes.getY()),
                           static_cast<float>(lanes.getBottom()));
    }

    for (auto lane = 1; lane < laneCount; ++lane)
    {
        const auto y = lanes.getY() + static_cast<int>(std::round(static_cast<float>(lane) * laneHeight));
        g.drawHorizontalLine(y, static_cast<float>(labels.getX()), static_cast<float>(lanes.getRight()));
    }

    g.drawRoundedRectangle(lanes.toFloat(), 5.0f, 1.0f);
    g.drawRoundedRectangle(labels.toFloat(), 5.0f, 1.0f);
}

void StepSequencerGrid::beginEditAt(juce::Point<int> position)
{
    if (! getStep)
        return;

    const auto lane = laneForPosition(position);
    if (lane >= 0)
    {
        switch (lane)
        {
            case 0: dragMode = DragMode::velocity; break;
            case 1: dragMode = DragMode::probability; break;
            case 2: dragMode = DragMode::timing; break;
            case 3: dragMode = DragMode::length; break;
            case 4: dragMode = DragMode::lock; break;
            case 5: dragMode = DragMode::ratchet; break;
            case 6: dragMode = DragMode::condition; break;
            case 7: dragMode = DragMode::slide; break;
            default: dragMode = DragMode::none; break;
        }

        editLaneAt(position);
        return;
    }

    const auto stepIndex = stepForPosition(position);
    const auto row = rowForPosition(position);

    if (stepIndex < 0 || row < 0)
        return;

    const auto step = getStep(stepIndex);
    const auto noteOffset = noteOffsetForRow(row);
    dragMode = step.enabled && step.noteOffset == noteOffset ? DragMode::erase
                                                             : DragMode::paint;

    editAt(position);
}

void StepSequencerGrid::editAt(juce::Point<int> position)
{
    if (! getStep || ! setStep || dragMode == DragMode::none)
        return;

    if (dragMode == DragMode::velocity
        || dragMode == DragMode::probability
        || dragMode == DragMode::timing
        || dragMode == DragMode::length
        || dragMode == DragMode::lock
        || dragMode == DragMode::ratchet
        || dragMode == DragMode::condition
        || dragMode == DragMode::slide)
    {
        editLaneAt(position);
        return;
    }

    const auto stepIndex = stepForPosition(position);
    const auto row = rowForPosition(position);

    if (stepIndex < 0 || row < 0 || (stepIndex == lastEditedStep && row == lastEditedRow))
        return;

    lastEditedStep = stepIndex;
    lastEditedRow = row;

    auto step = getStep(stepIndex);
    const auto noteOffset = noteOffsetForRow(row);

    if (dragMode == DragMode::erase)
    {
        if (! step.enabled || step.noteOffset != noteOffset)
            return;

        step.enabled = false;
    }
    else
    {
        if (step.enabled && step.noteOffset == noteOffset)
            return;

        step.enabled = true;
        step.noteOffset = noteOffset;
        step.velocity = 0.85f;
        step.probability = 1.0f;
        step.timing = (stepIndex % 2) != 0 ? 0.65f : 0.0f;
        step.length = 1.0f;
        step.lock = 0.0f;
        step.ratchet = 1;
        step.condition = 0;
        step.slide = false;
    }

    setStep(stepIndex, step);
    repaint();
}

void StepSequencerGrid::editLaneAt(juce::Point<int> position)
{
    if (! getStep || ! setStep)
        return;

    const auto lanes = laneBounds();
    if (lanes.isEmpty())
        return;

    const auto stepIndex = laneStepForPosition(position);
    if (stepIndex < 0)
        return;

    auto step = getStep(stepIndex);
    if (! step.enabled)
    {
        step.enabled = true;
        step.noteOffset = 0;
        step.velocity = 0.82f;
        step.probability = 1.0f;
        step.timing = 0.0f;
        step.length = 1.0f;
        step.lock = 0.0f;
        step.ratchet = 1;
        step.condition = 0;
        step.slide = false;
    }

    const auto laneHeight = static_cast<float>(lanes.getHeight()) / static_cast<float>(laneCount);
    auto laneTop = static_cast<float>(lanes.getY());

    if (dragMode == DragMode::probability)
        laneTop += laneHeight;
    else if (dragMode == DragMode::timing)
        laneTop += laneHeight * 2.0f;
    else if (dragMode == DragMode::length)
        laneTop += laneHeight * 3.0f;
    else if (dragMode == DragMode::lock)
        laneTop += laneHeight * 4.0f;
    else if (dragMode == DragMode::ratchet)
        laneTop += laneHeight * 5.0f;
    else if (dragMode == DragMode::condition)
        laneTop += laneHeight * 6.0f;
    else if (dragMode == DragMode::slide)
        laneTop += laneHeight * 7.0f;

    const auto normalisedY = (static_cast<float>(position.y) - laneTop) / juce::jmax(1.0f, laneHeight);
    const auto value = juce::jlimit(0.0f, 1.0f, 1.0f - normalisedY);

    switch (dragMode)
    {
        case DragMode::velocity:
            step.velocity = juce::jlimit(0.1f, 1.0f, value);
            break;

        case DragMode::probability:
            step.probability = value;
            break;

        case DragMode::timing:
            step.timing = value;
            break;

        case DragMode::length:
            step.length = juce::jlimit(0.1f, 1.0f, value);
            break;

        case DragMode::lock:
            step.lock = value;
            break;

        case DragMode::ratchet:
            step.ratchet = juce::jlimit(Sequencer::PatternSequencer::minRatchet,
                                        Sequencer::PatternSequencer::maxRatchet,
                                        1 + static_cast<int>(std::round(value * static_cast<float>(Sequencer::PatternSequencer::maxRatchet - 1))));
            break;

        case DragMode::condition:
            step.condition = juce::jlimit(Sequencer::PatternSequencer::minCondition,
                                          Sequencer::PatternSequencer::maxCondition,
                                          static_cast<int>(std::round(value * static_cast<float>(Sequencer::PatternSequencer::maxCondition))));
            break;

        case DragMode::slide:
            step.slide = value >= 0.5f;
            if (step.slide)
                step.ratchet = 1;
            break;

        case DragMode::none:
        case DragMode::paint:
        case DragMode::erase:
            return;
    }

    setStep(stepIndex, step);
    repaint();
}

void StepSequencerGrid::cycleTimingAt(juce::Point<int> position)
{
    if (! getStep || ! setStep)
        return;

    const auto stepIndex = stepForPosition(position);
    if (stepIndex < 0)
        return;

    auto step = getStep(stepIndex);
    if (! step.enabled)
        return;

    if (step.timing < 0.25f)
        step.timing = 0.5f;
    else if (step.timing < 0.75f)
        step.timing = 1.0f;
    else
        step.timing = 0.0f;

    setStep(stepIndex, step);
    repaint();
}

void StepSequencerGrid::nudgeTimingAt(juce::Point<int> position, float delta)
{
    if (! getStep || ! setStep)
        return;

    const auto stepIndex = stepForPosition(position);
    if (stepIndex < 0)
        return;

    auto step = getStep(stepIndex);
    if (! step.enabled)
        return;

    step.timing = juce::jlimit(0.0f, 1.0f, step.timing + delta);
    setStep(stepIndex, step);
    repaint();
}

void StepSequencerGrid::nudgeLaneAt(juce::Point<int> position, float delta)
{
    if (! getStep || ! setStep)
        return;

    const auto lane = laneForPosition(position);
    const auto stepIndex = laneStepForPosition(position);
    if (lane < 0 || stepIndex < 0)
        return;

    auto step = getStep(stepIndex);
    if (! step.enabled)
        return;

    switch (lane)
    {
        case 0:
            step.velocity = juce::jlimit(0.1f, 1.0f, step.velocity + delta);
            break;

        case 1:
            step.probability = juce::jlimit(0.0f, 1.0f, step.probability + delta);
            break;

        case 2:
            step.timing = juce::jlimit(0.0f, 1.0f, step.timing + delta);
            break;

        case 3:
            step.length = juce::jlimit(0.1f, 1.0f, step.length + delta);
            break;

        case 4:
            step.lock = juce::jlimit(0.0f, 1.0f, step.lock + delta);
            break;

        case 5:
            step.ratchet = juce::jlimit(Sequencer::PatternSequencer::minRatchet,
                                        Sequencer::PatternSequencer::maxRatchet,
                                        step.ratchet + (delta >= 0.0f ? 1 : -1));
            break;

        case 6:
            step.condition = juce::jlimit(Sequencer::PatternSequencer::minCondition,
                                          Sequencer::PatternSequencer::maxCondition,
                                          step.condition + (delta >= 0.0f ? 1 : -1));
            break;

        case 7:
            step.slide = delta >= 0.0f ? true : false;
            if (step.slide)
                step.ratchet = 1;
            break;

        default:
            return;
    }

    setStep(stepIndex, step);
    repaint();
}
}
