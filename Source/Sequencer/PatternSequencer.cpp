#include "PatternSequencer.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace Sequencer
{
namespace
{
constexpr int conditionPhraseCycleCount = 4;

int wrapConditionCycle(int cycle) noexcept
{
    return ((cycle % conditionPhraseCycleCount) + conditionPhraseCycleCount) % conditionPhraseCycleCount;
}

int conditionCycleForAbsoluteStep(int absoluteStep) noexcept
{
    const auto cycle = static_cast<int>(std::floor(static_cast<double>(absoluteStep)
                                                   / static_cast<double>(PatternSequencer::numSteps)));
    return wrapConditionCycle(cycle);
}
}

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
    currentCycle = 0;
    activeStepLock.store(0.0f, std::memory_order_relaxed);
    activeStepIndex.store(-1, std::memory_order_relaxed);
    activeSceneChainIndex.store(isSceneChainEnabled() ? 0 : -1, std::memory_order_relaxed);
    activeNotes.fill(-1);
    activeNoteCount = 0;
    activeNotesSlideToNext = false;
    clearPendingChordEvents();
    lastHostPpqPosition = std::nullopt;
    wasHostPlaying = false;
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
        stepTiming[safeIndex].load(),
        stepLengths[safeIndex].load(),
        stepLocks[safeIndex].load(),
        stepRatchets[safeIndex].load(),
        stepConditions[safeIndex].load(),
        stepSlides[safeIndex].load() > 0
    };
}

void PatternSequencer::setStep(int index, Step step)
{
    step = normaliseStep(step);

    const auto safeIndex = static_cast<size_t>(juce::jlimit(0, numSteps - 1, index));
    stepEnabled[safeIndex].store(step.enabled ? 1 : 0);
    stepNoteOffset[safeIndex].store(step.noteOffset);
    stepVelocity[safeIndex].store(step.velocity);
    stepProbability[safeIndex].store(step.probability);
    stepTiming[safeIndex].store(step.timing);
    stepLengths[safeIndex].store(step.length);
    stepLocks[safeIndex].store(step.lock);
    stepRatchets[safeIndex].store(step.ratchet);
    stepConditions[safeIndex].store(step.condition);
    stepSlides[safeIndex].store(step.slide ? 1 : 0);
}

Step PatternSequencer::normaliseStep(Step step) const
{
    step.noteOffset = quantizeNoteOffset(juce::jlimit(minNoteOffset, maxNoteOffset, step.noteOffset));
    step.velocity = juce::jlimit(0.0f, 1.0f, step.velocity);
    step.probability = juce::jlimit(0.0f, 1.0f, step.probability);
    step.timing = juce::jlimit(0.0f, 1.0f, step.timing);
    step.length = juce::jlimit(0.1f, 1.0f, step.length);
    step.lock = juce::jlimit(0.0f, 1.0f, step.lock);
    step.ratchet = juce::jlimit(minRatchet, maxRatchet, step.ratchet);
    step.condition = juce::jlimit(minCondition, maxCondition, step.condition);
    if (! step.enabled || step.ratchet > 1)
        step.slide = false;
    return step;
}

Step PatternSequencer::readStoredStep(const StepStorage& storage, int index) const
{
    const auto safeIndex = static_cast<size_t>(juce::jlimit(0, numSteps - 1, index));
    return {
        storage.enabled[safeIndex].load() > 0,
        storage.noteOffset[safeIndex].load(),
        storage.velocity[safeIndex].load(),
        storage.probability[safeIndex].load(),
        storage.timing[safeIndex].load(),
        storage.length[safeIndex].load(),
        storage.lock[safeIndex].load(),
        storage.ratchet[safeIndex].load(),
        storage.condition[safeIndex].load(),
        storage.slide[safeIndex].load() > 0
    };
}

void PatternSequencer::writeStoredStep(StepStorage& storage, int index, Step step)
{
    step = normaliseStep(step);
    const auto safeIndex = static_cast<size_t>(juce::jlimit(0, numSteps - 1, index));
    storage.enabled[safeIndex].store(step.enabled ? 1 : 0);
    storage.noteOffset[safeIndex].store(step.noteOffset);
    storage.velocity[safeIndex].store(step.velocity);
    storage.probability[safeIndex].store(step.probability);
    storage.timing[safeIndex].store(step.timing);
    storage.length[safeIndex].store(step.length);
    storage.lock[safeIndex].store(step.lock);
    storage.ratchet[safeIndex].store(step.ratchet);
    storage.condition[safeIndex].store(step.condition);
    storage.slide[safeIndex].store(step.slide ? 1 : 0);
}

PatternSequencer::SceneControls PatternSequencer::readCurrentSceneControls() const
{
    SceneControls controls;
    controls.root = readParameter(sequencerRoot, 36.0f);
    controls.gate = readParameter(sequencerGate, 0.55f);
    controls.swing = readParameter(sequencerSwing, 0.0f);
    controls.grooveMode = readParameter(sequencerGrooveMode, 0.0f);
    controls.scale = readParameter(sequencerScale, 0.0f);
    controls.chordMode = readParameter(sequencerChordMode, 0.0f);
    controls.chordVoicing = readParameter(sequencerChordVoicing, 0.0f);
    controls.chordStrum = readParameter(sequencerChordStrum, 0.0f);
    controls.accent = readParameter(sequencerAccent, 0.35f);
    controls.octave = readParameter(sequencerOctave, 0.0f);
    controls.probability = readParameter(sequencerProbability, 1.0f);
    return controls;
}

PatternSequencer::SceneControls PatternSequencer::readStoredSceneControls(const SceneControlStorage& storage) const
{
    SceneControls controls;
    controls.root = storage.root.load(std::memory_order_acquire);
    controls.gate = storage.gate.load(std::memory_order_acquire);
    controls.swing = storage.swing.load(std::memory_order_acquire);
    controls.grooveMode = storage.grooveMode.load(std::memory_order_acquire);
    controls.scale = storage.scale.load(std::memory_order_acquire);
    controls.chordMode = storage.chordMode.load(std::memory_order_acquire);
    controls.chordVoicing = storage.chordVoicing.load(std::memory_order_acquire);
    controls.chordStrum = storage.chordStrum.load(std::memory_order_acquire);
    controls.accent = storage.accent.load(std::memory_order_acquire);
    controls.octave = storage.octave.load(std::memory_order_acquire);
    controls.probability = storage.probability.load(std::memory_order_acquire);
    return controls;
}

void PatternSequencer::writeStoredSceneControls(SceneControlStorage& storage, SceneControls controls)
{
    storage.root.store(juce::jlimit(0.0f, 127.0f, controls.root), std::memory_order_release);
    storage.gate.store(juce::jlimit(0.05f, 0.95f, controls.gate), std::memory_order_release);
    storage.swing.store(juce::jlimit(0.0f, 0.65f, controls.swing), std::memory_order_release);
    storage.grooveMode.store(juce::jlimit(0.0f, 7.0f, controls.grooveMode), std::memory_order_release);
    storage.scale.store(juce::jlimit(0.0f, 4.0f, controls.scale), std::memory_order_release);
    storage.chordMode.store(juce::jlimit(0.0f, 10.0f, controls.chordMode), std::memory_order_release);
    storage.chordVoicing.store(juce::jlimit(0.0f, 4.0f, controls.chordVoicing), std::memory_order_release);
    storage.chordStrum.store(juce::jlimit(0.0f, 1.0f, controls.chordStrum), std::memory_order_release);
    storage.accent.store(juce::jlimit(0.0f, 1.0f, controls.accent), std::memory_order_release);
    storage.octave.store(juce::jlimit(-2.0f, 2.0f, controls.octave), std::memory_order_release);
    storage.probability.store(juce::jlimit(0.0f, 1.0f, controls.probability), std::memory_order_release);
}

int PatternSequencer::getPlaybackSceneIndex() const
{
    const auto chainLength = sceneChainLength.load(std::memory_order_acquire);
    if (chainLength <= 0)
        return -1;

    return juce::jlimit(0,
                        maxSceneChainLength - 1,
                        wrapConditionCycle(currentCycle) % chainLength);
}

Step PatternSequencer::readPlaybackStep(int index, int sceneIndex) const
{
    if (sceneIndex >= 0)
        return readStoredStep(sceneChainSteps[static_cast<size_t>(juce::jlimit(0, maxSceneChainLength - 1, sceneIndex))], index);

    return getStep(index);
}

Step PatternSequencer::getPlaybackStep(int index) const
{
    const auto sceneIndex = getPlaybackSceneIndex();
    activeSceneChainIndex.store(sceneIndex, std::memory_order_relaxed);
    return readPlaybackStep(index, sceneIndex);
}

int PatternSequencer::getQuantizedNoteOffset(int noteOffset) const
{
    return quantizeNoteOffset(juce::jlimit(minNoteOffset, maxNoteOffset, noteOffset));
}

PatternSequencer::ChordNoteArray PatternSequencer::getChordNotes(int rootNote, int octaveOffset, int noteOffset, int& noteCount) const
{
    return getChordNotes(rootNote, octaveOffset, noteOffset, noteCount, readCurrentSceneControls());
}

PatternSequencer::ChordNoteArray PatternSequencer::getChordNotes(int rootNote,
                                                                  int octaveOffset,
                                                                  int noteOffset,
                                                                  int& noteCount,
                                                                  const SceneControls& controls) const
{
    ChordNoteArray notes {};
    notes.fill(-1);
    noteCount = 0;
    std::array<int, maxChordNotes> intervals {};
    auto intervalCount = getChordIntervalCount(intervals, controls);
    applyChordVoicing(intervals, intervalCount, controls);
    const auto baseNoteNumber = rootNote + octaveOffset + quantizeNoteOffset(noteOffset, controls);

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

int PatternSequencer::getChordIntervalCount(std::array<int, maxChordNotes>& intervals, const SceneControls& controls) const
{
    intervals.fill(0);

    switch (static_cast<int>(std::round(juce::jlimit(0.0f, 10.0f, controls.chordMode))))
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

        case 6:
            intervals = { 0, 4, 7, 11, 0 };
            return 4;

        case 7:
            intervals = { 0, 4, 7, 10, 0 };
            return 4;

        case 8:
            intervals = { 0, 5, 7, 0, 0 };
            return 3;

        case 9:
            intervals = { 0, 4, 7, 10, 14 };
            return 5;

        case 10:
            intervals = { 0, 7, 10, 14, 17 };
            return 5;

        case 0:
        default:
            intervals = { 0, 0, 0, 0, 0 };
            return 1;
    }
}

void PatternSequencer::applyChordVoicing(std::array<int, maxChordNotes>& intervals, int& intervalCount, const SceneControls& controls) const
{
    if (intervalCount <= 1)
        return;

    switch (static_cast<int>(std::round(juce::jlimit(0.0f, 4.0f, controls.chordVoicing))))
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
    return getChordStrumOffset(stepLength, noteIndex, noteCount, readCurrentSceneControls());
}

int PatternSequencer::getChordStrumOffset(int stepLength, int noteIndex, int noteCount, const SceneControls& controls) const
{
    if (noteIndex <= 0 || noteCount <= 1 || stepLength <= 1)
        return 0;

    const auto strumAmount = juce::jlimit(0.0f, 1.0f, controls.chordStrum);
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
        stepLengths[static_cast<size_t>(step)].store(1.0f);
        stepLocks[static_cast<size_t>(step)].store(0.0f);
        stepRatchets[static_cast<size_t>(step)].store(1);
        stepConditions[static_cast<size_t>(step)].store(0);
        stepSlides[static_cast<size_t>(step)].store(0);
    }

    currentCycle = 0;
    activeStepLock.store(0.0f, std::memory_order_relaxed);
    activeStepIndex.store(-1, std::memory_order_relaxed);
    activeNotes.fill(-1);
    activeNoteCount = 0;
    activeNotesSlideToNext = false;
    clearPendingChordEvents();
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
        newStep.length = juce::jlimit(0.18f, 1.0f, 0.36f + (nextRandomFloat() * 0.64f));
        newStep.lock = newStep.enabled ? juce::jlimit(0.0f, 1.0f, nextRandomFloat() * amount) : 0.0f;
        newStep.ratchet = (newStep.enabled && nextRandomFloat() < amount * 0.18f)
            ? juce::jlimit(minRatchet, maxRatchet, 2 + static_cast<int>(nextRandomFloat() * 3.0f))
            : 1;
        newStep.slide = newStep.enabled
            && newStep.ratchet <= 1
            && step > 0
            && nextRandomFloat() < amount * 0.14f;

        if (step == 0 || step == 4 || step == 8 || step == 12)
        {
            newStep.enabled = true;
            newStep.noteOffset = step == 0 ? 0 : newStep.noteOffset;
            newStep.probability = 1.0f;
            newStep.timing = 0.0f;
            newStep.length = juce::jmax(newStep.length, 0.72f);
            newStep.lock = 0.0f;
            newStep.ratchet = 1;
            newStep.slide = false;
        }

        setStep(step, newStep);
    }
}

void PatternSequencer::setSceneChain(const SceneChain& scenes, int sceneCount)
{
    SceneControlChain controls {};
    const auto currentControls = readCurrentSceneControls();
    for (auto& control : controls)
        control = currentControls;

    setSceneChain(scenes, controls, sceneCount);
}

void PatternSequencer::setSceneChain(const SceneChain& scenes, const SceneControlChain& controls, int sceneCount)
{
    const auto safeSceneCount = juce::jlimit(0, maxSceneChainLength, sceneCount);
    for (auto sceneIndex = 0; sceneIndex < safeSceneCount; ++sceneIndex)
    {
        writeStoredSceneControls(sceneChainControls[static_cast<size_t>(sceneIndex)],
                                 controls[static_cast<size_t>(sceneIndex)]);

        for (auto stepIndex = 0; stepIndex < numSteps; ++stepIndex)
            writeStoredStep(sceneChainSteps[static_cast<size_t>(sceneIndex)],
                            stepIndex,
                            scenes[static_cast<size_t>(sceneIndex)][static_cast<size_t>(stepIndex)]);
    }

    sceneChainLength.store(safeSceneCount, std::memory_order_release);
    activeSceneChainIndex.store(safeSceneCount > 0 ? wrapConditionCycle(currentCycle) % safeSceneCount : -1,
                                std::memory_order_relaxed);
}

void PatternSequencer::clearSceneChain()
{
    sceneChainLength.store(0, std::memory_order_release);
    activeSceneChainIndex.store(-1, std::memory_order_relaxed);
}

bool PatternSequencer::isSceneChainEnabled() const noexcept
{
    return sceneChainLength.load(std::memory_order_acquire) > 0;
}

int PatternSequencer::getActiveSceneChainIndex() const noexcept
{
    return activeSceneChainIndex.load(std::memory_order_relaxed);
}

float PatternSequencer::getActiveStepLock() const noexcept
{
    return activeStepLock.load(std::memory_order_relaxed);
}

int PatternSequencer::getActiveStepIndex() const noexcept
{
    return activeStepIndex.load(std::memory_order_relaxed);
}

void PatternSequencer::process(juce::MidiBuffer& midi, int numSamples, double bpm, HostPosition hostPosition)
{
    if (numSamples <= 0)
    {
        lastHostPpqPosition = hostPosition.ppqPosition;
        wasHostPlaying = hostPosition.isPlaying;
        return;
    }

    if (! isEnabled())
    {
        if (activeNoteCount > 0)
            addNoteOffsForActiveNotes(midi, 0);

        pendingNoteOffSamples = -1.0;
        samplesUntilNextStep = 0.0;
        currentCycle = 0;
        clearPendingChordEvents();
        activeStepLock.store(0.0f, std::memory_order_relaxed);
        activeStepIndex.store(-1, std::memory_order_relaxed);
        activeSceneChainIndex.store(-1, std::memory_order_relaxed);
        activeNotesSlideToNext = false;
        lastHostPpqPosition = std::nullopt;
        wasHostPlaying = false;
        return;
    }

    if (hostPosition.isAvailable && ! hostPosition.isPlaying)
    {
        if (activeNoteCount > 0)
            addNoteOffsForActiveNotes(midi, 0);

        pendingNoteOffSamples = -1.0;
        samplesUntilNextStep = 0.0;
        currentCycle = 0;
        clearPendingChordEvents();
        activeStepLock.store(0.0f, std::memory_order_relaxed);
        activeStepIndex.store(-1, std::memory_order_relaxed);
        activeSceneChainIndex.store(-1, std::memory_order_relaxed);
        activeNotesSlideToNext = false;
        lastHostPpqPosition = hostPosition.ppqPosition;
        wasHostPlaying = false;
        return;
    }

    const auto baseStepLengthSamples = getStepLengthSamples(bpm);
    alignToHostPosition(midi, hostPosition, baseStepLengthSamples, numSamples, bpm);
    processPendingChordEvents(midi, numSamples);

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

    auto nextStepOffset = samplesUntilNextStep;

    while (nextStepOffset < static_cast<double>(numSamples))
    {
        const auto eventOffset = juce::jlimit(0, numSamples - 1, static_cast<int>(std::round(nextStepOffset)));
        const auto sceneIndex = getPlaybackSceneIndex();
        const auto sceneControls = sceneIndex >= 0
            ? readStoredSceneControls(sceneChainControls[static_cast<size_t>(sceneIndex)])
            : readCurrentSceneControls();
        activeSceneChainIndex.store(sceneIndex, std::memory_order_relaxed);
        const auto currentStepDuration = getStepDurationSamples(baseStepLengthSamples, currentStep, sceneControls);
        const auto step = readPlaybackStep(currentStep, sceneIndex);
        activeStepLock.store(step.enabled ? step.lock : 0.0f, std::memory_order_relaxed);
        activeStepIndex.store(currentStep, std::memory_order_relaxed);
        const auto gateAmount = juce::jlimit(0.05f,
                                             1.0f,
                                             sceneControls.gate * juce::jlimit(0.1f, 1.0f, step.length));
        const auto gateSamples = juce::jlimit(1, juce::jmax(1, currentStepDuration - 1),
                                             static_cast<int>(static_cast<float>(currentStepDuration) * gateAmount));

        const auto willTrigger = shouldTriggerStep(step, sceneControls);
        const auto shouldSlideIntoStep = willTrigger
            && activeNotesSlideToNext
            && activeNoteCount > 0;
        const auto slideSourceNotes = activeNotes;
        const auto slideSourceNoteCount = activeNoteCount;

        if (activeNoteCount > 0 && ! shouldSlideIntoStep)
        {
            addNoteOffsForActiveNotes(midi, eventOffset);
            pendingNoteOffSamples = -1.0;
        }
        else if (shouldSlideIntoStep)
        {
            pendingNoteOffSamples = -1.0;
        }

        if (willTrigger)
        {
            const auto root = static_cast<int>(std::round(juce::jlimit(0.0f, 127.0f, sceneControls.root)));
            const auto octaveOffset = static_cast<int>(std::round(juce::jlimit(-2.0f, 2.0f, sceneControls.octave))) * 12;
            const auto accent = juce::jlimit(0.0f, 1.0f, sceneControls.accent);
            const auto isAnchorStep = currentStep == 0 || currentStep == 4 || currentStep == 8 || currentStep == 12;
            const auto velocity = isAnchorStep
                ? juce::jlimit(0.0f, 1.0f, step.velocity + ((1.0f - step.velocity) * accent))
                : juce::jlimit(0.0f, 1.0f, step.velocity * (1.0f - (accent * 0.12f)));
            auto noteCount = 0;
            const auto notes = getChordNotes(root, octaveOffset, step.noteOffset, noteCount, sceneControls);
            const auto ratchetCount = juce::jlimit(minRatchet, maxRatchet, step.ratchet);

            if (ratchetCount <= 1)
            {
                activeNoteCount = noteCount;
                activeNotes = notes;
                const auto shouldSlideOutOfStep = step.slide && noteCount > 0;
                const auto noteOffOffset = eventOffset + (shouldSlideOutOfStep ? currentStepDuration + 1 : gateSamples);

                for (auto noteIndex = 0; noteIndex < noteCount; ++noteIndex)
                {
                    const auto noteVelocity = getChordNoteVelocity(velocity, noteIndex);
                    const auto maxNoteOnOffset = noteOffOffset < numSamples
                        ? juce::jmax(eventOffset, noteOffOffset - 1)
                        : numSamples - 1;
                    const auto noteOnOffset = juce::jlimit(eventOffset,
                                                           maxNoteOnOffset,
                                                           eventOffset + getChordStrumOffset(currentStepDuration, noteIndex, noteCount, sceneControls));
                    midi.addEvent(juce::MidiMessage::noteOn(1, notes[static_cast<size_t>(noteIndex)], noteVelocity), noteOnOffset);
                }

                if (shouldSlideIntoStep)
                    addNoteOffsForNotes(midi,
                                        slideSourceNotes,
                                        slideSourceNoteCount,
                                        juce::jmin(numSamples - 1, eventOffset + 1));

                if (noteOffOffset < numSamples)
                {
                    addNoteOffsForActiveNotes(midi, noteOffOffset);
                    pendingNoteOffSamples = -1.0;
                }
                else
                {
                    pendingNoteOffSamples = static_cast<double>(noteOffOffset - numSamples);
                    activeNotesSlideToNext = shouldSlideOutOfStep;
                }
            }
            else
            {
                pendingNoteOffSamples = -1.0;
                activeNotesSlideToNext = false;
                const auto ratchetSpacing = static_cast<double>(currentStepDuration) / static_cast<double>(ratchetCount);
                const auto ratchetGateSamples = juce::jlimit(1,
                                                             juce::jmax(1, static_cast<int>(std::round(ratchetSpacing)) - 1),
                                                             gateSamples);
                const auto strumStepLength = 1;

                for (auto ratchetIndex = 0; ratchetIndex < ratchetCount; ++ratchetIndex)
                {
                    const auto hitStart = static_cast<double>(eventOffset)
                        + (static_cast<double>(ratchetIndex) * ratchetSpacing);
                    const auto hitVelocity = juce::jlimit(0.0f,
                                                          1.0f,
                                                          velocity * (1.0f - (0.07f * static_cast<float>(ratchetIndex))));
                    emitOrQueueChordEvent(midi,
                                          numSamples,
                                          hitStart,
                                          true,
                                          notes,
                                          noteCount,
                                          hitVelocity,
                                          strumStepLength);
                    emitOrQueueChordEvent(midi,
                                          numSamples,
                                          hitStart + static_cast<double>(ratchetGateSamples),
                                          false,
                                          notes,
                                          noteCount,
                                          0.0f,
                                          strumStepLength);
                }
            }

            if (ratchetCount <= 1 && pendingNoteOffSamples < 0.0)
                activeNotesSlideToNext = false;
        }

        currentStep = (currentStep + 1) % numSteps;
        if (currentStep == 0)
            currentCycle = wrapConditionCycle(currentCycle + 1);

        nextStepOffset += static_cast<double>(currentStepDuration);
    }

    samplesUntilNextStep = nextStepOffset - static_cast<double>(numSamples);
    lastHostPpqPosition = hostPosition.ppqPosition;
    wasHostPlaying = hostPosition.isPlaying;
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

double PatternSequencer::getStepLengthPpq() const
{
    const auto rateIndex = static_cast<int>(std::round(readParameter(sequencerRate, 1.0f)));

    switch (rateIndex)
    {
        case 0: return 0.5;   // 1/8
        case 2: return 0.125; // 1/32
        case 1:
        default:
            return 0.25;      // 1/16
    }
}

int PatternSequencer::getStepDurationSamples(int baseStepLengthSamples, int stepIndex, const SceneControls& controls) const
{
    const auto currentDelay = getStepDelaySamples(baseStepLengthSamples, stepIndex, controls);
    const auto nextDelay = getStepDelaySamples(baseStepLengthSamples, (stepIndex + 1) % numSteps, controls);
    const auto duration = baseStepLengthSamples + nextDelay - currentDelay;
    return juce::jmax(1, duration);
}

int PatternSequencer::getStepDelaySamples(int baseStepLengthSamples, int stepIndex, const SceneControls& controls) const
{
    const auto safeIndex = juce::jlimit(0, numSteps - 1, stepIndex);
    const auto swing = juce::jlimit(0.0f, 0.65f, controls.swing);
    const auto maxDelay = static_cast<float>(baseStepLengthSamples) * swing * 0.5f;
    const auto sceneIndex = getPlaybackSceneIndex();
    const auto timing = juce::jlimit(0.0f, 1.0f, readPlaybackStep(safeIndex, sceneIndex).timing);
    const auto mode = static_cast<int>(std::round(juce::jlimit(0.0f, 7.0f, controls.grooveMode)));
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

        case 4: // House Shuffle
            if (! isAnchorStep)
                weight = timing > 0.0f
                    ? timing
                    : (stepIndex == 3 || stepIndex == 7 || stepIndex == 11 || stepIndex == 15 ? 0.42f
                        : isOffbeatStep ? 0.58f
                                        : 0.16f);
            break;

        case 5: // Tech House Tight
            weight = isAnchorStep ? 0.0f
                                  : (timing > 0.0f ? timing * 0.72f
                                                   : (isOffbeatStep ? 0.28f : 0.08f));
            break;

        case 6: // Minimal Skip
            if (! isAnchorStep)
                weight = timing > 0.0f
                    ? timing * 0.82f
                    : (stepIndex == 5 || stepIndex == 7 || stepIndex == 11 || stepIndex == 13 || stepIndex == 15 ? 0.48f
                                                                                                                   : 0.14f);
            break;

        case 7: // Techno Drive
            weight = isAnchorStep ? 0.0f
                                  : (timing > 0.0f ? timing * 0.55f
                                                   : (stepIndex == 3 || stepIndex == 7 || stepIndex == 11 || stepIndex == 15 ? 0.26f
                                                                                                                            : 0.12f));
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
    return quantizeNoteOffset(noteOffset, readCurrentSceneControls());
}

int PatternSequencer::quantizeNoteOffset(int noteOffset, const SceneControls& controls) const
{
    const auto scaleMode = static_cast<int>(std::round(juce::jlimit(0.0f, 4.0f, controls.scale)));
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

void PatternSequencer::addNoteOffsForNotes(juce::MidiBuffer& midi,
                                           const ChordNoteArray& notes,
                                           int noteCount,
                                           int samplePosition) const
{
    for (auto noteIndex = 0; noteIndex < noteCount; ++noteIndex)
    {
        const auto noteNumber = notes[static_cast<size_t>(noteIndex)];
        if (noteNumber >= 0)
            midi.addEvent(juce::MidiMessage::noteOff(1, noteNumber), samplePosition);
    }
}

void PatternSequencer::addNoteOffsForActiveNotes(juce::MidiBuffer& midi, int samplePosition)
{
    addNoteOffsForNotes(midi, activeNotes, activeNoteCount, samplePosition);

    activeNotes.fill(-1);
    activeNoteCount = 0;
    activeNotesSlideToNext = false;
}

void PatternSequencer::clearPendingChordEvents()
{
    for (auto& event : pendingChordEvents)
    {
        event.samplesUntil = -1.0;
        event.notes.fill(-1);
        event.noteCount = 0;
        event.velocity = 0.0f;
        event.strumStepLength = 1;
        event.noteOn = false;
    }

    pendingChordEventCount = 0;
}

void PatternSequencer::emitChordEvent(juce::MidiBuffer& midi,
                                      const PendingChordEvent& event,
                                      int samplePosition)
{
    const auto safeSamplePosition = juce::jmax(0, samplePosition);

    if (! event.noteOn)
    {
        if (activeNoteCount > 0)
            addNoteOffsForActiveNotes(midi, safeSamplePosition);
        else
        {
            for (auto noteIndex = 0; noteIndex < event.noteCount; ++noteIndex)
            {
                const auto noteNumber = event.notes[static_cast<size_t>(noteIndex)];
                if (noteNumber >= 0)
                    midi.addEvent(juce::MidiMessage::noteOff(1, noteNumber), safeSamplePosition);
            }
        }

        return;
    }

    if (activeNoteCount > 0)
        addNoteOffsForActiveNotes(midi, safeSamplePosition);

    activeNoteCount = event.noteCount;
    activeNotes = event.notes;

    for (auto noteIndex = 0; noteIndex < event.noteCount; ++noteIndex)
    {
        const auto noteNumber = event.notes[static_cast<size_t>(noteIndex)];
        if (noteNumber < 0)
            continue;

        const auto noteVelocity = getChordNoteVelocity(event.velocity, noteIndex);
        const auto noteOnOffset = safeSamplePosition + getChordStrumOffset(event.strumStepLength,
                                                                           noteIndex,
                                                                           event.noteCount);
        midi.addEvent(juce::MidiMessage::noteOn(1, noteNumber, noteVelocity), noteOnOffset);
    }
}

void PatternSequencer::queueChordEvent(double samplesUntil,
                                       bool noteOn,
                                       ChordNoteArray notes,
                                       int noteCount,
                                       float velocity,
                                       int strumStepLength)
{
    if (pendingChordEventCount >= static_cast<int>(pendingChordEvents.size()))
        return;

    auto& event = pendingChordEvents[static_cast<size_t>(pendingChordEventCount++)];
    event.samplesUntil = juce::jmax(0.0, samplesUntil);
    event.notes = notes;
    event.noteCount = juce::jlimit(0, maxChordNotes, noteCount);
    event.velocity = juce::jlimit(0.0f, 1.0f, velocity);
    event.strumStepLength = juce::jmax(1, strumStepLength);
    event.noteOn = noteOn;
}

void PatternSequencer::emitOrQueueChordEvent(juce::MidiBuffer& midi,
                                             int numSamples,
                                             double samplePosition,
                                             bool noteOn,
                                             ChordNoteArray notes,
                                             int noteCount,
                                             float velocity,
                                             int strumStepLength)
{
    PendingChordEvent event;
    event.samplesUntil = samplePosition;
    event.notes = notes;
    event.noteCount = juce::jlimit(0, maxChordNotes, noteCount);
    event.velocity = juce::jlimit(0.0f, 1.0f, velocity);
    event.strumStepLength = juce::jmax(1, strumStepLength);
    event.noteOn = noteOn;

    if (samplePosition < static_cast<double>(numSamples))
    {
        emitChordEvent(midi, event, static_cast<int>(std::round(samplePosition)));
        return;
    }

    queueChordEvent(samplePosition - static_cast<double>(numSamples),
                    noteOn,
                    notes,
                    noteCount,
                    velocity,
                    strumStepLength);
}

void PatternSequencer::processPendingChordEvents(juce::MidiBuffer& midi, int numSamples)
{
    while (true)
    {
        auto bestIndex = -1;
        auto bestSamplesUntil = static_cast<double>(numSamples);
        auto bestIsNoteOn = true;

        for (auto index = 0; index < pendingChordEventCount; ++index)
        {
            const auto& event = pendingChordEvents[static_cast<size_t>(index)];
            if (event.samplesUntil >= static_cast<double>(numSamples))
                continue;

            if (bestIndex < 0
                || event.samplesUntil < bestSamplesUntil
                || (std::abs(event.samplesUntil - bestSamplesUntil) < 0.5 && bestIsNoteOn && ! event.noteOn))
            {
                bestIndex = index;
                bestSamplesUntil = event.samplesUntil;
                bestIsNoteOn = event.noteOn;
            }
        }

        if (bestIndex < 0)
            break;

        const auto event = pendingChordEvents[static_cast<size_t>(bestIndex)];
        emitChordEvent(midi, event, static_cast<int>(std::round(event.samplesUntil)));

        --pendingChordEventCount;
        if (bestIndex < pendingChordEventCount)
            pendingChordEvents[static_cast<size_t>(bestIndex)] = pendingChordEvents[static_cast<size_t>(pendingChordEventCount)];
    }

    for (auto index = 0; index < pendingChordEventCount; ++index)
        pendingChordEvents[static_cast<size_t>(index)].samplesUntil -= static_cast<double>(numSamples);
}

float PatternSequencer::nextRandomFloat()
{
    randomState = (1664525u * randomState) + 1013904223u;
    return static_cast<float>(randomState & 0x00ffffffu) / static_cast<float>(0x01000000u);
}

bool PatternSequencer::shouldTriggerStep(const Step& step, const SceneControls& controls)
{
    if (! step.enabled || ! stepConditionAllows(step.condition))
        return false;

    return nextRandomFloat() <= juce::jlimit(0.0f, 1.0f, step.probability * juce::jlimit(0.0f, 1.0f, controls.probability));
}

bool PatternSequencer::stepConditionAllows(int condition) const
{
    switch (juce::jlimit(minCondition, maxCondition, condition))
    {
        case 1: // Odd phrase cycles: bars 1 and 3.
            return (currentCycle % 2) == 0;

        case 2: // Even phrase cycles: bars 2 and 4.
            return (currentCycle % 2) != 0;

        case 3: // Fill cycle: bar 4 of a four-cycle phrase.
        {
            const auto chainLength = sceneChainLength.load(std::memory_order_acquire);
            if (chainLength > 0)
                return chainLength == 1 || (wrapConditionCycle(currentCycle) % chainLength) == chainLength - 1;

            return currentCycle == conditionPhraseCycleCount - 1;
        }

        case 0:
        default:
            return true;
    }
}

void PatternSequencer::alignToHostPosition(juce::MidiBuffer& midi,
                                           HostPosition hostPosition,
                                           int baseStepLengthSamples,
                                           int numSamples,
                                           double bpm)
{
    if (! hostPosition.ppqPosition)
    {
        lastHostPpqPosition = hostPosition.ppqPosition;
        wasHostPlaying = hostPosition.isPlaying;
        return;
    }

    const auto stepLengthPpq = getStepLengthPpq();
    if (stepLengthPpq <= 0.0)
        return;

    auto shouldRealign = ! wasHostPlaying || ! lastHostPpqPosition;

    if (lastHostPpqPosition)
    {
        const auto safeBpm = juce::jlimit(20.0, 300.0, bpm);
        const auto expectedPpqAdvance = (static_cast<double>(numSamples) / juce::jmax(1.0, currentSampleRate))
                                      * (safeBpm / 60.0);
        const auto actualPpqAdvance = *hostPosition.ppqPosition - *lastHostPpqPosition;
        shouldRealign = shouldRealign
            || std::abs(actualPpqAdvance - expectedPpqAdvance) > juce::jmax(stepLengthPpq * 0.5, expectedPpqAdvance * 3.0);
    }

    if (! shouldRealign)
        return;

    if (activeNoteCount > 0)
        addNoteOffsForActiveNotes(midi, 0);

    pendingNoteOffSamples = -1.0;
    clearPendingChordEvents();
    activeNotesSlideToNext = false;

    const auto continuousStep = *hostPosition.ppqPosition / stepLengthPpq;
    const auto stepFloor = std::floor(continuousStep);
    const auto fraction = juce::jlimit(0.0, 1.0, continuousStep - stepFloor);
    const auto absoluteStep = static_cast<int>(stepFloor);
    const auto hostStepIndex = ((absoluteStep % numSteps) + numSteps) % numSteps;
    const auto samplePositionInStep = fraction * static_cast<double>(baseStepLengthSamples);
    const auto sceneIndex = getPlaybackSceneIndex();
    const auto controls = sceneIndex >= 0
        ? readStoredSceneControls(sceneChainControls[static_cast<size_t>(sceneIndex)])
        : readCurrentSceneControls();
    const auto currentStepDelay = static_cast<double>(getStepDelaySamples(baseStepLengthSamples, hostStepIndex, controls));

    if (samplePositionInStep <= currentStepDelay)
    {
        currentStep = hostStepIndex;
        currentCycle = conditionCycleForAbsoluteStep(absoluteStep);
        samplesUntilNextStep = juce::jmax(0.0, currentStepDelay - samplePositionInStep);
    }
    else
    {
        const auto nextAbsoluteStep = absoluteStep + 1;
        const auto nextStep = ((nextAbsoluteStep % numSteps) + numSteps) % numSteps;
        currentStep = nextStep;
        currentCycle = conditionCycleForAbsoluteStep(nextAbsoluteStep);
        samplesUntilNextStep = ((1.0 - fraction) * static_cast<double>(baseStepLengthSamples))
                             + static_cast<double>(getStepDelaySamples(baseStepLengthSamples, nextStep, controls));
    }
}

float PatternSequencer::readParameter(std::atomic<float>* parameter, float fallback) const
{
    return parameter != nullptr ? parameter->load() : fallback;
}
}
