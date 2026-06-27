#include "EffectsRack.h"

#include <algorithm>
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
    fxPumpDepth = parameters.getRawParameterValue(Parameters::ID::fxPumpDepth);
    fxPumpShape = parameters.getRawParameterValue(Parameters::ID::fxPumpShape);
    fxPumpPhase = parameters.getRawParameterValue(Parameters::ID::fxPumpPhase);
    fxTremoloEnabled = parameters.getRawParameterValue(Parameters::ID::fxTremoloEnabled);
    fxTremoloRate = parameters.getRawParameterValue(Parameters::ID::fxTremoloRate);
    fxTremoloDepth = parameters.getRawParameterValue(Parameters::ID::fxTremoloDepth);
    fxTremoloPan = parameters.getRawParameterValue(Parameters::ID::fxTremoloPan);
    fxTremoloShape = parameters.getRawParameterValue(Parameters::ID::fxTremoloShape);
    fxTremoloPhase = parameters.getRawParameterValue(Parameters::ID::fxTremoloPhase);
    fxChorusEnabled = parameters.getRawParameterValue(Parameters::ID::fxChorusEnabled);
    fxChorusRate = parameters.getRawParameterValue(Parameters::ID::fxChorusRate);
    fxChorusDepth = parameters.getRawParameterValue(Parameters::ID::fxChorusDepth);
    fxChorusMix = parameters.getRawParameterValue(Parameters::ID::fxChorusMix);
    fxDelayEnabled = parameters.getRawParameterValue(Parameters::ID::fxDelayEnabled);
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
    macroDirt = parameters.getRawParameterValue(Parameters::ID::macroDirt);
    macroSpace = parameters.getRawParameterValue(Parameters::ID::macroSpace);
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
    toneLowCutState.assign(static_cast<size_t>(preparedChannels), 0.0f);
    toneTiltState.assign(static_cast<size_t>(preparedChannels), 0.0f);
    eqLowState.assign(static_cast<size_t>(preparedChannels), 0.0f);
    eqHighState.assign(static_cast<size_t>(preparedChannels), 0.0f);
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
    std::fill(toneLowCutState.begin(), toneLowCutState.end(), 0.0f);
    std::fill(toneTiltState.begin(), toneTiltState.end(), 0.0f);
    std::fill(eqLowState.begin(), eqLowState.end(), 0.0f);
    std::fill(eqHighState.begin(), eqHighState.end(), 0.0f);
    std::fill(bitcrushHeldSample.begin(), bitcrushHeldSample.end(), 0.0f);
    std::fill(bitcrushHoldCounter.begin(), bitcrushHoldCounter.end(), 0);
    std::fill(widthLowState.begin(), widthLowState.end(), 0.0f);
    pumpPhase = 0.0;
    tremoloPhase = 0.0;
    pumpSmoothedGain = 1.0f;
    delayWritePosition = 0;
}

void EffectsRack::process(juce::AudioBuffer<float>& buffer, float outputGainDb, double bpm, std::optional<double> ppqPosition)
{
    processTone(buffer);
    processEq(buffer);
    processDistortion(buffer);
    processBitcrush(buffer);
    processPump(buffer, bpm, ppqPosition);
    processTremolo(buffer, bpm, ppqPosition);
    processPhaser(buffer);
    processFlanger(buffer);
    processChorus(buffer);
    processDelay(buffer);
    processReverb(buffer);
    processWidth(buffer);
    applyOutputGainAndSafety(buffer, outputGainDb);
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

    const auto amount = readParameter(fxDistortionAmount, 0.2f);
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
    if (readParameter(fxPumpEnabled, 0.0f) < 0.5f)
    {
        pumpSmoothedGain = 1.0f;
        return;
    }

    const auto safeSampleRate = juce::jmax(1.0, currentSampleRate);
    const auto safeBpm = juce::jlimit(20.0, 300.0, bpm);
    const auto rateIndex = juce::jlimit(0, 3, static_cast<int>(std::round(readParameter(fxPumpRate, 0.0f))));
    const auto cyclesPerBeat = pumpCyclesPerBeat(rateIndex);
    const auto phaseIncrement = (safeBpm / 60.0) * cyclesPerBeat / safeSampleRate;
    const auto depth = juce::jlimit(0.0f, 1.0f, readParameter(fxPumpDepth, 0.35f));
    const auto shape = juce::jlimit(0.0f, 1.0f, readParameter(fxPumpShape, 0.45f));
    const auto phaseOffset = juce::jlimit(0.0f, 1.0f, readParameter(fxPumpPhase, 0.0f));
    const auto curve = juce::jmap(shape, 0.35f, 4.5f);
    const auto smoothing = 1.0f - std::exp(-1.0f / static_cast<float>(safeSampleRate * 0.0025));

    if (ppqPosition.has_value())
    {
        pumpPhase = std::fmod(*ppqPosition * cyclesPerBeat, 1.0);
        if (pumpPhase < 0.0)
            pumpPhase += 1.0;
    }

    for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        auto shapedPhase = std::fmod(pumpPhase + static_cast<double>(phaseOffset), 1.0);
        if (shapedPhase < 0.0)
            shapedPhase += 1.0;

        const auto recovery = static_cast<float>(shapedPhase);
        const auto duckAmount = std::pow(1.0f - recovery, curve);
        const auto targetGain = juce::jlimit(0.0f, 1.0f, 1.0f - (depth * duckAmount));
        pumpSmoothedGain += (targetGain - pumpSmoothedGain) * smoothing;

        for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample(channel, sampleIndex, buffer.getSample(channel, sampleIndex) * pumpSmoothedGain);

        pumpPhase += phaseIncrement;
        while (pumpPhase >= 1.0)
            pumpPhase -= 1.0;
    }
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

void EffectsRack::processDelay(juce::AudioBuffer<float>& buffer)
{
    const auto space = readParameter(macroSpace, 0.0f);
    const auto isEnabled = readParameter(fxDelayEnabled, 0.0f) >= 0.5f;
    if ((! isEnabled && space <= 0.001f) || delayBuffer.getNumSamples() == 0)
        return;

    const auto delaySamples = juce::jlimit(1, delayBuffer.getNumSamples() - 1,
                                          static_cast<int>(readParameter(fxDelayTime, 0.25f) * currentSampleRate));
    const auto feedback = juce::jlimit(0.0f, 0.85f, isEnabled ? readParameter(fxDelayFeedback, 0.25f) : 0.18f + (space * 0.22f));
    const auto baseMix = isEnabled ? readParameter(fxDelayMix, 0.2f) : 0.0f;
    const auto mix = juce::jlimit(0.0f, 0.55f, juce::jmax(baseMix, space * 0.28f));
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
    const auto isEnabled = readParameter(fxReverbEnabled, 0.0f) >= 0.5f;
    if (! isEnabled && space <= 0.001f)
        return;

    juce::Reverb::Parameters reverbParameters;
    reverbParameters.roomSize = juce::jlimit(0.0f, 1.0f, isEnabled ? readParameter(fxReverbSize, 0.35f) : 0.35f + (space * 0.4f));
    reverbParameters.damping = readParameter(fxReverbDamping, 0.45f);
    const auto wetMix = juce::jlimit(0.0f, 0.65f, juce::jmax(isEnabled ? readParameter(fxReverbMix, 0.2f) : 0.0f, space * 0.35f));
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

    const auto width = juce::jlimit(0.0f, 1.6f, readParameter(fxWidthAmount, 1.15f));
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
