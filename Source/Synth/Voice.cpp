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
    osc2Warp = parameters.getRawParameterValue(Parameters::ID::osc2Warp);
    oscWarpMode = parameters.getRawParameterValue(Parameters::ID::oscWarpMode);
    osc2WarpMode = parameters.getRawParameterValue(Parameters::ID::osc2WarpMode);
    oscWavetablePosition = parameters.getRawParameterValue(Parameters::ID::oscWavetablePosition);
    osc2WavetablePosition = parameters.getRawParameterValue(Parameters::ID::osc2WavetablePosition);
    for (size_t index = 0; index < Oscillator::customWavePointCount; ++index)
    {
        oscCustomWave[index] = parameters.getRawParameterValue(Parameters::ID::oscCustomWave[index]);
        osc2CustomWave[index] = parameters.getRawParameterValue(Parameters::ID::osc2CustomWave[index]);
    }
    for (size_t frameIndex = 1; frameIndex < Oscillator::customWaveFrameCount; ++frameIndex)
    {
        for (size_t pointIndex = 0; pointIndex < Oscillator::customWavePointCount; ++pointIndex)
        {
            oscCustomWaveFrames[frameIndex - 1][pointIndex] =
                parameters.getRawParameterValue(Parameters::customWaveMorphFrameParameterID(false, frameIndex, pointIndex));
            osc2CustomWaveFrames[frameIndex - 1][pointIndex] =
                parameters.getRawParameterValue(Parameters::customWaveMorphFrameParameterID(true, frameIndex, pointIndex));
        }
    }
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
    stepLfoSync = parameters.getRawParameterValue(Parameters::ID::stepLfoSync);
    stepLfoSyncRate = parameters.getRawParameterValue(Parameters::ID::stepLfoSyncRate);
    stepLfoRate = parameters.getRawParameterValue(Parameters::ID::stepLfoRate);
    stepLfoDepth = parameters.getRawParameterValue(Parameters::ID::stepLfoDepth);
    stepLfoSlew = parameters.getRawParameterValue(Parameters::ID::stepLfoSlew);
    for (size_t index = 0; index < stepLfoValues.size(); ++index)
        stepLfoValues[index] = parameters.getRawParameterValue(Parameters::ID::stepLfoValue[index]);

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
        modMatrixPolarities[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixPolarity[index]);
        modMatrixCurves[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixCurve[index]);
        modMatrixRangeMins[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixRangeMin[index]);
        modMatrixRangeMaxes[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixRangeMax[index]);
        modMatrixSlews[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixSlew[index]);
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
    modRouteSmoothedValues.fill(0.0f);
    stepLfoPhase = 0.0f;
    stepLfoSmoothedValue = 0.0f;
}

void Voice::setHostBpm(double bpm) noexcept
{
    hostBpm = juce::jlimit(20.0, 300.0, bpm);
}

void Voice::setActiveVoiceLoad(int activeVoiceCount) noexcept
{
    activeVoiceLoad = juce::jlimit(1, 16, activeVoiceCount);
}

void Voice::setSequencerLock(int destinationIndex, float amount) noexcept
{
    sequencerLockDestination = destinationIndex;
    sequencerLockAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void Voice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    noteVelocity = velocity;
    notePosition = juce::jlimit(-1.0f, 1.0f, (static_cast<float>(midiNoteNumber) - 60.0f) / 36.0f);
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
    noiseTickEnvelope = 1.0f;
    noiseDigitalHoldSamples = 1;
    voiceAgeSamples = 0;
    controlSamplesUntilUpdate = 0;

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

    stepLfoPhase = 0.0f;
    stepLfoSmoothedValue = 0.0f;
    modRouteSmoothedValues.fill(0.0f);
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
    pitchBendNormalised = juce::jlimit(-1.0f, 1.0f, normalised);
    pitchBendSemitones = pitchBendNormalised * 2.0f;
}

void Voice::controllerMoved(int controllerNumber, int newControllerValue)
{
    if (controllerNumber == 1)
        modWheel = static_cast<float>(newControllerValue) / 127.0f;
    else if (controllerNumber == 121)
    {
        modWheel = 0.0f;
        aftertouch = 0.0f;
        pitchBendNormalised = 0.0f;
    }
}

void Voice::aftertouchChanged(int newAftertouchValue)
{
    aftertouch = static_cast<float>(juce::jlimit(0, 127, newAftertouchValue)) / 127.0f;
}

void Voice::channelPressureChanged(int newChannelPressureValue)
{
    aftertouch = static_cast<float>(juce::jlimit(0, 127, newChannelPressureValue)) / 127.0f;
}

void Voice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (! isVoiceActive())
        return;

    for (auto sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        const auto envelopeValue = ampEnvelope.process();
        if (controlSamplesUntilUpdate <= 0)
        {
            const auto samplesToAdvance = juce::jlimit(1,
                                                       controlUpdateIntervalSamples,
                                                       numSamples - sampleIndex);
            updateVoiceParameters(envelopeValue, samplesToAdvance);
            controlSamplesUntilUpdate = samplesToAdvance;
        }

        const auto noiseSample = currentNoiseGain > 0.0001f ? processNoiseSample(currentNoiseTypeIndex) : 0.0f;
        const auto unisonSample = renderUnisonStack();
        const auto centeredSample = ((currentSubGain > 0.0001f ? subOscillator.process() * currentSubGain : 0.0f) * 0.75f)
            + (noiseSample * currentNoiseGain * 0.45f);

        auto leftSample = (unisonSample.left + centeredSample) * currentSourceCompensation;
        auto rightSample = (unisonSample.right + centeredSample) * currentSourceCompensation;
        leftSample = leftFilter.process(leftSample);
        rightSample = rightFilter.process(rightSample);
        leftSample = distortion.process(leftSample, currentMacroDrive);
        rightSample = distortion.process(rightSample, currentMacroDrive);
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
        --controlSamplesUntilUpdate;

        if (! ampEnvelope.isActive())
        {
            clearCurrentNote();
            break;
        }
    }
}

bool Voice::refreshCustomWavetableFrames(const CustomWaveParameterPoints& baseFrameParameters,
                                         const CustomWaveMorphParameterFrames& morphFrameParameters,
                                         Oscillator::CustomWaveFrames& frameCache,
                                         bool& cacheInitialised)
{
    auto changed = ! cacheInitialised;

    for (size_t point = 0; point < Oscillator::customWavePointCount; ++point)
    {
        const auto value = readParameter(baseFrameParameters[point], 0.5f);
        if (! cacheInitialised || std::abs(frameCache[0][point] - value) > 0.000001f)
        {
            frameCache[0][point] = value;
            changed = true;
        }
    }

    for (size_t frameIndex = 1; frameIndex < Oscillator::customWaveFrameCount; ++frameIndex)
    {
        for (size_t point = 0; point < Oscillator::customWavePointCount; ++point)
        {
            const auto value = readParameter(morphFrameParameters[frameIndex - 1][point],
                                             frameCache[0][point]);
            if (! cacheInitialised || std::abs(frameCache[frameIndex][point] - value) > 0.000001f)
            {
                frameCache[frameIndex][point] = value;
                changed = true;
            }
        }
    }

    cacheInitialised = true;
    return changed;
}

void Voice::updateVoiceParameters(float envelopeValue, int samplesToAdvance)
{
    samplesToAdvance = juce::jmax(1, samplesToAdvance);
    updateGlide(samplesToAdvance);
    modEnvelope.setParameters(
        readParameter(modEnv1Attack, 0.01f),
        readParameter(modEnv1Decay, 0.22f),
        readParameter(modEnv1Sustain, 0.0f),
        readParameter(modEnv1Release, 0.12f));

    const auto lfoValue = processLfo(samplesToAdvance) * readParameter(lfo1Depth, 0.45f);
    const auto lfo2Value = processLfo2(samplesToAdvance) * readParameter(lfo2Depth, 0.25f);
    const auto stepLfoValue = processStepLfo(samplesToAdvance);
    auto modEnvelopeRaw = 0.0f;
    for (auto sample = 0; sample < samplesToAdvance; ++sample)
        modEnvelopeRaw = modEnvelope.process();
    const auto modEnvelopeValue = modEnvelopeRaw * readParameter(modEnv1Depth, 0.5f);
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

        const auto sourceValue = evaluateModulationSource(sourceIndex, lfoValue, lfo2Value, stepLfoValue, modEnvelopeValue);
        const auto contribution = Modulation::processRouteValue(sourceValue,
                                                                modMatrixPolarities[index],
                                                                modMatrixCurves[index],
                                                                modMatrixRangeMins[index],
                                                                modMatrixRangeMaxes[index],
                                                                modMatrixSlews[index],
                                                                modRouteSmoothedValues[index],
                                                                samplesToAdvance,
                                                                currentSampleRate)
            * amount;

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
    currentOsc1Gain = readParameter(osc1Level, 1.0f);
    currentOsc2Gain = juce::jlimit(0.0f, 1.0f, readParameter(osc2Level, 0.0f) + currentOsc2LevelOffset);

    const auto waveIndex = static_cast<int>(readParameter(oscWave, 1.0f));
    const auto waveform = static_cast<Waveform>(juce::jlimit(0, 7, waveIndex));

    const auto octaveOffset = static_cast<int>(readParameter(oscOctave, 0.0f)) * 12.0f;
    const auto tuneOffset = readParameter(oscTune, 0.0f);
    const auto pitchRatio = std::pow(2.0f, (octaveOffset + tuneOffset + pitchBendSemitones) / 12.0f);

    const auto osc2WaveIndex = static_cast<int>(readParameter(osc2Wave, 1.0f));
    const auto osc2Waveform = static_cast<Waveform>(juce::jlimit(0, 7, osc2WaveIndex));

    const auto osc2OctaveOffset = static_cast<int>(readParameter(osc2Octave, 0.0f)) * 12.0f;
    const auto motion = readParameter(macroMotion, 0.0f);
    const auto warp = readParameter(macroWarp, 0.0f);
    const auto osc2TuneOffset = readParameter(osc2Tune, 0.0f) + (motion * 5.0f) + (warp * 7.0f) + (osc2TuneMod * 12.0f);
    const auto osc2PitchRatio = std::pow(2.0f, (osc2OctaveOffset + osc2TuneOffset + pitchBendSemitones) / 12.0f);
    const auto oscillatorWarpAmount = juce::jlimit(0.0f, 1.0f, readParameter(oscWarp, 0.0f) + (warp * 0.32f) + (oscWarpMod * 0.55f) + (sequenceWarpMod * 0.45f));
    const auto oscillator2WarpAmount = juce::jlimit(0.0f, 1.0f, readParameter(osc2Warp, 0.0f) + (warp * 0.32f) + (oscWarpMod * 0.55f) + (sequenceWarpMod * 0.45f));
    const auto oscillatorWarpMode = juce::jlimit(0, 3, juce::roundToInt(readParameter(oscWarpMode, 0.0f)));
    const auto oscillator2WarpMode = juce::jlimit(0, 3, juce::roundToInt(readParameter(osc2WarpMode, 0.0f)));
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
    const auto osc1UsesCustomWave = waveform == Waveform::custom;
    const auto osc2UsesCustomWave = osc2Waveform == Waveform::custom;
    const auto shouldRefreshCustomFrames = (voiceAgeSamples % 64) == 0;
    const auto osc1CustomFramesChanged = osc1UsesCustomWave
        && (! osc1CustomFramesInitialised
            || shouldRefreshCustomFrames
            || osc1CustomFramesAppliedVoices <= 0)
        && refreshCustomWavetableFrames(oscCustomWave,
                                        oscCustomWaveFrames,
                                        osc1CustomFrameCache,
                                        osc1CustomFramesInitialised);
    const auto osc2CustomFramesChanged = osc2UsesCustomWave
        && (! osc2CustomFramesInitialised
            || shouldRefreshCustomFrames
            || osc2CustomFramesAppliedVoices <= 0)
        && refreshCustomWavetableFrames(osc2CustomWave,
                                        osc2CustomWaveFrames,
                                        osc2CustomFrameCache,
                                        osc2CustomFramesInitialised);

    currentActiveUnisonVoices = getBudgetedUnisonVoiceCount();
    const auto detuneCents = readParameter(unisonDetune, 0.0f) * 24.0f;
    const auto shouldApplyOsc1CustomFrames = osc1UsesCustomWave
        && (osc1CustomFramesChanged || osc1CustomFramesAppliedVoices < currentActiveUnisonVoices);
    const auto shouldApplyOsc2CustomFrames = osc2UsesCustomWave
        && (osc2CustomFramesChanged || osc2CustomFramesAppliedVoices < currentActiveUnisonVoices);
    for (auto voiceIndex = 0; voiceIndex < currentActiveUnisonVoices; ++voiceIndex)
    {
        const auto detuneRatio = std::pow(2.0f, (getUnisonPosition(voiceIndex, currentActiveUnisonVoices) * detuneCents) / 1200.0f);
        oscillators[static_cast<size_t>(voiceIndex)].setWaveform(waveform);
        oscillators[static_cast<size_t>(voiceIndex)].setWarp(oscillatorWarpAmount);
        oscillators[static_cast<size_t>(voiceIndex)].setWarpMode(oscillatorWarpMode);
        oscillators[static_cast<size_t>(voiceIndex)].setWavetablePosition(osc1WavetablePosition);
        if (shouldApplyOsc1CustomFrames)
            oscillators[static_cast<size_t>(voiceIndex)].setCustomWavetableFrames(osc1CustomFrameCache);
        oscillators[static_cast<size_t>(voiceIndex)].setFrequency(currentFrequencyHz * pitchRatio * detuneRatio);
        oscillators2[static_cast<size_t>(voiceIndex)].setWaveform(osc2Waveform);
        oscillators2[static_cast<size_t>(voiceIndex)].setWarp(oscillator2WarpAmount);
        oscillators2[static_cast<size_t>(voiceIndex)].setWarpMode(oscillator2WarpMode);
        oscillators2[static_cast<size_t>(voiceIndex)].setWavetablePosition(osc2WavetablePositionValue);
        if (shouldApplyOsc2CustomFrames)
            oscillators2[static_cast<size_t>(voiceIndex)].setCustomWavetableFrames(osc2CustomFrameCache);
        oscillators2[static_cast<size_t>(voiceIndex)].setFrequency(currentFrequencyHz * osc2PitchRatio * detuneRatio);
    }

    const auto blend = currentActiveUnisonVoices > 1 ? readParameter(unisonBlend, 0.65f) : 0.0f;
    const auto spread = readParameter(monoMode, 0.0f) >= 0.5f ? 0.0f : readParameter(unisonSpread, 0.0f);
    const auto voiceWeight = currentActiveUnisonVoices > 1 ? juce::jmap(blend, 0.0f, 1.0f, 0.2f, 1.0f) : 1.0f;
    auto weightTotal = 0.0f;
    for (auto voiceIndex = 0; voiceIndex < currentActiveUnisonVoices; ++voiceIndex)
    {
        const auto position = getUnisonPosition(voiceIndex, currentActiveUnisonVoices);
        const auto pan = position * spread;
        const auto leftPan = pan <= 0.0f ? 1.0f : 1.0f - pan;
        const auto rightPan = pan >= 0.0f ? 1.0f : 1.0f + pan;
        const auto weight = currentActiveUnisonVoices > 1 ? voiceWeight * (1.0f + (std::abs(position) * blend * 0.15f)) : 1.0f;
        currentUnisonLeftGains[static_cast<size_t>(voiceIndex)] = weight * leftPan;
        currentUnisonRightGains[static_cast<size_t>(voiceIndex)] = weight * rightPan;
        weightTotal += weight;
    }

    if (weightTotal > 1.0f)
    {
        const auto compensation = 1.0f / weightTotal;
        for (auto voiceIndex = 0; voiceIndex < currentActiveUnisonVoices; ++voiceIndex)
        {
            currentUnisonLeftGains[static_cast<size_t>(voiceIndex)] *= compensation;
            currentUnisonRightGains[static_cast<size_t>(voiceIndex)] *= compensation;
        }
    }

    osc1CustomFramesAppliedVoices = osc1UsesCustomWave ? juce::jmax(osc1CustomFramesAppliedVoices, currentActiveUnisonVoices) : 0;
    osc2CustomFramesAppliedVoices = osc2UsesCustomWave ? juce::jmax(osc2CustomFramesAppliedVoices, currentActiveUnisonVoices) : 0;

    subOscillator.setFrequency(currentFrequencyHz * 0.5f);

    ampEnvelope.setParameters(
        readParameter(ampAttack, 0.01f),
        readParameter(ampDecay, 0.18f),
        readParameter(ampSustain, 0.65f),
        readParameter(ampRelease, 0.22f));

    const auto tone = readParameter(macroTone, 0.0f);
    const auto dirt = readParameter(macroDirt, 0.0f);
    const auto weight = readParameter(macroWeight, 0.0f);
    currentSubGain = juce::jlimit(0.0f, 1.15f, readParameter(subLevel, 0.0f) + (weight * 0.32f));
    currentNoiseGain = readParameter(noiseLevel, 0.0f);
    currentNoiseTypeIndex = juce::jlimit(0, 6, static_cast<int>(std::round(readParameter(noiseType, 0.0f))));
    currentMacroDrive = juce::jlimit(0.0f, 0.95f, readParameter(driveAmount, 0.18f) + (dirt * 0.55f) + currentDriveOffset);
    const auto sourceWeight = currentOsc1Gain + currentOsc2Gain + (currentSubGain * 0.75f) + (currentNoiseGain * 0.45f);
    currentSourceCompensation = 1.0f / juce::jmax(1.0f, sourceWeight);
    const auto decaySeconds = juce::jlimit(0.005f, 0.6f, readParameter(noiseDecay, 0.18f));
    noiseTickDecayMultiplier = std::exp(-1.0f / juce::jmax(1.0f, static_cast<float>(currentSampleRate) * decaySeconds));

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

void Voice::updateGlide(int samplesToAdvance)
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
    glideSamplesRemaining = juce::jmax(0, glideSamplesRemaining - juce::jmax(1, samplesToAdvance));
}

float Voice::processLfo(int samplesToAdvance)
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
                                           (rateHz * static_cast<float>(juce::jmax(1, samplesToAdvance)))
                                               / static_cast<float>(juce::jmax(1.0, currentSampleRate)));

    const auto previousPhase = lfoPhase;
    lfoPhase += (juce::jlimit(0.01f, 80.0f, rateHz) * static_cast<float>(juce::jmax(1, samplesToAdvance)))
        / static_cast<float>(currentSampleRate);

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

float Voice::processLfo2(int samplesToAdvance)
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
    lfo2Phase += (juce::jlimit(0.01f, 80.0f, rateHz) * static_cast<float>(juce::jmax(1, samplesToAdvance)))
        / static_cast<float>(currentSampleRate);

    if (lfo2Phase >= 1.0f)
    {
        lfo2Phase -= std::floor(lfo2Phase);
        if (previousPhase < 1.0f)
            lfo2StepValue = (modulationRandom.nextFloat() * 2.0f) - 1.0f;
    }

    return juce::jlimit(-1.0f, 1.0f, value);
}

float Voice::processStepLfo(int samplesToAdvance)
{
    return Modulation::processStepLfo(stepLfoSync,
                                      stepLfoSyncRate,
                                      stepLfoRate,
                                      stepLfoDepth,
                                      stepLfoSlew,
                                      stepLfoValues,
                                      stepLfoPhase,
                                      stepLfoSmoothedValue,
                                      samplesToAdvance,
                                      currentSampleRate,
                                      hostBpm);
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

float Voice::evaluateModulationSource(int sourceIndex, float lfoValue, float lfo2Value, float stepLfoValue, float modEnvelopeValue) const
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
        case 16: return modWheel;
        case 17: return aftertouch;
        case 18: return pitchBendNormalised;
        case 19: return notePosition;
        case Modulation::stepLfoSourceIndex: return stepLfoValue;
        default: return 0.0f;
    }
}

Voice::StereoSample Voice::renderUnisonStack()
{
    StereoSample sample;
    const auto useOsc1 = currentOsc1Gain > 0.0001f;
    const auto useOsc2 = currentOsc2Gain > 0.0001f;

    if (! useOsc1 && ! useOsc2)
        return sample;

    for (auto voiceIndex = 0; voiceIndex < currentActiveUnisonVoices; ++voiceIndex)
    {
        auto voiceSample = 0.0f;
        if (useOsc1)
            voiceSample += oscillators[static_cast<size_t>(voiceIndex)].process() * currentOsc1Gain;
        if (useOsc2)
            voiceSample += oscillators2[static_cast<size_t>(voiceIndex)].process() * currentOsc2Gain;

        sample.left += voiceSample * currentUnisonLeftGains[static_cast<size_t>(voiceIndex)];
        sample.right += voiceSample * currentUnisonRightGains[static_cast<size_t>(voiceIndex)];
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
            const auto output = highPassed * noiseTickEnvelope;
            noiseTickEnvelope *= noiseTickDecayMultiplier;
            return output;
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

int Voice::getBudgetedUnisonVoiceCount() const
{
    const auto requested = getUnisonVoiceCount();
    const auto loadLimit = activeVoiceLoad >= 7 ? 2
        : activeVoiceLoad >= 5 ? 3
        : activeVoiceLoad >= 3 ? 5
        : maxUnisonVoices;

    return juce::jlimit(1, maxUnisonVoices, juce::jmin(requested, loadLimit));
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
