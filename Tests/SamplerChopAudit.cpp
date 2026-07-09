#include "../Source/PluginProcessor.h"

#include <array>
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

bool seedEmptyRoutesWithStaleShape(NateVSTAudioProcessor& processor)
{
    for (size_t index = 0; index < Parameters::ID::modMatrixSource.size(); ++index)
    {
        if (! setPlainParameter(processor, Parameters::ID::modMatrixSource[index], 0.0f)
            || ! setPlainParameter(processor, Parameters::ID::modMatrixDestination[index], 0.0f)
            || ! setPlainParameter(processor, Parameters::ID::modMatrixAmount[index], 0.0f)
            || ! setPlainParameter(processor, Parameters::ID::modMatrixPolarity[index], 3.0f)
            || ! setPlainParameter(processor, Parameters::ID::modMatrixCurve[index], 4.0f)
            || ! setPlainParameter(processor, Parameters::ID::modMatrixRangeMin[index], 0.25f)
            || ! setPlainParameter(processor, Parameters::ID::modMatrixRangeMax[index], 0.55f)
            || ! setPlainParameter(processor, Parameters::ID::modMatrixSlew[index], 0.62f))
        {
            return false;
        }
    }

    return true;
}

bool routeShapeIsNeutral(NateVSTAudioProcessor& processor, size_t index)
{
    return near(readPlainParameter(processor, Parameters::ID::modMatrixPolarity[index]), 0.0f)
        && near(readPlainParameter(processor, Parameters::ID::modMatrixCurve[index]), 0.0f)
        && near(readPlainParameter(processor, Parameters::ID::modMatrixRangeMin[index]), -1.0f)
        && near(readPlainParameter(processor, Parameters::ID::modMatrixRangeMax[index]), 1.0f)
        && near(readPlainParameter(processor, Parameters::ID::modMatrixSlew[index]), 0.0f);
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

bool writeTransientSample(const juce::File& file)
{
    juce::AudioBuffer<float> buffer(2, 44100);
    buffer.clear();

    const std::array<float, 8> transientPositions { 0.0f, 0.075f, 0.19f, 0.33f, 0.48f, 0.62f, 0.77f, 0.90f };
    for (size_t transientIndex = 0; transientIndex < transientPositions.size(); ++transientIndex)
    {
        const auto startSample = juce::jlimit(0,
                                              buffer.getNumSamples() - 1,
                                              juce::roundToInt(transientPositions[transientIndex]
                                                               * static_cast<float>(buffer.getNumSamples())));
        const auto burstLength = transientIndex == 0 ? 2200 : 1500;
        const auto frequency = 180.0 + (static_cast<double>(transientIndex) * 65.0);
        for (auto offset = 0; offset < burstLength && startSample + offset < buffer.getNumSamples(); ++offset)
        {
            const auto envelope = std::exp(-static_cast<double>(offset) / 310.0);
            const auto phase = (static_cast<double>(offset) / 44100.0) * frequency * juce::MathConstants<double>::twoPi;
            const auto value = static_cast<float>((0.84 * envelope) * std::sin(phase));
            buffer.addSample(0, startSample + offset, value);
            buffer.addSample(1, startSample + offset, value * (transientIndex % 2 == 0 ? 0.82f : 0.98f));
        }
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

bool verifyTransientDetectionWritesCustomSlices(const juce::File& sampleFile)
{
    NateVSTAudioProcessor processor;
    if (! configureSampleOnly(processor, sampleFile))
        return false;

    const auto transientCount = processor.detectSampleTransientSlices();
    if (transientCount < 5)
    {
        std::cerr << "Expected at least 5 detected transients, got " << transientCount << '\n';
        return false;
    }

    if (! near(readPlainParameter(processor, Parameters::ID::sampleEnabled), 1.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::samplePlaybackMode), 2.0f))
    {
        std::cerr << "Transient detection did not leave sample enabled in Slice Keys mode\n";
        return false;
    }

    auto previousEnd = 0.0f;
    auto movedStartCount = 0;
    for (size_t index = 0; index < Parameters::ID::sampleSliceCustom.size(); ++index)
    {
        const auto custom = readPlainParameter(processor, Parameters::ID::sampleSliceCustom[index]);
        const auto start = readPlainParameter(processor, Parameters::ID::sampleSliceStart[index]);
        const auto end = readPlainParameter(processor, Parameters::ID::sampleSliceEnd[index]);
        const auto equalStart = static_cast<float>(index) / static_cast<float>(Parameters::ID::sampleSliceCustom.size());

        if (! near(custom, 1.0f)
            || start < -0.0001f
            || end > 1.0001f
            || end <= start + 0.006f
            || start + 0.0005f < previousEnd)
        {
            std::cerr << "Unsafe detected slice " << index
                      << " custom=" << custom
                      << " start=" << start
                      << " end=" << end
                      << " previousEnd=" << previousEnd << '\n';
            return false;
        }

        if (index > 0 && std::abs(start - equalStart) > 0.018f)
            ++movedStartCount;

        previousEnd = end;
    }

    if (movedStartCount < 4)
    {
        std::cerr << "Transient detection did not move enough slice starts, moved "
                  << movedStartCount << '\n';
        return false;
    }

    const auto first = renderSlice(processor, 60);
    const auto fifth = renderSlice(processor, 64);
    return first.finite
        && fifth.finite
        && first.rms > 0.003f
        && fifth.rms > 0.003f
        && first.peak < 1.4f
        && fifth.peak < 1.4f;
}

bool verifyBeatGridSlicingAndClear(const juce::File& sampleFile)
{
    NateVSTAudioProcessor processor;
    if (! configureSampleOnly(processor, sampleFile))
        return false;

    if (processor.detectSampleTransientSlices() < 0)
        return false;

    if (! processor.sliceSampleToBeatGrid())
    {
        std::cerr << "Beat-grid slicing action returned false\n";
        return false;
    }

    for (size_t index = 0; index < Parameters::ID::sampleSliceCustom.size(); ++index)
    {
        const auto expectedStart = static_cast<float>(index) / static_cast<float>(Parameters::ID::sampleSliceCustom.size());
        const auto expectedEnd = static_cast<float>(index + 1) / static_cast<float>(Parameters::ID::sampleSliceCustom.size());
        if (! near(readPlainParameter(processor, Parameters::ID::sampleSliceCustom[index]), 1.0f)
            || ! near(readPlainParameter(processor, Parameters::ID::sampleSliceStart[index]), expectedStart)
            || ! near(readPlainParameter(processor, Parameters::ID::sampleSliceEnd[index]), expectedEnd))
        {
            std::cerr << "Beat-grid slice " << index
                      << " was not reset to equal divisions: custom="
                      << readPlainParameter(processor, Parameters::ID::sampleSliceCustom[index])
                      << " start=" << readPlainParameter(processor, Parameters::ID::sampleSliceStart[index])
                      << " end=" << readPlainParameter(processor, Parameters::ID::sampleSliceEnd[index]) << '\n';
            return false;
        }
    }

    if (! near(readPlainParameter(processor, Parameters::ID::samplePlaybackMode), 2.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sampleStart), 0.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::sampleEnd), 0.125f))
    {
        std::cerr << "Beat-grid slicing did not focus the first slice in Slice Keys mode\n";
        return false;
    }

    constexpr auto sliceIndex = 3;
    setPlainParameter(processor, Parameters::ID::sampleSliceReverse[sliceIndex], 1.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceTranspose[sliceIndex], 7.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceGain[sliceIndex], -2.0f);
    setPlainParameter(processor, Parameters::ID::sampleSlicePan[sliceIndex], -0.45f);
    setPlainParameter(processor, Parameters::ID::sampleSliceProbability[sliceIndex], 0.62f);
    setPlainParameter(processor, Parameters::ID::sampleSliceStutter[sliceIndex], 1.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceChoke[sliceIndex], 1.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceStutterRepeats[sliceIndex], 5.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceNudge[sliceIndex], -2.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceFade[sliceIndex], 0.7f);

    if (! processor.clearSampleSliceMarker(sliceIndex))
    {
        std::cerr << "Clear-slice action returned false\n";
        return false;
    }

    const auto expectedStart = static_cast<float>(sliceIndex) / static_cast<float>(Parameters::ID::sampleSliceCustom.size());
    const auto expectedEnd = static_cast<float>(sliceIndex + 1) / static_cast<float>(Parameters::ID::sampleSliceCustom.size());
    return near(readPlainParameter(processor, Parameters::ID::sampleSliceCustom[sliceIndex]), 0.0f)
        && near(readPlainParameter(processor, Parameters::ID::sampleSliceStart[sliceIndex]), expectedStart)
        && near(readPlainParameter(processor, Parameters::ID::sampleSliceEnd[sliceIndex]), expectedEnd)
        && near(readPlainParameter(processor, Parameters::ID::sampleSliceReverse[sliceIndex]), 0.0f)
        && near(readPlainParameter(processor, Parameters::ID::sampleSliceTranspose[sliceIndex]), 0.0f)
        && near(readPlainParameter(processor, Parameters::ID::sampleSliceGain[sliceIndex]), -6.0f)
        && near(readPlainParameter(processor, Parameters::ID::sampleSlicePan[sliceIndex]), 0.0f)
        && near(readPlainParameter(processor, Parameters::ID::sampleSliceProbability[sliceIndex]), 1.0f)
        && near(readPlainParameter(processor, Parameters::ID::sampleSliceStutter[sliceIndex]), 0.0f)
        && near(readPlainParameter(processor, Parameters::ID::sampleSliceChoke[sliceIndex]), 0.0f)
        && near(readPlainParameter(processor, Parameters::ID::sampleSliceStutterRepeats[sliceIndex]), 3.0f)
        && near(readPlainParameter(processor, Parameters::ID::sampleSliceNudge[sliceIndex]), 0.0f)
        && near(readPlainParameter(processor, Parameters::ID::sampleSliceFade[sliceIndex]), 0.0f)
        && near(readPlainParameter(processor, Parameters::ID::sampleStart), expectedStart)
        && near(readPlainParameter(processor, Parameters::ID::sampleEnd), expectedEnd);
}

bool verifySampleLabProActions(const juce::File& sampleFile)
{
    NateVSTAudioProcessor processor;
    if (! configureSampleOnly(processor, sampleFile))
        return false;

    setPlainParameter(processor, Parameters::ID::sampleSliceProbability[7], 0.72f);
    setPlainParameter(processor, Parameters::ID::sampleSliceStutter[7], 1.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceStutterRepeats[7], 4.0f);
    setPlainParameter(processor, Parameters::ID::sampleSliceNudge[7], -1.8f);

    if (! processor.sendSampleSlicesToSequencer())
    {
        std::cerr << "Slice-to-sequencer action returned false\n";
        return false;
    }

    const auto firstStep = processor.getSequencerStep(0);
    const auto variationStep = processor.getSequencerStep(12);
    if (! firstStep.enabled
        || firstStep.noteOffset != 0
        || ! variationStep.enabled
        || variationStep.noteOffset != 7
        || variationStep.ratchet != 4
        || variationStep.condition != 1
        || std::abs(variationStep.probability - 0.72f) > 0.01f
        || std::abs(readPlainParameter(processor, Parameters::ID::sequencerRoot) - 60.0f) > 0.01f
        || readPlainParameter(processor, Parameters::ID::sequencerEnabled) < 0.5f
        || std::abs(readPlainParameter(processor, Parameters::ID::samplePlaybackMode) - 2.0f) > 0.01f)
    {
        std::cerr << "Slice-to-sequencer action did not build expected slice-key pattern: first note "
                  << firstStep.noteOffset
                  << " variation note " << variationStep.noteOffset
                  << " ratchet " << variationStep.ratchet
                  << " condition " << variationStep.condition
                  << " probability " << variationStep.probability << '\n';
        return false;
    }

    setPlainParameter(processor, Parameters::ID::sampleStart, 0.08f);
    setPlainParameter(processor, Parameters::ID::sampleEnd, 0.92f);
    if (! processor.sendSampleRegionToWavetable(true))
    {
        std::cerr << "Sample-to-wavetable action returned false\n";
        return false;
    }

    auto frameRange = [&processor] (size_t frameIndex)
    {
        auto minimum = 1.0f;
        auto maximum = 0.0f;
        for (size_t pointIndex = 0; pointIndex < Parameters::ID::osc2CustomWave.size(); ++pointIndex)
        {
            const auto parameterID = frameIndex == 0
                ? juce::String(Parameters::ID::osc2CustomWave[pointIndex])
                : Parameters::customWaveMorphFrameParameterID(true, frameIndex, pointIndex);
            const auto value = readPlainParameter(processor, parameterID, 0.5f);
            minimum = juce::jmin(minimum, value);
            maximum = juce::jmax(maximum, value);
        }

        return maximum - minimum;
    };

    if (std::abs(readPlainParameter(processor, Parameters::ID::osc2Wave) - 7.0f) > 0.01f
        || readPlainParameter(processor, Parameters::ID::osc2Level) < 0.80f
        || frameRange(0) < 0.35f
        || frameRange(4) < 0.35f)
    {
        std::cerr << "Sample-to-wavetable action did not populate Osc 2 custom frames: wave "
                  << readPlainParameter(processor, Parameters::ID::osc2Wave)
                  << " level " << readPlainParameter(processor, Parameters::ID::osc2Level)
                  << " range0 " << frameRange(0)
                  << " range4 " << frameRange(4) << '\n';
        return false;
    }

    return true;
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

    if (! seedEmptyRoutesWithStaleShape(processor))
        return false;

    if (! processor.randomizeUkgVocalChop())
        return false;

    auto sampleRouteCount = 0;
    auto sampleRoutesHaveNeutralShape = true;
    for (size_t index = 0; index < Parameters::ID::modMatrixSource.size(); ++index)
    {
        const auto enabled = readPlainParameter(processor, Parameters::ID::modMatrixEnabled[index]);
        const auto destination = static_cast<int>(std::round(readPlainParameter(processor, Parameters::ID::modMatrixDestination[index])));
        if (enabled >= 0.5f && (destination == 12 || destination == 14 || destination == 15))
        {
            ++sampleRouteCount;
            sampleRoutesHaveNeutralShape = sampleRoutesHaveNeutralShape && routeShapeIsNeutral(processor, index);
        }
    }

    return near(readPlainParameter(processor, Parameters::ID::sampleEnabled), 1.0f)
        && near(readPlainParameter(processor, Parameters::ID::samplePlaybackMode), 1.0f)
        && near(readPlainParameter(processor, Parameters::ID::sampleStutterEnabled), 1.0f)
        && std::abs(readPlainParameter(processor, Parameters::ID::samplePitchRamp)) >= 4.5f
        && readPlainParameter(processor, Parameters::ID::sampleMix) >= 0.6f
        && sampleRouteCount >= 3
        && sampleRoutesHaveNeutralShape;
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
    const auto transientSampleFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("NateVST_SamplerChopAudit_Transient.wav");
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

    if (! writeTransientSample(transientSampleFile))
    {
        std::cerr << "Could not write temporary sampler transient audit sample\n";
        return 1;
    }

    if (! verifyTransientDetectionWritesCustomSlices(transientSampleFile))
    {
        std::cerr << "Transient slice detection did not create usable custom Slice Keys regions\n";
        return 1;
    }

    if (! verifyBeatGridSlicingAndClear(transientSampleFile))
    {
        std::cerr << "Beat-grid slicing and clear marker actions did not reset Slice Keys safely\n";
        return 1;
    }

    if (! verifySampleLabProActions(transientSampleFile))
    {
        std::cerr << "Sample Lab Pro actions did not create sequencer and wavetable material\n";
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
    transientSampleFile.deleteFile();
    nudgeFadeSampleFile.deleteFile();
    std::cout << "Sampler chop audit passed for load defaults, transient detection, beat-grid slicing, marker clear, Slice Keys, Sample Lab Pro actions, probability, nudge/fade, missing-file restore, and UKG chop setup.\n";
    return 0;
}
