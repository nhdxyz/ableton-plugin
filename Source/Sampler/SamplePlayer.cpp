#include "SamplePlayer.h"

#include "../Modulation/LfoShapes.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr auto chopFadeSamples = 64;
constexpr auto stutterFadeSamples = 48;
constexpr auto zeroCrossSearchSamples = 256;
constexpr auto sliceKeyRootNote = 60;
constexpr std::array<float, 8> slicePitchLadder { -12.0f, -7.0f, -5.0f, 0.0f, 3.0f, 7.0f, 10.0f, 12.0f };
constexpr std::array<float, 8> garageSlicePitch { -12.0f, 0.0f, 7.0f, -5.0f, 0.0f, 12.0f, 3.0f, -7.0f };

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

float advanceChaosRandomWalk(float currentValue, juce::Random& random, float cyclesThisStep)
{
    const auto safeCycles = juce::jlimit(0.0f, 1.0f, cyclesThisStep);
    const auto randomStep = ((random.nextFloat() * 2.0f) - 1.0f)
        * juce::jlimit(0.00025f, 0.05f, std::sqrt(safeCycles) * 0.08f);
    const auto damping = juce::jlimit(0.00001f, 0.45f, safeCycles * 0.35f);

    return juce::jlimit(-1.0f, 1.0f, (currentValue * (1.0f - damping)) + randomStep);
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
    sampleEngineMode = parameters.getRawParameterValue(Parameters::ID::sampleEngineMode);
    sampleGrainSize = parameters.getRawParameterValue(Parameters::ID::sampleGrainSize);
    sampleGrainSpray = parameters.getRawParameterValue(Parameters::ID::sampleGrainSpray);
    sampleSpectralFreeze = parameters.getRawParameterValue(Parameters::ID::sampleSpectralFreeze);
    sampleTranspose = parameters.getRawParameterValue(Parameters::ID::sampleTranspose);
    samplePitchRamp = parameters.getRawParameterValue(Parameters::ID::samplePitchRamp);
    sampleGain = parameters.getRawParameterValue(Parameters::ID::sampleGain);
    sampleMix = parameters.getRawParameterValue(Parameters::ID::sampleMix);
    sampleStutterEnabled = parameters.getRawParameterValue(Parameters::ID::sampleStutterEnabled);
    sampleStutterRate = parameters.getRawParameterValue(Parameters::ID::sampleStutterRate);
    sampleStutterRepeats = parameters.getRawParameterValue(Parameters::ID::sampleStutterRepeats);
    sampleSliceStyle = parameters.getRawParameterValue(Parameters::ID::sampleSliceStyle);
    for (size_t index = 0; index < sampleSliceCustom.size(); ++index)
    {
        sampleSliceCustom[index] = parameters.getRawParameterValue(Parameters::ID::sampleSliceCustom[index]);
        sampleSliceStart[index] = parameters.getRawParameterValue(Parameters::ID::sampleSliceStart[index]);
        sampleSliceEnd[index] = parameters.getRawParameterValue(Parameters::ID::sampleSliceEnd[index]);
        sampleSliceReverse[index] = parameters.getRawParameterValue(Parameters::ID::sampleSliceReverse[index]);
        sampleSliceTranspose[index] = parameters.getRawParameterValue(Parameters::ID::sampleSliceTranspose[index]);
        sampleSliceGain[index] = parameters.getRawParameterValue(Parameters::ID::sampleSliceGain[index]);
        sampleSlicePan[index] = parameters.getRawParameterValue(Parameters::ID::sampleSlicePan[index]);
        sampleSliceProbability[index] = parameters.getRawParameterValue(Parameters::ID::sampleSliceProbability[index]);
        sampleSliceStutter[index] = parameters.getRawParameterValue(Parameters::ID::sampleSliceStutter[index]);
        sampleSliceChoke[index] = parameters.getRawParameterValue(Parameters::ID::sampleSliceChoke[index]);
        sampleSliceStutterRepeats[index] = parameters.getRawParameterValue(Parameters::ID::sampleSliceStutterRepeats[index]);
        sampleSliceNudge[index] = parameters.getRawParameterValue(Parameters::ID::sampleSliceNudge[index]);
        sampleSliceFade[index] = parameters.getRawParameterValue(Parameters::ID::sampleSliceFade[index]);
    }
    for (size_t index = 0; index < modMatrixSources.size(); ++index)
    {
        modMatrixSources[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixSource[index]);
        modMatrixDestinations[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixDestination[index]);
        modMatrixAmounts[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixAmount[index]);
        modMatrixEnabled[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixEnabled[index]);
        modMatrixPolarities[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixPolarity[index]);
        modMatrixCurves[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixCurve[index]);
        modMatrixRangeMins[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixRangeMin[index]);
        modMatrixRangeMaxes[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixRangeMax[index]);
        modMatrixSlews[index] = parameters.getRawParameterValue(Parameters::ID::modMatrixSlew[index]);
    }
    for (size_t index = 0; index < lfo1CurvePoints.size(); ++index)
        lfo1CurvePoints[index] = parameters.getRawParameterValue(Parameters::ID::lfo1Curve[index]);

    macroTone = parameters.getRawParameterValue(Parameters::ID::macroTone);
    macroDirt = parameters.getRawParameterValue(Parameters::ID::macroDirt);
    macroMotion = parameters.getRawParameterValue(Parameters::ID::macroMotion);
    macroSpace = parameters.getRawParameterValue(Parameters::ID::macroSpace);
    macroWeight = parameters.getRawParameterValue(Parameters::ID::macroWeight);
    macroBounce = parameters.getRawParameterValue(Parameters::ID::macroBounce);
    macroWarp = parameters.getRawParameterValue(Parameters::ID::macroWarp);
    macroThrow = parameters.getRawParameterValue(Parameters::ID::macroThrow);
    lfo1Rate = parameters.getRawParameterValue(Parameters::ID::lfo1Rate);
    lfo1Sync = parameters.getRawParameterValue(Parameters::ID::lfo1Sync);
    lfo1SyncRate = parameters.getRawParameterValue(Parameters::ID::lfo1SyncRate);
    lfo1Shape = parameters.getRawParameterValue(Parameters::ID::lfo1Shape);
    lfo1Depth = parameters.getRawParameterValue(Parameters::ID::lfo1Depth);
    lfo1Phase = parameters.getRawParameterValue(Parameters::ID::lfo1Phase);
    lfo2Rate = parameters.getRawParameterValue(Parameters::ID::lfo2Rate);
    lfo2Sync = parameters.getRawParameterValue(Parameters::ID::lfo2Sync);
    lfo2SyncRate = parameters.getRawParameterValue(Parameters::ID::lfo2SyncRate);
    lfo2Shape = parameters.getRawParameterValue(Parameters::ID::lfo2Shape);
    lfo2Depth = parameters.getRawParameterValue(Parameters::ID::lfo2Depth);
    lfo2Phase = parameters.getRawParameterValue(Parameters::ID::lfo2Phase);
    stepLfoSync = parameters.getRawParameterValue(Parameters::ID::stepLfoSync);
    stepLfoSyncRate = parameters.getRawParameterValue(Parameters::ID::stepLfoSyncRate);
    stepLfoRate = parameters.getRawParameterValue(Parameters::ID::stepLfoRate);
    stepLfoDepth = parameters.getRawParameterValue(Parameters::ID::stepLfoDepth);
    stepLfoSlew = parameters.getRawParameterValue(Parameters::ID::stepLfoSlew);
    for (size_t index = 0; index < stepLfoValues.size(); ++index)
        stepLfoValues[index] = parameters.getRawParameterValue(Parameters::ID::stepLfoValue[index]);
    modEnv1Attack = parameters.getRawParameterValue(Parameters::ID::modEnv1Attack);
    modEnv1Decay = parameters.getRawParameterValue(Parameters::ID::modEnv1Decay);
    modEnv1Sustain = parameters.getRawParameterValue(Parameters::ID::modEnv1Sustain);
    modEnv1Release = parameters.getRawParameterValue(Parameters::ID::modEnv1Release);
    modEnv1Depth = parameters.getRawParameterValue(Parameters::ID::modEnv1Depth);
}

void SamplePlayer::prepare(double sampleRate)
{
    playbackSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    sampleModEnvelope.setSampleRate(playbackSampleRate);
    sampleModEnvelope.reset();
    sampleModEnvelopeValue = 0.0f;
    sampleModVelocity = 0.0f;
    sampleModWheel = 0.0f;
    sampleModAftertouch = 0.0f;
    sampleModPitchBend = 0.0f;
    sampleModNote = 0.0f;
    sampleModActiveNotes = 0;
    sampleModLfoStepValue = (sampleModulationRandom.nextFloat() * 2.0f) - 1.0f;
    sampleModSmoothRandomStartValue = sampleModLfoStepValue;
    sampleModSmoothRandomValue = sampleModLfoStepValue;
    sampleModChaosValue = ((sampleModulationRandom.nextFloat() * 2.0f) - 1.0f) * 0.25f;
    sampleModLfo2StepValue = (sampleModulationRandom.nextFloat() * 2.0f) - 1.0f;
    sampleModStepLfoPhase = 0.0f;
    sampleModStepLfoSmoothedValue = 0.0f;
    sampleModRouteSmoothedValues.fill(0.0f);
}

void SamplePlayer::clear()
{
    const juce::SpinLock::ScopedLockType lock(sampleLock);
    sampleData.reset();
    voices = {};
    region = {};
    sampleModLfoPhase = 0.0f;
    sampleModLfoStepValue = (sampleModulationRandom.nextFloat() * 2.0f) - 1.0f;
    sampleModSmoothRandomStartValue = sampleModLfoStepValue;
    sampleModSmoothRandomValue = sampleModLfoStepValue;
    sampleModChaosValue = ((sampleModulationRandom.nextFloat() * 2.0f) - 1.0f) * 0.25f;
    sampleModLfo2Phase = 0.0f;
    sampleModLfo2StepValue = (sampleModulationRandom.nextFloat() * 2.0f) - 1.0f;
    sampleModStepLfoPhase = 0.0f;
    sampleModStepLfoSmoothedValue = 0.0f;
    sampleModRouteSmoothedValues.fill(0.0f);
    sampleModEnvelope.reset();
    sampleModEnvelopeValue = 0.0f;
    sampleModVelocity = 0.0f;
    sampleModWheel = 0.0f;
    sampleModAftertouch = 0.0f;
    sampleModPitchBend = 0.0f;
    sampleModNote = 0.0f;
    sampleModActiveNotes = 0;
}

void SamplePlayer::stopAllVoices()
{
    const juce::SpinLock::ScopedLockType lock(sampleLock);
    voices = {};
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

bool SamplePlayer::loadBuffer(const juce::AudioBuffer<float>& buffer, double sourceSampleRate, const juce::String& name)
{
    if (buffer.getNumSamples() <= 1 || buffer.getNumChannels() <= 0)
        return false;

    const auto channelCount = juce::jlimit(1, 2, buffer.getNumChannels());
    const auto length = buffer.getNumSamples();

    auto newSample = std::make_shared<SampleData>();
    newSample->buffer.setSize(channelCount, length);
    newSample->buffer.clear();
    for (auto channel = 0; channel < channelCount; ++channel)
        newSample->buffer.copyFrom(channel, 0, buffer, channel, 0, length);

    if (buffer.getNumChannels() == 1 && channelCount > 1)
        newSample->buffer.copyFrom(1, 0, newSample->buffer, 0, 0, length);

    newSample->sourceSampleRate = sourceSampleRate > 0.0 ? sourceSampleRate : 44100.0;
    newSample->fileName = name.isNotEmpty() ? name : juce::String("Recorded Snippet");

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

std::optional<SampleContentRange> SamplePlayer::findContentRange(float threshold, double paddingMs) const
{
    std::shared_ptr<SampleData> data;
    {
        const juce::SpinLock::ScopedLockType lock(sampleLock);
        data = sampleData;
    }

    if (data == nullptr || data->buffer.getNumSamples() <= 1 || data->buffer.getNumChannels() <= 0)
        return std::nullopt;

    const auto totalSamples = data->buffer.getNumSamples();
    const auto channelCount = data->buffer.getNumChannels();
    const auto safeThreshold = juce::jlimit(0.0001f, 0.25f, threshold);
    auto firstAudibleSample = -1;
    auto lastAudibleSample = -1;

    for (auto sampleIndex = 0; sampleIndex < totalSamples; ++sampleIndex)
    {
        auto peak = 0.0f;
        for (auto channel = 0; channel < channelCount; ++channel)
            peak = juce::jmax(peak, std::abs(data->buffer.getSample(channel, sampleIndex)));

        if (peak >= safeThreshold)
        {
            if (firstAudibleSample < 0)
                firstAudibleSample = sampleIndex;
            lastAudibleSample = sampleIndex;
        }
    }

    if (firstAudibleSample < 0 || lastAudibleSample <= firstAudibleSample)
        return std::nullopt;

    const auto paddingSamples = juce::jlimit(0,
                                            totalSamples / 2,
                                            static_cast<int>(std::round((paddingMs / 1000.0) * data->sourceSampleRate)));
    firstAudibleSample = juce::jmax(0, firstAudibleSample - paddingSamples);
    lastAudibleSample = juce::jmin(totalSamples - 1, lastAudibleSample + paddingSamples);

    SampleContentRange range;
    range.start = static_cast<float>(firstAudibleSample) / static_cast<float>(totalSamples);
    range.end = static_cast<float>(lastAudibleSample + 1) / static_cast<float>(totalSamples);
    return range;
}

SliceDetectionResult SamplePlayer::detectTransientSliceRegions() const
{
    SliceDetectionResult result;
    for (size_t index = 0; index < result.regions.size(); ++index)
    {
        result.regions[index].start = static_cast<float>(index) / static_cast<float>(result.regions.size());
        result.regions[index].end = static_cast<float>(index + 1) / static_cast<float>(result.regions.size());
        result.regions[index].transient = index == 0;
    }

    std::shared_ptr<SampleData> data;
    {
        const juce::SpinLock::ScopedLockType lock(sampleLock);
        data = sampleData;
    }

    if (data == nullptr || data->buffer.getNumSamples() <= 128 || data->buffer.getNumChannels() <= 0)
        return result;

    const auto totalSamples = data->buffer.getNumSamples();
    result.valid = true;

    const auto hopSize = juce::jlimit(64, 512, totalSamples / 256);
    const auto windowSize = juce::jlimit(hopSize, hopSize * 4, static_cast<int>(data->sourceSampleRate * 0.010));
    const auto frameCount = juce::jmax(1, (totalSamples + hopSize - 1) / hopSize);
    std::vector<float> envelope(static_cast<size_t>(frameCount), 0.0f);

    auto meanEnvelope = 0.0f;
    auto maxEnvelope = 0.0f;
    for (auto frame = 0; frame < frameCount; ++frame)
    {
        const auto startSample = frame * hopSize;
        const auto endSample = juce::jmin(totalSamples, startSample + windowSize);
        auto sum = 0.0f;
        auto count = 0;

        for (auto sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex)
        {
            sum += std::abs(mixedSampleAt(data->buffer, sampleIndex));
            ++count;
        }

        const auto energy = count > 0 ? sum / static_cast<float>(count) : 0.0f;
        envelope[static_cast<size_t>(frame)] = energy;
        meanEnvelope += energy;
        maxEnvelope = juce::jmax(maxEnvelope, energy);
    }

    meanEnvelope /= static_cast<float>(frameCount);
    const auto threshold = juce::jmax(maxEnvelope * 0.12f, meanEnvelope * 1.65f);
    const auto minimumBoundarySample = juce::jmax(hopSize * 2,
                                                 static_cast<int>(data->sourceSampleRate * 0.035));
    const auto maximumBoundarySample = juce::jmax(0, totalSamples - minimumBoundarySample);
    const auto minimumSpacing = juce::jmax(totalSamples / 64,
                                           static_cast<int>(data->sourceSampleRate * 0.045));

    struct Candidate
    {
        int sample = 0;
        float strength = 0.0f;
    };

    std::vector<Candidate> candidates;
    for (auto frame = 1; frame < frameCount; ++frame)
    {
        const auto current = envelope[static_cast<size_t>(frame)];
        const auto previous = envelope[static_cast<size_t>(frame - 1)];
        const auto onset = current - (previous * 0.78f);
        const auto sample = juce::jlimit(0, totalSamples - 1, frame * hopSize);

        if (sample >= minimumBoundarySample
            && sample <= maximumBoundarySample
            && current >= threshold
            && onset >= threshold * 0.32f)
        {
            candidates.push_back({ sample, onset + (current * 0.2f) });
        }
    }

    std::sort(candidates.begin(), candidates.end(), [] (const Candidate& left, const Candidate& right)
    {
        return left.strength > right.strength;
    });

    std::vector<int> boundaries;
    boundaries.reserve(result.regions.size() + 1);
    boundaries.push_back(0);

    for (const auto& candidate : candidates)
    {
        if (boundaries.size() >= result.regions.size())
            break;

        auto tooClose = false;
        for (const auto boundary : boundaries)
        {
            if (std::abs(boundary - candidate.sample) < minimumSpacing)
            {
                tooClose = true;
                break;
            }
        }

        if (! tooClose)
        {
            boundaries.push_back(candidate.sample);
            ++result.transientCount;
        }
    }

    for (size_t index = 1; boundaries.size() < result.regions.size() && index < result.regions.size(); ++index)
    {
        const auto gridBoundary = static_cast<int>((static_cast<int64_t>(index) * totalSamples)
                                                   / static_cast<int64_t>(result.regions.size()));
        auto tooClose = false;
        for (const auto boundary : boundaries)
        {
            if (std::abs(boundary - gridBoundary) < minimumSpacing / 2)
            {
                tooClose = true;
                break;
            }
        }

        if (! tooClose)
            boundaries.push_back(gridBoundary);
    }

    std::sort(boundaries.begin(), boundaries.end());
    boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());

    for (size_t index = 1; boundaries.size() < result.regions.size() && index < result.regions.size(); ++index)
    {
        const auto gridBoundary = static_cast<int>((static_cast<int64_t>(index) * totalSamples)
                                                   / static_cast<int64_t>(result.regions.size()));
        if (std::find(boundaries.begin(), boundaries.end(), gridBoundary) == boundaries.end())
        {
            boundaries.push_back(gridBoundary);
            std::sort(boundaries.begin(), boundaries.end());
        }
    }

    while (boundaries.size() < result.regions.size())
    {
        auto widestGapIndex = 0;
        auto widestGap = 0;
        for (size_t index = 0; index < boundaries.size(); ++index)
        {
            const auto nextBoundary = index + 1 < boundaries.size() ? boundaries[index + 1] : totalSamples;
            const auto gap = nextBoundary - boundaries[index];
            if (gap > widestGap)
            {
                widestGap = gap;
                widestGapIndex = static_cast<int>(index);
            }
        }

        const auto nextBoundary = static_cast<size_t>(widestGapIndex) + 1 < boundaries.size()
            ? boundaries[static_cast<size_t>(widestGapIndex) + 1]
            : totalSamples;
        const auto midpoint = boundaries[static_cast<size_t>(widestGapIndex)]
            + juce::jmax(1, (nextBoundary - boundaries[static_cast<size_t>(widestGapIndex)]) / 2);
        boundaries.push_back(juce::jlimit(1, totalSamples - 1, midpoint));
        std::sort(boundaries.begin(), boundaries.end());
        boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());
    }

    boundaries.resize(result.regions.size());
    boundaries.push_back(totalSamples);

    for (size_t index = 0; index < result.regions.size(); ++index)
    {
        const auto startSample = juce::jlimit(0, totalSamples - 1, boundaries[index]);
        const auto endSample = juce::jlimit(startSample + 1, totalSamples, boundaries[index + 1]);
        auto& sliceRegion = result.regions[index];
        sliceRegion.start = static_cast<float>(startSample) / static_cast<float>(totalSamples);
        sliceRegion.end = static_cast<float>(endSample) / static_cast<float>(totalSamples);
        sliceRegion.transient = index == 0 || startSample != static_cast<int>((static_cast<int64_t>(index) * totalSamples)
                                                                              / static_cast<int64_t>(result.regions.size()));
    }

    return result;
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
    newRegion.pan = juce::jlimit(-1.0f, 1.0f, newRegion.pan);
    newRegion.probability = juce::jlimit(0.0f, 1.0f, newRegion.probability);
    newRegion.fade = juce::jlimit(0.0f, 1.0f, newRegion.fade);
    region = newRegion;
}

bool SamplePlayer::triggerAudition(int midiNoteNumber, float velocity, double bpm)
{
    const juce::SpinLock::ScopedLockType lock(sampleLock);

    if (sampleData == nullptr || sampleData->buffer.getNumSamples() == 0)
        return false;

    triggerSampleModulationNoteOn(velocity);
    updateSampleModulation(0, bpm, std::nullopt);
    startVoice(*sampleData, midiNoteNumber, velocity, bpm, true);
    return true;
}

bool SamplePlayer::triggerSliceAudition(int sliceIndex, int midiNoteNumber, float velocity, double bpm)
{
    const juce::SpinLock::ScopedLockType lock(sampleLock);

    if (sampleData == nullptr || sampleData->buffer.getNumSamples() == 0)
        return false;

    triggerSampleModulationNoteOn(velocity);
    updateSampleModulation(0, bpm, std::nullopt);
    startVoice(*sampleData, midiNoteNumber, velocity, bpm, true, juce::jlimit(0, 7, sliceIndex), true);
    return true;
}

void SamplePlayer::render(juce::AudioBuffer<float>& outputBuffer,
                          const juce::MidiBuffer& midi,
                          double bpm,
                          std::optional<double> ppqPosition)
{
    if (readParameter(sampleEnabled, 0.0f) < 0.5f)
        return;

    const juce::SpinLock::ScopedTryLockType lock(sampleLock);
    if (! lock.isLocked() || sampleData == nullptr || sampleData->buffer.getNumSamples() == 0)
        return;

    handleSampleModulationMidi(midi);
    updateSampleModulation(outputBuffer.getNumSamples(), bpm, ppqPosition);

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
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            voices = {};
            releaseSampleModulationNote();
        }
    }

    if (cursor < outputBuffer.getNumSamples())
        renderActiveVoices(*sampleData, outputBuffer, cursor, outputBuffer.getNumSamples() - cursor);
}

void SamplePlayer::handleSampleModulationMidi(const juce::MidiBuffer& midi)
{
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            sampleModNote = juce::jlimit(-1.0f, 1.0f, (static_cast<float>(message.getNoteNumber()) - 60.0f) / 36.0f);
            triggerSampleModulationNoteOn(message.getFloatVelocity());
        }
        else if (message.isNoteOff())
        {
            sampleModActiveNotes = juce::jmax(0, sampleModActiveNotes - 1);
            if (sampleModActiveNotes == 0)
                sampleModEnvelope.noteOff();
        }
        else if (message.isController())
        {
            if (message.getControllerNumber() == 1)
                sampleModWheel = static_cast<float>(message.getControllerValue()) / 127.0f;
            else if (message.isResetAllControllers())
            {
                sampleModWheel = 0.0f;
                sampleModAftertouch = 0.0f;
                sampleModPitchBend = 0.0f;
            }
        }
        else if (message.isPitchWheel())
        {
            sampleModPitchBend = juce::jlimit(-1.0f,
                                               1.0f,
                                               (static_cast<float>(message.getPitchWheelValue()) - 8192.0f) / 8192.0f);
        }
        else if (message.isAftertouch())
        {
            sampleModAftertouch = static_cast<float>(message.getAfterTouchValue()) / 127.0f;
        }
        else if (message.isChannelPressure())
        {
            sampleModAftertouch = static_cast<float>(message.getChannelPressureValue()) / 127.0f;
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            releaseSampleModulationNote();
        }
    }
}

void SamplePlayer::triggerSampleModulationNoteOn(float velocity)
{
    sampleModVelocity = juce::jlimit(0.0f, 1.0f, velocity);
    sampleModActiveNotes = juce::jmin(128, sampleModActiveNotes + 1);
    sampleModEnvelope.noteOn();
}

void SamplePlayer::releaseSampleModulationNote()
{
    sampleModActiveNotes = 0;
    sampleModEnvelope.noteOff();
}

float SamplePlayer::processSampleModulationEnvelope(int numSamples)
{
    sampleModEnvelopeParameters.attack = juce::jmax(0.001f, readParameter(modEnv1Attack, 0.01f));
    sampleModEnvelopeParameters.decay = juce::jmax(0.001f, readParameter(modEnv1Decay, 0.2f));
    sampleModEnvelopeParameters.sustain = juce::jlimit(0.0f, 1.0f, readParameter(modEnv1Sustain, 0.5f));
    sampleModEnvelopeParameters.release = juce::jmax(0.001f, readParameter(modEnv1Release, 0.2f));
    sampleModEnvelope.setParameters(sampleModEnvelopeParameters);

    const auto samplesToProcess = juce::jmax(1, numSamples);
    for (auto sampleIndex = 0; sampleIndex < samplesToProcess; ++sampleIndex)
        sampleModEnvelopeValue = sampleModEnvelope.getNextSample();

    return juce::jlimit(0.0f, 1.0f, sampleModEnvelopeValue * readParameter(modEnv1Depth, 1.0f));
}

void SamplePlayer::updateSampleModulation(int numSamples, double bpm, std::optional<double> ppqPosition)
{
    sampleModulation = {};

    const auto lfoValue = processSampleModulationLfo(numSamples, bpm, ppqPosition);
    const auto lfo2Value = processSampleModulationLfo2(numSamples, bpm, ppqPosition);
    const auto stepLfoValue = processSampleStepLfo(numSamples, bpm, ppqPosition);
    const auto modEnvelopeValue = processSampleModulationEnvelope(numSamples);

    for (size_t index = 0; index < modMatrixSources.size(); ++index)
    {
        const auto sourceIndex = static_cast<int>(std::round(readParameter(modMatrixSources[index], 0.0f)));
        const auto destinationIndex = static_cast<int>(std::round(readParameter(modMatrixDestinations[index], 0.0f)));
        const auto amount = readParameter(modMatrixAmounts[index], 0.0f);
        const auto enabled = readParameter(modMatrixEnabled[index], 1.0f) >= 0.5f;

        if (! enabled || sourceIndex == 0 || ! Modulation::isSampleDestination(destinationIndex) || std::abs(amount) <= 0.0001f)
            continue;

        const auto sourceValue = evaluateSampleModulationSource(sourceIndex, lfoValue, lfo2Value, stepLfoValue, modEnvelopeValue);
        const auto contribution = Modulation::processRouteValue(sourceValue,
                                                                modMatrixPolarities[index],
                                                                modMatrixCurves[index],
                                                                modMatrixRangeMins[index],
                                                                modMatrixRangeMaxes[index],
                                                                modMatrixSlews[index],
                                                                sampleModRouteSmoothedValues[index],
                                                                numSamples,
                                                                playbackSampleRate)
            * amount;

        switch (destinationIndex)
        {
            case 12: sampleModulation.start += contribution; break;
            case 13: sampleModulation.mix += contribution; break;
            case 14: sampleModulation.pitch += contribution; break;
            case 15: sampleModulation.ramp += contribution; break;
            case 16: sampleModulation.stutter += contribution; break;
            default: break;
        }
    }

    sampleModulation.start = juce::jlimit(-1.0f, 1.0f, sampleModulation.start);
    sampleModulation.mix = juce::jlimit(-1.0f, 1.0f, sampleModulation.mix);
    sampleModulation.pitch = juce::jlimit(-1.0f, 1.0f, sampleModulation.pitch);
    sampleModulation.ramp = juce::jlimit(-1.0f, 1.0f, sampleModulation.ramp);
    sampleModulation.stutter = juce::jlimit(-1.0f, 1.0f, sampleModulation.stutter);
}

float SamplePlayer::processSampleModulationLfo(int numSamples, double bpm, std::optional<double> ppqPosition)
{
    const auto shapeIndex = static_cast<int>(std::round(readParameter(lfo1Shape, 0.0f)));
    const auto syncEnabled = readParameter(lfo1Sync, 1.0f) >= 0.5f;
    const auto syncRateIndex = static_cast<int>(std::round(readParameter(lfo1SyncRate, 1.0f)));
    const auto cyclesPerBeat = Modulation::cyclesPerBeatForSyncRate(syncRateIndex);
    const auto rateHz = syncEnabled
        ? static_cast<float>((juce::jlimit(20.0, 300.0, bpm) / 60.0) * cyclesPerBeat)
        : readParameter(lfo1Rate, 1.0f);
    const auto phaseOffset = readParameter(lfo1Phase, 0.0f);

    if (syncEnabled)
        if (const auto syncedPhase = Modulation::phaseFromPpq(ppqPosition, cyclesPerBeat))
            sampleModLfoPhase = *syncedPhase;

    const auto phase = Modulation::LfoShapes::normalisePhase(sampleModLfoPhase + phaseOffset);
    const auto value = Modulation::LfoShapes::shapeValueWithCurve(shapeIndex,
                                                                  phase,
                                                                  lfo1CurvePoints,
                                                                  sampleModLfoStepValue);

    const auto smoothProgress = phase * phase * (3.0f - (2.0f * phase));
    sampleModSmoothRandomValue = juce::jlimit(-1.0f,
                                              1.0f,
                                              sampleModSmoothRandomStartValue
                                                  + ((sampleModLfoStepValue - sampleModSmoothRandomStartValue) * smoothProgress));
    sampleModChaosValue = advanceChaosRandomWalk(sampleModChaosValue,
                                                 sampleModulationRandom,
                                                 (juce::jlimit(0.01f, 80.0f, rateHz)
                                                    * static_cast<float>(juce::jmax(1, numSamples)))
                                                    / static_cast<float>(juce::jmax(1.0, playbackSampleRate)));

    const auto previousPhase = sampleModLfoPhase;
    sampleModLfoPhase += (juce::jlimit(0.01f, 80.0f, rateHz) * static_cast<float>(juce::jmax(1, numSamples))) / static_cast<float>(playbackSampleRate);

    if (sampleModLfoPhase >= 1.0f)
    {
        sampleModLfoPhase -= std::floor(sampleModLfoPhase);
        if (previousPhase < 1.0f)
        {
            sampleModSmoothRandomStartValue = sampleModLfoStepValue;
            sampleModLfoStepValue = (sampleModulationRandom.nextFloat() * 2.0f) - 1.0f;
        }
    }

    return juce::jlimit(-1.0f, 1.0f, value) * readParameter(lfo1Depth, 0.45f);
}

float SamplePlayer::processSampleModulationLfo2(int numSamples, double bpm, std::optional<double> ppqPosition)
{
    const auto shapeIndex = static_cast<int>(std::round(readParameter(lfo2Shape, 1.0f)));
    const auto syncEnabled = readParameter(lfo2Sync, 1.0f) >= 0.5f;
    const auto syncRateIndex = static_cast<int>(std::round(readParameter(lfo2SyncRate, 3.0f)));
    const auto cyclesPerBeat = Modulation::cyclesPerBeatForSyncRate(syncRateIndex);
    const auto rateHz = syncEnabled
        ? static_cast<float>((juce::jlimit(20.0, 300.0, bpm) / 60.0) * cyclesPerBeat)
        : readParameter(lfo2Rate, 1.5f);
    const auto phaseOffset = readParameter(lfo2Phase, 0.25f);

    if (syncEnabled)
        if (const auto syncedPhase = Modulation::phaseFromPpq(ppqPosition, cyclesPerBeat))
            sampleModLfo2Phase = *syncedPhase;

    const auto phase = Modulation::LfoShapes::normalisePhase(sampleModLfo2Phase + phaseOffset);
    const auto value = Modulation::LfoShapes::shapeValue(shapeIndex, phase, sampleModLfo2StepValue);

    const auto previousPhase = sampleModLfo2Phase;
    sampleModLfo2Phase += (juce::jlimit(0.01f, 80.0f, rateHz) * static_cast<float>(juce::jmax(1, numSamples))) / static_cast<float>(playbackSampleRate);

    if (sampleModLfo2Phase >= 1.0f)
    {
        sampleModLfo2Phase -= std::floor(sampleModLfo2Phase);
        if (previousPhase < 1.0f)
            sampleModLfo2StepValue = (sampleModulationRandom.nextFloat() * 2.0f) - 1.0f;
    }

    return juce::jlimit(-1.0f, 1.0f, value) * readParameter(lfo2Depth, 0.25f);
}

float SamplePlayer::processSampleStepLfo(int numSamples, double bpm, std::optional<double> ppqPosition)
{
    return Modulation::processStepLfo(stepLfoSync,
                                      stepLfoSyncRate,
                                      stepLfoRate,
                                      stepLfoDepth,
                                      stepLfoSlew,
                                      stepLfoValues,
                                      sampleModStepLfoPhase,
                                      sampleModStepLfoSmoothedValue,
                                      numSamples,
                                      playbackSampleRate,
                                      bpm,
                                      ppqPosition);
}

float SamplePlayer::evaluateSampleModulationSource(int sourceIndex, float lfoValue, float lfo2Value, float stepLfoValue, float modEnvelopeValue) const
{
    switch (sourceIndex)
    {
        case 1: return lfoValue;
        case 2: return modEnvelopeValue;
        case 3: return sampleModVelocity;
        case 4: return readParameter(macroTone, 0.0f);
        case 5: return readParameter(macroDirt, 0.0f);
        case 6: return readParameter(macroMotion, 0.0f);
        case 7: return readParameter(macroSpace, 0.0f);
        case 8: return readParameter(macroWeight, 0.0f);
        case 9: return readParameter(macroBounce, 0.0f);
        case 10: return readParameter(macroWarp, 0.0f);
        case 11: return readParameter(macroThrow, 0.0f);
        case 12: return sampleModLfoStepValue;
        case 13: return sampleModSmoothRandomValue;
        case 14: return sampleModChaosValue;
        case 15: return lfo2Value;
        case 16: return sampleModWheel;
        case 17: return sampleModAftertouch;
        case 18: return sampleModPitchBend;
        case 19: return sampleModNote;
        case Modulation::stepLfoSourceIndex: return stepLfoValue;
        default: return 0.0f;
    }
}

int SamplePlayer::sliceIndexForMidiNote(int midiNoteNumber) const
{
    const auto offset = midiNoteNumber - sliceKeyRootNote;
    return (offset % 8 + 8) % 8;
}

void SamplePlayer::startVoice(const SampleData& data,
                              int midiNoteNumber,
                              float velocity,
                              double bpm,
                              bool forceOneShot,
                              int forcedSliceIndex,
                              bool ignoreSliceProbability)
{
    const auto playbackMode = static_cast<int>(std::round(readParameter(samplePlaybackMode, 1.0f)));
    const auto hasForcedSlice = forcedSliceIndex >= 0;
    const auto sliceIndex = hasForcedSlice
        ? juce::jlimit(0, 7, forcedSliceIndex)
        : ((! forceOneShot && playbackMode == 2) ? sliceIndexForMidiNote(midiNoteNumber) : -1);
    if (sliceIndex >= 0 && sliceChokeEnabled(sliceIndex))
    {
        for (auto& voice : voices)
            if (voice.active && voice.sliceIndex >= 0 && sliceChokeEnabled(voice.sliceIndex))
                voice.active = false;
    }

    auto* voiceToUse = std::find_if(voices.begin(), voices.end(), [] (const Voice& voice)
    {
        return ! voice.active;
    });

    if (voiceToUse == voices.end())
        voiceToUse = voices.begin();

    const auto currentRegion = currentRegionFor(data, sliceIndex);
    if (currentRegion.endSample <= currentRegion.startSample + 1)
        return;
    if (sliceIndex >= 0
        && ! ignoreSliceProbability
        && sliceTriggerRandom.nextFloat() > juce::jlimit(0.0f, 1.0f, currentRegion.probability))
    {
        return;
    }

    const auto engineMode = sliceIndex >= 0
        ? 0
        : juce::jlimit(0, 3, static_cast<int>(std::round(readParameter(sampleEngineMode, 0.0f))));
    const auto grainSizeSeconds = juce::jlimit(0.01f, 0.4f, readParameter(sampleGrainSize, 0.08f));
    const auto grainSpray = juce::jlimit(0.0f, 1.0f, readParameter(sampleGrainSpray, 0.0f));
    const auto spectralFreeze = juce::jlimit(0.0f, 1.0f, readParameter(sampleSpectralFreeze, 0.0f));
    const auto sourceSpan = juce::jmax(1, currentRegion.endSample - currentRegion.startSample);
    const auto grainSizeSamples = juce::jlimit(64,
                                               sourceSpan,
                                               static_cast<int>(std::round(grainSizeSeconds * data.sourceSampleRate)));
    const auto reverse = currentRegion.reverse;
    const auto transpose = currentRegion.transposeSemitones
        + (sliceIndex >= 0 ? 0.0f : static_cast<float>(midiNoteNumber - 60))
        + (sampleModulation.pitch * 12.0f);
    const auto sourceRatio = data.sourceSampleRate / playbackSampleRate;
    const auto pitchRatio = std::pow(2.0, static_cast<double>(transpose) / 12.0);
    const auto enginePitchSmear = engineMode == 2 ? (1.0 - static_cast<double>(spectralFreeze) * 0.92) : 1.0;

    voiceToUse->active = true;
    voiceToUse->startSample = currentRegion.startSample;
    voiceToUse->endSample = currentRegion.endSample;
    voiceToUse->midiNoteNumber = midiNoteNumber;
    voiceToUse->sliceIndex = sliceIndex;
    voiceToUse->reverse = reverse;
    voiceToUse->gated = ! forceOneShot && playbackMode == 0;
    voiceToUse->velocity = velocity;
    voiceToUse->gain = currentRegion.gain;
    voiceToUse->pan = currentRegion.pan;
    voiceToUse->sourceRatio = sourceRatio;
    voiceToUse->baseTransposeSemitones = transpose;
    voiceToUse->pitchRampSemitones = juce::jlimit(-24.0f, 24.0f, readParameter(samplePitchRamp, 0.0f) + (sampleModulation.ramp * 12.0f));
    voiceToUse->increment = sourceRatio * pitchRatio * enginePitchSmear * (reverse ? -1.0 : 1.0);
    voiceToUse->position = reverse ? static_cast<double>(currentRegion.endSample - 1)
                                   : static_cast<double>(currentRegion.startSample);
    voiceToUse->engineMode = engineMode;
    voiceToUse->grainSizeSamples = grainSizeSamples;
    voiceToUse->grainSpray = grainSpray;
    voiceToUse->spectralFreeze = spectralFreeze;
    voiceToUse->grainResetsRemaining = 0;
    voiceToUse->grainIntervalSamples = juce::jmax(16.0, grainSizeSeconds * playbackSampleRate * (engineMode == 3 ? 0.38 : 0.72));
    voiceToUse->samplesUntilGrain = voiceToUse->grainIntervalSamples;
    if (engineMode == 1 || engineMode == 3 || (engineMode == 2 && spectralFreeze > 0.02f))
    {
        voiceToUse->grainResetsRemaining = engineMode == 2 ? 96 : engineMode == 3 ? 64 : 32;
        if (engineMode == 3)
            voiceToUse->pan = juce::jlimit(-1.0f, 1.0f, currentRegion.pan + ((sampleModulationRandom.nextFloat() * 2.0f) - 1.0f) * 0.55f);
        resetVoiceToEngineGrain(*voiceToUse);
    }
    voiceToUse->stutterEnabled = (sliceIndex >= 0 ? sliceStutterEnabled(sliceIndex) : readParameter(sampleStutterEnabled, 0.0f) >= 0.5f)
        || sampleModulation.stutter > 0.05f;
    voiceToUse->stutterRepeatsRemaining = voiceToUse->stutterEnabled
        ? juce::jlimit(1,
                       8,
                       (sliceIndex >= 0 ? sliceStutterRepeats(sliceIndex)
                                        : static_cast<int>(std::round(readParameter(sampleStutterRepeats, 3.0f))))
                           + static_cast<int>(std::round(sampleModulation.stutter * 4.0f)))
        : 0;
    voiceToUse->stutterIntervalSamples = stutterIntervalSamplesForRate(
        juce::jlimit(0, 2, static_cast<int>(std::round(readParameter(sampleStutterRate, 1.0f)))),
        bpm,
        playbackSampleRate);
    voiceToUse->samplesUntilStutter = voiceToUse->stutterIntervalSamples;
    const auto sliceFadeSamples = fadeSamplesForSpan(currentRegion.endSample - currentRegion.startSample, currentRegion.fade);
    voiceToUse->fadeInTotalSamples = sliceFadeSamples;
    voiceToUse->fadeInSamplesRemaining = voiceToUse->fadeInTotalSamples;
    voiceToUse->fadeOutTotalSamples = sliceFadeSamples;
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
    const auto gain = voice.gain
                    * juce::jlimit(0.0f, 1.0f, readParameter(sampleMix, 0.75f) + (sampleModulation.mix * 0.45f))
                    * voice.velocity;
    const auto pan = juce::jlimit(-1.0f, 1.0f, voice.pan);
    const auto leftPanGain = pan > 0.0f ? 1.0f - pan : 1.0f;
    const auto rightPanGain = pan < 0.0f ? 1.0f + pan : 1.0f;

    if (sourceChannels == 0 || outputChannels == 0)
    {
        voice.active = false;
        return;
    }

    for (auto sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        if (voice.stutterEnabled && voice.stutterRepeatsRemaining > 0 && voice.samplesUntilStutter <= 0.0)
        {
            if (voice.engineMode > 0)
                resetVoiceToEngineGrain(voice);
            else
                voice.position = voice.reverse ? static_cast<double>(voice.endSample - 1)
                                               : static_cast<double>(voice.startSample);
            voice.fadeInTotalSamples = juce::jmin(stutterFadeSamples, voice.fadeOutTotalSamples);
            voice.fadeInSamplesRemaining = voice.fadeInTotalSamples;
            --voice.stutterRepeatsRemaining;
            voice.samplesUntilStutter += voice.stutterIntervalSamples;
        }
        else if (voice.engineMode > 0 && voice.grainResetsRemaining > 0 && voice.samplesUntilGrain <= 0.0)
        {
            resetVoiceToEngineGrain(voice);
            voice.fadeInTotalSamples = juce::jmin(stutterFadeSamples, voice.fadeOutTotalSamples);
            voice.fadeInSamplesRemaining = voice.fadeInTotalSamples;
            --voice.grainResetsRemaining;
            voice.samplesUntilGrain += voice.grainIntervalSamples;
        }

        if ((! voice.reverse && voice.position >= static_cast<double>(voice.endSample - 1))
            || (voice.reverse && voice.position <= static_cast<double>(voice.startSample)))
        {
            voice.active = false;
            return;
        }

        const auto floorPosition = juce::jlimit(voice.startSample,
                                                voice.endSample - 1,
                                                static_cast<int>(std::floor(voice.position)));
        const auto fraction = juce::jlimit(0.0f,
                                           1.0f,
                                           static_cast<float>(voice.position - static_cast<double>(floorPosition)));
        const auto incrementMagnitude = juce::jmax(0.0001, std::abs(voice.increment));
        const auto sourceSamplesUntilEnd = voice.reverse
            ? juce::jmax(0.0, voice.position - static_cast<double>(voice.startSample))
            : juce::jmax(0.0, static_cast<double>(voice.endSample - 1) - voice.position);
        const auto fadeOutGain = juce::jlimit(0.0f,
                                              1.0f,
                                              static_cast<float>((sourceSamplesUntilEnd / incrementMagnitude)
                                                                 / static_cast<double>(juce::jmax(1, voice.fadeOutTotalSamples))));

        for (auto channel = 0; channel < outputChannels; ++channel)
        {
            const auto sourceChannel = juce::jmin(channel, sourceChannels - 1);
            const auto panGain = outputChannels >= 2
                ? (channel == 0 ? leftPanGain : channel == 1 ? rightPanGain : 1.0f)
                : 1.0f;
            const auto current = data.buffer.getSample(sourceChannel, floorPosition);
            const auto nextIndex = juce::jlimit(voice.startSample, voice.endSample - 1, floorPosition + 1);
            const auto next = data.buffer.getSample(sourceChannel, nextIndex);
            const auto sample = current + ((next - current) * fraction);
            auto fadeGain = 1.0f;
            if (voice.fadeInSamplesRemaining > 0)
                fadeGain = 1.0f - (static_cast<float>(voice.fadeInSamplesRemaining)
                                    / static_cast<float>(juce::jmax(1, voice.fadeInTotalSamples)));

            outputBuffer.addSample(channel, startSampleInBlock + sampleIndex, sample * gain * panGain * fadeGain * fadeOutGain);
        }

        if (voice.fadeInSamplesRemaining > 0)
            --voice.fadeInSamplesRemaining;

        voice.increment = incrementForVoice(voice);
        if (voice.engineMode == 2)
            voice.increment *= 1.0 - (static_cast<double>(voice.spectralFreeze) * 0.92);
        voice.position += voice.increment;
        if (voice.stutterEnabled)
            voice.samplesUntilStutter -= 1.0;
        if (voice.engineMode > 0)
            voice.samplesUntilGrain -= 1.0;
    }
}

void SamplePlayer::resetVoiceToEngineGrain(Voice& voice)
{
    const auto sourceSpan = juce::jmax(1, voice.endSample - voice.startSample);
    const auto grainSize = juce::jlimit(64, sourceSpan, voice.grainSizeSamples > 0 ? voice.grainSizeSamples : sourceSpan);
    const auto maxStart = juce::jmax(voice.startSample, voice.endSample - grainSize);
    auto grainStart = voice.startSample;

    if (voice.engineMode == 2)
    {
        const auto freezeOffset = static_cast<int>(std::round(voice.spectralFreeze * static_cast<float>(maxStart - voice.startSample)));
        grainStart = juce::jlimit(voice.startSample, maxStart, voice.startSample + freezeOffset);
    }
    else
    {
        const auto available = juce::jmax(0, maxStart - voice.startSample);
        const auto spray = voice.engineMode == 3 ? 1.0f : juce::jlimit(0.0f, 1.0f, voice.grainSpray);
        const auto spraySamples = static_cast<int>(std::round(static_cast<float>(available) * spray));
        grainStart += spraySamples > 0 ? sampleModulationRandom.nextInt(spraySamples + 1) : 0;
    }

    voice.position = voice.reverse ? static_cast<double>(juce::jlimit(voice.startSample, voice.endSample - 1, grainStart + grainSize - 1))
                                   : static_cast<double>(juce::jlimit(voice.startSample, voice.endSample - 1, grainStart));
}

SampleRegion SamplePlayer::currentRegionFor(const SampleData& data, int sliceIndex) const
{
    const auto numSamples = data.buffer.getNumSamples();
    const auto sliceMode = sliceIndex >= 0;
    const auto safeSliceIndex = juce::jlimit(0, 7, sliceIndex);
    const auto customSlice = sliceMode && sliceHasCustomSettings(safeSliceIndex);
    const auto startNorm = sliceMode
        ? juce::jlimit(0.0f,
                       1.0f,
                       customSlice
                           ? readParameter(sampleSliceStart[static_cast<size_t>(safeSliceIndex)], static_cast<float>(safeSliceIndex) / 8.0f)
                           : static_cast<float>(safeSliceIndex) / 8.0f)
        : juce::jlimit(0.0f, 1.0f, readParameter(sampleStart, 0.0f) + (sampleModulation.start * 0.25f));
    const auto endNorm = sliceMode
        ? juce::jlimit(0.0f,
                       1.0f,
                       customSlice
                           ? readParameter(sampleSliceEnd[static_cast<size_t>(safeSliceIndex)], static_cast<float>(safeSliceIndex + 1) / 8.0f)
                           : static_cast<float>(safeSliceIndex + 1) / 8.0f)
        : juce::jlimit(0.0f, 1.0f, readParameter(sampleEnd, 1.0f));
    const auto orderedStart = juce::jmin(startNorm, endNorm);
    const auto orderedEnd = juce::jmax(startNorm, endNorm);
    const auto minimumWindow = juce::jmin(numSamples, 64);

    SampleRegion currentRegion;
    currentRegion.startSample = juce::jlimit(0, juce::jmax(0, numSamples - minimumWindow), static_cast<int>(orderedStart * static_cast<float>(numSamples)));
    currentRegion.endSample = juce::jlimit(currentRegion.startSample + minimumWindow, numSamples, static_cast<int>(orderedEnd * static_cast<float>(numSamples)));
    const auto sliceSettings = sliceMode ? slicePlaybackSettings(sliceIndex) : SlicePlaybackSettings {};
    if (sliceMode && std::abs(sliceSettings.nudgePercent) > 0.001f)
    {
        const auto nudgeSamples = static_cast<int>(std::round((sliceSettings.nudgePercent / 100.0f) * static_cast<float>(numSamples)));
        auto shiftedStart = currentRegion.startSample + nudgeSamples;
        auto shiftedEnd = currentRegion.endSample + nudgeSamples;
        if (shiftedStart < 0)
        {
            shiftedEnd -= shiftedStart;
            shiftedStart = 0;
        }
        if (shiftedEnd > numSamples)
        {
            shiftedStart -= shiftedEnd - numSamples;
            shiftedEnd = numSamples;
        }

        currentRegion.startSample = juce::jlimit(0, juce::jmax(0, numSamples - minimumWindow), shiftedStart);
        currentRegion.endSample = juce::jlimit(currentRegion.startSample + minimumWindow, numSamples, shiftedEnd);
    }
    currentRegion.reverse = sliceMode
        ? sliceSettings.reverse
        : readParameter(sampleReverse, 0.0f) > 0.5f;
    currentRegion.gain = juce::Decibels::decibelsToGain(sliceMode
                                                            ? sliceSettings.gainDb
                                                            : readParameter(sampleGain, -6.0f));
    currentRegion.transposeSemitones = sliceMode
        ? sliceSettings.transposeSemitones
        : readParameter(sampleTranspose, 0.0f);
    currentRegion.pan = sliceMode ? sliceSettings.pan : 0.0f;
    currentRegion.probability = sliceMode ? sliceSettings.probability : 1.0f;
    currentRegion.fade = sliceMode ? sliceSettings.fade : 0.0f;
    return snapRegionToCleanBoundaries(data.buffer, currentRegion);
}

SamplePlayer::SlicePlaybackSettings SamplePlayer::slicePlaybackSettings(int sliceIndex) const
{
    sliceIndex = juce::jlimit(0, 7, sliceIndex);
    if (! sliceHasCustomSettings(sliceIndex))
        return defaultSlicePlaybackSettings(sliceIndex);

    const auto safeIndex = static_cast<size_t>(sliceIndex);
    SlicePlaybackSettings settings;
    settings.reverse = readParameter(sampleSliceReverse[safeIndex], 0.0f) >= 0.5f;
    settings.gainDb = readParameter(sampleSliceGain[safeIndex], -6.0f);
    settings.transposeSemitones = readParameter(sampleSliceTranspose[safeIndex], 0.0f);
    settings.pan = juce::jlimit(-1.0f, 1.0f, readParameter(sampleSlicePan[safeIndex], 0.0f));
    settings.probability = juce::jlimit(0.0f, 1.0f, readParameter(sampleSliceProbability[safeIndex], 1.0f));
    settings.stutter = readParameter(sampleSliceStutter[safeIndex], 0.0f) >= 0.5f;
    settings.choke = readParameter(sampleSliceChoke[safeIndex], 0.0f) >= 0.5f;
    settings.stutterRepeats = juce::jlimit(1, 8, static_cast<int>(std::round(readParameter(sampleSliceStutterRepeats[safeIndex], 3.0f))));
    settings.nudgePercent = juce::jlimit(-5.0f, 5.0f, readParameter(sampleSliceNudge[safeIndex], 0.0f));
    settings.fade = juce::jlimit(0.0f, 1.0f, readParameter(sampleSliceFade[safeIndex], 0.0f));
    return settings;
}

SamplePlayer::SlicePlaybackSettings SamplePlayer::defaultSlicePlaybackSettings(int sliceIndex) const
{
    sliceIndex = juce::jlimit(0, 7, sliceIndex);
    const auto safeIndex = static_cast<size_t>(sliceIndex);
    const auto styleIndex = juce::jlimit(0, 4, static_cast<int>(std::round(readParameter(sampleSliceStyle, 0.0f))));

    SlicePlaybackSettings settings;
    switch (styleIndex)
    {
        case 1: // Pitch
            settings.transposeSemitones = slicePitchLadder[safeIndex];
            settings.gainDb = -7.0f + static_cast<float>(sliceIndex % 3);
            break;

        case 2: // Reverse
            settings.reverse = (sliceIndex % 2) != 0;
            settings.transposeSemitones = slicePitchLadder[static_cast<size_t>((sliceIndex + 2) % 8)];
            settings.gainDb = -7.5f;
            break;

        case 3: // Stutter
            settings.transposeSemitones = garageSlicePitch[static_cast<size_t>((sliceIndex + 1) % 8)] * 0.5f;
            settings.gainDb = -8.0f;
            settings.stutter = true;
            settings.choke = true;
            settings.stutterRepeats = 2 + (sliceIndex % 4);
            break;

        case 4: // Garage
            settings.reverse = sliceIndex == 2 || sliceIndex == 6;
            settings.transposeSemitones = garageSlicePitch[safeIndex];
            settings.gainDb = -8.5f + static_cast<float>(sliceIndex % 4) * 0.8f;
            settings.stutter = sliceIndex == 3 || sliceIndex == 7;
            settings.choke = true;
            settings.stutterRepeats = sliceIndex == 7 ? 5 : 3;
            break;

        case 0: // Clean
        default:
            break;
    }

    return settings;
}

bool SamplePlayer::sliceHasCustomSettings(int sliceIndex) const
{
    sliceIndex = juce::jlimit(0, 7, sliceIndex);
    return readParameter(sampleSliceCustom[static_cast<size_t>(sliceIndex)], 0.0f) >= 0.5f;
}

bool SamplePlayer::sliceChokeEnabled(int sliceIndex) const
{
    return slicePlaybackSettings(sliceIndex).choke;
}

bool SamplePlayer::sliceStutterEnabled(int sliceIndex) const
{
    return slicePlaybackSettings(sliceIndex).stutter;
}

int SamplePlayer::sliceStutterRepeats(int sliceIndex) const
{
    return slicePlaybackSettings(sliceIndex).stutterRepeats;
}

int SamplePlayer::fadeSamplesForSpan(int sourceSpan, float fadeAmount) const
{
    const auto baseFade = boundaryFadeSamplesForSpan(sourceSpan);
    const auto maxMusicalFade = juce::jmin(juce::jmax(baseFade, sourceSpan / 3),
                                          static_cast<int>(playbackSampleRate * 0.08));
    const auto fadeSamples = baseFade + static_cast<int>(std::round(juce::jlimit(0.0f, 1.0f, fadeAmount)
                                                                    * static_cast<float>(juce::jmax(0, maxMusicalFade - baseFade))));
    return juce::jlimit(1, juce::jmax(1, sourceSpan / 2), fadeSamples);
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
