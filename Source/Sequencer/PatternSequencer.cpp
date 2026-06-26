#include "PatternSequencer.h"

#include <cmath>

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
    activeNote = -1;
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
    step.noteOffset = juce::jlimit(-24, 24, step.noteOffset);
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

    if (activeNote >= 0 && pendingNoteOffSamples >= 0.0)
    {
        if (pendingNoteOffSamples < static_cast<double>(numSamples))
        {
            midi.addEvent(juce::MidiMessage::noteOff(1, activeNote), static_cast<int>(pendingNoteOffSamples));
            activeNote = -1;
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

        if (activeNote >= 0)
        {
            midi.addEvent(juce::MidiMessage::noteOff(1, activeNote), eventOffset);
            activeNote = -1;
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
            activeNote = juce::jlimit(0, 127, root + octaveOffset + step.noteOffset);
            midi.addEvent(juce::MidiMessage::noteOn(1, activeNote, velocity), eventOffset);

            const auto noteOffOffset = eventOffset + gateSamples;
            if (noteOffOffset < numSamples)
            {
                midi.addEvent(juce::MidiMessage::noteOff(1, activeNote), noteOffOffset);
                activeNote = -1;
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
