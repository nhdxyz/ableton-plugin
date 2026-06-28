#include "EffectsRack.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
float onePoleAlpha(float frequency, double sampleRate)
{
    const auto safeFrequency = juce::jlimit(1.0f, 20000.0f, frequency);
    const auto safeSampleRate = juce::jmax(1.0, sampleRate);
    return 1.0f - std::exp((-juce::MathConstants<float>::twoPi * safeFrequency) / static_cast<float>(safeSampleRate));
}

double pumpCyclesPerBeat(int rateIndex)
{
    switch (rateIndex)
    {
        case 1: return 2.0; // 1/8
        case 2: return 3.0; // 1/8 triplet
        case 3: return 4.0; // 1/16
        default: return 1.0; // 1/4
    }
}

double lfoCyclesPerBeat(int rateIndex)
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

double delaySecondsForRate(int rateIndex, double bpm)
{
    const auto quarterNoteSeconds = 60.0 / juce::jlimit(20.0, 300.0, bpm);

    switch (rateIndex)
    {
        case 0: return quarterNoteSeconds;          // 1/4
        case 2: return quarterNoteSeconds * 0.75;   // 1/8 dotted
        case 3: return quarterNoteSeconds / 3.0;    // 1/8 triplet
        case 4: return quarterNoteSeconds * 0.25;   // 1/16
        case 5: return quarterNoteSeconds * 0.375;  // 1/16 dotted
        case 1:
        default:
            return quarterNoteSeconds * 0.5;        // 1/8
    }
}

float customPumpDuckAmount(float recovery, const std::array<float, 8>& customCurve)
{
    const auto safeRecovery = juce::jlimit(0.0f, 1.0f, recovery);
    const auto scaledIndex = safeRecovery * static_cast<float>(customCurve.size() - 1);
    const auto leftIndex = juce::jlimit(0, static_cast<int>(customCurve.size() - 1), static_cast<int>(std::floor(scaledIndex)));
    const auto rightIndex = juce::jmin(leftIndex + 1, static_cast<int>(customCurve.size() - 1));
    const auto amount = scaledIndex - static_cast<float>(leftIndex);

    return juce::jlimit(0.0f, 1.0f, juce::jmap(amount, customCurve[static_cast<size_t>(leftIndex)], customCurve[static_cast<size_t>(rightIndex)]));
}

float pumpDuckAmount(float recovery, float shape, int curveIndex, const std::array<float, 8>& customCurve)
{
    const auto safeRecovery = juce::jlimit(0.0f, 1.0f, recovery);
    const auto safeShape = juce::jlimit(0.0f, 1.0f, shape);
    const auto inverseRecovery = 1.0f - safeRecovery;

    switch (curveIndex)
    {
        case 1: // Tight
            return std::pow(inverseRecovery, juce::jmap(safeShape, 2.2f, 8.0f));

        case 2: // Garage
        {
            const auto hold = juce::jmap(safeShape, 0.10f, 0.28f);
            if (safeRecovery <= hold)
                return 1.0f;

            const auto release = juce::jlimit(0.0f, 1.0f, (safeRecovery - hold) / juce::jmax(0.001f, 1.0f - hold));
            return std::pow(1.0f - release, juce::jmap(safeShape, 1.1f, 4.0f));
        }

        case 3: // Stutter
        {
            const auto pulsePhase = std::fmod(safeRecovery * 4.0f, 1.0f);
            const auto pulseDuck = pulsePhase < 0.32f
                ? 1.0f
                : std::pow(1.0f - ((pulsePhase - 0.32f) / 0.68f), juce::jmap(safeShape, 1.0f, 6.0f));
            const auto baseDuck = std::pow(inverseRecovery, juce::jmap(safeShape, 0.6f, 2.0f));
            return juce::jlimit(0.0f, 1.0f, juce::jmax(baseDuck * 0.45f, pulseDuck * 0.7f));
        }

        case 4: // Gate
        {
            const auto gateLength = juce::jmap(safeShape, 0.22f, 0.58f);
            if (safeRecovery <= gateLength)
                return 1.0f;

            const auto tail = juce::jlimit(0.0f, 1.0f, (safeRecovery - gateLength) / juce::jmax(0.001f, 1.0f - gateLength));
            return std::pow(1.0f - tail, 8.0f);
        }

        case 5: // Custom
            return customPumpDuckAmount(safeRecovery, customCurve);

        case 0:
        default:
            return std::pow(inverseRecovery, juce::jmap(safeShape, 0.35f, 4.5f));
    }
}
}

namespace Effects
{
EffectsRack::EffectsRack(Parameters::APVTS& state)
    : parameters(state)
{
    fxDistortionEnabled = parameters.getRawParameterValue(Parameters::ID::fxDistortionEnabled);
    fxDistortionAmount = parameters.getRawParameterValue(Parameters::ID::fxDistortionAmount);
    fxBitcrushEnabled = parameters.getRawParameterValue(Parameters::ID::fxBitcrushEnabled);
    fxBitcrushBits = parameters.getRawParameterValue(Parameters::ID::fxBitcrushBits);
    fxBitcrushDownsample = parameters.getRawParameterValue(Parameters::ID::fxBitcrushDownsample);
    fxBitcrushMix = parameters.getRawParameterValue(Parameters::ID::fxBitcrushMix);
    fxPumpEnabled = parameters.getRawParameterValue(Parameters::ID::fxPumpEnabled);
    fxPumpRate = parameters.getRawParameterValue(Parameters::ID::fxPumpRate);
    fxPumpCurve = parameters.getRawParameterValue(Parameters::ID::fxPumpCurve);
    for (size_t index = 0; index < fxPumpCustomCurve.size(); ++index)
        fxPumpCustomCurve[index] = parameters.getRawParameterValue(Parameters::ID::fxPumpCustomCurve[index]);
    fxPumpDepth = parameters.getRawParameterValue(Parameters::ID::fxPumpDepth);
    fxPumpShape = parameters.getRawParameterValue(Parameters::ID::fxPumpShape);
    fxPumpPhase = parameters.getRawParameterValue(Parameters::ID::fxPumpPhase);
    fxTremoloEnabled = parameters.getRawParameterValue(Parameters::ID::fxTremoloEnabled);
    fxTremoloRate = parameters.getRawParameterValue(Parameters::ID::fxTremoloRate);
    fxTremoloDepth = parameters.getRawParameterValue(Parameters::ID::fxTremoloDepth);
    fxTremoloPan = parameters.getRawParameterValue(Parameters::ID::fxTremoloPan);
    fxTremoloShape = parameters.getRawParameterValue(Parameters::ID::fxTremoloShape);
    fxTremoloPhase = parameters.getRawParameterValue(Parameters::ID::fxTremoloPhase);
    fxRingEnabled = parameters.getRawParameterValue(Parameters::ID::fxRingEnabled);
    fxRingFrequency = parameters.getRawParameterValue(Parameters::ID::fxRingFrequency);
    fxRingDepth = parameters.getRawParameterValue(Parameters::ID::fxRingDepth);
    fxRingMix = parameters.getRawParameterValue(Parameters::ID::fxRingMix);
    fxRingBias = parameters.getRawParameterValue(Parameters::ID::fxRingBias);
    fxCombEnabled = parameters.getRawParameterValue(Parameters::ID::fxCombEnabled);
    fxCombFrequency = parameters.getRawParameterValue(Parameters::ID::fxCombFrequency);
    fxCombFeedback = parameters.getRawParameterValue(Parameters::ID::fxCombFeedback);
    fxCombDamping = parameters.getRawParameterValue(Parameters::ID::fxCombDamping);
    fxCombMix = parameters.getRawParameterValue(Parameters::ID::fxCombMix);
    fxChorusEnabled = parameters.getRawParameterValue(Parameters::ID::fxChorusEnabled);
    fxChorusRate = parameters.getRawParameterValue(Parameters::ID::fxChorusRate);
    fxChorusDepth = parameters.getRawParameterValue(Parameters::ID::fxChorusDepth);
    fxChorusMix = parameters.getRawParameterValue(Parameters::ID::fxChorusMix);
    fxDelayEnabled = parameters.getRawParameterValue(Parameters::ID::fxDelayEnabled);
    fxDelaySync = parameters.getRawParameterValue(Parameters::ID::fxDelaySync);
    fxDelayRate = parameters.getRawParameterValue(Parameters::ID::fxDelayRate);
    fxDelayTime = parameters.getRawParameterValue(Parameters::ID::fxDelayTime);
    fxDelayFeedback = parameters.getRawParameterValue(Parameters::ID::fxDelayFeedback);
    fxDelayMix = parameters.getRawParameterValue(Parameters::ID::fxDelayMix);
    fxReverbEnabled = parameters.getRawParameterValue(Parameters::ID::fxReverbEnabled);
    fxReverbSize = parameters.getRawParameterValue(Parameters::ID::fxReverbSize);
    fxReverbDamping = parameters.getRawParameterValue(Parameters::ID::fxReverbDamping);
    fxReverbMix = parameters.getRawParameterValue(Parameters::ID::fxReverbMix);
    fxWidthEnabled = parameters.getRawParameterValue(Parameters::ID::fxWidthEnabled);
    fxWidthAmount = parameters.getRawParameterValue(Parameters::ID::fxWidthAmount);
    fxWidthMonoCutoff = parameters.getRawParameterValue(Parameters::ID::fxWidthMonoCutoff);
    fxToneEnabled = parameters.getRawParameterValue(Parameters::ID::fxToneEnabled);
    fxToneTilt = parameters.getRawParameterValue(Parameters::ID::fxToneTilt);
    fxToneLowCut = parameters.getRawParameterValue(Parameters::ID::fxToneLowCut);
    fxEqEnabled = parameters.getRawParameterValue(Parameters::ID::fxEqEnabled);
    fxEqLowGain = parameters.getRawParameterValue(Parameters::ID::fxEqLowGain);
    fxEqMidGain = parameters.getRawParameterValue(Parameters::ID::fxEqMidGain);
    fxEqHighGain = parameters.getRawParameterValue(Parameters::ID::fxEqHighGain);
    fxEqTrim = parameters.getRawParameterValue(Parameters::ID::fxEqTrim);
    fxPhaserEnabled = parameters.getRawParameterValue(Parameters::ID::fxPhaserEnabled);
    fxPhaserRate = parameters.getRawParameterValue(Parameters::ID::fxPhaserRate);
    fxPhaserDepth = parameters.getRawParameterValue(Parameters::ID::fxPhaserDepth);
    fxPhaserMix = parameters.getRawParameterValue(Parameters::ID::fxPhaserMix);
    fxGuardEnabled = parameters.getRawParameterValue(Parameters::ID::fxGuardEnabled);
    fxGuardPush = parameters.getRawParameterValue(Parameters::ID::fxGuardPush);
    fxGuardCeiling = parameters.getRawParameterValue(Parameters::ID::fxGuardCeiling);
    fxFlangerEnabled = parameters.getRawParameterValue(Parameters::ID::fxFlangerEnabled);
    fxFlangerRate = parameters.getRawParameterValue(Parameters::ID::fxFlangerRate);
    fxFlangerDepth = parameters.getRawParameterValue(Parameters::ID::fxFlangerDepth);
    fxFlangerFeedback = parameters.getRawParameterValue(Parameters::ID::fxFlangerFeedback);
    fxFlangerMix = parameters.getRawParameterValue(Parameters::ID::fxFlangerMix);
    for (size_t index = 0; index < fxOrder.size(); ++index)
        fxOrder[index] = parameters.getRawParameterValue(Parameters::ID::fxOrder[index]);
    for (size_t index = 0; index < modMatrixSources.size(); ++index)
    {
        modMatrixSources[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixSource[index]);
        modMatrixDestinations[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixDestination[index]);
        modMatrixAmounts[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixAmount[index]);
        modMatrixEnabled[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixEnabled[index]);
    }
    for (size_t index = 0; index < lfo1CurvePoints.size(); ++index)
        lfo1CurvePoints[index] = parameters.getRawParameterValue(Parameters::ID::lfo1Curve[index]);

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
    lfo2Rate = parameters.getRawParameterValue(Parameters::ID::lfo2Rate);
    lfo2Sync = parameters.getRawParameterValue(Parameters::ID::lfo2Sync);
    lfo2SyncRate = parameters.getRawParameterValue(Parameters::ID::lfo2SyncRate);
    lfo2Shape = parameters.getRawParameterValue(Parameters::ID::lfo2Shape);
    lfo2Depth = parameters.getRawParameterValue(Parameters::ID::lfo2Depth);
    lfo2Phase = parameters.getRawParameterValue(Parameters::ID::lfo2Phase);
}

void EffectsRack::prepare(double sampleRate, int maximumBlockSize, int numChannels)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    preparedChannels = juce::jmax(1, numChannels);

    juce::dsp::ProcessSpec spec {
        currentSampleRate,
        static_cast<juce::uint32>(juce::jmax(1, maximumBlockSize)),
        static_cast<juce::uint32>(preparedChannels)
    };

    phaser.prepare(spec);
    flanger.prepare(spec);
    chorus.prepare(spec);
    reverb.setSampleRate(currentSampleRate);
    delayBuffer.setSize(preparedChannels, static_cast<int>(std::ceil(currentSampleRate * 2.0)));
    combBuffer.setSize(preparedChannels, static_cast<int>(std::ceil(currentSampleRate * 0.12)));
    toneLowCutState.assign(static_cast<size_t>(preparedChannels), 0.0f);
    toneTiltState.assign(static_cast<size_t>(preparedChannels), 0.0f);
    eqLowState.assign(static_cast<size_t>(preparedChannels), 0.0f);
    eqHighState.assign(static_cast<size_t>(preparedChannels), 0.0f);
    combDampingState.assign(static_cast<size_t>(preparedChannels), 0.0f);
    bitcrushHeldSample.assign(static_cast<size_t>(preparedChannels), 0.0f);
    bitcrushHoldCounter.assign(static_cast<size_t>(preparedChannels), 0);
    widthLowState.assign(static_cast<size_t>(preparedChannels), 0.0f);
    reset();
}

void EffectsRack::reset()
{
    phaser.reset();
    flanger.reset();
    chorus.reset();
    reverb.reset();
    delayBuffer.clear();
    combBuffer.clear();
    std::fill(toneLowCutState.begin(), toneLowCutState.end(), 0.0f);
    std::fill(toneTiltState.begin(), toneTiltState.end(), 0.0f);
    std::fill(eqLowState.begin(), eqLowState.end(), 0.0f);
    std::fill(eqHighState.begin(), eqHighState.end(), 0.0f);
    std::fill(combDampingState.begin(), combDampingState.end(), 0.0f);
    std::fill(bitcrushHeldSample.begin(), bitcrushHeldSample.end(), 0.0f);
    std::fill(bitcrushHoldCounter.begin(), bitcrushHoldCounter.end(), 0);
    std::fill(widthLowState.begin(), widthLowState.end(), 0.0f);
    pumpPhase = 0.0;
    tremoloPhase = 0.0;
    ringPhase = 0.0;
    fxModLfoPhase = 0.0f;
    fxModLfoStepValue = (fxModulationRandom.nextFloat() * 2.0f) - 1.0f;
    fxModSmoothRandomStartValue = fxModLfoStepValue;
    fxModSmoothRandomValue = fxModLfoStepValue;
    fxModChaosValue = ((fxModulationRandom.nextFloat() * 2.0f) - 1.0f) * 0.25f;
    fxModLfo2Phase = 0.0f;
    fxModLfo2StepValue = (fxModulationRandom.nextFloat() * 2.0f) - 1.0f;
    sequencerLockDestination = 0;
    sequencerLockAmount = 0.0f;
    pumpSmoothedGain = 1.0f;
    pumpMeterPhase.store(0.0f, std::memory_order_relaxed);
    pumpMeterGain.store(1.0f, std::memory_order_relaxed);
    pumpMeterReduction.store(0.0f, std::memory_order_relaxed);
    pumpMeterActive.store(false, std::memory_order_relaxed);
    delayWritePosition = 0;
    combWritePosition = 0;
}

void EffectsRack::setSequencerLock(int destinationIndex, float amount) noexcept
{
    sequencerLockDestination = destinationIndex;
    sequencerLockAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void EffectsRack::process(juce::AudioBuffer<float>& buffer, float outputGainDb, double bpm, std::optional<double> ppqPosition)
{
    updateFxModulation(buffer.getNumSamples(), bpm, ppqPosition);

    const auto moduleOrder = orderedModuleIndices();
    for (const auto moduleIndex : moduleOrder)
        processModule(moduleIndex, buffer, bpm, ppqPosition);

    applyOutputGainAndSafety(buffer, outputGainDb);
}

void EffectsRack::getPumpMeterLevels(float& phase, float& gain, float& reduction, bool& active) const noexcept
{
    phase = pumpMeterPhase.load(std::memory_order_relaxed);
    gain = pumpMeterGain.load(std::memory_order_relaxed);
    reduction = pumpMeterReduction.load(std::memory_order_relaxed);
    active = pumpMeterActive.load(std::memory_order_relaxed);
}

void EffectsRack::updateFxModulation(int numSamples, double bpm, std::optional<double> ppqPosition)
{
    fxModulation = {};

    const auto lfoValue = processFxModulationLfo(numSamples, bpm, ppqPosition);
    const auto lfo2Value = processFxModulationLfo2(numSamples, bpm, ppqPosition);

    for (size_t index = 0; index < modMatrixSources.size(); ++index)
    {
        const auto sourceIndex = static_cast<int>(std::round(readParameter(modMatrixSources[index], 0.0f)));
        const auto destinationIndex = static_cast<int>(std::round(readParameter(modMatrixDestinations[index], 0.0f)));
        const auto amount = readParameter(modMatrixAmounts[index], 0.0f);
        const auto enabled = readParameter(modMatrixEnabled[index], 1.0f) >= 0.5f;

        if (! enabled || sourceIndex == 0 || destinationIndex < 7 || std::abs(amount) <= 0.0001f)
            continue;

        const auto contribution = evaluateFxModulationSource(sourceIndex, lfoValue, lfo2Value) * amount;

        switch (destinationIndex)
        {
            case 7: fxModulation.pumpDepth += contribution; break;
            case 8: fxModulation.delayMix += contribution; break;
            case 9: fxModulation.reverbMix += contribution; break;
            case 10: fxModulation.width += contribution; break;
            case 11: fxModulation.drive += contribution; break;
            default: break;
        }
    }

    switch (sequencerLockDestination)
    {
        case 2: fxModulation.drive += sequencerLockAmount; break;
        case 4: fxModulation.pumpDepth += sequencerLockAmount; break;
        case 5: fxModulation.delayMix += sequencerLockAmount; break;
        case 6: fxModulation.reverbMix += sequencerLockAmount; break;
        default: break;
    }

    fxModulation.drive = juce::jlimit(-1.0f, 1.0f, fxModulation.drive);
    fxModulation.pumpDepth = juce::jlimit(-1.0f, 1.0f, fxModulation.pumpDepth);
    fxModulation.delayMix = juce::jlimit(-1.0f, 1.0f, fxModulation.delayMix);
    fxModulation.reverbMix = juce::jlimit(-1.0f, 1.0f, fxModulation.reverbMix);
    fxModulation.width = juce::jlimit(-1.0f, 1.0f, fxModulation.width);
}

float EffectsRack::processFxModulationLfo(int numSamples, double bpm, std::optional<double> ppqPosition)
{
    const auto shapeIndex = static_cast<int>(std::round(readParameter(lfo1Shape, 0.0f)));
    const auto syncEnabled = readParameter(lfo1Sync, 1.0f) >= 0.5f;
    const auto rateHz = syncEnabled
        ? static_cast<float>((juce::jlimit(20.0, 300.0, bpm) / 60.0) * lfoCyclesPerBeat(static_cast<int>(std::round(readParameter(lfo1SyncRate, 1.0f)))))
        : readParameter(lfo1Rate, 1.0f);
    const auto phaseOffset = readParameter(lfo1Phase, 0.0f);

    if (syncEnabled && ppqPosition.has_value())
    {
        fxModLfoPhase = static_cast<float>(std::fmod(*ppqPosition * lfoCyclesPerBeat(static_cast<int>(std::round(readParameter(lfo1SyncRate, 1.0f)))), 1.0));
        if (fxModLfoPhase < 0.0f)
            fxModLfoPhase += 1.0f;
    }

    auto phase = std::fmod(fxModLfoPhase + phaseOffset + 1.0f, 1.0f);
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
            value = fxModLfoStepValue;
            break;

        case 5:
            value = evaluateFxLfoCurve(phase);
            break;

        default:
            value = std::sin(juce::MathConstants<float>::twoPi * phase);
            break;
    }

    const auto smoothProgress = phase * phase * (3.0f - (2.0f * phase));
    fxModSmoothRandomValue = juce::jlimit(-1.0f,
                                          1.0f,
                                          fxModSmoothRandomStartValue
                                              + ((fxModLfoStepValue - fxModSmoothRandomStartValue) * smoothProgress));
    fxModChaosValue = advanceChaosRandomWalk(fxModChaosValue,
                                             fxModulationRandom,
                                             (juce::jlimit(0.01f, 80.0f, rateHz)
                                                * static_cast<float>(juce::jmax(1, numSamples)))
                                                / static_cast<float>(juce::jmax(1.0, currentSampleRate)));

    const auto previousPhase = fxModLfoPhase;
    fxModLfoPhase += (juce::jlimit(0.01f, 80.0f, rateHz) * static_cast<float>(juce::jmax(1, numSamples))) / static_cast<float>(currentSampleRate);

    if (fxModLfoPhase >= 1.0f)
    {
        fxModLfoPhase -= std::floor(fxModLfoPhase);
        if (previousPhase < 1.0f)
        {
            fxModSmoothRandomStartValue = fxModLfoStepValue;
            fxModLfoStepValue = (fxModulationRandom.nextFloat() * 2.0f) - 1.0f;
        }
    }

    return juce::jlimit(-1.0f, 1.0f, value) * readParameter(lfo1Depth, 0.45f);
}

float EffectsRack::processFxModulationLfo2(int numSamples, double bpm, std::optional<double> ppqPosition)
{
    const auto shapeIndex = static_cast<int>(std::round(readParameter(lfo2Shape, 1.0f)));
    const auto syncEnabled = readParameter(lfo2Sync, 1.0f) >= 0.5f;
    const auto syncRateIndex = static_cast<int>(std::round(readParameter(lfo2SyncRate, 3.0f)));
    const auto rateHz = syncEnabled
        ? static_cast<float>((juce::jlimit(20.0, 300.0, bpm) / 60.0) * lfoCyclesPerBeat(syncRateIndex))
        : readParameter(lfo2Rate, 1.5f);
    const auto phaseOffset = readParameter(lfo2Phase, 0.25f);

    if (syncEnabled && ppqPosition.has_value())
    {
        fxModLfo2Phase = static_cast<float>(std::fmod(*ppqPosition * lfoCyclesPerBeat(syncRateIndex), 1.0));
        if (fxModLfo2Phase < 0.0f)
            fxModLfo2Phase += 1.0f;
    }

    const auto phase = std::fmod(fxModLfo2Phase + phaseOffset + 1.0f, 1.0f);
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
            value = fxModLfo2StepValue;
            break;

        default:
            value = std::sin(juce::MathConstants<float>::twoPi * phase);
            break;
    }

    const auto previousPhase = fxModLfo2Phase;
    fxModLfo2Phase += (juce::jlimit(0.01f, 80.0f, rateHz) * static_cast<float>(juce::jmax(1, numSamples))) / static_cast<float>(currentSampleRate);

    if (fxModLfo2Phase >= 1.0f)
    {
        fxModLfo2Phase -= std::floor(fxModLfo2Phase);
        if (previousPhase < 1.0f)
            fxModLfo2StepValue = (fxModulationRandom.nextFloat() * 2.0f) - 1.0f;
    }

    return juce::jlimit(-1.0f, 1.0f, value) * readParameter(lfo2Depth, 0.25f);
}

float EffectsRack::evaluateFxLfoCurve(float phase) const
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

float EffectsRack::evaluateFxModulationSource(int sourceIndex, float lfoValue, float lfo2Value) const
{
    switch (sourceIndex)
    {
        case 1: return lfoValue;
        case 4: return readParameter(macroTone, 0.0f);
        case 5: return readParameter(macroDirt, 0.0f);
        case 6: return readParameter(macroMotion, 0.0f);
        case 7: return readParameter(macroSpace, 0.0f);
        case 8: return readParameter(macroWeight, 0.0f);
        case 9: return readParameter(macroBounce, 0.0f);
        case 10: return readParameter(macroWarp, 0.0f);
        case 11: return readParameter(macroThrow, 0.0f);
        case 12: return fxModLfoStepValue;
        case 13: return fxModSmoothRandomValue;
        case 14: return fxModChaosValue;
        case 15: return lfo2Value;
        default: return 0.0f;
    }
}

std::array<int, EffectsRack::fxModuleCount> EffectsRack::orderedModuleIndices() const
{
    std::array<int, fxModuleCount> ordered {};
    std::array<bool, fxModuleCount> used {};
    auto writeIndex = size_t { 0 };

    for (size_t slotIndex = 0; slotIndex < fxOrder.size(); ++slotIndex)
    {
        const auto moduleIndex = juce::jlimit(0,
                                             static_cast<int>(fxModuleCount - 1),
                                             static_cast<int>(std::round(readParameter(fxOrder[slotIndex], static_cast<float>(slotIndex)))));

        if (moduleIndex == guardModuleIndex || used[static_cast<size_t>(moduleIndex)])
            continue;

        ordered[writeIndex++] = moduleIndex;
        used[static_cast<size_t>(moduleIndex)] = true;
    }

    for (auto moduleIndex = 0; moduleIndex < static_cast<int>(fxModuleCount); ++moduleIndex)
    {
        if (moduleIndex == guardModuleIndex || used[static_cast<size_t>(moduleIndex)])
            continue;

        ordered[writeIndex++] = moduleIndex;
        used[static_cast<size_t>(moduleIndex)] = true;
    }

    while (writeIndex < ordered.size())
        ordered[writeIndex++] = guardModuleIndex;

    return ordered;
}

void EffectsRack::processModule(int moduleIndex,
                                juce::AudioBuffer<float>& buffer,
                                double bpm,
                                std::optional<double> ppqPosition)
{
    switch (moduleIndex)
    {
        case 0: processTone(buffer); break;
        case 1: processEq(buffer); break;
        case 2: processDistortion(buffer); break;
        case 3: processBitcrush(buffer); break;
        case 4: processPump(buffer, bpm, ppqPosition); break;
        case 5: processTremolo(buffer, bpm, ppqPosition); break;
        case 6: processRingMod(buffer); break;
        case 7: processComb(buffer); break;
        case 8: processPhaser(buffer); break;
        case 9: processFlanger(buffer); break;
        case 10: processChorus(buffer); break;
        case 11: processDelay(buffer, bpm); break;
        case 12: processReverb(buffer); break;
        case 13: processWidth(buffer); break;
        default: break;
    }
}

void EffectsRack::processTone(juce::AudioBuffer<float>& buffer)
{
    if (readParameter(fxToneEnabled, 0.0f) < 0.5f || toneLowCutState.empty() || toneTiltState.empty())
        return;

    const auto lowCut = readParameter(fxToneLowCut, 30.0f);
    const auto tilt = juce::jlimit(-1.0f, 1.0f, readParameter(fxToneTilt, 0.0f));
    const auto tiltDepth = std::abs(tilt);
    const auto lowCutAlpha = onePoleAlpha(lowCut, currentSampleRate);
    const auto tiltAlpha = onePoleAlpha(1000.0f, currentSampleRate);
    const auto lowGain = tilt < 0.0f ? 1.0f + (tiltDepth * 0.8f) : 1.0f - (tiltDepth * 0.35f);
    const auto highGain = tilt > 0.0f ? 1.0f + (tiltDepth * 0.8f) : 1.0f - (tiltDepth * 0.35f);
    const auto compensation = 1.0f / (1.0f + (tiltDepth * 0.28f));
    const auto channels = juce::jmin(buffer.getNumChannels(), static_cast<int>(toneLowCutState.size()));

    for (auto channel = 0; channel < channels; ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);
        auto& lowCutState = toneLowCutState[static_cast<size_t>(channel)];
        auto& tiltState = toneTiltState[static_cast<size_t>(channel)];

        for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
        {
            const auto input = samples[sampleIndex];
            lowCutState += lowCutAlpha * (input - lowCutState);
            const auto highPassed = input - lowCutState;
            tiltState += tiltAlpha * (highPassed - tiltState);
            const auto low = tiltState;
            const auto high = highPassed - low;
            samples[sampleIndex] = ((low * lowGain) + (high * highGain)) * compensation;
        }
    }
}

void EffectsRack::processEq(juce::AudioBuffer<float>& buffer)
{
    if (readParameter(fxEqEnabled, 0.0f) < 0.5f || eqLowState.empty() || eqHighState.empty())
        return;

    const auto lowGain = juce::Decibels::decibelsToGain(readParameter(fxEqLowGain, 0.0f));
    const auto midGain = juce::Decibels::decibelsToGain(readParameter(fxEqMidGain, 0.0f));
    const auto highGain = juce::Decibels::decibelsToGain(readParameter(fxEqHighGain, 0.0f));
    const auto trimGain = juce::Decibels::decibelsToGain(readParameter(fxEqTrim, 0.0f));
    const auto compensation = 1.0f / juce::jmax(1.0f, (lowGain + midGain + highGain) / 3.0f);
    const auto lowAlpha = onePoleAlpha(180.0f, currentSampleRate);
    const auto highAlpha = onePoleAlpha(3600.0f, currentSampleRate);
    const auto channels = juce::jmin(buffer.getNumChannels(), static_cast<int>(eqLowState.size()));

    for (auto channel = 0; channel < channels; ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);
        auto& lowState = eqLowState[static_cast<size_t>(channel)];
        auto& highState = eqHighState[static_cast<size_t>(channel)];

        for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
        {
            const auto input = samples[sampleIndex];
            lowState += lowAlpha * (input - lowState);
            highState += highAlpha * (input - highState);

            const auto low = lowState;
            const auto high = input - highState;
            const auto mid = input - low - high;
            samples[sampleIndex] = ((low * lowGain) + (mid * midGain) + (high * highGain)) * trimGain * compensation;
        }
    }
}

void EffectsRack::processDistortion(juce::AudioBuffer<float>& buffer)
{
    if (readParameter(fxDistortionEnabled, 0.0f) < 0.5f)
        return;

    const auto amount = juce::jlimit(0.0f, 1.0f, readParameter(fxDistortionAmount, 0.2f) + (fxModulation.drive * 0.45f));
    const auto drive = juce::jmap(juce::jlimit(0.0f, 1.0f, amount), 1.0f, 24.0f);
    const auto makeup = 1.0f / std::sqrt(drive);

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);

        for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
            samples[sampleIndex] = std::tanh(samples[sampleIndex] * drive) * makeup;
    }
}

void EffectsRack::processBitcrush(juce::AudioBuffer<float>& buffer)
{
    if (readParameter(fxBitcrushEnabled, 0.0f) < 0.5f || bitcrushHeldSample.empty() || bitcrushHoldCounter.empty())
        return;

    const auto bits = juce::jlimit(4, 16, static_cast<int>(std::round(readParameter(fxBitcrushBits, 12.0f))));
    const auto holdSamples = juce::jlimit(1, 32, static_cast<int>(std::round(readParameter(fxBitcrushDownsample, 1.0f))));
    const auto mix = juce::jlimit(0.0f, 1.0f, readParameter(fxBitcrushMix, 0.25f));
    const auto maxQuantised = static_cast<float>((1 << (bits - 1)) - 1);
    const auto channels = juce::jmin(buffer.getNumChannels(), static_cast<int>(bitcrushHeldSample.size()));

    for (auto channel = 0; channel < channels; ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);
        auto& heldSample = bitcrushHeldSample[static_cast<size_t>(channel)];
        auto& holdCounter = bitcrushHoldCounter[static_cast<size_t>(channel)];

        for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
        {
            const auto input = samples[sampleIndex];

            if (holdCounter <= 0)
            {
                const auto clipped = juce::jlimit(-1.0f, 1.0f, input);
                heldSample = std::round(clipped * maxQuantised) / maxQuantised;
                holdCounter = holdSamples;
            }

            --holdCounter;
            samples[sampleIndex] = (input * (1.0f - mix)) + (heldSample * mix);
        }
    }
}

void EffectsRack::processPump(juce::AudioBuffer<float>& buffer, double bpm, std::optional<double> ppqPosition)
{
    const auto bounce = readParameter(macroBounce, 0.0f);
    const auto isEnabled = readParameter(fxPumpEnabled, 0.0f) >= 0.5f;
    if (! isEnabled && bounce <= 0.001f)
    {
        pumpSmoothedGain = 1.0f;
        pumpMeterPhase.store(static_cast<float>(pumpPhase), std::memory_order_relaxed);
        pumpMeterGain.store(1.0f, std::memory_order_relaxed);
        pumpMeterReduction.store(0.0f, std::memory_order_relaxed);
        pumpMeterActive.store(false, std::memory_order_relaxed);
        return;
    }

    const auto safeSampleRate = juce::jmax(1.0, currentSampleRate);
    const auto safeBpm = juce::jlimit(20.0, 300.0, bpm);
    const auto rateIndex = juce::jlimit(0, 3, static_cast<int>(std::round(isEnabled ? readParameter(fxPumpRate, 0.0f) : 1.0f)));
    const auto cyclesPerBeat = pumpCyclesPerBeat(rateIndex);
    const auto phaseIncrement = (safeBpm / 60.0) * cyclesPerBeat / safeSampleRate;
    const auto curveIndex = juce::jlimit(0, 5, static_cast<int>(std::round(readParameter(fxPumpCurve, 0.0f))));
    std::array<float, 8> customCurve {
        1.0f, 0.82f, 0.62f, 0.44f, 0.28f, 0.16f, 0.07f, 0.0f
    };

    for (size_t index = 0; index < customCurve.size(); ++index)
        customCurve[index] = juce::jlimit(0.0f, 1.0f, readParameter(fxPumpCustomCurve[index], customCurve[index]));

    const auto depth = juce::jlimit(0.0f, 1.0f, (isEnabled ? readParameter(fxPumpDepth, 0.35f) : 0.0f) + (bounce * 0.5f) + (fxModulation.pumpDepth * 0.5f));
    const auto shape = juce::jlimit(0.0f, 1.0f, readParameter(fxPumpShape, 0.45f) + (bounce * 0.16f));
    const auto phaseOffset = juce::jlimit(0.0f, 1.0f, readParameter(fxPumpPhase, 0.0f));
    const auto smoothing = 1.0f - std::exp(-1.0f / static_cast<float>(safeSampleRate * 0.0025));

    if (ppqPosition.has_value())
    {
        pumpPhase = std::fmod(*ppqPosition * cyclesPerBeat, 1.0);
        if (pumpPhase < 0.0)
            pumpPhase += 1.0;
    }

    auto peakReduction = 0.0f;

    for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        auto shapedPhase = std::fmod(pumpPhase + static_cast<double>(phaseOffset), 1.0);
        if (shapedPhase < 0.0)
            shapedPhase += 1.0;

        const auto recovery = static_cast<float>(shapedPhase);
        const auto duckAmount = pumpDuckAmount(recovery, shape, curveIndex, customCurve);
        const auto targetGain = juce::jlimit(0.0f, 1.0f, 1.0f - (depth * duckAmount));
        pumpSmoothedGain += (targetGain - pumpSmoothedGain) * smoothing;
        peakReduction = juce::jmax(peakReduction, 1.0f - pumpSmoothedGain);

        for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample(channel, sampleIndex, buffer.getSample(channel, sampleIndex) * pumpSmoothedGain);

        pumpPhase += phaseIncrement;
        while (pumpPhase >= 1.0)
            pumpPhase -= 1.0;
    }

    pumpMeterPhase.store(static_cast<float>(pumpPhase), std::memory_order_relaxed);
    pumpMeterGain.store(juce::jlimit(0.0f, 1.0f, pumpSmoothedGain), std::memory_order_relaxed);
    pumpMeterReduction.store(juce::jlimit(0.0f, 1.0f, peakReduction), std::memory_order_relaxed);
    pumpMeterActive.store(depth > 0.001f, std::memory_order_relaxed);
}

void EffectsRack::processTremolo(juce::AudioBuffer<float>& buffer, double bpm, std::optional<double> ppqPosition)
{
    if (readParameter(fxTremoloEnabled, 0.0f) < 0.5f)
        return;

    const auto safeSampleRate = juce::jmax(1.0, currentSampleRate);
    const auto safeBpm = juce::jlimit(20.0, 300.0, bpm);
    const auto rateIndex = juce::jlimit(0, 3, static_cast<int>(std::round(readParameter(fxTremoloRate, 1.0f))));
    const auto cyclesPerBeat = pumpCyclesPerBeat(rateIndex);
    const auto phaseIncrement = (safeBpm / 60.0) * cyclesPerBeat / safeSampleRate;
    const auto depth = juce::jlimit(0.0f, 1.0f, readParameter(fxTremoloDepth, 0.28f));
    const auto panAmount = juce::jlimit(0.0f, 1.0f, readParameter(fxTremoloPan, 0.25f));
    const auto shape = juce::jlimit(0.0f, 1.0f, readParameter(fxTremoloShape, 0.45f));
    const auto phaseOffset = juce::jlimit(0.0f, 1.0f, readParameter(fxTremoloPhase, 0.0f));
    const auto curve = juce::jmap(shape, 0.55f, 5.0f);
    const auto hasStereo = buffer.getNumChannels() >= 2;
    constexpr auto rootTwo = 1.41421356237f;

    if (ppqPosition.has_value())
    {
        tremoloPhase = std::fmod(*ppqPosition * cyclesPerBeat, 1.0);
        if (tremoloPhase < 0.0)
            tremoloPhase += 1.0;
    }

    for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        auto phase = std::fmod(tremoloPhase + static_cast<double>(phaseOffset), 1.0);
        if (phase < 0.0)
            phase += 1.0;

        const auto phaseFloat = static_cast<float>(phase);
        const auto sine = 0.5f - (0.5f * std::cos(juce::MathConstants<float>::twoPi * phaseFloat));
        const auto movement = std::pow(sine, curve);
        const auto tremoloGain = juce::jlimit(0.0f, 1.0f, 1.0f - (depth * movement));

        if (hasStereo && panAmount > 0.001f)
        {
            const auto panPhase = std::sin(juce::MathConstants<float>::twoPi * phaseFloat);
            const auto angle = juce::jmap((panPhase * panAmount * 0.5f) + 0.5f,
                                          0.0f,
                                          1.0f,
                                          0.0f,
                                          juce::MathConstants<float>::halfPi);
            const auto autoPanLeft = std::cos(angle) * rootTwo;
            const auto autoPanRight = std::sin(angle) * rootTwo;
            const auto leftGain = ((1.0f - panAmount) + (autoPanLeft * panAmount)) * tremoloGain;
            const auto rightGain = ((1.0f - panAmount) + (autoPanRight * panAmount)) * tremoloGain;

            buffer.setSample(0, sampleIndex, buffer.getSample(0, sampleIndex) * leftGain);
            buffer.setSample(1, sampleIndex, buffer.getSample(1, sampleIndex) * rightGain);

            for (auto channel = 2; channel < buffer.getNumChannels(); ++channel)
                buffer.setSample(channel, sampleIndex, buffer.getSample(channel, sampleIndex) * tremoloGain);
        }
        else
        {
            for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
                buffer.setSample(channel, sampleIndex, buffer.getSample(channel, sampleIndex) * tremoloGain);
        }

        tremoloPhase += phaseIncrement;
        while (tremoloPhase >= 1.0)
            tremoloPhase -= 1.0;
    }
}

void EffectsRack::processRingMod(juce::AudioBuffer<float>& buffer)
{
    if (readParameter(fxRingEnabled, 0.0f) < 0.5f)
        return;

    const auto safeSampleRate = juce::jmax(1.0, currentSampleRate);
    const auto frequency = juce::jlimit(0.25f, 2500.0f, readParameter(fxRingFrequency, 72.0f));
    const auto depth = juce::jlimit(0.0f, 1.0f, readParameter(fxRingDepth, 0.35f));
    const auto mix = juce::jlimit(0.0f, 1.0f, readParameter(fxRingMix, 0.18f));
    const auto bias = juce::jlimit(0.0f, 1.0f, readParameter(fxRingBias, 0.45f));
    const auto phaseIncrement = static_cast<double>(frequency) / safeSampleRate;
    const auto makeUp = 1.0f / (1.0f + (depth * mix * 0.45f));

    for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        const auto bipolarCarrier = std::sin(juce::MathConstants<float>::twoPi * static_cast<float>(ringPhase));
        const auto unipolarCarrier = (bipolarCarrier * 0.5f) + 0.5f;
        const auto carrier = juce::jmap(bias, bipolarCarrier, unipolarCarrier);
        const auto modulator = (1.0f - depth) + (carrier * depth);

        for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const auto input = buffer.getSample(channel, sampleIndex);
            const auto wet = input * modulator;
            buffer.setSample(channel, sampleIndex, ((input * (1.0f - mix)) + (wet * mix)) * makeUp);
        }

        ringPhase += phaseIncrement;
        while (ringPhase >= 1.0)
            ringPhase -= 1.0;
    }
}

void EffectsRack::processComb(juce::AudioBuffer<float>& buffer)
{
    if (readParameter(fxCombEnabled, 0.0f) < 0.5f || combBuffer.getNumSamples() <= 1 || combDampingState.empty())
        return;

    const auto frequency = juce::jlimit(25.0f, 2400.0f, readParameter(fxCombFrequency, 180.0f));
    const auto feedback = juce::jlimit(-0.82f, 0.82f, readParameter(fxCombFeedback, 0.28f));
    const auto damping = juce::jlimit(0.0f, 1.0f, readParameter(fxCombDamping, 0.35f));
    const auto mix = juce::jlimit(0.0f, 1.0f, readParameter(fxCombMix, 0.16f));
    const auto delaySamples = juce::jlimit(1,
                                           combBuffer.getNumSamples() - 1,
                                           static_cast<int>(std::round(currentSampleRate / static_cast<double>(frequency))));
    const auto dampingCutoff = juce::jmap(1.0f - damping, 450.0f, 12000.0f);
    const auto dampingAlpha = onePoleAlpha(dampingCutoff, currentSampleRate);
    const auto compensation = 1.0f / (1.0f + (std::abs(feedback) * mix * 0.75f));
    const auto channels = juce::jmin(juce::jmin(buffer.getNumChannels(), combBuffer.getNumChannels()),
                                     static_cast<int>(combDampingState.size()));

    for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        const auto readPosition = (combWritePosition + combBuffer.getNumSamples() - delaySamples) % combBuffer.getNumSamples();

        for (auto channel = 0; channel < channels; ++channel)
        {
            const auto input = buffer.getSample(channel, sampleIndex);
            const auto delayed = combBuffer.getSample(channel, readPosition);
            auto& dampingState = combDampingState[static_cast<size_t>(channel)];
            dampingState += dampingAlpha * (delayed - dampingState);
            combBuffer.setSample(channel, combWritePosition, input + (dampingState * feedback));
            buffer.setSample(channel, sampleIndex, (input + (delayed * mix)) * compensation);
        }

        combWritePosition = (combWritePosition + 1) % combBuffer.getNumSamples();
    }
}

void EffectsRack::processPhaser(juce::AudioBuffer<float>& buffer)
{
    if (readParameter(fxPhaserEnabled, 0.0f) < 0.5f)
        return;

    phaser.setRate(readParameter(fxPhaserRate, 0.32f));
    phaser.setDepth(readParameter(fxPhaserDepth, 0.42f));
    phaser.setCentreFrequency(juce::jmap(readParameter(fxPhaserDepth, 0.42f), 450.0f, 1400.0f));
    phaser.setFeedback(0.18f);
    phaser.setMix(readParameter(fxPhaserMix, 0.22f));

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    phaser.process(context);
}

void EffectsRack::processChorus(juce::AudioBuffer<float>& buffer)
{
    if (readParameter(fxChorusEnabled, 0.0f) < 0.5f)
        return;

    chorus.setRate(readParameter(fxChorusRate, 0.35f));
    chorus.setDepth(readParameter(fxChorusDepth, 0.35f));
    chorus.setCentreDelay(7.0f);
    chorus.setFeedback(0.08f);
    chorus.setMix(readParameter(fxChorusMix, 0.25f));

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    chorus.process(context);
}

void EffectsRack::processFlanger(juce::AudioBuffer<float>& buffer)
{
    if (readParameter(fxFlangerEnabled, 0.0f) < 0.5f)
        return;

    flanger.setRate(readParameter(fxFlangerRate, 0.22f));
    flanger.setDepth(readParameter(fxFlangerDepth, 0.32f));
    flanger.setCentreDelay(2.4f);
    flanger.setFeedback(readParameter(fxFlangerFeedback, 0.18f));
    flanger.setMix(readParameter(fxFlangerMix, 0.18f));

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    flanger.process(context);
}

void EffectsRack::processDelay(juce::AudioBuffer<float>& buffer, double bpm)
{
    const auto space = readParameter(macroSpace, 0.0f);
    const auto throwAmount = readParameter(macroThrow, 0.0f);
    const auto isEnabled = readParameter(fxDelayEnabled, 0.0f) >= 0.5f;
    if ((! isEnabled && space <= 0.001f && throwAmount <= 0.001f) || delayBuffer.getNumSamples() == 0)
        return;

    const auto syncEnabled = readParameter(fxDelaySync, 0.0f) >= 0.5f;
    const auto delaySeconds = syncEnabled
        ? delaySecondsForRate(static_cast<int>(std::round(readParameter(fxDelayRate, 1.0f))), bpm)
        : static_cast<double>(readParameter(fxDelayTime, 0.25f));
    const auto delaySamples = juce::jlimit(1, delayBuffer.getNumSamples() - 1,
                                          static_cast<int>(delaySeconds * currentSampleRate));
    const auto fallbackFeedback = 0.18f + (space * 0.22f) + (throwAmount * 0.42f);
    const auto feedback = juce::jlimit(0.0f, 0.85f, (isEnabled ? readParameter(fxDelayFeedback, 0.25f) : fallbackFeedback) + (throwAmount * 0.18f));
    const auto baseMix = isEnabled ? juce::jlimit(0.0f, 1.0f, readParameter(fxDelayMix, 0.2f) + (fxModulation.delayMix * 0.42f)) : 0.0f;
    const auto mix = juce::jlimit(0.0f, 0.55f, juce::jmax(baseMix, juce::jmax(space * 0.28f, throwAmount * 0.48f)));
    const auto channels = juce::jmin(buffer.getNumChannels(), delayBuffer.getNumChannels());

    for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        const auto readPosition = (delayWritePosition + delayBuffer.getNumSamples() - delaySamples) % delayBuffer.getNumSamples();

        for (auto channel = 0; channel < channels; ++channel)
        {
            const auto input = buffer.getSample(channel, sampleIndex);
            const auto delayed = delayBuffer.getSample(channel, readPosition);
            buffer.setSample(channel, sampleIndex, input + (delayed * mix));
            delayBuffer.setSample(channel, delayWritePosition, input + (delayed * feedback));
        }

        delayWritePosition = (delayWritePosition + 1) % delayBuffer.getNumSamples();
    }
}

void EffectsRack::processReverb(juce::AudioBuffer<float>& buffer)
{
    const auto space = readParameter(macroSpace, 0.0f);
    const auto throwAmount = readParameter(macroThrow, 0.0f);
    const auto isEnabled = readParameter(fxReverbEnabled, 0.0f) >= 0.5f;
    if (! isEnabled && space <= 0.001f && throwAmount <= 0.001f)
        return;

    juce::Reverb::Parameters reverbParameters;
    reverbParameters.roomSize = juce::jlimit(0.0f, 1.0f, (isEnabled ? readParameter(fxReverbSize, 0.35f) : 0.35f + (space * 0.4f)) + (throwAmount * 0.18f));
    reverbParameters.damping = readParameter(fxReverbDamping, 0.45f);
    const auto baseReverbMix = isEnabled ? juce::jlimit(0.0f, 1.0f, readParameter(fxReverbMix, 0.2f) + (fxModulation.reverbMix * 0.42f)) : 0.0f;
    const auto wetMix = juce::jlimit(0.0f, 0.65f, juce::jmax(baseReverbMix,
                                                             juce::jmax(space * 0.35f, throwAmount * 0.46f)));
    reverbParameters.wetLevel = wetMix;
    reverbParameters.dryLevel = 1.0f - (wetMix * 0.35f);
    reverbParameters.width = 1.0f;
    reverbParameters.freezeMode = 0.0f;
    reverb.setParameters(reverbParameters);

    if (buffer.getNumChannels() >= 2)
    {
        reverb.processStereo(buffer.getWritePointer(0),
                             buffer.getWritePointer(1),
                             buffer.getNumSamples());
        return;
    }

    if (buffer.getNumChannels() == 1)
        reverb.processMono(buffer.getWritePointer(0), buffer.getNumSamples());
}

void EffectsRack::processWidth(juce::AudioBuffer<float>& buffer)
{
    if (readParameter(fxWidthEnabled, 0.0f) < 0.5f || buffer.getNumChannels() < 2 || widthLowState.size() < 2)
        return;

    const auto width = juce::jlimit(0.0f, 1.6f, readParameter(fxWidthAmount, 1.15f) + (fxModulation.width * 0.5f));
    const auto monoCutoff = readParameter(fxWidthMonoCutoff, 120.0f);
    const auto lowAlpha = onePoleAlpha(monoCutoff, currentSampleRate);
    const auto sideCompensation = 1.0f / (1.0f + (juce::jmax(0.0f, width - 1.0f) * 0.18f));
    auto& lowLeftState = widthLowState[0];
    auto& lowRightState = widthLowState[1];
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        const auto inputLeft = left[sampleIndex];
        const auto inputRight = right[sampleIndex];

        lowLeftState += lowAlpha * (inputLeft - lowLeftState);
        lowRightState += lowAlpha * (inputRight - lowRightState);

        const auto monoLow = (lowLeftState + lowRightState) * 0.5f;
        const auto highLeft = inputLeft - lowLeftState;
        const auto highRight = inputRight - lowRightState;
        const auto highMid = (highLeft + highRight) * 0.5f;
        const auto highSide = (highLeft - highRight) * 0.5f * width;

        left[sampleIndex] = (monoLow + highMid + highSide) * sideCompensation;
        right[sampleIndex] = (monoLow + highMid - highSide) * sideCompensation;
    }
}

void EffectsRack::applyOutputGainAndSafety(juce::AudioBuffer<float>& buffer, float outputGainDb)
{
    const auto macroCompensatedOutput = outputGainDb - (readParameter(macroDirt, 0.0f) * 4.5f);
    const auto guardEnabled = readParameter(fxGuardEnabled, 0.0f) >= 0.5f;
    const auto guardPush = guardEnabled ? juce::jlimit(0.0f, 1.0f, readParameter(fxGuardPush, 0.0f)) : 0.0f;
    const auto guardCeiling = guardEnabled ? juce::jlimit(0.65f, 0.98f, readParameter(fxGuardCeiling, 0.92f)) : 0.98f;
    const auto gain = juce::Decibels::decibelsToGain(macroCompensatedOutput + (guardPush * 9.0f));
    const auto guardShape = 1.0f + (guardPush * 2.8f);
    const auto guardNormaliser = std::tanh(guardShape);

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);

        for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
        {
            const auto driven = samples[sampleIndex] * gain;
            samples[sampleIndex] = guardEnabled
                ? (std::tanh(driven * guardShape) / guardNormaliser) * guardCeiling
                : softClip(driven);
        }
    }
}

float EffectsRack::softClip(float sample) const
{
    return std::tanh(sample * 1.2f) * 0.98f;
}

float EffectsRack::readParameter(std::atomic<float>* parameter, float fallback) const
{
    return parameter != nullptr ? parameter->load() : fallback;
}
}
