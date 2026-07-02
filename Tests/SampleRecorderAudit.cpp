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

bool configureCapturePatch(NateVSTAudioProcessor& processor)
{
    return setPlainParameter(processor, Parameters::ID::sampleRecordSource, 0.0f)
        && setPlainParameter(processor, Parameters::ID::oscWave, 1.0f)
        && setPlainParameter(processor, Parameters::ID::osc2Wave, 3.0f)
        && setPlainParameter(processor, Parameters::ID::osc1Level, 0.82f)
        && setPlainParameter(processor, Parameters::ID::osc2Level, 0.34f)
        && setPlainParameter(processor, Parameters::ID::subLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::ampAttack, 0.001f)
        && setPlainParameter(processor, Parameters::ID::ampDecay, 0.18f)
        && setPlainParameter(processor, Parameters::ID::ampSustain, 0.35f)
        && setPlainParameter(processor, Parameters::ID::ampRelease, 0.18f)
        && setPlainParameter(processor, Parameters::ID::filterCutoff, 3600.0f)
        && setPlainParameter(processor, Parameters::ID::filterResonance, 0.25f)
        && setPlainParameter(processor, Parameters::ID::sampleEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxReverbEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxPumpEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::outputGain, 0.0f);
}

bool configureSamplePlayback(NateVSTAudioProcessor& processor)
{
    return setPlainParameter(processor, Parameters::ID::osc1Level, 0.0f)
        && setPlainParameter(processor, Parameters::ID::osc2Level, 0.0f)
        && setPlainParameter(processor, Parameters::ID::subLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sampleEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::samplePlaybackMode, 1.0f)
        && setPlainParameter(processor, Parameters::ID::sampleMix, 1.0f)
        && setPlainParameter(processor, Parameters::ID::sampleGain, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxReverbEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxPumpEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::outputGain, 0.0f);
}

struct RenderStats
{
    float peak = 0.0f;
    float rms = 0.0f;
    bool finite = true;
};

RenderStats processBlocks(NateVSTAudioProcessor& processor,
                          int blocks,
                          int noteOnBlock,
                          int noteOffBlock,
                          int midiNote)
{
    juce::AudioBuffer<float> buffer(2, 512);
    RenderStats stats;
    double sumSquares = 0.0;
    auto sampleCount = 0;

    for (auto block = 0; block < blocks; ++block)
    {
        buffer.clear();
        juce::MidiBuffer midi;
        if (block == noteOnBlock)
            midi.addEvent(juce::MidiMessage::noteOn(1, midiNote, static_cast<juce::uint8>(112)), 0);
        if (block == noteOffBlock)
            midi.addEvent(juce::MidiMessage::noteOff(1, midiNote), 0);

        processor.processBlock(buffer, midi);
        for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const auto* samples = buffer.getReadPointer(channel);
            for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                const auto value = samples[sample];
                if (! std::isfinite(value))
                    stats.finite = false;

                stats.peak = juce::jmax(stats.peak, std::abs(value));
                sumSquares += static_cast<double>(value) * static_cast<double>(value);
                ++sampleCount;
            }
        }
    }

    stats.rms = sampleCount > 0 ? static_cast<float>(std::sqrt(sumSquares / static_cast<double>(sampleCount))) : 0.0f;
    return stats;
}

float recordCommittedOverviewPeak(float outputGainDb)
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);
    configureCapturePatch(processor);
    setPlainParameter(processor, Parameters::ID::outputGain, outputGainDb);

    processor.beginSampleCapture();
    processBlocks(processor, 36, 0, 24, 60);
    if (! processor.commitSampleCaptureToSampler())
        return -1.0f;

    const auto overview = processor.createSamplePeakOverview(96);
    auto peak = 0.0f;
    for (const auto value : overview.maximums)
        peak = juce::jmax(peak, std::abs(value));
    for (const auto value : overview.minimums)
        peak = juce::jmax(peak, std::abs(value));

    const auto capturePath = processor.getLoadedSamplePath();
    if (capturePath.isNotEmpty())
    {
        const juce::File captureFile(capturePath);
        if (captureFile.existsAsFile())
            captureFile.deleteFile();
    }

    return peak;
}

float recordHostInputOverviewPeak(float inputPeak, float* sourcePeak = nullptr)
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);
    if (! setPlainParameter(processor, Parameters::ID::sampleRecordSource, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::osc1Level, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::osc2Level, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::subLevel, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::sampleEnabled, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f))
    {
        return -1.0f;
    }

    processor.beginSampleCapture();
    juce::AudioBuffer<float> buffer(2, 512);
    auto phase = 0.0;
    const auto phaseDelta = juce::MathConstants<double>::twoPi * 440.0 / 44100.0;
    for (auto block = 0; block < 32; ++block)
    {
        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto value = static_cast<float>(std::sin(phase)) * inputPeak;
            buffer.setSample(0, sample, value);
            buffer.setSample(1, sample, value * 0.65f);
            phase += phaseDelta;
            if (phase >= juce::MathConstants<double>::twoPi)
                phase -= juce::MathConstants<double>::twoPi;
        }

        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);
    }

    if (sourcePeak != nullptr)
        *sourcePeak = processor.getSampleCaptureSourcePeak();

    if (! processor.commitSampleCaptureToSampler())
        return -1.0f;

    const auto overview = processor.createSamplePeakOverview(96);
    auto peak = 0.0f;
    for (const auto value : overview.maximums)
        peak = juce::jmax(peak, std::abs(value));
    for (const auto value : overview.minimums)
        peak = juce::jmax(peak, std::abs(value));

    const auto capturePath = processor.getLoadedSamplePath();
    if (capturePath.isNotEmpty())
    {
        const juce::File captureFile(capturePath);
        if (captureFile.existsAsFile())
            captureFile.deleteFile();
    }

    return peak;
}

void feedHostInputBlocks(NateVSTAudioProcessor& processor, float inputPeak, int blocks)
{
    juce::AudioBuffer<float> buffer(2, 512);
    auto phase = 0.0;
    const auto phaseDelta = juce::MathConstants<double>::twoPi * 440.0 / 44100.0;
    for (auto block = 0; block < blocks; ++block)
    {
        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto value = static_cast<float>(std::sin(phase)) * inputPeak;
            buffer.setSample(0, sample, value);
            buffer.setSample(1, sample, value * 0.65f);
            phase += phaseDelta;
            if (phase >= juce::MathConstants<double>::twoPi)
                phase -= juce::MathConstants<double>::twoPi;
        }

        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);
    }
}
}

int main()
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);
    const auto captureCapacity = processor.getSampleCaptureCapacitySeconds();
    if (captureCapacity < 7.9f || captureCapacity > 8.1f)
    {
        std::cerr << "Unexpected recorder rolling-buffer capacity: " << captureCapacity << "s\n";
        return 1;
    }

    if (! configureCapturePatch(processor))
    {
        std::cerr << "Could not configure recorder capture patch\n";
        return 1;
    }

    if (processor.commitSampleCaptureToSampler())
    {
        std::cerr << "Recorder committed with no captured audio\n";
        return 1;
    }

    processor.beginSampleCapture();
    if (! processor.isSampleCaptureEnabled())
    {
        std::cerr << "Recorder did not enter capture mode\n";
        return 1;
    }

    const auto captureStats = processBlocks(processor, 36, 0, 24, 60);
    if (! captureStats.finite || captureStats.peak < 0.03f || captureStats.rms < 0.003f)
    {
        std::cerr << "Capture source render was not healthy: peak " << captureStats.peak
                  << " rms " << captureStats.rms << '\n';
        return 1;
    }

    const auto postFxSourcePeak = processor.getSampleCaptureSourcePeak();
    if (postFxSourcePeak < 0.01f || postFxSourcePeak > 1.0f)
    {
        std::cerr << "Recorder Post-FX source peak was not tracked: " << postFxSourcePeak << '\n';
        return 1;
    }

    const auto capturedSeconds = processor.getSampleCaptureDurationSeconds();
    if (capturedSeconds < 0.35f || capturedSeconds > 0.45f)
    {
        std::cerr << "Unexpected recorder duration before commit: " << capturedSeconds << "s\n";
        return 1;
    }

    if (! processor.commitSampleCaptureToSampler())
    {
        std::cerr << "Recorder did not commit captured audio to the sampler\n";
        return 1;
    }

    if (processor.isSampleCaptureEnabled())
    {
        std::cerr << "Recorder stayed armed after commit\n";
        return 1;
    }

    if (! processor.hasLoadedSample())
    {
        std::cerr << "Committed recorder audio was not loaded into the sampler\n";
        return 1;
    }

    const auto overview = processor.createSamplePeakOverview(96);
    if (overview.totalSamples < 2048 || overview.maximums.empty())
    {
        std::cerr << "Committed recorder sample overview is empty or too short: "
                  << overview.totalSamples << " samples\n";
        return 1;
    }

    auto overviewPeak = 0.0f;
    for (const auto peak : overview.maximums)
        overviewPeak = juce::jmax(overviewPeak, std::abs(peak));
    for (const auto peak : overview.minimums)
        overviewPeak = juce::jmax(overviewPeak, std::abs(peak));

    if (overviewPeak < 0.02f)
    {
        std::cerr << "Committed recorder sample overview is too quiet: " << overviewPeak << '\n';
        return 1;
    }

    if (! configureSamplePlayback(processor))
    {
        std::cerr << "Could not configure sampler playback after recorder commit\n";
        return 1;
    }

    const auto sampleStats = processBlocks(processor, 20, 0, 14, 60);
    if (! sampleStats.finite || sampleStats.peak < 0.02f || sampleStats.rms < 0.001f)
    {
        std::cerr << "Committed recorder sample did not render audibly: peak "
                  << sampleStats.peak << " rms " << sampleStats.rms << '\n';
        return 1;
    }

    const auto unityCapturePeak = recordCommittedOverviewPeak(0.0f);
    const auto quietCapturePeak = recordCommittedOverviewPeak(-24.0f);
    if (unityCapturePeak <= 0.02f || quietCapturePeak <= 0.0f || quietCapturePeak >= unityCapturePeak * 0.25f)
    {
        std::cerr << "Recorder capture does not appear to include post-FX/output gain: unity "
                  << unityCapturePeak << " quiet " << quietCapturePeak << '\n';
        return 1;
    }

    auto hostInputSourcePeak = 0.0f;
    const auto hostInputPeak = recordHostInputOverviewPeak(0.18f, &hostInputSourcePeak);
    if (hostInputPeak < 0.08f || hostInputPeak > 0.24f)
    {
        std::cerr << "Recorder Host Input source did not capture routed input as expected: "
                  << hostInputPeak << '\n';
        return 1;
    }
    if (hostInputSourcePeak < 0.16f || hostInputSourcePeak > 0.2f)
    {
        std::cerr << "Recorder Host Input source peak was not tracked: "
                  << hostInputSourcePeak << '\n';
        return 1;
    }

    NateVSTAudioProcessor thresholdProcessor;
    thresholdProcessor.prepareToPlay(44100.0, 512);
    if (! setPlainParameter(thresholdProcessor, Parameters::ID::sampleRecordSource, 1.0f)
        || ! setPlainParameter(thresholdProcessor, Parameters::ID::sampleRecordStart, 2.0f)
        || ! setPlainParameter(thresholdProcessor, Parameters::ID::osc1Level, 0.0f)
        || ! setPlainParameter(thresholdProcessor, Parameters::ID::osc2Level, 0.0f)
        || ! setPlainParameter(thresholdProcessor, Parameters::ID::subLevel, 0.0f)
        || ! setPlainParameter(thresholdProcessor, Parameters::ID::noiseLevel, 0.0f)
        || ! setPlainParameter(thresholdProcessor, Parameters::ID::sampleEnabled, 0.0f)
        || ! setPlainParameter(thresholdProcessor, Parameters::ID::sequencerEnabled, 0.0f))
    {
        std::cerr << "Could not configure threshold recorder patch\n";
        return 1;
    }

    thresholdProcessor.beginSampleCapture();
    if (! thresholdProcessor.isSampleCaptureEnabled() || ! thresholdProcessor.isSampleCaptureWaitingForThreshold())
    {
        std::cerr << "Recorder did not arm for threshold capture\n";
        return 1;
    }

    feedHostInputBlocks(thresholdProcessor, 0.01f, 12);
    if (! thresholdProcessor.isSampleCaptureWaitingForThreshold()
        || thresholdProcessor.getSampleCaptureDurationSeconds() > 0.001f)
    {
        std::cerr << "Recorder threshold mode captured below-threshold input: "
                  << thresholdProcessor.getSampleCaptureDurationSeconds() << "s\n";
        return 1;
    }

    feedHostInputBlocks(thresholdProcessor, 0.12f, 12);
    if (thresholdProcessor.isSampleCaptureWaitingForThreshold()
        || thresholdProcessor.getSampleCaptureDurationSeconds() < 0.1f)
    {
        std::cerr << "Recorder threshold mode did not start after above-threshold input: "
                  << thresholdProcessor.getSampleCaptureDurationSeconds() << "s\n";
        return 1;
    }

    if (! thresholdProcessor.commitSampleCaptureToSampler())
    {
        std::cerr << "Recorder threshold capture did not commit\n";
        return 1;
    }

    const auto thresholdCapturePath = thresholdProcessor.getLoadedSamplePath();
    if (thresholdCapturePath.isNotEmpty())
    {
        const juce::File captureFile(thresholdCapturePath);
        if (captureFile.existsAsFile())
            captureFile.deleteFile();
    }

    const auto capturePath = processor.getLoadedSamplePath();
    if (capturePath.isNotEmpty())
    {
        const juce::File captureFile(capturePath);
        if (captureFile.existsAsFile())
            captureFile.deleteFile();
    }

    std::cout << "Sample recorder audit passed for capture, commit, overview, and sampler playback.\n";
    return 0;
}
