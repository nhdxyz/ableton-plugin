#include "EffectsRack.h"

#include <cmath>

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

    chorus.prepare(spec);
    reverb.setSampleRate(currentSampleRate);
    delayBuffer.setSize(preparedChannels, static_cast<int>(std::ceil(currentSampleRate * 2.0)));
    reset();
}

void EffectsRack::reset()
{
    chorus.reset();
    reverb.reset();
    delayBuffer.clear();
    delayWritePosition = 0;
}

void EffectsRack::process(juce::AudioBuffer<float>& buffer, float outputGainDb)
{
    processDistortion(buffer);
    processChorus(buffer);
    processDelay(buffer);
    processReverb(buffer);
    applyOutputGainAndSafety(buffer, outputGainDb);
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
    const auto gain = juce::Decibels::decibelsToGain(macroCompensatedOutput);

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);

        for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
            samples[sampleIndex] = softClip(samples[sampleIndex] * gain);
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
