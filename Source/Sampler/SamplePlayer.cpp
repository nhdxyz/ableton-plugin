#include "SamplePlayer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr auto chopFadeSamples = 64;
constexpr auto stutterFadeSamples = 48;
constexpr auto zeroCrossSearchSamples = 256;

double stutterIntervalSamplesForRate(int rateIndex, double bpm, double sampleRate)
{
    const auto safeBpm = juce::jlimit(20.0, 300.0, bpm);
    const auto safeSampleRate = juce::jmax(1.0, sampleRate);
    const auto quarterNoteSamples = (60.0 / safeBpm) * safeSampleRate;

    switch (rateIndex)
    {
        case 0: return quarterNoteSamples / 2.0; // 1/8
        case 2: return quarterNoteSamples / 8.0; // 1/32
        case 1:
        default:
            return quarterNoteSamples / 4.0; // 1/16
    }
}

int boundaryFadeSamplesForSpan(int sourceSpan)
{
    return juce::jlimit(8, chopFadeSamples, juce::jmax(1, sourceSpan / 4));
}

float mixedSampleAt(const juce::AudioBuffer<float>& buffer, int sampleIndex)
{
    const auto channelCount = buffer.getNumChannels();
    if (channelCount <= 0 || buffer.getNumSamples() <= 0)
        return 0.0f;

    sampleIndex = juce::jlimit(0, buffer.getNumSamples() - 1, sampleIndex);

    auto mixedSample = 0.0f;
    for (auto channel = 0; channel < channelCount; ++channel)
        mixedSample += buffer.getSample(channel, sampleIndex);

    return mixedSample / static_cast<float>(channelCount);
}

bool crossesZero(float previous, float current)
{
    return (previous <= 0.0f && current >= 0.0f)
        || (previous >= 0.0f && current <= 0.0f);
}

int findNearestCleanBoundary(const juce::AudioBuffer<float>& buffer, int targetSample, int minimumSample, int maximumSample)
{
    if (buffer.getNumSamples() <= 1)
        return targetSample;

    minimumSample = juce::jlimit(0, buffer.getNumSamples() - 1, minimumSample);
    maximumSample = juce::jlimit(minimumSample, buffer.getNumSamples() - 1, maximumSample);
    if (maximumSample <= minimumSample)
        return minimumSample;

    targetSample = juce::jlimit(minimumSample, maximumSample, targetSample);

    const auto searchStart = juce::jmax(minimumSample, targetSample - zeroCrossSearchSamples);
    const auto searchEnd = juce::jmin(maximumSample, targetSample + zeroCrossSearchSamples);
    auto bestSample = targetSample;
    auto bestScore = std::numeric_limits<float>::max();

    for (auto sampleIndex = searchStart; sampleIndex <= searchEnd; ++sampleIndex)
    {
        const auto current = mixedSampleAt(buffer, sampleIndex);
        const auto previous = mixedSampleAt(buffer, juce::jmax(minimumSample, sampleIndex - 1));
        const auto distance = static_cast<float>(std::abs(sampleIndex - targetSample));
        const auto crossingPenalty = crossesZero(previous, current) ? 0.0f : 5.0f;
        const auto score = crossingPenalty + (std::abs(current) * 80.0f) + (distance * 0.012f);

        if (score < bestScore)
        {
            bestScore = score;
            bestSample = sampleIndex;
        }
    }

    return bestSample;
}

Sampler::SampleRegion snapRegionToCleanBoundaries(const juce::AudioBuffer<float>& buffer, Sampler::SampleRegion region)
{
    const auto numSamples = buffer.getNumSamples();
    if (numSamples <= 1)
        return region;

    const auto minimumWindow = juce::jmin(numSamples, 64);
    const auto originalStart = region.startSample;
    const auto originalEnd = region.endSample;

    if (region.startSample > 0)
        region.startSample = findNearestCleanBoundary(buffer,
                                                      region.startSample,
                                                      0,
                                                      juce::jmax(0, region.endSample - minimumWindow));

    if (region.endSample < numSamples)
    {
        const auto targetEndBoundary = juce::jlimit(region.startSample + minimumWindow - 1,
                                                   numSamples - 1,
                                                   region.endSample - 1);
        const auto snappedEndBoundary = findNearestCleanBoundary(buffer,
                                                                 targetEndBoundary,
                                                                 region.startSample + minimumWindow - 1,
                                                                 numSamples - 1);
        region.endSample = snappedEndBoundary + 1;
    }

    if (region.endSample <= region.startSample + 1)
    {
        region.startSample = originalStart;
        region.endSample = originalEnd;
    }

    return region;
}
}

namespace Sampler
{
SamplePlayer::SamplePlayer(Parameters::APVTS& state)
    : parameters(state)
{
    formatManager.registerBasicFormats();
    sampleEnabled = parameters.getRawParameterValue(Parameters::ID::sampleEnabled);
    sampleStart = parameters.getRawParameterValue(Parameters::ID::sampleStart);
    sampleEnd = parameters.getRawParameterValue(Parameters::ID::sampleEnd);
    sampleReverse = parameters.getRawParameterValue(Parameters::ID::sampleReverse);
    samplePlaybackMode = parameters.getRawParameterValue(Parameters::ID::samplePlaybackMode);
    sampleTranspose = parameters.getRawParameterValue(Parameters::ID::sampleTranspose);
    samplePitchRamp = parameters.getRawParameterValue(Parameters::ID::samplePitchRamp);
    sampleGain = parameters.getRawParameterValue(Parameters::ID::sampleGain);
    sampleMix = parameters.getRawParameterValue(Parameters::ID::sampleMix);
    sampleStutterEnabled = parameters.getRawParameterValue(Parameters::ID::sampleStutterEnabled);
    sampleStutterRate = parameters.getRawParameterValue(Parameters::ID::sampleStutterRate);
    sampleStutterRepeats = parameters.getRawParameterValue(Parameters::ID::sampleStutterRepeats);
}

void SamplePlayer::prepare(double sampleRate)
{
    playbackSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
}

void SamplePlayer::clear()
{
    const juce::SpinLock::ScopedLockType lock(sampleLock);
    sampleData.reset();
    voices = {};
    region = {};
}

bool SamplePlayer::loadFile(const juce::File& file)
{
    if (! file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
        return false;

    const auto maxLoadedSamples = static_cast<juce::int64>(30.0 * reader->sampleRate);
    const auto length = static_cast<int>(juce::jmin<juce::int64>(reader->lengthInSamples, maxLoadedSamples));

    auto newSample = std::make_shared<SampleData>();
    newSample->buffer.setSize(static_cast<int>(reader->numChannels), length);
    newSample->sourceSampleRate = reader->sampleRate;
    newSample->fileName = file.getFileName();
    reader->read(&newSample->buffer, 0, length, 0, true, true);

    {
        const juce::SpinLock::ScopedLockType lock(sampleLock);
        sampleData = std::move(newSample);
        voices = {};
        region.startSample = 0;
        region.endSample = length;
        region.reverse = false;
        region.gain = 1.0f;
        region.transposeSemitones = 0.0f;
    }

    return true;
}

bool SamplePlayer::hasSample() const
{
    const juce::SpinLock::ScopedLockType lock(sampleLock);
    return sampleData != nullptr && sampleData->buffer.getNumSamples() > 0;
}

juce::String SamplePlayer::getLoadedFileName() const
{
    const juce::SpinLock::ScopedLockType lock(sampleLock);
    return sampleData != nullptr ? sampleData->fileName : juce::String {};
}

SamplePeakOverview SamplePlayer::createPeakOverview(int pointCount) const
{
    pointCount = juce::jlimit(32, 512, pointCount);

    std::shared_ptr<SampleData> data;
    {
        const juce::SpinLock::ScopedLockType lock(sampleLock);
        data = sampleData;
    }

    SamplePeakOverview overview;
    if (data == nullptr || data->buffer.getNumSamples() <= 0 || data->buffer.getNumChannels() <= 0)
        return overview;

    overview.fileName = data->fileName;
    overview.totalSamples = data->buffer.getNumSamples();
    overview.sourceSampleRate = data->sourceSampleRate;
    overview.minimums.assign(static_cast<size_t>(pointCount), 0.0f);
    overview.maximums.assign(static_cast<size_t>(pointCount), 0.0f);

    const auto channelCount = data->buffer.getNumChannels();
    const auto totalSamples = data->buffer.getNumSamples();

    for (auto pointIndex = 0; pointIndex < pointCount; ++pointIndex)
    {
        const auto startSample = static_cast<int>((static_cast<int64_t>(pointIndex) * totalSamples) / pointCount);
        const auto endSample = juce::jmax(startSample + 1,
                                          static_cast<int>((static_cast<int64_t>(pointIndex + 1) * totalSamples) / pointCount));
        auto minimum = 0.0f;
        auto maximum = 0.0f;

        for (auto sampleIndex = startSample; sampleIndex < juce::jmin(endSample, totalSamples); ++sampleIndex)
        {
            auto mixedSample = 0.0f;
            for (auto channel = 0; channel < channelCount; ++channel)
                mixedSample += data->buffer.getSample(channel, sampleIndex);

            mixedSample /= static_cast<float>(channelCount);
            minimum = juce::jmin(minimum, mixedSample);
            maximum = juce::jmax(maximum, mixedSample);
        }

        overview.minimums[static_cast<size_t>(pointIndex)] = juce::jlimit(-1.0f, 1.0f, minimum);
        overview.maximums[static_cast<size_t>(pointIndex)] = juce::jlimit(-1.0f, 1.0f, maximum);
    }

    return overview;
}

SampleRegion SamplePlayer::getRegion() const
{
    const juce::SpinLock::ScopedLockType lock(sampleLock);
    return region;
}

void SamplePlayer::setRegion(SampleRegion newRegion)
{
    const juce::SpinLock::ScopedLockType lock(sampleLock);

    const auto sampleLength = sampleData != nullptr ? sampleData->buffer.getNumSamples() : 0;
    newRegion.startSample = juce::jlimit(0, juce::jmax(0, sampleLength - 1), newRegion.startSample);
    newRegion.endSample = juce::jlimit(newRegion.startSample, sampleLength, newRegion.endSample);
    newRegion.gain = juce::jlimit(0.0f, 2.0f, newRegion.gain);
    newRegion.transposeSemitones = juce::jlimit(-36.0f, 36.0f, newRegion.transposeSemitones);
    region = newRegion;
}

bool SamplePlayer::triggerAudition(int midiNoteNumber, float velocity, double bpm)
{
    const juce::SpinLock::ScopedLockType lock(sampleLock);

    if (sampleData == nullptr || sampleData->buffer.getNumSamples() == 0)
        return false;

    startVoice(*sampleData, midiNoteNumber, velocity, bpm, true);
    return true;
}

void SamplePlayer::render(juce::AudioBuffer<float>& outputBuffer, const juce::MidiBuffer& midi, double bpm)
{
    if (readParameter(sampleEnabled, 0.0f) < 0.5f)
        return;

    const juce::SpinLock::ScopedTryLockType lock(sampleLock);
    if (! lock.isLocked() || sampleData == nullptr || sampleData->buffer.getNumSamples() == 0)
        return;

    auto cursor = 0;

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        const auto eventPosition = juce::jlimit(0, outputBuffer.getNumSamples(), metadata.samplePosition);

        if (eventPosition > cursor)
        {
            renderActiveVoices(*sampleData, outputBuffer, cursor, eventPosition - cursor);
            cursor = eventPosition;
        }

        if (message.isNoteOn())
            startVoice(*sampleData, message.getNoteNumber(), message.getFloatVelocity(), bpm, false);
        else if (message.isNoteOff())
            stopVoicesForNote(message.getNoteNumber());
    }

    if (cursor < outputBuffer.getNumSamples())
        renderActiveVoices(*sampleData, outputBuffer, cursor, outputBuffer.getNumSamples() - cursor);
}

void SamplePlayer::startVoice(const SampleData& data, int midiNoteNumber, float velocity, double bpm, bool forceOneShot)
{
    auto* voiceToUse = std::find_if(voices.begin(), voices.end(), [] (const Voice& voice)
    {
        return ! voice.active;
    });

    if (voiceToUse == voices.end())
        voiceToUse = voices.begin();

    const auto currentRegion = currentRegionFor(data);
    if (currentRegion.endSample <= currentRegion.startSample + 1)
        return;

    const auto reverse = currentRegion.reverse;
    const auto transpose = currentRegion.transposeSemitones + static_cast<float>(midiNoteNumber - 60);
    const auto sourceRatio = data.sourceSampleRate / playbackSampleRate;
    const auto pitchRatio = std::pow(2.0, static_cast<double>(transpose) / 12.0);

    voiceToUse->active = true;
    voiceToUse->startSample = currentRegion.startSample;
    voiceToUse->endSample = currentRegion.endSample;
    voiceToUse->midiNoteNumber = midiNoteNumber;
    voiceToUse->reverse = reverse;
    voiceToUse->gated = ! forceOneShot && readParameter(samplePlaybackMode, 1.0f) < 0.5f;
    voiceToUse->velocity = velocity;
    voiceToUse->sourceRatio = sourceRatio;
    voiceToUse->baseTransposeSemitones = transpose;
    voiceToUse->pitchRampSemitones = readParameter(samplePitchRamp, 0.0f);
    voiceToUse->increment = sourceRatio * pitchRatio * (reverse ? -1.0 : 1.0);
    voiceToUse->position = reverse ? static_cast<double>(currentRegion.endSample - 1)
                                   : static_cast<double>(currentRegion.startSample);
    voiceToUse->stutterEnabled = readParameter(sampleStutterEnabled, 0.0f) >= 0.5f;
    voiceToUse->stutterRepeatsRemaining = voiceToUse->stutterEnabled
        ? juce::jlimit(1, 8, static_cast<int>(std::round(readParameter(sampleStutterRepeats, 3.0f))))
        : 0;
    voiceToUse->stutterIntervalSamples = stutterIntervalSamplesForRate(
        juce::jlimit(0, 2, static_cast<int>(std::round(readParameter(sampleStutterRate, 1.0f)))),
        bpm,
        playbackSampleRate);
    voiceToUse->samplesUntilStutter = voiceToUse->stutterIntervalSamples;
    voiceToUse->fadeInTotalSamples = boundaryFadeSamplesForSpan(currentRegion.endSample - currentRegion.startSample);
    voiceToUse->fadeInSamplesRemaining = voiceToUse->fadeInTotalSamples;
}

void SamplePlayer::stopVoicesForNote(int midiNoteNumber)
{
    for (auto& voice : voices)
        if (voice.active && voice.gated && voice.midiNoteNumber == midiNoteNumber)
            voice.active = false;
}

void SamplePlayer::renderActiveVoices(const SampleData& data,
                                      juce::AudioBuffer<float>& outputBuffer,
                                      int startSampleInBlock,
                                      int numSamples)
{
    if (numSamples <= 0)
        return;

    for (auto& voice : voices)
        if (voice.active)
            renderVoice(voice, data, outputBuffer, startSampleInBlock, numSamples);
}

void SamplePlayer::renderVoice(Voice& voice,
                               const SampleData& data,
                               juce::AudioBuffer<float>& outputBuffer,
                               int startSampleInBlock,
                               int numSamples)
{
    const auto sourceChannels = data.buffer.getNumChannels();
    const auto outputChannels = outputBuffer.getNumChannels();
    const auto gain = juce::Decibels::decibelsToGain(readParameter(sampleGain, -6.0f))
                    * readParameter(sampleMix, 0.75f)
                    * voice.velocity;

    if (sourceChannels == 0 || outputChannels == 0)
    {
        voice.active = false;
        return;
    }

    for (auto sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        if (voice.stutterEnabled && voice.stutterRepeatsRemaining > 0 && voice.samplesUntilStutter <= 0.0)
        {
            voice.position = voice.reverse ? static_cast<double>(voice.endSample - 1)
                                           : static_cast<double>(voice.startSample);
            voice.fadeInTotalSamples = juce::jmin(stutterFadeSamples, boundaryFadeSamplesForSpan(voice.endSample - voice.startSample));
            voice.fadeInSamplesRemaining = voice.fadeInTotalSamples;
            --voice.stutterRepeatsRemaining;
            voice.samplesUntilStutter += voice.stutterIntervalSamples;
        }

        const auto floorPosition = static_cast<int>(std::floor(voice.position));

        if ((! voice.reverse && floorPosition >= voice.endSample - 1)
            || (voice.reverse && floorPosition <= voice.startSample))
        {
            voice.active = false;
            return;
        }

        const auto fraction = static_cast<float>(voice.position - static_cast<double>(floorPosition));
        const auto incrementMagnitude = juce::jmax(0.0001, std::abs(voice.increment));
        const auto sourceSamplesUntilEnd = voice.reverse
            ? juce::jmax(0.0, voice.position - static_cast<double>(voice.startSample))
            : juce::jmax(0.0, static_cast<double>(voice.endSample - 1) - voice.position);
        const auto boundaryFadeSamples = boundaryFadeSamplesForSpan(voice.endSample - voice.startSample);
        const auto fadeOutGain = juce::jlimit(0.0f,
                                              1.0f,
                                              static_cast<float>((sourceSamplesUntilEnd / incrementMagnitude)
                                                                 / static_cast<double>(boundaryFadeSamples)));

        for (auto channel = 0; channel < outputChannels; ++channel)
        {
            const auto sourceChannel = juce::jmin(channel, sourceChannels - 1);
            const auto current = data.buffer.getSample(sourceChannel, floorPosition);
            const auto nextIndex = juce::jlimit(voice.startSample, voice.endSample - 1, floorPosition + (voice.reverse ? -1 : 1));
            const auto next = data.buffer.getSample(sourceChannel, nextIndex);
            const auto sample = current + ((next - current) * fraction);
            auto fadeGain = 1.0f;
            if (voice.fadeInSamplesRemaining > 0)
                fadeGain = 1.0f - (static_cast<float>(voice.fadeInSamplesRemaining)
                                    / static_cast<float>(juce::jmax(1, voice.fadeInTotalSamples)));

            outputBuffer.addSample(channel, startSampleInBlock + sampleIndex, sample * gain * fadeGain * fadeOutGain);
        }

        if (voice.fadeInSamplesRemaining > 0)
            --voice.fadeInSamplesRemaining;

        voice.increment = incrementForVoice(voice);
        voice.position += voice.increment;
        if (voice.stutterEnabled)
            voice.samplesUntilStutter -= 1.0;
    }
}

SampleRegion SamplePlayer::currentRegionFor(const SampleData& data) const
{
    const auto numSamples = data.buffer.getNumSamples();
    const auto startNorm = juce::jlimit(0.0f, 1.0f, readParameter(sampleStart, 0.0f));
    const auto endNorm = juce::jlimit(0.0f, 1.0f, readParameter(sampleEnd, 1.0f));
    const auto orderedStart = juce::jmin(startNorm, endNorm);
    const auto orderedEnd = juce::jmax(startNorm, endNorm);
    const auto minimumWindow = juce::jmin(numSamples, 64);

    SampleRegion currentRegion;
    currentRegion.startSample = juce::jlimit(0, juce::jmax(0, numSamples - minimumWindow), static_cast<int>(orderedStart * static_cast<float>(numSamples)));
    currentRegion.endSample = juce::jlimit(currentRegion.startSample + minimumWindow, numSamples, static_cast<int>(orderedEnd * static_cast<float>(numSamples)));
    currentRegion.reverse = readParameter(sampleReverse, 0.0f) > 0.5f;
    currentRegion.gain = juce::Decibels::decibelsToGain(readParameter(sampleGain, -6.0f));
    currentRegion.transposeSemitones = readParameter(sampleTranspose, 0.0f);
    return snapRegionToCleanBoundaries(data.buffer, currentRegion);
}

double SamplePlayer::incrementForVoice(const Voice& voice) const
{
    const auto rampedTranspose = voice.baseTransposeSemitones
        + (voice.pitchRampSemitones * static_cast<float>(rampProgressForVoice(voice)));
    const auto pitchRatio = std::pow(2.0, static_cast<double>(rampedTranspose) / 12.0);
    return voice.sourceRatio * pitchRatio * (voice.reverse ? -1.0 : 1.0);
}

double SamplePlayer::rampProgressForVoice(const Voice& voice) const
{
    const auto sourceSpan = juce::jmax(1.0, static_cast<double>(voice.endSample - voice.startSample));
    const auto distance = voice.reverse
        ? static_cast<double>(voice.endSample - 1) - voice.position
        : voice.position - static_cast<double>(voice.startSample);
    return juce::jlimit(0.0, 1.0, distance / sourceSpan);
}

float SamplePlayer::readParameter(std::atomic<float>* parameter, float fallback) const
{
    return parameter != nullptr ? parameter->load() : fallback;
}
}
