#include "StepSequencerGrid.h"

#include <cmath>

namespace UI
{
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
        g.setColour(noteOffset == 0 ? juce::Colour(0xff223038) : (row % 2 == 0 ? juce::Colour(0xff182126) : juce::Colour(0xff151e22)));
        g.fillRect(juce::Rectangle<float>(static_cast<float>(bounds.getX()), y, static_cast<float>(bounds.getWidth()), cellHeight - 1.0f));

        auto labelArea = juce::Rectangle<float>(static_cast<float>(noteLabels.getX()),
                                                y,
                                                static_cast<float>(noteLabels.getWidth()),
                                                cellHeight - 1.0f).reduced(4.0f, 0.0f);
        const auto noteName = juce::MidiMessage::getMidiNoteName(juce::jlimit(0, 127, rootNote + noteOffset), true, true, 3);
        g.setColour(noteOffset == 0 ? juce::Colour(0xff8ee6c9) : juce::Colour(0xffa8b6b8));
        g.setFont(juce::FontOptions(noteOffset == 0 ? 9.5f : 9.0f, noteOffset == 0 ? juce::Font::bold : juce::Font::plain));
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
                cellHeight).reduced(3.0f);

            const auto alpha = juce::jlimit(0.35f, 1.0f, step.velocity);
            g.setColour(juce::Colour(0xff8ee6c9).withAlpha(alpha));
            g.fillRoundedRectangle(cell, 3.0f);
            if (step.timing > 0.0f)
            {
                const auto timingWidth = juce::jmax(3.0f, cell.getWidth() * juce::jlimit(0.0f, 1.0f, step.timing));
                auto timingMarker = cell.withY(cell.getBottom() - 3.0f).withHeight(3.0f).withWidth(timingWidth);
                g.setColour(juce::Colour(0xffffc857).withAlpha(0.9f));
                g.fillRoundedRectangle(timingMarker, 1.5f);
            }
            if (step.probability < 0.995f)
            {
                const auto probabilityWidth = juce::jmax(3.0f, cell.getWidth() * juce::jlimit(0.0f, 1.0f, step.probability));
                auto probabilityMarker = cell.withHeight(3.0f).withWidth(probabilityWidth);
                g.setColour(juce::Colour(0xffb7a4ff).withAlpha(0.9f));
                g.fillRoundedRectangle(probabilityMarker, 1.5f);
            }
            if ((stepIndex % 4) == 0)
            {
                auto anchorDot = cell;
                g.setColour(juce::Colour(0xffedf7f4).withAlpha(0.58f));
                g.fillEllipse(anchorDot.removeFromTop(8.0f).removeFromRight(8.0f).reduced(2.0f));
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

void StepSequencerGrid::mouseDown(const juce::MouseEvent& event)
{
    lastEditedStep = -1;
    lastEditedRow = -1;

    if (event.mods.isPopupMenu() || event.mods.isCommandDown())
    {
        cycleTimingAt(event.getPosition());
        return;
    }

    editAt(event.getPosition());
}

void StepSequencerGrid::mouseDrag(const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu() || event.mods.isCommandDown())
        return;

    editAt(event.getPosition());
}

void StepSequencerGrid::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    nudgeTimingAt(event.getPosition(), wheel.deltaY >= 0.0f ? 0.1f : -0.1f);
}

juce::Rectangle<int> StepSequencerGrid::gridBounds() const
{
    auto bounds = getLocalBounds().reduced(2);
    bounds.removeFromTop(18);
    bounds.removeFromLeft(44);
    return bounds;
}

juce::Rectangle<int> StepSequencerGrid::noteLabelBounds() const
{
    auto bounds = getLocalBounds().reduced(2);
    bounds.removeFromTop(18);
    return bounds.removeFromLeft(40);
}

juce::Rectangle<int> StepSequencerGrid::stepHeaderBounds() const
{
    auto bounds = getLocalBounds().reduced(2);
    bounds.removeFromLeft(44);
    return bounds.removeFromTop(16);
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
    return 6 - juce::jlimit(0, numRows - 1, row);
}

int StepSequencerGrid::rowForNoteOffset(int noteOffset) const
{
    return juce::jlimit(0, numRows - 1, 6 - juce::jlimit(-6, 6, noteOffset));
}

void StepSequencerGrid::editAt(juce::Point<int> position)
{
    if (! getStep || ! setStep)
        return;

    const auto stepIndex = stepForPosition(position);
    const auto row = rowForPosition(position);

    if (stepIndex < 0 || row < 0 || (stepIndex == lastEditedStep && row == lastEditedRow))
        return;

    lastEditedStep = stepIndex;
    lastEditedRow = row;

    auto step = getStep(stepIndex);
    const auto noteOffset = noteOffsetForRow(row);

    if (step.enabled && step.noteOffset == noteOffset)
    {
        step.enabled = false;
    }
    else
    {
        step.enabled = true;
        step.noteOffset = noteOffset;
        step.velocity = 0.85f;
        step.probability = 1.0f;
        step.timing = (stepIndex % 2) != 0 ? 0.65f : 0.0f;
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
}
