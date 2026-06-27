#include "PatternSequencer.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace Sequencer
{
PatternSequencer::PatternSequencer(Parameters::APVTS& state)
    : parameters(state)
{
    sequencerEnabled = parameters.getRawParameterValue(Parameters::ID::sequencerEnabled);
    sequencerRate = parameters.getRawParameterValue(Parameters::ID::sequencerRate);
    sequencerRoot = parameters.getRawParameterValue(Parameters::ID::sequencerRoot);
    sequencerGate = parameters.getRawParameterValue(Parameters::ID::sequencerGate);
    sequencerSwing = parameters.getRawParameterValue(Parameters::ID::sequencerSwing);
    sequencerGrooveMode = parameters.getRawParameterValue(Parameters::ID::sequencerGrooveMode);
    sequencerScale = parameters.getRawParameterValue(Parameters::ID::sequencerScale);
    sequencerChordMode = parameters.getRawParameterValue(Parameters::ID::sequencerChordMode);
    sequencerChordVoicing = parameters.getRawParameterValue(Parameters::ID::sequencerChordVoicing);
    sequencerChordStrum = parameters.getRawParameterValue(Parameters::ID::sequencerChordStrum);
    sequencerAccent = parameters.getRawParameterValue(Parameters::ID::sequencerAccent);
    sequencerOctave = parameters.getRawParameterValue(Parameters::ID::sequencerOctave);
    sequencerProbability = parameters.getRawParameterValue(Parameters::ID::sequencerProbability);

    clear();
}

void PatternSequencer::prepare(double sampleRate)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    reset();
}

void PatternSequencer::reset()
{
    samplesUntilNextStep = 0.0;
    pendingNoteOffSamples = -1.0;
    currentStep = 0;
    activeNotes.fill(-1);
    activeNoteCount = 0;
}

bool PatternSequencer::isEnabled() const
{
    return readParameter(sequencerEnabled, 0.0f) > 0.5f;
}

Step PatternSequencer::getStep(int index) const
{
    const auto safeIndex = static_cast<size_t>(juce::jlimit(0, numSteps - 1, index));

    return {
        stepEnabled[safeIndex].load() > 0,
        stepNoteOffset[safeIndex].load(),
        stepVelocity[safeIndex].load(),
        stepProbability[safeIndex].load(),
        stepTiming[safeIndex].load()
    };
}

void PatternSequencer::setStep(int index, Step step)
{
    step.noteOffset = quantizeNoteOffset(juce::jlimit(-24, 24, step.noteOffset));
    step.velocity = juce::jlimit(0.0f, 1.0f, step.velocity);
    step.probability = juce::jlimit(0.0f, 1.0f, step.probability);
    step.timing = juce::jlimit(0.0f, 1.0f, step.timing);

    const auto safeIndex = static_cast<size_t>(juce::jlimit(0, numSteps - 1, index));
    stepEnabled[safeIndex].store(step.enabled ? 1 : 0);
    stepNoteOffset[safeIndex].store(step.noteOffset);
    stepVelocity[safeIndex].store(step.velocity);
    stepProbability[safeIndex].store(step.probability);
    stepTiming[safeIndex].store(step.timing);
}

int PatternSequencer::getQuantizedNoteOffset(int noteOffset) const
{
    return quantizeNoteOffset(juce::jlimit(-24, 24, noteOffset));
}

PatternSequencer::ChordNoteArray PatternSequencer::getChordNotes(int rootNote, int octaveOffset, int noteOffset, int& noteCount) const
{
    ChordNoteArray notes {};
    notes.fill(-1);
    noteCount = 0;
    std::array<int, maxChordNotes> intervals {};
    auto intervalCount = getChordIntervalCount(intervals);
    applyChordVoicing(intervals, intervalCount);
    const auto baseNoteNumber = rootNote + octaveOffset + quantizeNoteOffset(noteOffset);

    auto addInterval = [&notes, &noteCount, baseNoteNumber] (int interval)
    {
        if (noteCount >= maxChordNotes)
            return;

        const auto noteNumber = juce::jlimit(0, 127, baseNoteNumber + interval);
        for (auto noteIndex = 0; noteIndex < noteCount; ++noteIndex)
            if (notes[static_cast<size_t>(noteIndex)] == noteNumber)
                return;

        notes[static_cast<size_t>(noteCount++)] = noteNumber;
    };

    for (auto intervalIndex = 0; intervalIndex < intervalCount; ++intervalIndex)
        addInterval(intervals[static_cast<size_t>(intervalIndex)]);

    return notes;
}

int PatternSequencer::getChordIntervalCount(std::array<int, maxChordNotes>& intervals) const
{
    intervals.fill(0);

    switch (static_cast<int>(std::round(readParameter(sequencerChordMode, 0.0f))))
    {
        case 1:
            intervals = { 0, 7, 0, 0, 0 };
            return 2;

        case 2:
            intervals = { 0, 3, 7, 0, 0 };
            return 3;

        case 3:
            intervals = { 0, 3, 7, 10, 0 };
            return 4;

        case 4:
            intervals = { 0, 4, 7, 0, 0 };
            return 3;

        case 5:
            intervals = { 0, 3, 7, 10, 14 };
            return 5;

        case 0:
        default:
            intervals = { 0, 0, 0, 0, 0 };
            return 1;
    }
}

void PatternSequencer::applyChordVoicing(std::array<int, maxChordNotes>& intervals, int& intervalCount) const
{
    if (intervalCount <= 1)
        return;

    switch (static_cast<int>(std::round(readParameter(sequencerChordVoicing, 0.0f))))
    {
        case 1:
        {
            const auto first = intervals[0] + 12;
            for (auto index = 0; index < intervalCount - 1; ++index)
                intervals[static_cast<size_t>(index)] = intervals[static_cast<size_t>(index + 1)];
            intervals[static_cast<size_t>(intervalCount - 1)] = first;
            break;
        }

        case 2:
        {
            const auto moveCount = juce::jmin(2, intervalCount - 1);
            std::array<int, maxChordNotes> voiced {};
            auto writeIndex = 0;

            for (auto index = moveCount; index < intervalCount; ++index)
                voiced[static_cast<size_t>(writeIndex++)] = intervals[static_cast<size_t>(index)];

            for (auto index = 0; index < moveCount; ++index)
                voiced[static_cast<size_t>(writeIndex++)] = intervals[static_cast<size_t>(index)] + 12;

            intervals = voiced;
            break;
        }

        case 3:
        {
            for (auto index = 1; index < intervalCount; index += 2)
                intervals[static_cast<size_t>(index)] += 12;

            std::sort(intervals.begin(), intervals.begin() + intervalCount);
            break;
        }

        case 4:
        {
            if (intervalCount >= 3)
            {
                intervals[static_cast<size_t>(intervalCount - 2)] -= 12;
                std::sort(intervals.begin(), intervals.begin() + intervalCount);
            }
            break;
        }

        case 0:
        default:
            break;
    }
}

float PatternSequencer::getChordNoteVelocity(float velocity, int noteIndex) const
{
    if (noteIndex <= 0)
        return juce::jlimit(0.0f, 1.0f, velocity);

    const auto trim = 0.92f - (0.04f * static_cast<float>(noteIndex - 1));
    return juce::jlimit(0.0f, 1.0f, velocity * juce::jmax(0.72f, trim));
}

int PatternSequencer::getChordStrumOffset(int stepLength, int noteIndex, int noteCount) const
{
    if (noteIndex <= 0 || noteCount <= 1 || stepLength <= 1)
        return 0;

    const auto strumAmount = juce::jlimit(0.0f, 1.0f, readParameter(sequencerChordStrum, 0.0f));
    if (strumAmount <= 0.0f)
        return 0;

    const auto maxSpread = static_cast<float>(stepLength) * 0.18f * strumAmount;
    const auto notePosition = static_cast<float>(noteIndex) / static_cast<float>(juce::jmax(1, noteCount - 1));
    return juce::roundToInt(maxSpread * notePosition);
}

void PatternSequencer::clear()
{
    for (auto step = 0; step < numSteps; ++step)
    {
        stepEnabled[static_cast<size_t>(step)].store(0);
        stepNoteOffset[static_cast<size_t>(step)].store(0);
        stepVelocity[static_cast<size_t>(step)].store(0.8f);
        stepProbability[static_cast<size_t>(step)].store(1.0f);
        stepTiming[static_cast<size_t>(step)].store(0.0f);
    }
}

void PatternSequencer::randomize(float amount)
{
    const auto density = juce::jlimit(0.15f, 0.95f, 0.25f + (amount * 0.65f));
    const auto range = juce::jlimit(3, 12, static_cast<int>(3.0f + (amount * 10.0f)));

    for (auto step = 0; step < numSteps; ++step)
    {
        Step newStep;
        newStep.enabled = nextRandomFloat() < density;
        newStep.noteOffset = juce::jlimit(minNoteOffset, maxNoteOffset, static_cast<int>(std::round((nextRandomFloat() * 2.0f - 1.0f) * static_cast<float>(range))));
        newStep.velocity = juce::jlimit(0.35f, 1.0f, 0.45f + (nextRandomFloat() * 0.55f));
        newStep.probability = juce::jlimit(0.45f, 1.0f, 0.55f + (nextRandomFloat() * 0.45f));
        newStep.timing = (step % 4 == 0) ? 0.0f : juce::jlimit(0.0f, 1.0f, nextRandomFloat() * amount);

        if (step == 0 || step == 4 || step == 8 || step == 12)
        {
            newStep.enabled = true;
            newStep.noteOffset = step == 0 ? 0 : newStep.noteOffset;
            newStep.probability = 1.0f;
            newStep.timing = 0.0f;
        }

        setStep(step, newStep);
    }
}

void PatternSequencer::process(juce::MidiBuffer& midi, int numSamples, double bpm)
{
    if (! isEnabled() || numSamples <= 0)
        return;

    if (activeNoteCount > 0 && pendingNoteOffSamples >= 0.0)
    {
        if (pendingNoteOffSamples < static_cast<double>(numSamples))
        {
            addNoteOffsForActiveNotes(midi, static_cast<int>(pendingNoteOffSamples));
            pendingNoteOffSamples = -1.0;
        }
        else
        {
            pendingNoteOffSamples -= static_cast<double>(numSamples);
        }
    }

    const auto baseStepLengthSamples = getStepLengthSamples(bpm);

    auto nextStepOffset = samplesUntilNextStep;

    while (nextStepOffset < static_cast<double>(numSamples))
    {
        const auto eventOffset = juce::jlimit(0, numSamples - 1, static_cast<int>(std::round(nextStepOffset)));
        const auto currentStepDuration = getStepDurationSamples(baseStepLengthSamples, currentStep);
        const auto gateSamples = juce::jlimit(1, juce::jmax(1, currentStepDuration - 1),
                                             static_cast<int>(static_cast<float>(currentStepDuration) * readParameter(sequencerGate, 0.55f)));

        if (activeNoteCount > 0)
        {
            addNoteOffsForActiveNotes(midi, eventOffset);
            pendingNoteOffSamples = -1.0;
        }

        const auto step = getStep(currentStep);
        if (shouldTriggerStep(step))
        {
            const auto root = static_cast<int>(std::round(readParameter(sequencerRoot, 36.0f)));
            const auto octaveOffset = static_cast<int>(std::round(readParameter(sequencerOctave, 0.0f))) * 12;
            const auto accent = readParameter(sequencerAccent, 0.35f);
            const auto isAnchorStep = currentStep == 0 || currentStep == 4 || currentStep == 8 || currentStep == 12;
            const auto velocity = isAnchorStep
                ? juce::jlimit(0.0f, 1.0f, step.velocity + ((1.0f - step.velocity) * accent))
                : juce::jlimit(0.0f, 1.0f, step.velocity * (1.0f - (accent * 0.12f)));
            auto noteCount = 0;
            const auto notes = getChordNotes(root, octaveOffset, step.noteOffset, noteCount);
            activeNoteCount = noteCount;
            activeNotes = notes;
            const auto noteOffOffset = eventOffset + gateSamples;

            for (auto noteIndex = 0; noteIndex < noteCount; ++noteIndex)
            {
                const auto noteVelocity = getChordNoteVelocity(velocity, noteIndex);
                const auto maxNoteOnOffset = noteOffOffset < numSamples
                    ? juce::jmax(eventOffset, noteOffOffset - 1)
                    : numSamples - 1;
                const auto noteOnOffset = juce::jlimit(eventOffset,
                                                       maxNoteOnOffset,
                                                       eventOffset + getChordStrumOffset(currentStepDuration, noteIndex, noteCount));
                midi.addEvent(juce::MidiMessage::noteOn(1, notes[static_cast<size_t>(noteIndex)], noteVelocity), noteOnOffset);
            }

            if (noteOffOffset < numSamples)
            {
                addNoteOffsForActiveNotes(midi, noteOffOffset);
                pendingNoteOffSamples = -1.0;
            }
            else
            {
                pendingNoteOffSamples = static_cast<double>(noteOffOffset - numSamples);
            }
        }

        currentStep = (currentStep + 1) % numSteps;
        nextStepOffset += static_cast<double>(currentStepDuration);
    }

    samplesUntilNextStep = nextStepOffset - static_cast<double>(numSamples);
}

int PatternSequencer::getStepLengthSamples(double bpm) const
{
    const auto safeBpm = juce::jlimit(20.0, 300.0, bpm);
    const auto quarterNoteSamples = (60.0 / safeBpm) * currentSampleRate;
    const auto rateIndex = static_cast<int>(std::round(readParameter(sequencerRate, 1.0f)));

    switch (rateIndex)
    {
        case 0: return juce::jmax(1, static_cast<int>(quarterNoteSamples / 2.0));
        case 2: return juce::jmax(1, static_cast<int>(quarterNoteSamples / 8.0));
        case 1:
        default:
            return juce::jmax(1, static_cast<int>(quarterNoteSamples / 4.0));
    }
}

int PatternSequencer::getStepDurationSamples(int baseStepLengthSamples, int stepIndex) const
{
    const auto currentDelay = getStepDelaySamples(baseStepLengthSamples, stepIndex);
    const auto nextDelay = getStepDelaySamples(baseStepLengthSamples, (stepIndex + 1) % numSteps);
    const auto duration = baseStepLengthSamples + nextDelay - currentDelay;
    return juce::jmax(1, duration);
}

int PatternSequencer::getStepDelaySamples(int baseStepLengthSamples, int stepIndex) const
{
    const auto safeIndex = static_cast<size_t>(juce::jlimit(0, numSteps - 1, stepIndex));
    const auto swing = juce::jlimit(0.0f, 0.65f, readParameter(sequencerSwing, 0.0f));
    const auto maxDelay = static_cast<float>(baseStepLengthSamples) * swing * 0.5f;
    const auto timing = juce::jlimit(0.0f, 1.0f, stepTiming[safeIndex].load());
    const auto mode = static_cast<int>(std::round(readParameter(sequencerGrooveMode, 0.0f)));
    const auto isAnchorStep = stepIndex == 0 || stepIndex == 4 || stepIndex == 8 || stepIndex == 12;
    const auto isOffbeatStep = (stepIndex % 2) != 0;

    auto weight = 0.0f;
    switch (mode)
    {
        case 1:
            weight = timing;
            break;

        case 2:
            if (! isAnchorStep)
                weight = timing > 0.0f ? timing : (isOffbeatStep ? 0.62f : 0.18f);
            break;

        case 3:
            weight = isAnchorStep ? 0.0f : timing * 0.65f;
            break;

        case 0:
        default:
            weight = isOffbeatStep ? 1.0f : 0.0f;
            break;
    }

    return static_cast<int>(std::round(maxDelay * juce::jlimit(0.0f, 1.0f, weight)));
}

int PatternSequencer::quantizeNoteOffset(int noteOffset) const
{
    const auto scaleMode = static_cast<int>(std::round(readParameter(sequencerScale, 0.0f)));
    if (scaleMode <= 0)
        return noteOffset;

    auto bestOffset = noteOffset;
    auto bestDistance = 128;

    for (auto candidate = -24; candidate <= 24; ++candidate)
    {
        if (! isOffsetInScale(candidate, scaleMode))
            continue;

        const auto distance = std::abs(candidate - noteOffset);
        if (distance < bestDistance || (distance == bestDistance && std::abs(candidate) < std::abs(bestOffset)))
        {
            bestOffset = candidate;
            bestDistance = distance;
        }
    }

    return bestOffset;
}

bool PatternSequencer::isOffsetInScale(int noteOffset, int scaleMode) const
{
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

void PatternSequencer::addNoteOffsForActiveNotes(juce::MidiBuffer& midi, int samplePosition)
{
    for (auto noteIndex = 0; noteIndex < activeNoteCount; ++noteIndex)
    {
        const auto noteNumber = activeNotes[static_cast<size_t>(noteIndex)];
        if (noteNumber >= 0)
            midi.addEvent(juce::MidiMessage::noteOff(1, noteNumber), samplePosition);
    }

    activeNotes.fill(-1);
    activeNoteCount = 0;
}

float PatternSequencer::nextRandomFloat()
{
    randomState = (1664525u * randomState) + 1013904223u;
    return static_cast<float>(randomState & 0x00ffffffu) / static_cast<float>(0x01000000u);
}

bool PatternSequencer::shouldTriggerStep(const Step& step)
{
    const auto globalProbability = readParameter(sequencerProbability, 1.0f);
    return step.enabled && nextRandomFloat() <= juce::jlimit(0.0f, 1.0f, step.probability * globalProbability);
}

float PatternSequencer::readParameter(std::atomic<float>* parameter, float fallback) const
{
    return parameter != nullptr ? parameter->load() : fallback;
}
}
