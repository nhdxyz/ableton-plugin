#include "Voice.h"

#include <cmath>

namespace Synth
{
namespace
{
double cyclesPerBeatForLfoSync(int rateIndex)
{
    switch (rateIndex)
    {
        case 1: return 2.0; // 1/8
        case 2: return 3.0; // 1/8 triplet
        case 3: return 4.0; // 1/16
        default: return 1.0; // 1/4
    }
}

float advanceChaosRandomWalk(float currentValue, juce::Random& random, float cyclesThisStep)
{
    const auto safeCycles = juce::jlimit(0.0f, 1.0f, cyclesThisStep);
    const auto randomStep = ((random.nextFloat() * 2.0f) - 1.0f)
        * juce::jlimit(0.00025f, 0.05f, std::sqrt(safeCycles) * 0.08f);
    const auto damping = juce::jlimit(0.00001f, 0.45f, safeCycles * 0.35f);

    return juce::jlimit(-1.0f, 1.0f, (currentValue * (1.0f - damping)) + randomStep);
}
}

Voice::Voice(Parameters::APVTS& state)
    : parameters(state)
{
    oscWave = parameters.getRawParameterValue(Parameters::ID::oscWave);
    oscOctave = parameters.getRawParameterValue(Parameters::ID::oscOctave);
    oscTune = parameters.getRawParameterValue(Parameters::ID::oscTune);
    osc1Level = parameters.getRawParameterValue(Parameters::ID::osc1Level);
    osc2Wave = parameters.getRawParameterValue(Parameters::ID::osc2Wave);
    osc2Octave = parameters.getRawParameterValue(Parameters::ID::osc2Octave);
    osc2Tune = parameters.getRawParameterValue(Parameters::ID::osc2Tune);
    osc2Level = parameters.getRawParameterValue(Parameters::ID::osc2Level);
    subLevel = parameters.getRawParameterValue(Parameters::ID::subLevel);
    noiseLevel = parameters.getRawParameterValue(Parameters::ID::noiseLevel);
    noiseType = parameters.getRawParameterValue(Parameters::ID::noiseType);
    noiseDecay = parameters.getRawParameterValue(Parameters::ID::noiseDecay);
    oscWarp = parameters.getRawParameterValue(Parameters::ID::oscWarp);
    oscWavetablePosition = parameters.getRawParameterValue(Parameters::ID::oscWavetablePosition);
    osc2WavetablePosition = parameters.getRawParameterValue(Parameters::ID::osc2WavetablePosition);
    ampAttack = parameters.getRawParameterValue(Parameters::ID::ampAttack);
    ampDecay = parameters.getRawParameterValue(Parameters::ID::ampDecay);
    ampSustain = parameters.getRawParameterValue(Parameters::ID::ampSustain);
    ampRelease = parameters.getRawParameterValue(Parameters::ID::ampRelease);
    filterCutoff = parameters.getRawParameterValue(Parameters::ID::filterCutoff);
    filterResonance = parameters.getRawParameterValue(Parameters::ID::filterResonance);
    filterEnvAmount = parameters.getRawParameterValue(Parameters::ID::filterEnvAmount);
    filterMode = parameters.getRawParameterValue(Parameters::ID::filterMode);
    filterCharacter = parameters.getRawParameterValue(Parameters::ID::filterCharacter);
    filterSlope = parameters.getRawParameterValue(Parameters::ID::filterSlope);
    driveAmount = parameters.getRawParameterValue(Parameters::ID::driveAmount);
    monoMode = parameters.getRawParameterValue(Parameters::ID::monoMode);
    glideTime = parameters.getRawParameterValue(Parameters::ID::glideTime);
    unisonVoices = parameters.getRawParameterValue(Parameters::ID::unisonVoices);
    unisonDetune = parameters.getRawParameterValue(Parameters::ID::unisonDetune);
    unisonBlend = parameters.getRawParameterValue(Parameters::ID::unisonBlend);
    unisonSpread = parameters.getRawParameterValue(Parameters::ID::unisonSpread);
    macroTone = parameters.getRawParameterValue(Parameters::ID::macroTone);
    macroDirt = parameters.getRawParameterValue(Parameters::ID::macroDirt);
    macroMotion = parameters.getRawParameterValue(Parameters::ID::macroMotion);
    macroSpace = parameters.getRawParameterValue(Parameters::ID::macroSpace);
    macroWeight = parameters.getRawParameterValue(Parameters::ID::macroWeight);
    macroBounce = parameters.getRawParameterValue(Parameters::ID::macroBounce);
    macroWarp = parameters.getRawParameterValue(Parameters::ID::macroWarp);
    macroThrow = parameters.getRawParameterValue(Parameters::ID::macroThrow);
    lfo1Rate = parameters.getRawParameterValue(Parameters::ID::lfo1Rate);
    lfo1Sync = parameters.getRawParameterValue(Parameters::ID::lfo1Sync);
    lfo1SyncRate = parameters.getRawParameterValue(Parameters::ID::lfo1SyncRate);
    lfo1Shape = parameters.getRawParameterValue(Parameters::ID::lfo1Shape);
    lfo1Depth = parameters.getRawParameterValue(Parameters::ID::lfo1Depth);
    lfo1Phase = parameters.getRawParameterValue(Parameters::ID::lfo1Phase);
    lfo1Retrigger = parameters.getRawParameterValue(Parameters::ID::lfo1Retrigger);
    for (size_t index = 0; index < lfo1CurvePoints.size(); ++index)
        lfo1CurvePoints[index] = parameters.getRawParameterValue(Parameters::ID::lfo1Curve[index]);

    lfo2Rate = parameters.getRawParameterValue(Parameters::ID::lfo2Rate);
    lfo2Sync = parameters.getRawParameterValue(Parameters::ID::lfo2Sync);
    lfo2SyncRate = parameters.getRawParameterValue(Parameters::ID::lfo2SyncRate);
    lfo2Shape = parameters.getRawParameterValue(Parameters::ID::lfo2Shape);
    lfo2Depth = parameters.getRawParameterValue(Parameters::ID::lfo2Depth);
    lfo2PhaseParam = parameters.getRawParameterValue(Parameters::ID::lfo2Phase);
    lfo2Retrigger = parameters.getRawParameterValue(Parameters::ID::lfo2Retrigger);

    modEnv1Attack = parameters.getRawParameterValue(Parameters::ID::modEnv1Attack);
    modEnv1Decay = parameters.getRawParameterValue(Parameters::ID::modEnv1Decay);
    modEnv1Sustain = parameters.getRawParameterValue(Parameters::ID::modEnv1Sustain);
    modEnv1Release = parameters.getRawParameterValue(Parameters::ID::modEnv1Release);
    modEnv1Depth = parameters.getRawParameterValue(Parameters::ID::modEnv1Depth);

    for (size_t index = 0; index < modMatrixSources.size(); ++index)
    {
        modMatrixSources[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixSource[index]);
        modMatrixDestinations[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixDestination[index]);
        modMatrixAmounts[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixAmount[index]);
        modMatrixEnabled[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixEnabled[index]);
    }
}

bool Voice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<Sound*>(sound) != nullptr;
}

void Voice::prepare(double sampleRate, int maximumBlockSize)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;

    for (auto& oscillator : oscillators)
        oscillator.prepare(sampleRate);

    for (auto& oscillator : oscillators2)
        oscillator.prepare(sampleRate);

    subOscillator.prepare(sampleRate);
    subOscillator.setWaveform(Waveform::sine);
    ampEnvelope.setSampleRate(sampleRate);
    modEnvelope.setSampleRate(sampleRate);
    leftFilter.prepare(sampleRate, maximumBlockSize);
    rightFilter.prepare(sampleRate, maximumBlockSize);
}

void Voice::setHostBpm(double bpm) noexcept
{
    hostBpm = juce::jlimit(20.0, 300.0, bpm);
}

void Voice::setSequencerLock(int destinationIndex, float amount) noexcept
{
    sequencerLockDestination = destinationIndex;
    sequencerLockAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void Voice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    noteVelocity = velocity;
    pitchWheelMoved(currentPitchWheelPosition);
    targetFrequencyHz = frequencyForNote(midiNoteNumber);
    const auto glideSeconds = readParameter(glideTime, 0.0f);
    const auto shouldGlide = hasPreviousNoteFrequency
        && readParameter(monoMode, 0.0f) >= 0.5f
        && glideSeconds > 0.001f;

    if (shouldGlide)
    {
        glideStartFrequencyHz = juce::jmax(1.0f, currentFrequencyHz);
        glideTotalSamples = juce::jmax(1, static_cast<int>(std::round(glideSeconds * currentSampleRate)));
        glideSamplesRemaining = glideTotalSamples;
    }
    else
    {
        currentFrequencyHz = targetFrequencyHz;
        glideSamplesRemaining = 0;
        glideTotalSamples = 0;
    }

    hasPreviousNoteFrequency = true;

    for (auto& oscillator : oscillators)
        oscillator.reset();

    for (auto& oscillator : oscillators2)
        oscillator.reset();

    subOscillator.reset();
    leftFilter.reset();
    rightFilter.reset();
    ampEnvelope.reset();
    ampEnvelope.noteOn();
    modEnvelope.reset();
    modEnvelope.noteOn();
    noiseColourState = 0.0f;
    noiseBrownState = 0.0f;
    noiseCrackleState = 0.0f;
    noiseDigitalHeldSample = (noiseRandom.nextFloat() * 2.0f) - 1.0f;
    noiseDigitalHoldSamples = 1;
    voiceAgeSamples = 0;

    if (readParameter(lfo1Retrigger, 1.0f) >= 0.5f)
    {
        lfoPhase = 0.0f;
        lfoStepValue = (modulationRandom.nextFloat() * 2.0f) - 1.0f;
        lfoSmoothRandomStartValue = lfoStepValue;
        lfoSmoothRandomValue = lfoStepValue;
        lfoChaosValue = ((modulationRandom.nextFloat() * 2.0f) - 1.0f) * 0.25f;
    }

    if (readParameter(lfo2Retrigger, 1.0f) >= 0.5f)
    {
        lfo2Phase = 0.0f;
        lfo2StepValue = (modulationRandom.nextFloat() * 2.0f) - 1.0f;
    }
}

void Voice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampEnvelope.noteOff();
        modEnvelope.noteOff();
        return;
    }

    clearCurrentNote();
    ampEnvelope.reset();
    modEnvelope.reset();
}

void Voice::pitchWheelMoved(int newPitchWheelValue)
{
    const auto normalised = (static_cast<float>(newPitchWheelValue) - 8192.0f) / 8192.0f;
    pitchBendSemitones = juce::jlimit(-1.0f, 1.0f, normalised) * 2.0f;
}

void Voice::controllerMoved(int, int)
{
}

void Voice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (! isVoiceActive())
        return;

    for (auto sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        const auto envelopeValue = ampEnvelope.process();
        updateVoiceParameters(envelopeValue);

        const auto osc1Gain = readParameter(osc1Level, 1.0f);
        const auto osc2Gain = juce::jlimit(0.0f, 1.0f, readParameter(osc2Level, 0.0f) + currentOsc2LevelOffset);
        const auto weight = readParameter(macroWeight, 0.0f);
        const auto subGain = juce::jlimit(0.0f, 1.15f, readParameter(subLevel, 0.0f) + (weight * 0.32f));
        const auto noiseGain = readParameter(noiseLevel, 0.0f);
        const auto sourceWeight = osc1Gain + osc2Gain + (subGain * 0.75f) + (noiseGain * 0.45f);
        const auto sourceCompensation = 1.0f / juce::jmax(1.0f, sourceWeight);
        const auto noiseTypeIndex = juce::jlimit(0, 6, static_cast<int>(std::round(readParameter(noiseType, 0.0f))));
        const auto noiseSample = processNoiseSample(noiseTypeIndex);
        const auto unisonSample = renderUnisonStack(osc1Gain, osc2Gain);
        const auto centeredSample = (subOscillator.process() * subGain * 0.75f)
            + (noiseSample * noiseGain * 0.45f);

        auto leftSample = (unisonSample.left + centeredSample) * sourceCompensation;
        auto rightSample = (unisonSample.right + centeredSample) * sourceCompensation;
        leftSample = leftFilter.process(leftSample);
        rightSample = rightFilter.process(rightSample);
        const auto dirt = readParameter(macroDirt, 0.0f);
        const auto macroDrive = juce::jlimit(0.0f, 0.95f, readParameter(driveAmount, 0.18f) + (dirt * 0.55f) + currentDriveOffset);
        leftSample = distortion.process(leftSample, macroDrive);
        rightSample = distortion.process(rightSample, macroDrive);
        leftSample *= envelopeValue * noteVelocity * 0.28f;
        rightSample *= envelopeValue * noteVelocity * 0.28f;

        if (outputBuffer.getNumChannels() == 1)
        {
            outputBuffer.addSample(0, startSample + sampleIndex, (leftSample + rightSample) * 0.5f);
        }
        else
        {
            outputBuffer.addSample(0, startSample + sampleIndex, leftSample);
            outputBuffer.addSample(1, startSample + sampleIndex, rightSample);

            for (auto channel = 2; channel < outputBuffer.getNumChannels(); ++channel)
                outputBuffer.addSample(channel, startSample + sampleIndex, (leftSample + rightSample) * 0.5f);
        }

        ++voiceAgeSamples;

        if (! ampEnvelope.isActive())
        {
            clearCurrentNote();
            break;
        }
    }
}

void Voice::updateVoiceParameters(float envelopeValue)
{
    updateGlide();
    modEnvelope.setParameters(
        readParameter(modEnv1Attack, 0.01f),
        readParameter(modEnv1Decay, 0.22f),
        readParameter(modEnv1Sustain, 0.0f),
        readParameter(modEnv1Release, 0.12f));

    const auto lfoValue = processLfo() * readParameter(lfo1Depth, 0.45f);
    const auto lfo2Value = processLfo2() * readParameter(lfo2Depth, 0.25f);
    const auto modEnvelopeValue = modEnvelope.process() * readParameter(modEnv1Depth, 0.5f);
    auto cutoffMod = 0.0f;
    auto resonanceMod = 0.0f;
    auto filterEnvMod = 0.0f;
    auto driveMod = 0.0f;
    auto osc2TuneMod = 0.0f;
    auto osc2LevelMod = 0.0f;
    auto oscWarpMod = 0.0f;
    auto oscWavetablePositionMod = 0.0f;
    auto osc2WavetablePositionMod = 0.0f;

    for (size_t index = 0; index < modMatrixSources.size(); ++index)
    {
        const auto sourceIndex = static_cast<int>(std::round(readParameter(modMatrixSources[index], 0.0f)));
        const auto destinationIndex = static_cast<int>(std::round(readParameter(modMatrixDestinations[index], 0.0f)));
        const auto amount = readParameter(modMatrixAmounts[index], 0.0f);
        const auto enabled = readParameter(modMatrixEnabled[index], 1.0f) >= 0.5f;

        if (! enabled || sourceIndex == 0 || destinationIndex == 0 || std::abs(amount) <= 0.0001f)
            continue;

        const auto contribution = evaluateModulationSource(sourceIndex, lfoValue, lfo2Value, modEnvelopeValue) * amount;

        switch (destinationIndex)
        {
            case 1: cutoffMod += contribution; break;
            case 2: resonanceMod += contribution; break;
            case 3: filterEnvMod += contribution; break;
            case 4: driveMod += contribution; break;
            case 5: osc2TuneMod += contribution; break;
            case 6: osc2LevelMod += contribution; break;
            case 17: oscWarpMod += contribution; break;
            case 18: oscWavetablePositionMod += contribution; break;
            case 19: osc2WavetablePositionMod += contribution; break;
            default: break;
        }
    }

    cutoffMod = juce::jlimit(-1.0f, 1.0f, cutoffMod);
    resonanceMod = juce::jlimit(-1.0f, 1.0f, resonanceMod);
    filterEnvMod = juce::jlimit(-1.0f, 1.0f, filterEnvMod);
    driveMod = juce::jlimit(-1.0f, 1.0f, driveMod);
    osc2TuneMod = juce::jlimit(-1.0f, 1.0f, osc2TuneMod);
    osc2LevelMod = juce::jlimit(-1.0f, 1.0f, osc2LevelMod);
    oscWarpMod = juce::jlimit(-1.0f, 1.0f, oscWarpMod);
    oscWavetablePositionMod = juce::jlimit(-1.0f, 1.0f, oscWavetablePositionMod);
    osc2WavetablePositionMod = juce::jlimit(-1.0f, 1.0f, osc2WavetablePositionMod);
    const auto sequenceCutoffMod = sequencerLockDestination == 1 ? sequencerLockAmount : 0.0f;
    const auto sequenceDriveMod = sequencerLockDestination == 2 ? sequencerLockAmount : 0.0f;
    const auto sequenceWarpMod = sequencerLockDestination == 3 ? sequencerLockAmount : 0.0f;
    const auto sequenceWavetable1Mod = sequencerLockDestination == 7 ? sequencerLockAmount : 0.0f;
    const auto sequenceWavetable2Mod = sequencerLockDestination == 8 ? sequencerLockAmount : 0.0f;
    currentDriveOffset = (driveMod * 0.45f) + (sequenceDriveMod * 0.38f);
    currentOsc2LevelOffset = osc2LevelMod * 0.75f;

    const auto waveIndex = static_cast<int>(readParameter(oscWave, 1.0f));
    const auto waveform = static_cast<Waveform>(juce::jlimit(0, 6, waveIndex));

    const auto octaveOffset = static_cast<int>(readParameter(oscOctave, 0.0f)) * 12.0f;
    const auto tuneOffset = readParameter(oscTune, 0.0f);
    const auto pitchRatio = std::pow(2.0f, (octaveOffset + tuneOffset + pitchBendSemitones) / 12.0f);

    const auto osc2WaveIndex = static_cast<int>(readParameter(osc2Wave, 1.0f));
    const auto osc2Waveform = static_cast<Waveform>(juce::jlimit(0, 6, osc2WaveIndex));

    const auto osc2OctaveOffset = static_cast<int>(readParameter(osc2Octave, 0.0f)) * 12.0f;
    const auto motion = readParameter(macroMotion, 0.0f);
    const auto warp = readParameter(macroWarp, 0.0f);
    const auto osc2TuneOffset = readParameter(osc2Tune, 0.0f) + (motion * 5.0f) + (warp * 7.0f) + (osc2TuneMod * 12.0f);
    const auto osc2PitchRatio = std::pow(2.0f, (osc2OctaveOffset + osc2TuneOffset + pitchBendSemitones) / 12.0f);
    const auto oscillatorWarpAmount = juce::jlimit(0.0f, 1.0f, readParameter(oscWarp, 0.0f) + (warp * 0.32f) + (oscWarpMod * 0.55f) + (sequenceWarpMod * 0.45f));
    const auto osc1WavetablePosition = juce::jlimit(0.0f, 1.0f, readParameter(oscWavetablePosition, 0.0f)
        + (motion * 0.16f)
        + (warp * 0.12f)
        + (oscWavetablePositionMod * 0.5f)
        + (sequenceWavetable1Mod * 0.55f));
    const auto osc2WavetablePositionValue = juce::jlimit(0.0f, 1.0f, readParameter(osc2WavetablePosition, 0.35f)
        + (motion * 0.18f)
        + (warp * 0.14f)
        + (osc2WavetablePositionMod * 0.5f)
        + (sequenceWavetable2Mod * 0.55f));

    const auto activeUnisonVoices = getUnisonVoiceCount();
    const auto detuneCents = readParameter(unisonDetune, 0.0f) * 24.0f;
    for (auto voiceIndex = 0; voiceIndex < activeUnisonVoices; ++voiceIndex)
    {
        const auto detuneRatio = std::pow(2.0f, (getUnisonPosition(voiceIndex, activeUnisonVoices) * detuneCents) / 1200.0f);
        oscillators[static_cast<size_t>(voiceIndex)].setWaveform(waveform);
        oscillators[static_cast<size_t>(voiceIndex)].setWarp(oscillatorWarpAmount);
        oscillators[static_cast<size_t>(voiceIndex)].setWavetablePosition(osc1WavetablePosition);
        oscillators[static_cast<size_t>(voiceIndex)].setFrequency(currentFrequencyHz * pitchRatio * detuneRatio);
        oscillators2[static_cast<size_t>(voiceIndex)].setWaveform(osc2Waveform);
        oscillators2[static_cast<size_t>(voiceIndex)].setWarp(oscillatorWarpAmount);
        oscillators2[static_cast<size_t>(voiceIndex)].setWavetablePosition(osc2WavetablePositionValue);
        oscillators2[static_cast<size_t>(voiceIndex)].setFrequency(currentFrequencyHz * osc2PitchRatio * detuneRatio);
    }

    subOscillator.setFrequency(currentFrequencyHz * 0.5f);

    ampEnvelope.setParameters(
        readParameter(ampAttack, 0.01f),
        readParameter(ampDecay, 0.18f),
        readParameter(ampSustain, 0.65f),
        readParameter(ampRelease, 0.22f));

    const auto tone = readParameter(macroTone, 0.0f);
    const auto envAmount = juce::jlimit(-1.0f, 1.0f, readParameter(filterEnvAmount, 0.15f) + (motion * 0.35f) + (warp * 0.18f) + (filterEnvMod * 0.65f));
    const auto cutoffScale = std::pow(2.0f, envAmount * envelopeValue * 4.0f);
    const auto toneCutoffScale = std::pow(2.0f, tone * 2.5f);
    const auto matrixCutoffScale = std::pow(2.0f, cutoffMod * 4.0f);
    const auto sequenceCutoffScale = std::pow(2.0f, sequenceCutoffMod * 3.0f);
    auto macroResonance = juce::jlimit(0.1f, 1.4f, readParameter(filterResonance, 0.45f) + (tone * 0.22f) + (warp * 0.14f) + (resonanceMod * 0.45f));
    const auto filterModeIndex = static_cast<int>(readParameter(filterMode, 0.0f));
    const auto mode = static_cast<Filter::Mode>(juce::jlimit(0, 2, filterModeIndex));
    const auto filterCharacterIndex = static_cast<int>(readParameter(filterCharacter, 0.0f));
    const auto character = static_cast<Filter::Character>(juce::jlimit(0, 3, filterCharacterIndex));
    const auto filterSlopeIndex = static_cast<int>(readParameter(filterSlope, 0.0f));
    const auto slope = static_cast<Filter::Slope>(juce::jlimit(0, 1, filterSlopeIndex));
    auto cutoff = readParameter(filterCutoff, 1800.0f) * cutoffScale * toneCutoffScale * matrixCutoffScale * sequenceCutoffScale;
    const auto baseDrive = juce::jlimit(0.0f, 0.95f, readParameter(driveAmount, 0.18f) + currentDriveOffset);

    switch (character)
    {
        case Filter::Character::clean:
            break;

        case Filter::Character::warm:
            cutoff *= 0.94f;
            macroResonance *= 0.94f;
            break;

        case Filter::Character::acid:
            cutoff *= 1.08f;
            macroResonance += 0.22f + (baseDrive * 0.12f);
            break;

        case Filter::Character::dirty:
            cutoff *= 0.88f;
            macroResonance *= 0.82f;
            break;
    }

    macroResonance = juce::jlimit(0.1f, 1.6f, macroResonance);
    leftFilter.setMode(mode);
    rightFilter.setMode(mode);
    leftFilter.setCharacter(character, baseDrive);
    rightFilter.setCharacter(character, baseDrive);
    leftFilter.setSlope(slope);
    rightFilter.setSlope(slope);
    leftFilter.setCutoffAndResonance(cutoff, macroResonance);
    rightFilter.setCutoffAndResonance(cutoff, macroResonance);
}

void Voice::updateGlide()
{
    if (glideSamplesRemaining <= 0 || glideTotalSamples <= 0)
    {
        currentFrequencyHz = targetFrequencyHz;
        return;
    }

    const auto progress = 1.0f - (static_cast<float>(glideSamplesRemaining) / static_cast<float>(glideTotalSamples));
    const auto start = juce::jmax(1.0f, glideStartFrequencyHz);
    const auto target = juce::jmax(1.0f, targetFrequencyHz);
    currentFrequencyHz = start * std::pow(target / start, progress);
    --glideSamplesRemaining;
}

float Voice::processLfo()
{
    const auto shapeIndex = static_cast<int>(std::round(readParameter(lfo1Shape, 0.0f)));
    const auto syncEnabled = readParameter(lfo1Sync, 1.0f) >= 0.5f;
    const auto rateHz = syncEnabled
        ? static_cast<float>((hostBpm / 60.0) * cyclesPerBeatForLfoSync(static_cast<int>(std::round(readParameter(lfo1SyncRate, 1.0f)))))
        : readParameter(lfo1Rate, 1.0f);
    const auto phaseOffset = readParameter(lfo1Phase, 0.0f);
    const auto phase = std::fmod(lfoPhase + phaseOffset + 1.0f, 1.0f);
    auto value = 0.0f;

    switch (shapeIndex)
    {
        case 1:
            value = phase < 0.25f ? phase * 4.0f
                : phase < 0.75f ? 2.0f - (phase * 4.0f)
                : (phase * 4.0f) - 4.0f;
            break;

        case 2:
            value = (phase * 2.0f) - 1.0f;
            break;

        case 3:
            value = phase < 0.5f ? 1.0f : -1.0f;
            break;

        case 4:
            value = lfoStepValue;
            break;

        case 5:
            value = evaluateLfoCurve(phase);
            break;

        default:
            value = std::sin(juce::MathConstants<float>::twoPi * phase);
            break;
    }

    const auto smoothProgress = phase * phase * (3.0f - (2.0f * phase));
    lfoSmoothRandomValue = juce::jlimit(-1.0f,
                                        1.0f,
                                        lfoSmoothRandomStartValue
                                            + ((lfoStepValue - lfoSmoothRandomStartValue) * smoothProgress));
    lfoChaosValue = advanceChaosRandomWalk(lfoChaosValue,
                                           modulationRandom,
                                           rateHz / static_cast<float>(juce::jmax(1.0, currentSampleRate)));

    const auto previousPhase = lfoPhase;
    lfoPhase += juce::jlimit(0.01f, 80.0f, rateHz) / static_cast<float>(currentSampleRate);

    if (lfoPhase >= 1.0f)
    {
        lfoPhase -= std::floor(lfoPhase);
        if (previousPhase < 1.0f)
        {
            lfoSmoothRandomStartValue = lfoStepValue;
            lfoStepValue = (modulationRandom.nextFloat() * 2.0f) - 1.0f;
        }
    }

    return juce::jlimit(-1.0f, 1.0f, value);
}

float Voice::processLfo2()
{
    const auto shapeIndex = static_cast<int>(std::round(readParameter(lfo2Shape, 1.0f)));
    const auto syncEnabled = readParameter(lfo2Sync, 1.0f) >= 0.5f;
    const auto rateHz = syncEnabled
        ? static_cast<float>((hostBpm / 60.0) * cyclesPerBeatForLfoSync(static_cast<int>(std::round(readParameter(lfo2SyncRate, 3.0f)))))
        : readParameter(lfo2Rate, 1.5f);
    const auto phaseOffset = readParameter(lfo2PhaseParam, 0.25f);
    const auto phase = std::fmod(lfo2Phase + phaseOffset + 1.0f, 1.0f);
    auto value = 0.0f;

    switch (shapeIndex)
    {
        case 1:
            value = phase < 0.25f ? phase * 4.0f
                : phase < 0.75f ? 2.0f - (phase * 4.0f)
                : (phase * 4.0f) - 4.0f;
            break;

        case 2:
            value = (phase * 2.0f) - 1.0f;
            break;

        case 3:
            value = phase < 0.5f ? 1.0f : -1.0f;
            break;

        case 4:
            value = lfo2StepValue;
            break;

        default:
            value = std::sin(juce::MathConstants<float>::twoPi * phase);
            break;
    }

    const auto previousPhase = lfo2Phase;
    lfo2Phase += juce::jlimit(0.01f, 80.0f, rateHz) / static_cast<float>(currentSampleRate);

    if (lfo2Phase >= 1.0f)
    {
        lfo2Phase -= std::floor(lfo2Phase);
        if (previousPhase < 1.0f)
            lfo2StepValue = (modulationRandom.nextFloat() * 2.0f) - 1.0f;
    }

    return juce::jlimit(-1.0f, 1.0f, value);
}

float Voice::evaluateLfoCurve(float phase) const
{
    constexpr auto pointCount = 8;
    const auto scaledPhase = juce::jlimit(0.0f, 0.999999f, phase) * static_cast<float>(pointCount);
    const auto leftIndex = static_cast<int>(std::floor(scaledPhase)) % pointCount;
    const auto rightIndex = (leftIndex + 1) % pointCount;
    const auto fraction = scaledPhase - std::floor(scaledPhase);
    const auto leftValue = readParameter(lfo1CurvePoints[static_cast<size_t>(leftIndex)], 0.0f);
    const auto rightValue = readParameter(lfo1CurvePoints[static_cast<size_t>(rightIndex)], 0.0f);

    return juce::jlimit(-1.0f, 1.0f, leftValue + ((rightValue - leftValue) * fraction));
}

float Voice::evaluateModulationSource(int sourceIndex, float lfoValue, float lfo2Value, float modEnvelopeValue) const
{
    switch (sourceIndex)
    {
        case 1: return lfoValue;
        case 2: return modEnvelopeValue;
        case 3: return noteVelocity;
        case 4: return readParameter(macroTone, 0.0f);
        case 5: return readParameter(macroDirt, 0.0f);
        case 6: return readParameter(macroMotion, 0.0f);
        case 7: return readParameter(macroSpace, 0.0f);
        case 8: return readParameter(macroWeight, 0.0f);
        case 9: return readParameter(macroBounce, 0.0f);
        case 10: return readParameter(macroWarp, 0.0f);
        case 11: return readParameter(macroThrow, 0.0f);
        case 12: return lfoStepValue;
        case 13: return lfoSmoothRandomValue;
        case 14: return lfoChaosValue;
        case 15: return lfo2Value;
        default: return 0.0f;
    }
}

Voice::StereoSample Voice::renderUnisonStack(float osc1Gain, float osc2Gain)
{
    StereoSample sample;
    const auto activeUnisonVoices = getUnisonVoiceCount();
    const auto blend = activeUnisonVoices > 1 ? readParameter(unisonBlend, 0.65f) : 0.0f;
    const auto spread = readParameter(monoMode, 0.0f) >= 0.5f ? 0.0f : readParameter(unisonSpread, 0.0f);
    const auto voiceWeight = activeUnisonVoices > 1 ? juce::jmap(blend, 0.0f, 1.0f, 0.2f, 1.0f) : 1.0f;
    auto weightTotal = 0.0f;

    for (auto voiceIndex = 0; voiceIndex < activeUnisonVoices; ++voiceIndex)
    {
        const auto position = getUnisonPosition(voiceIndex, activeUnisonVoices);
        const auto pan = position * spread;
        const auto leftPan = pan <= 0.0f ? 1.0f : 1.0f - pan;
        const auto rightPan = pan >= 0.0f ? 1.0f : 1.0f + pan;
        const auto weight = activeUnisonVoices > 1 ? voiceWeight * (1.0f + (std::abs(position) * blend * 0.15f)) : 1.0f;
        const auto voiceSample = (oscillators[static_cast<size_t>(voiceIndex)].process() * osc1Gain)
            + (oscillators2[static_cast<size_t>(voiceIndex)].process() * osc2Gain);

        sample.left += voiceSample * weight * leftPan;
        sample.right += voiceSample * weight * rightPan;
        weightTotal += weight;
    }

    if (weightTotal > 1.0f)
    {
        const auto compensation = 1.0f / weightTotal;
        sample.left *= compensation;
        sample.right *= compensation;
    }

    return sample;
}

float Voice::processNoiseSample(int noiseTypeIndex)
{
    const auto raw = (noiseRandom.nextFloat() * 2.0f) - 1.0f;

    switch (noiseTypeIndex)
    {
        case 1: // Pink
            noiseColourState += (raw - noiseColourState) * 0.045f;
            return juce::jlimit(-1.0f, 1.0f, (noiseColourState * 1.8f) + (raw * 0.18f));

        case 2: // Brown
            noiseBrownState = juce::jlimit(-1.0f, 1.0f, (noiseBrownState * 0.996f) + (raw * 0.018f));
            return noiseBrownState;

        case 3: // Air
        {
            noiseColourState += (raw - noiseColourState) * 0.018f;
            return juce::jlimit(-1.0f, 1.0f, (raw - noiseColourState) * 1.35f);
        }

        case 4: // Tick
        {
            noiseColourState += (raw - noiseColourState) * 0.018f;
            const auto highPassed = juce::jlimit(-1.0f, 1.0f, (raw - noiseColourState) * 1.5f);
            const auto decaySeconds = juce::jlimit(0.005f, 0.6f, readParameter(noiseDecay, 0.18f));
            const auto decay = std::exp(-static_cast<float>(voiceAgeSamples)
                                        / juce::jmax(1.0f, static_cast<float>(currentSampleRate) * decaySeconds));
            return highPassed * decay;
        }

        case 5: // Vinyl
        {
            noiseColourState += (raw - noiseColourState) * 0.035f;
            noiseCrackleState *= 0.90f;
            if (noiseRandom.nextFloat() > 0.9965f)
                noiseCrackleState += ((noiseRandom.nextFloat() * 2.0f) - 1.0f) * 0.9f;

            return juce::jlimit(-1.0f, 1.0f, (noiseColourState * 0.55f) + (raw * 0.12f) + noiseCrackleState);
        }

        case 6: // Digital
        {
            if (--noiseDigitalHoldSamples <= 0)
            {
                noiseDigitalHeldSample = raw;
                noiseDigitalHoldSamples = juce::jmax(1, static_cast<int>(currentSampleRate / 6500.0));
            }

            return juce::jlimit(-1.0f, 1.0f, std::round(noiseDigitalHeldSample * 9.0f) / 9.0f);
        }

        case 0:
        default:
            return raw;
    }
}

int Voice::getUnisonVoiceCount() const
{
    return juce::jlimit(1, maxUnisonVoices, static_cast<int>(std::round(readParameter(unisonVoices, 1.0f))));
}

float Voice::getUnisonPosition(int voiceIndex, int voiceCount) const
{
    if (voiceCount <= 1)
        return 0.0f;

    return juce::jmap(static_cast<float>(voiceIndex), 0.0f, static_cast<float>(voiceCount - 1), -1.0f, 1.0f);
}

float Voice::frequencyForNote(int midiNoteNumber) const
{
    return static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));
}

float Voice::readParameter(std::atomic<float>* parameter, float fallback) const
{
    return parameter != nullptr ? parameter->load() : fallback;
}
}
