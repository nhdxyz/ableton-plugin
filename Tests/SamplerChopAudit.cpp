#include "../Source/PluginProcessor.h"

#include <cmath>
#include <iostream>
#include <memory>

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

float readPlainParameter(NateVSTAudioProcessor& processor, const juce::String& parameterID, float fallback = 0.0f)
{
    if (auto* parameter = processor.getValueTreeState().getParameter(parameterID))
        return parameter->convertFrom0to1(parameter->getValue());

    return fallback;
}

bool near(float value, float expected, float tolerance = 0.0005f)
{
    return std::abs(value - expected) <= tolerance;
}

bool writeTestSample(const juce::File& file)
{
    juce::AudioBuffer<float> buffer(2, 44100);
    buffer.clear();

    for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto sliceIndex = juce::jlimit(0, 7, (sample * 8) / buffer.getNumSamples());
        const auto frequency = 180.0 + (static_cast<double>(sliceIndex) * 70.0);
        const auto phase = (static_cast<double>(sample) / 44100.0) * frequency * juce::MathConstants<double>::twoPi;
        const auto value = static_cast<float>(std::sin(phase) * 0.72);
        buffer.setSample(0, sample, value);
        buffer.setSample(1, sample, value * (sliceIndex % 2 == 0 ? 0.78f : 0.96f));
    }

    file.deleteFile();
    if (! file.getParentDirectory().createDirectory())
        return false;

    juce::WavAudioFormat format;
    std::unique_ptr<juce::FileOutputStream> fileStream(file.createOutputStream());
    if (fileStream == nullptr || ! fileStream->openedOk())
        return false;

    std::unique_ptr<juce::OutputStream> stream(std::move(fileStream));
    const auto options = juce::AudioFormatWriterOptions {}
        .withSampleRate(44100.0)
        .withNumChannels(2)
        .withBitsPerSample(16);
    auto writer = format.createWriterFor(stream, options);
    return writer != nullptr && writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}

bool writeStepSample(const juce::File& file)
{
    juce::AudioBuffer<float> buffer(2, 44100);
    buffer.clear();

    for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto sliceIndex = juce::jlimit(0, 7, (sample * 8) / buffer.getNumSamples());
        const auto value = 0.10f + (static_cast<float>(sliceIndex) * 0.10f);
        buffer.setSample(0, sample, value);
        buffer.setSample(1, sample, value);
    }

    file.deleteFile();
    if (! file.getParentDirectory().createDirectory())
        return false;

    juce::WavAudioFormat format;
    std::unique_ptr<juce::FileOutputStream> fileStream(file.createOutputStream());
    if (fileStream == nullptr || ! fileStream->openedOk())
        return false;

    std::unique_ptr<juce::OutputStream> stream(std::move(fileStream));
    const auto options = juce::AudioFormatWriterOptions {}
        .withSampleRate(44100.0)
        .withNumChannels(2)
        .withBitsPerSample(16);
    auto writer = format.createWriterFor(stream, options);
    return writer != nullptr && writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}

struct RenderStats
{
    float peak = 0.0f;
    float rms = 0.0f;
    bool finite = true;
};

RenderStats renderSlice(NateVSTAudioProcessor& processor, int midiNote)
{
    juce::AudioBuffer<float> buffer(2, 512);
    RenderStats stats;
    double sumSquares = 0.0;
    auto sampleCount = 0;

    for (auto block = 0; block < 8; ++block)
    {
        juce::MidiBuffer midi;
        if (block == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, midiNote, 1.0f), 0);
        if (block == 3)
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

RenderStats renderSliceFirstBlock(NateVSTAudioProcessor& processor, int midiNote)
{
    juce::AudioBuffer<float> buffer(2, 512);
    RenderStats stats;
    double sumSquares = 0.0;
    auto sampleCount = 0;
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, midiNote, 1.0f), 0);

    processor.processBlock(buffer, midi);
    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const auto* samples = buffer.getReadPointer(channel);
        for (auto sample = 0; sample < juce::jmin(128, buffer.getNumSamples()); ++sample)
        {
            const auto value = samples[sample];
            if (! std::isfinite(value))
                stats.finite = false;

            stats.peak = juce::jmax(stats.peak, std::abs(value));
            sumSquares += static_cast<double>(value) * static_cast<double>(value);
            ++sampleCount;
        }
    }

    stats.rms = sampleCount > 0 ? static_cast<float>(std::sqrt(sumSquares / static_cast<double>(sampleCount))) : 0.0f;
    return stats;
}

bool configureSampleOnly(NateVSTAudioProcessor& processor, const juce::File& sampleFile)
{
    processor.prepareToPlay(44100.0, 512);
    if (! processor.loadSampleFile(sampleFile))
        return false;

    return setPlainParameter(processor, Parameters::ID::osc1Level, 0.0f)
        && setPlainParameter(processor, Parameters::ID::osc2Level, 0.0f)
        && setPlainParameter(processor, Parameters::ID::subLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sampleEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::samplePlaybackMode, 2.0f)
        && setPlainParameter(processor, Parameters::ID::sampleMix, 1.0f)
        && setPlainParameter(processor, Parameters::ID::sampleGain, -1.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxReverbEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxPumpEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::outputGain, 0.0f);
}

bool verifyLoadResetsSliceDefaults(const juce::File& sampleFile)
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);
    setPlainParameter(processor, Parameters::ID::sampleSliceCustom[3], 1.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceStart[3], 0.91f);
    setPlainParameter(processor, Parameters::ID::sampleSliceEnd[3], 0.96f);
    setPlainParameter(processor, Parameters::ID::sampleSliceProbability[3], 0.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceChoke[3], 1.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceNudge[3], 4.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceFade[3], 0.8f);

    if (! processor.loadSampleFile(sampleFile))
        return false;

    for (size_t index = 0; index < Parameters::ID::sampleSliceCustom.size(); ++index)
    {
        const auto expectedStart = static_cast<float>(index) / static_cast<float>(Parameters::ID::sampleSliceCustom.size());
        const auto expectedEnd = static_cast<float>(index + 1) / static_cast<float>(Parameters::ID::sampleSliceCustom.size());

        if (! near(readPlainParameter(processor, Parameters::ID::sampleSliceCustom[index]), 0.0f)
            || ! near(readPlainParameter(processor, Parameters::ID::sampleSliceStart[index]), expectedStart)
            || ! near(readPlainParameter(processor, Parameters::ID::sampleSliceEnd[index]), expectedEnd)
            || ! near(readPlainParameter(processor, Parameters::ID::sampleSliceReverse[index]), 0.0f)
            || ! near(readPlainParameter(processor, Parameters::ID::sampleSliceTranspose[index]), 0.0f)
            || ! near(readPlainParameter(processor, Parameters::ID::sampleSliceGain[index]), -6.0f)
            || ! near(readPlainParameter(processor, Parameters::ID::sampleSlicePan[index]), 0.0f)
            || ! near(readPlainParameter(processor, Parameters::ID::sampleSliceProbability[index]), 1.0f)
            || ! near(readPlainParameter(processor, Parameters::ID::sampleSliceStutter[index]), 0.0f)
            || ! near(readPlainParameter(processor, Parameters::ID::sampleSliceChoke[index]), 0.0f)
            || ! near(readPlainParameter(processor, Parameters::ID::sampleSliceStutterRepeats[index]), 3.0f)
            || ! near(readPlainParameter(processor, Parameters::ID::sampleSliceNudge[index]), 0.0f)
            || ! near(readPlainParameter(processor, Parameters::ID::sampleSliceFade[index]), 0.0f))
        {
            return false;
        }
    }

    return near(readPlainParameter(processor, Parameters::ID::sampleEnabled), 1.0f)
        && near(readPlainParameter(processor, Parameters::ID::samplePlaybackMode), 1.0f)
        && near(readPlainParameter(processor, Parameters::ID::sampleStart), 0.0f)
        && near(readPlainParameter(processor, Parameters::ID::sampleEnd), 1.0f);
}

bool verifySliceKeysRenderFinite(const juce::File& sampleFile)
{
    NateVSTAudioProcessor processor;
    if (! configureSampleOnly(processor, sampleFile))
        return false;

    constexpr auto sliceIndex = 3;
    setPlainParameter(processor, Parameters::ID::sampleSliceCustom[sliceIndex], 1.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceStart[sliceIndex], 0.375f);
    setPlainParameter(processor, Parameters::ID::sampleSliceEnd[sliceIndex], 0.5f);
    setPlainParameter(processor, Parameters::ID::sampleSliceGain[sliceIndex], -2.0f);
    setPlainParameter(processor, Parameters::ID::sampleSlicePan[sliceIndex], 0.45f);
    setPlainParameter(processor, Parameters::ID::sampleSliceTranspose[sliceIndex], 7.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceStutter[sliceIndex], 1.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceChoke[sliceIndex], 1.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceStutterRepeats[sliceIndex], 4.0f);

    const auto stats = renderSlice(processor, 60 + sliceIndex);
    return stats.finite && stats.rms > 0.01f && stats.peak > 0.05f && stats.peak < 1.4f;
}

bool configureNudgeFadeSlice(NateVSTAudioProcessor& processor, const juce::File& sampleFile, float nudgePercent, float fade)
{
    if (! configureSampleOnly(processor, sampleFile))
        return false;

    constexpr auto sliceIndex = 2;
    return setPlainParameter(processor, Parameters::ID::sampleSliceCustom[sliceIndex], 1.0f)
        && setPlainParameter(processor, Parameters::ID::sampleSliceStart[sliceIndex], 0.245f)
        && setPlainParameter(processor, Parameters::ID::sampleSliceEnd[sliceIndex], 0.265f)
        && setPlainParameter(processor, Parameters::ID::sampleSliceGain[sliceIndex], -1.0f)
        && setPlainParameter(processor, Parameters::ID::sampleSlicePan[sliceIndex], 0.0f)
        && setPlainParameter(processor, Parameters::ID::sampleSliceProbability[sliceIndex], 1.0f)
        && setPlainParameter(processor, Parameters::ID::sampleSliceStutter[sliceIndex], 0.0f)
        && setPlainParameter(processor, Parameters::ID::sampleSliceChoke[sliceIndex], 0.0f)
        && setPlainParameter(processor, Parameters::ID::sampleSliceNudge[sliceIndex], nudgePercent)
        && setPlainParameter(processor, Parameters::ID::sampleSliceFade[sliceIndex], fade);
}

bool verifySliceNudgeAndFadeAffectPlayback(const juce::File& sampleFile)
{
    constexpr auto sliceNote = 62;

    NateVSTAudioProcessor tightProcessor;
    if (! configureNudgeFadeSlice(tightProcessor, sampleFile, 0.0f, 0.0f))
        return false;
    const auto tight = renderSliceFirstBlock(tightProcessor, sliceNote);

    NateVSTAudioProcessor nudgedProcessor;
    if (! configureNudgeFadeSlice(nudgedProcessor, sampleFile, 5.0f, 0.0f))
        return false;
    const auto nudged = renderSliceFirstBlock(nudgedProcessor, sliceNote);

    NateVSTAudioProcessor smoothFadeProcessor;
    if (! configureNudgeFadeSlice(smoothFadeProcessor, sampleFile, 0.0f, 0.85f))
        return false;
    const auto smoothFade = renderSliceFirstBlock(smoothFadeProcessor, sliceNote);

    const auto passed = tight.finite
        && nudged.finite
        && smoothFade.finite
        && tight.rms > 0.02f
        && nudged.rms > tight.rms * 1.12f
        && smoothFade.rms < tight.rms * 0.72f;
    if (! passed)
    {
        std::cerr << "Nudge/fade stats: tight rms=" << tight.rms
                  << " nudged rms=" << nudged.rms
                  << " smooth rms=" << smoothFade.rms
                  << " tight peak=" << tight.peak
                  << " nudged peak=" << nudged.peak
                  << " smooth peak=" << smoothFade.peak << '\n';
    }

    return passed;
}

bool verifySliceProbabilitySuppresses(const juce::File& sampleFile)
{
    NateVSTAudioProcessor processor;
    if (! configureSampleOnly(processor, sampleFile))
        return false;

    constexpr auto sliceIndex = 3;
    setPlainParameter(processor, Parameters::ID::sampleSliceCustom[sliceIndex], 1.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceProbability[sliceIndex], 0.0f);

    const auto stats = renderSlice(processor, 60 + sliceIndex);
    return stats.finite && stats.rms < 0.0001f && stats.peak < 0.0001f;
}

bool verifyUkgChopRandomizer(const juce::File& sampleFile)
{
    NateVSTAudioProcessor processor;
    if (! configureSampleOnly(processor, sampleFile))
        return false;

    if (! processor.randomizeUkgVocalChop())
        return false;

    auto sampleRouteCount = 0;
    for (size_t index = 0; index < Parameters::ID::modMatrixSource.size(); ++index)
    {
        const auto enabled = readPlainParameter(processor, Parameters::ID::modMatrixEnabled[index]);
        const auto destination = static_cast<int>(std::round(readPlainParameter(processor, Parameters::ID::modMatrixDestination[index])));
        if (enabled >= 0.5f && (destination == 12 || destination == 14 || destination == 15))
            ++sampleRouteCount;
    }

    return near(readPlainParameter(processor, Parameters::ID::sampleEnabled), 1.0f)
        && near(readPlainParameter(processor, Parameters::ID::samplePlaybackMode), 1.0f)
        && near(readPlainParameter(processor, Parameters::ID::sampleStutterEnabled), 1.0f)
        && std::abs(readPlainParameter(processor, Parameters::ID::samplePitchRamp)) >= 4.5f
        && readPlainParameter(processor, Parameters::ID::sampleMix) >= 0.6f
        && sampleRouteCount >= 3;
}

bool verifyMissingSampleRestoreClearsStaleAudio(const juce::File& savedSampleFile, const juce::File& staleSampleFile)
{
    NateVSTAudioProcessor processor;
    if (! configureSampleOnly(processor, savedSampleFile))
        return false;

    juce::MemoryBlock savedState;
    processor.getStateInformation(savedState);

    savedSampleFile.deleteFile();

    if (! configureSampleOnly(processor, staleSampleFile) || ! processor.hasLoadedSample())
        return false;

    processor.setStateInformation(savedState.getData(), static_cast<int>(savedState.getSize()));

    if (! processor.hasMissingSampleReference()
        || processor.hasLoadedSample()
        || processor.getLoadedSampleName().isNotEmpty()
        || processor.getLoadedSamplePath() != savedSampleFile.getFullPathName())
    {
        return false;
    }

    const auto stats = renderSlice(processor, 60);
    return stats.finite && stats.rms < 0.0001f && stats.peak < 0.0001f;
}
}

int main()
{
    const auto sampleFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("NateVST_SamplerChopAudit.wav");
    const auto staleSampleFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("NateVST_SamplerChopAudit_Stale.wav");
    if (! writeTestSample(sampleFile))
    {
        std::cerr << "Could not write temporary sampler audit sample\n";
        return 1;
    }

    if (! verifyLoadResetsSliceDefaults(sampleFile))
    {
        std::cerr << "Sample load did not reset slice defaults and slice-key base state\n";
        return 1;
    }

    if (! verifySliceKeysRenderFinite(sampleFile))
    {
        std::cerr << "Custom Slice Keys playback was silent, unsafe, or non-finite\n";
        return 1;
    }

    if (! verifySliceProbabilitySuppresses(sampleFile))
    {
        std::cerr << "Slice probability zero did not suppress Slice Keys playback\n";
        return 1;
    }

    if (! verifyUkgChopRandomizer(sampleFile))
    {
        std::cerr << "UKG vocal chop randomizer did not set the expected sample/chop state\n";
        return 1;
    }

    if (! writeTestSample(staleSampleFile))
    {
        std::cerr << "Could not write temporary stale sampler audit sample\n";
        return 1;
    }

    if (! verifyMissingSampleRestoreClearsStaleAudio(sampleFile, staleSampleFile))
    {
        std::cerr << "Missing sample restore did not clear stale sample audio and preserve relink path\n";
        return 1;
    }

    const auto nudgeFadeSampleFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("NateVST_SamplerChopAudit_NudgeFade.wav");
    if (! writeStepSample(nudgeFadeSampleFile))
    {
        std::cerr << "Could not write temporary sampler nudge/fade audit sample\n";
        return 1;
    }

    if (! verifySliceNudgeAndFadeAffectPlayback(nudgeFadeSampleFile))
    {
        std::cerr << "Slice nudge/fade did not audibly affect Slice Keys playback\n";
        return 1;
    }

    sampleFile.deleteFile();
    staleSampleFile.deleteFile();
    nudgeFadeSampleFile.deleteFile();
    std::cout << "Sampler chop audit passed for load defaults, Slice Keys, probability, nudge/fade, missing-file restore, and UKG chop setup.\n";
    return 0;
}
