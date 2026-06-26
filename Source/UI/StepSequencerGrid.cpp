#include "StepSequencerGrid.h"

#include <cmath>

namespace UI
{
void StepSequencerGrid::setCallbacks(StepGetter getter, StepSetter setter)
{
    getStep = std::move(getter);
    setStep = std::move(setter);
}

void StepSequencerGrid::paint(juce::Graphics& g)
{
    const auto bounds = gridBounds();
    const auto cellWidth = static_cast<float>(bounds.getWidth()) / static_cast<float>(Sequencer::PatternSequencer::numSteps);
    const auto cellHeight = static_cast<float>(bounds.getHeight()) / static_cast<float>(numRows);

    g.setColour(juce::Colour(0xff101619));
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

    for (auto row = 0; row < numRows; ++row)
    {
        const auto y = bounds.getY() + (static_cast<float>(row) * cellHeight);
        const auto noteOffset = noteOffsetForRow(row);
        g.setColour(noteOffset == 0 ? juce::Colour(0xff223038) : juce::Colour(0xff182126));
        g.fillRect(juce::Rectangle<float>(static_cast<float>(bounds.getX()), y, static_cast<float>(bounds.getWidth()), cellHeight - 1.0f));
    }

    g.setColour(juce::Colour(0xff293339));
    for (auto step = 0; step <= Sequencer::PatternSequencer::numSteps; ++step)
    {
        const auto x = bounds.getX() + (static_cast<float>(step) * cellWidth);
        g.drawVerticalLine(static_cast<int>(std::round(x)), static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));
    }

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
            g.setColour(juce::Colour(0xff0d1113));
            g.drawRoundedRectangle(cell, 3.0f, 1.0f);
        }
    }

    g.setColour(juce::Colour(0xff344047));
    g.drawRoundedRectangle(bounds.toFloat(), 6.0f, 1.0f);
}

void StepSequencerGrid::mouseDown(const juce::MouseEvent& event)
{
    lastEditedStep = -1;
    lastEditedRow = -1;
    editAt(event.getPosition());
}

void StepSequencerGrid::mouseDrag(const juce::MouseEvent& event)
{
    editAt(event.getPosition());
}

juce::Rectangle<int> StepSequencerGrid::gridBounds() const
{
    return getLocalBounds().reduced(2);
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
    }

    setStep(stepIndex, step);
    repaint();
}
}
