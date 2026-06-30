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

bool configureShortHit(NateVSTAudioProcessor& processor, float delaySend)
{
    return setPlainParameter(processor, Parameters::ID::oscWave, 0.0f)
        && setPlainParameter(processor, Parameters::ID::osc1Level, 1.0f)
        && setPlainParameter(processor, Parameters::ID::osc2Level, 0.0f)
        && setPlainParameter(processor, Parameters::ID::subLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::ampAttack, 0.001f)
        && setPlainParameter(processor, Parameters::ID::ampDecay, 0.025f)
        && setPlainParameter(processor, Parameters::ID::ampSustain, 0.0f)
        && setPlainParameter(processor, Parameters::ID::ampRelease, 0.008f)
        && setPlainParameter(processor, Parameters::ID::filterCutoff, 20000.0f)
        && setPlainParameter(processor, Parameters::ID::filterResonance, 0.1f)
        && setPlainParameter(processor, Parameters::ID::driveAmount, 0.0f)
        && setPlainParameter(processor, Parameters::ID::outputGain, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sampleEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelaySync, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayRate, 4.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayTime, 0.12f)
        && setPlainParameter(processor, Parameters::ID::fxDelayFeedback, 0.58f)
        && setPlainParameter(processor, Parameters::ID::fxDelayMix, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxSendDelay, delaySend)
        && setPlainParameter(processor, Parameters::ID::fxReverbEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxReverbMix, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxSendReverb, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxSendTailKill, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 0.0f);
}

float renderDelayTail(float delaySend)
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    if (! configureShortHit(processor, delaySend))
        return 0.0f;

    juce::AudioBuffer<float> buffer(2, 512);
    auto lateTail = 0.0f;

    for (auto blockIndex = 0; blockIndex < 40; ++blockIndex)
    {
        juce::MidiBuffer midi;
        if (blockIndex == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(110)), 0);
        if (blockIndex == 1)
            midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);

        processor.processBlock(buffer, midi);
        if (blockIndex >= 12)
            lateTail = juce::jmax(lateTail, rmsForBuffer(buffer));
    }

    return lateTail;
}

struct TailKillStats
{
    float beforeKill = 0.0f;
    float afterKill = 0.0f;
};

TailKillStats renderTailKillStats()
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    if (! configureShortHit(processor, 0.85f))
        return {};

    juce::AudioBuffer<float> buffer(2, 512);
    TailKillStats stats;

    for (auto blockIndex = 0; blockIndex < 34; ++blockIndex)
    {
        juce::MidiBuffer midi;
        if (blockIndex == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(110)), 0);
        if (blockIndex == 1)
            midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
        if (blockIndex == 28)
            processor.requestFxSendTailKill();

        processor.processBlock(buffer, midi);
        const auto blockRms = rmsForBuffer(buffer);
        if (blockIndex >= 12 && blockIndex < 28)
            stats.beforeKill = juce::jmax(stats.beforeKill, blockRms);
        if (blockIndex >= 30)
            stats.afterKill = juce::jmax(stats.afterKill, blockRms);
    }

    return stats;
}
}

int main()
{
    NateVSTAudioProcessor tailProcessor;
    if (tailProcessor.getTailLengthSeconds() < 2.0)
    {
        std::cerr << "Processor reports an unrealistically short tail for delay/reverb/send effects: "
                  << tailProcessor.getTailLengthSeconds() << " seconds\n";
        return 1;
    }

    const auto dryTail = renderDelayTail(0.0f);
    const auto sendTail = renderDelayTail(0.85f);

    if (sendTail <= 0.0005f || sendTail <= dryTail * 5.0f)
    {
        std::cerr << "Send delay did not create an independent tail: dry "
                  << dryTail << " send " << sendTail << '\n';
        return 1;
    }

    const auto tailKillStats = renderTailKillStats();
    if (tailKillStats.beforeKill <= 0.0005f || tailKillStats.afterKill >= tailKillStats.beforeKill * 0.35f)
    {
        std::cerr << "One-shot send tail kill did not clear the delay bus: before "
                  << tailKillStats.beforeKill << " after " << tailKillStats.afterKill << '\n';
        return 1;
    }

    std::cout << "Effects send audit passed for send delay tail and one-shot tail kill.\n";
    return 0;
}
