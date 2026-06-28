#include "../Source/PluginProcessor.h"

#include <cmath>
#include <iostream>

namespace
{
bool setPlainParameter(NateVSTAudioProcessor& processor, const juce::String& parameterID, float value)
{
    if (auto* parameter = processor.getValueTreeState().getParameter(parameterID))
    {
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        return true;
    }

    return false;
}

float peakForBuffer(const juce::AudioBuffer<float>& buffer)
{
    auto peak = 0.0f;
    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const auto* samples = buffer.getReadPointer(channel);
        for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
            peak = juce::jmax(peak, std::abs(samples[sampleIndex]));
    }

    return peak;
}

float rmsForBuffer(const juce::AudioBuffer<float>& buffer)
{
    double sumSquares = 0.0;
    auto sampleCount = 0;

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const auto* samples = buffer.getReadPointer(channel);
        for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
        {
            const auto sample = static_cast<double>(samples[sampleIndex]);
            sumSquares += sample * sample;
            ++sampleCount;
        }
    }

    return sampleCount > 0 ? static_cast<float>(std::sqrt(sumSquares / static_cast<double>(sampleCount))) : 0.0f;
}

bool configureBasePatch(NateVSTAudioProcessor& processor)
{
    return setPlainParameter(processor, Parameters::ID::oscWave, 1.0f)
        && setPlainParameter(processor, Parameters::ID::osc1Level, 1.0f)
        && setPlainParameter(processor, Parameters::ID::osc2Level, 0.0f)
        && setPlainParameter(processor, Parameters::ID::subLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::filterCutoff, 20000.0f)
        && setPlainParameter(processor, Parameters::ID::filterResonance, 0.1f)
        && setPlainParameter(processor, Parameters::ID::driveAmount, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sampleEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDistortionEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxBitcrushEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxPumpEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayMix, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxSendDelay, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxReverbEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxReverbMix, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxSendReverb, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxWidthEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxGuardPush, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxGuardCeiling, 0.98f)
        && setPlainParameter(processor, Parameters::ID::fxGuardClipMix, 1.0f);
}

float renderShortHitPeak(float punch)
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    if (! configureBasePatch(processor)
        || ! setPlainParameter(processor, Parameters::ID::ampAttack, 0.001f)
        || ! setPlainParameter(processor, Parameters::ID::ampDecay, 0.035f)
        || ! setPlainParameter(processor, Parameters::ID::ampSustain, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::ampRelease, 0.01f)
        || ! setPlainParameter(processor, Parameters::ID::outputGain, -5.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxGuardGlue, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxGuardPunch, punch))
    {
        return 0.0f;
    }

    juce::AudioBuffer<float> buffer(2, 512);
    auto peak = 0.0f;

    for (auto blockIndex = 0; blockIndex < 4; ++blockIndex)
    {
        juce::MidiBuffer midi;
        if (blockIndex == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(110)), 0);
        if (blockIndex == 1)
            midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);

        processor.processBlock(buffer, midi);
        peak = juce::jmax(peak, peakForBuffer(buffer));
    }

    return peak;
}

float renderSustainedRms(float glue)
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    if (! configureBasePatch(processor)
        || ! setPlainParameter(processor, Parameters::ID::ampAttack, 0.001f)
        || ! setPlainParameter(processor, Parameters::ID::ampDecay, 0.02f)
        || ! setPlainParameter(processor, Parameters::ID::ampSustain, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::ampRelease, 0.2f)
        || ! setPlainParameter(processor, Parameters::ID::outputGain, -2.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxGuardGlue, glue)
        || ! setPlainParameter(processor, Parameters::ID::fxGuardPunch, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxGuardClipMix, 0.55f))
    {
        return 0.0f;
    }

    juce::AudioBuffer<float> buffer(2, 512);
    auto lateRms = 0.0f;

    for (auto blockIndex = 0; blockIndex < 24; ++blockIndex)
    {
        juce::MidiBuffer midi;
        if (blockIndex == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 48, static_cast<juce::uint8>(120)), 0);

        processor.processBlock(buffer, midi);
        if (blockIndex >= 14)
            lateRms = juce::jmax(lateRms, rmsForBuffer(buffer));
    }

    return lateRms;
}

float renderCeilingPeak()
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    if (! configureBasePatch(processor)
        || ! setPlainParameter(processor, Parameters::ID::ampAttack, 0.001f)
        || ! setPlainParameter(processor, Parameters::ID::ampDecay, 0.02f)
        || ! setPlainParameter(processor, Parameters::ID::ampSustain, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::outputGain, 6.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxGuardPush, 0.35f)
        || ! setPlainParameter(processor, Parameters::ID::fxGuardGlue, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxGuardPunch, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxGuardClipMix, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxGuardCeiling, 0.74f))
    {
        return 1.0f;
    }

    juce::AudioBuffer<float> buffer(2, 512);
    auto peak = 0.0f;

    for (auto blockIndex = 0; blockIndex < 12; ++blockIndex)
    {
        juce::MidiBuffer midi;
        if (blockIndex == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 48, static_cast<juce::uint8>(127)), 0);

        processor.processBlock(buffer, midi);
        peak = juce::jmax(peak, peakForBuffer(buffer));
    }

    return peak;
}
}

int main()
{
    const auto dryPeak = renderShortHitPeak(0.0f);
    const auto punchPeak = renderShortHitPeak(1.0f);
    if (dryPeak <= 0.0001f || punchPeak <= dryPeak * 1.015f)
    {
        std::cerr << "Guard punch did not lift the short-hit peak: dry "
                  << dryPeak << " punch " << punchPeak << '\n';
        return 1;
    }

    const auto ungluedRms = renderSustainedRms(0.0f);
    const auto gluedRms = renderSustainedRms(1.0f);
    if (ungluedRms <= 0.0001f || gluedRms >= ungluedRms * 0.86f)
    {
        std::cerr << "Guard glue did not reduce sustained RMS enough: dry "
                  << ungluedRms << " glued " << gluedRms << '\n';
        return 1;
    }

    const auto ceilingPeak = renderCeilingPeak();
    if (ceilingPeak > 0.745f)
    {
        std::cerr << "Guard clip mix did not respect ceiling: peak "
                  << ceilingPeak << '\n';
        return 1;
    }

    std::cout << "Guard dynamics audit passed for punch, glue compression, and clip ceiling.\n";
    return 0;
}
