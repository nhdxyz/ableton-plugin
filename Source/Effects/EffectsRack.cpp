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
}

namespace Effects
{
EffectsRack::EffectsRack(Parameters::APVTS& state)
    : parameters(state)
{
    fxDistortionEnabled = parameters.getRawParameterValue(Parameters::ID::fxDistortionEnabled);
    fxDistortionAmount = parameters.getRawParameterValue(Parameters::ID::fxDistortionAmount);
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
    fxToneEnabled = parameters.getRawParameterValue(Parameters::ID::fxToneEnabled);
    fxToneTilt = parameters.getRawParameterValue(Parameters::ID::fxToneTilt);
    fxToneLowCut = parameters.getRawParameterValue(Parameters::ID::fxToneLowCut);
    fxPhaserEnabled = parameters.getRawParameterValue(Parameters::ID::fxPhaserEnabled);
    fxPhaserRate = parameters.getRawParameterValue(Parameters::ID::fxPhaserRate);
    fxPhaserDepth = parameters.getRawParameterValue(Parameters::ID::fxPhaserDepth);
    fxPhaserMix = parameters.getRawParameterValue(Parameters::ID::fxPhaserMix);
    fxGuardEnabled = parameters.getRawParameterValue(Parameters::ID::fxGuardEnabled);
    fxGuardPush = parameters.getRawParameterValue(Parameters::ID::fxGuardPush);
    fxGuardCeiling = parameters.getRawParameterValue(Parameters::ID::fxGuardCeiling);
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
    chorus.prepare(spec);
    reverb.setSampleRate(currentSampleRate);
    delayBuffer.setSize(preparedChannels, static_cast<int>(std::ceil(currentSampleRate * 2.0)));
    toneLowCutState.assign(static_cast<size_t>(preparedChannels), 0.0f);
    toneTiltState.assign(static_cast<size_t>(preparedChannels), 0.0f);
    reset();
}

void EffectsRack::reset()
{
    phaser.reset();
    chorus.reset();
    reverb.reset();
    delayBuffer.clear();
    std::fill(toneLowCutState.begin(), toneLowCutState.end(), 0.0f);
    std::fill(toneTiltState.begin(), toneTiltState.end(), 0.0f);
    delayWritePosition = 0;
}

void EffectsRack::process(juce::AudioBuffer<float>& buffer, float outputGainDb)
{
    processTone(buffer);
    processDistortion(buffer);
    processPhaser(buffer);
    processChorus(buffer);
    processDelay(buffer);
    processReverb(buffer);
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
