#include "../Source/PluginProcessor.h"

#include <cmath>
#include <iostream>
#include <limits>

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

void feedHostInputContinuityBlocks(NateVSTAudioProcessor& processor, int blocks)
{
    juce::AudioBuffer<float> buffer(2, 512);
    for (auto block = 0; block < blocks; ++block)
    {
        const auto marker = 0.05f + (static_cast<float>(block) * 0.003f);
        buffer.clear();
        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            buffer.setSample(0, sample, marker);
            buffer.setSample(1, sample, marker * 0.5f);
        }

        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);
    }
}

bool recordConstantHostInputTake(NateVSTAudioProcessor& processor,
                                 float marker,
                                 int blocks,
                                 juce::String& committedPath)
{
    processor.beginSampleCapture();

    juce::AudioBuffer<float> buffer(2, 512);
    for (auto block = 0; block < blocks; ++block)
    {
        buffer.clear();
        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            buffer.setSample(0, sample, marker);
            buffer.setSample(1, sample, marker * 0.5f);
        }

        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);
    }

    if (! processor.commitSampleCaptureToSampler())
        return false;

    committedPath = processor.getLoadedSamplePath();
    return committedPath.isNotEmpty() && juce::File(committedPath).existsAsFile();
}

bool readAudioFile(const juce::File& file, juce::AudioBuffer<float>& destination)
{
    juce::AudioFormatManager manager;
    manager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(file));
    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->numChannels <= 0)
        return false;

    if (reader->lengthInSamples > std::numeric_limits<int>::max())
        return false;

    const auto channelCount = juce::jlimit(1, 2, static_cast<int>(reader->numChannels));
    const auto sampleCount = static_cast<int>(reader->lengthInSamples);
    destination.setSize(channelCount, sampleCount);
    destination.clear();
    return reader->read(&destination, 0, sampleCount, 0, true, true);
}
}

int main()
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);
    const auto captureCapacity = processor.getSampleCaptureCapacitySeconds();
    if (captureCapacity < 15.9f || captureCapacity > 16.1f)
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

    if (processor.getSampleCaptureTakeCount() != 1)
    {
        std::cerr << "Recorder did not register the committed capture as a recent take\n";
        return 1;
    }

    const auto latestTakePath = processor.getLatestSampleCaptureTakePath();
    if (latestTakePath.isEmpty() || latestTakePath != processor.getLoadedSamplePath())
    {
        std::cerr << "Recorder latest take path does not match the committed sample\n";
        return 1;
    }

    if (! juce::File(latestTakePath).existsAsFile())
    {
        std::cerr << "Recorder latest take path does not point to an exportable WAV\n";
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

    NateVSTAudioProcessor preRollProcessor;
    preRollProcessor.prepareToPlay(44100.0, 512);
    if (! setPlainParameter(preRollProcessor, Parameters::ID::sampleRecordSource, 1.0f)
        || ! setPlainParameter(preRollProcessor, Parameters::ID::sampleRecordStart, 2.0f)
        || ! setPlainParameter(preRollProcessor, Parameters::ID::sampleRecordPreRoll, 2.0f)
        || ! setPlainParameter(preRollProcessor, Parameters::ID::osc1Level, 0.0f)
        || ! setPlainParameter(preRollProcessor, Parameters::ID::osc2Level, 0.0f)
        || ! setPlainParameter(preRollProcessor, Parameters::ID::subLevel, 0.0f)
        || ! setPlainParameter(preRollProcessor, Parameters::ID::noiseLevel, 0.0f)
        || ! setPlainParameter(preRollProcessor, Parameters::ID::sampleEnabled, 0.0f)
        || ! setPlainParameter(preRollProcessor, Parameters::ID::sequencerEnabled, 0.0f))
    {
        std::cerr << "Could not configure threshold pre-roll recorder patch\n";
        return 1;
    }

    preRollProcessor.beginSampleCapture();
    const auto preRollSeconds = preRollProcessor.getSampleCapturePreRollDurationSeconds();
    if (preRollSeconds < 0.24f || preRollSeconds > 0.26f)
    {
        std::cerr << "Unexpected recorder pre-roll duration: " << preRollSeconds << "s\n";
        return 1;
    }

    feedHostInputBlocks(preRollProcessor, 0.02f, 32);
    if (! preRollProcessor.isSampleCaptureWaitingForThreshold()
        || preRollProcessor.getSampleCaptureDurationSeconds() > 0.001f)
    {
        std::cerr << "Recorder pre-roll leaked into capture before threshold: "
                  << preRollProcessor.getSampleCaptureDurationSeconds() << "s\n";
        return 1;
    }

    feedHostInputBlocks(preRollProcessor, 0.12f, 1);
    const auto preRolledCaptureSeconds = preRollProcessor.getSampleCaptureDurationSeconds();
    if (preRollProcessor.isSampleCaptureWaitingForThreshold()
        || preRolledCaptureSeconds < 0.25f
        || preRolledCaptureSeconds > 0.28f)
    {
        std::cerr << "Recorder pre-roll was not prepended at threshold start: "
                  << preRolledCaptureSeconds << "s\n";
        return 1;
    }

    if (! preRollProcessor.commitSampleCaptureToSampler())
    {
        std::cerr << "Recorder pre-roll capture did not commit\n";
        return 1;
    }

    const auto preRollCapturePath = preRollProcessor.getLoadedSamplePath();
    if (preRollCapturePath.isNotEmpty())
    {
        const juce::File captureFile(preRollCapturePath);
        if (captureFile.existsAsFile())
            captureFile.deleteFile();
    }

    NateVSTAudioProcessor lengthProcessor;
    lengthProcessor.prepareToPlay(44100.0, 512);
    if (! setPlainParameter(lengthProcessor, Parameters::ID::sampleRecordSource, 1.0f)
        || ! setPlainParameter(lengthProcessor, Parameters::ID::sampleRecordStart, 0.0f)
        || ! setPlainParameter(lengthProcessor, Parameters::ID::sampleRecordLength, 1.0f)
        || ! setPlainParameter(lengthProcessor, Parameters::ID::osc1Level, 0.0f)
        || ! setPlainParameter(lengthProcessor, Parameters::ID::osc2Level, 0.0f)
        || ! setPlainParameter(lengthProcessor, Parameters::ID::subLevel, 0.0f)
        || ! setPlainParameter(lengthProcessor, Parameters::ID::noiseLevel, 0.0f)
        || ! setPlainParameter(lengthProcessor, Parameters::ID::sampleEnabled, 0.0f)
        || ! setPlainParameter(lengthProcessor, Parameters::ID::sequencerEnabled, 0.0f))
    {
        std::cerr << "Could not configure fixed-length recorder patch\n";
        return 1;
    }

    lengthProcessor.beginSampleCapture();
    const auto oneBarTargetSeconds = lengthProcessor.getSampleCaptureTargetDurationSeconds();
    if (oneBarTargetSeconds < 1.85f || oneBarTargetSeconds > 2.05f)
    {
        std::cerr << "Unexpected one-bar recorder target: " << oneBarTargetSeconds << "s\n";
        return 1;
    }

    feedHostInputBlocks(lengthProcessor, 0.14f, 220);
    if (lengthProcessor.isSampleCaptureEnabled())
    {
        std::cerr << "Fixed-length recorder did not auto-stop\n";
        return 1;
    }

    const auto fixedLengthSeconds = lengthProcessor.getSampleCaptureDurationSeconds();
    if (std::abs(fixedLengthSeconds - oneBarTargetSeconds) > 0.025f)
    {
        std::cerr << "Fixed-length recorder duration mismatch: got "
                  << fixedLengthSeconds << "s expected " << oneBarTargetSeconds << "s\n";
        return 1;
    }

    if (! lengthProcessor.commitSampleCaptureToSampler())
    {
        std::cerr << "Fixed-length recorder capture did not commit\n";
        return 1;
    }

    const auto lengthCapturePath = lengthProcessor.getLoadedSamplePath();
    if (lengthCapturePath.isNotEmpty())
    {
        const juce::File captureFile(lengthCapturePath);
        if (captureFile.existsAsFile())
            captureFile.deleteFile();
    }

    NateVSTAudioProcessor continuityProcessor;
    continuityProcessor.prepareToPlay(44100.0, 512);
    if (! setPlainParameter(continuityProcessor, Parameters::ID::sampleRecordSource, 1.0f)
        || ! setPlainParameter(continuityProcessor, Parameters::ID::sampleRecordStart, 0.0f)
        || ! setPlainParameter(continuityProcessor, Parameters::ID::sampleRecordLength, 0.0f)
        || ! setPlainParameter(continuityProcessor, Parameters::ID::osc1Level, 0.0f)
        || ! setPlainParameter(continuityProcessor, Parameters::ID::osc2Level, 0.0f)
        || ! setPlainParameter(continuityProcessor, Parameters::ID::subLevel, 0.0f)
        || ! setPlainParameter(continuityProcessor, Parameters::ID::noiseLevel, 0.0f)
        || ! setPlainParameter(continuityProcessor, Parameters::ID::sampleEnabled, 0.0f)
        || ! setPlainParameter(continuityProcessor, Parameters::ID::sequencerEnabled, 0.0f))
    {
        std::cerr << "Could not configure recorder continuity patch\n";
        return 1;
    }

    constexpr auto continuityBlocks = 24;
    constexpr auto blockSize = 512;
    continuityProcessor.beginSampleCapture();
    feedHostInputContinuityBlocks(continuityProcessor, continuityBlocks);
    const auto continuitySeconds = continuityProcessor.getSampleCaptureDurationSeconds();
    const auto expectedContinuitySeconds = static_cast<float>(continuityBlocks * blockSize) / 44100.0f;
    if (std::abs(continuitySeconds - expectedContinuitySeconds) > 0.001f)
    {
        std::cerr << "Recorder continuity duration mismatch before commit: got "
                  << continuitySeconds << "s expected " << expectedContinuitySeconds << "s\n";
        return 1;
    }

    if (! continuityProcessor.commitSampleCaptureToSampler())
    {
        std::cerr << "Recorder continuity capture did not commit\n";
        return 1;
    }

    const auto continuityCapturePath = continuityProcessor.getLoadedSamplePath();
    if (continuityCapturePath.isEmpty())
    {
        std::cerr << "Recorder continuity capture did not produce a file-backed take\n";
        return 1;
    }

    const juce::File continuityCaptureFile(continuityCapturePath);
    juce::AudioBuffer<float> continuityCapture;
    if (! readAudioFile(continuityCaptureFile, continuityCapture))
    {
        std::cerr << "Could not read recorder continuity capture file\n";
        return 1;
    }

    const auto expectedSamples = continuityBlocks * blockSize;
    if (continuityCapture.getNumSamples() != expectedSamples)
    {
        std::cerr << "Recorder continuity sample count mismatch: got "
                  << continuityCapture.getNumSamples() << " expected " << expectedSamples << '\n';
        return 1;
    }

    for (auto block = 0; block < continuityBlocks; ++block)
    {
        const auto expectedMarker = 0.05f + (static_cast<float>(block) * 0.003f);
        const auto probeSample = (block * blockSize) + (blockSize / 2);
        const auto actualMarker = continuityCapture.getSample(0, probeSample);
        if (std::abs(actualMarker - expectedMarker) > 0.0025f)
        {
            std::cerr << "Recorder continuity marker mismatch at block " << block
                      << ": got " << actualMarker << " expected " << expectedMarker << '\n';
            return 1;
        }
    }

    if (continuityCaptureFile.existsAsFile())
        continuityCaptureFile.deleteFile();

    NateVSTAudioProcessor takeHistoryProcessor;
    takeHistoryProcessor.prepareToPlay(44100.0, 512);
    if (! setPlainParameter(takeHistoryProcessor, Parameters::ID::sampleRecordSource, 1.0f)
        || ! setPlainParameter(takeHistoryProcessor, Parameters::ID::sampleRecordStart, 0.0f)
        || ! setPlainParameter(takeHistoryProcessor, Parameters::ID::sampleRecordLength, 0.0f)
        || ! setPlainParameter(takeHistoryProcessor, Parameters::ID::osc1Level, 0.0f)
        || ! setPlainParameter(takeHistoryProcessor, Parameters::ID::osc2Level, 0.0f)
        || ! setPlainParameter(takeHistoryProcessor, Parameters::ID::subLevel, 0.0f)
        || ! setPlainParameter(takeHistoryProcessor, Parameters::ID::noiseLevel, 0.0f)
        || ! setPlainParameter(takeHistoryProcessor, Parameters::ID::sampleEnabled, 0.0f)
        || ! setPlainParameter(takeHistoryProcessor, Parameters::ID::sequencerEnabled, 0.0f))
    {
        std::cerr << "Could not configure recorder take-history patch\n";
        return 1;
    }

    juce::String firstTakePath;
    juce::String secondTakePath;
    if (! recordConstantHostInputTake(takeHistoryProcessor, 0.07f, 8, firstTakePath)
        || ! recordConstantHostInputTake(takeHistoryProcessor, 0.19f, 8, secondTakePath))
    {
        std::cerr << "Recorder take-history capture did not create two file-backed takes\n";
        return 1;
    }

    if (takeHistoryProcessor.getSampleCaptureTakeCount() != 2)
    {
        std::cerr << "Recorder take history count mismatch after two commits: "
                  << takeHistoryProcessor.getSampleCaptureTakeCount() << '\n';
        return 1;
    }

    const auto takeNames = takeHistoryProcessor.getSampleCaptureTakeNames(12);
    if (takeNames.size() < 2
        || takeNames[0] != juce::File(secondTakePath).getFileNameWithoutExtension()
        || takeNames[1] != juce::File(firstTakePath).getFileNameWithoutExtension())
    {
        std::cerr << "Recorder take names are not newest-first after two commits\n";
        return 1;
    }

    if (takeHistoryProcessor.getSelectedSampleCaptureTakeIndex() != 0
        || takeHistoryProcessor.getSelectedSampleCaptureTakePath() != secondTakePath)
    {
        std::cerr << "Newest recorder take was not selected after commit\n";
        return 1;
    }

    if (! takeHistoryProcessor.selectSampleCaptureTake(1)
        || takeHistoryProcessor.getSelectedSampleCaptureTakeIndex() != 1
        || takeHistoryProcessor.getSelectedSampleCaptureTakePath() != firstTakePath)
    {
        std::cerr << "Recorder could not select the older committed take\n";
        return 1;
    }

    if (takeHistoryProcessor.getLatestSampleCaptureTakePath() != secondTakePath)
    {
        std::cerr << "Selecting an older recorder take changed the latest-take pointer\n";
        return 1;
    }

    juce::AudioBuffer<float> selectedOlderTake;
    if (! readAudioFile(juce::File(takeHistoryProcessor.getSelectedSampleCaptureTakePath()), selectedOlderTake)
        || selectedOlderTake.getNumSamples() < 1024)
    {
        std::cerr << "Could not read selected older recorder take\n";
        return 1;
    }

    const auto selectedMarker = selectedOlderTake.getSample(0, selectedOlderTake.getNumSamples() / 2);
    if (std::abs(selectedMarker - 0.07f) > 0.0025f)
    {
        std::cerr << "Selected older recorder take has the wrong audio marker: "
                  << selectedMarker << '\n';
        return 1;
    }

    juce::AudioBuffer<float> newestTake;
    if (! readAudioFile(juce::File(secondTakePath), newestTake)
        || newestTake.getNumSamples() < 1024)
    {
        std::cerr << "Could not read newest recorder take after double-buffered capture\n";
        return 1;
    }

    const auto newestMarker = newestTake.getSample(0, newestTake.getNumSamples() / 2);
    if (std::abs(newestMarker - 0.19f) > 0.0025f)
    {
        std::cerr << "Newest recorder take has stale or wrong audio after buffer swap: "
                  << newestMarker << '\n';
        return 1;
    }

    for (const auto& takePath : { firstTakePath, secondTakePath })
    {
        const juce::File takeFile(takePath);
        if (takeFile.existsAsFile())
            takeFile.deleteFile();
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
