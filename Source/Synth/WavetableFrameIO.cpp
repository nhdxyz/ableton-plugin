#include "WavetableFrameIO.h"

#include "WavetableModel.h"

#include <algorithm>
#include <cmath>

namespace Synth::WavetableFrameIO
{
namespace
{
float clampNormalised(float value) noexcept
{
    if (! std::isfinite(value))
        return 0.5f;

    return std::clamp(value, 0.0f, 1.0f);
}

float clampBipolar(float value) noexcept
{
    if (! std::isfinite(value))
        return 0.0f;

    return std::clamp(value, -1.0f, 1.0f);
}

float readLinear(const float* samples, size_t sampleCount, float position) noexcept
{
    if (samples == nullptr || sampleCount == 0)
        return 0.0f;

    const auto clampedPosition = std::clamp(position, 0.0f, static_cast<float>(sampleCount - 1));
    const auto lowerIndex = std::min(static_cast<size_t>(clampedPosition), sampleCount - 1);
    const auto upperIndex = std::min(lowerIndex + 1, sampleCount - 1);
    const auto mix = clampedPosition - static_cast<float>(lowerIndex);
    const auto lower = clampBipolar(samples[lowerIndex]);
    const auto upper = clampBipolar(samples[upperIndex]);

    return lower + ((upper - lower) * mix);
}

ControlPoints importFrame(const float* samples, size_t sampleCount, float startSample, float endSample) noexcept
{
    ControlPoints points {};
    std::array<float, Oscillator::customWavePointCount> bipolar {};
    const auto span = std::max(1.0f, endSample - startSample);
    auto mean = 0.0f;

    for (size_t pointIndex = 0; pointIndex < bipolar.size(); ++pointIndex)
    {
        const auto phase = static_cast<float>(pointIndex) / static_cast<float>(bipolar.size());
        bipolar[pointIndex] = readLinear(samples, sampleCount, startSample + (phase * span));
        mean += bipolar[pointIndex];
    }

    mean /= static_cast<float>(bipolar.size());

    auto peak = 0.0f;
    for (auto& sample : bipolar)
    {
        sample -= mean;
        peak = std::max(peak, std::abs(sample));
    }

    if (peak <= 0.000001f)
    {
        points.fill(0.5f);
        return points;
    }

    for (size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex)
        points[pointIndex] = clampNormalised(0.5f + ((bipolar[pointIndex] / peak) * 0.5f));

    return points;
}
}

ControlFrameSet neutralFrameSet() noexcept
{
    ControlFrameSet frames {};
    for (auto& frame : frames)
        frame.fill(0.5f);

    return frames;
}

ControlFrameSet importContiguousSamples(const float* samples, size_t sampleCount) noexcept
{
    if (samples == nullptr || sampleCount < 2)
        return neutralFrameSet();

    ControlFrameSet frames {};
    const auto frameSpan = static_cast<float>(sampleCount) / static_cast<float>(frames.size());

    for (size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex)
    {
        const auto startSample = static_cast<float>(frameIndex) * frameSpan;
        const auto endSample = std::min(static_cast<float>(sampleCount - 1),
                                        static_cast<float>(frameIndex + 1) * frameSpan);
        frames[frameIndex] = importFrame(samples, sampleCount, startSample, endSample);
    }

    return frames;
}

std::vector<float> renderContiguousFrames(const ControlFrameSet& frames, size_t samplesPerFrame)
{
    samplesPerFrame = std::max<size_t>(2, samplesPerFrame);

    std::vector<float> samples;
    samples.resize(frames.size() * samplesPerFrame);

    for (size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex)
    {
        const auto frame = WavetableModel::frameFromNormalisedControlPoints(frames[frameIndex]);

        for (size_t sampleIndex = 0; sampleIndex < samplesPerFrame; ++sampleIndex)
        {
            const auto modelIndex = (sampleIndex * frame.size()) / samplesPerFrame;
            samples[(frameIndex * samplesPerFrame) + sampleIndex] = clampBipolar(frame[std::min(modelIndex, frame.size() - 1)]);
        }
    }

    return samples;
}

float maxFrameRange(const ControlFrameSet& frames) noexcept
{
    auto maxRange = 0.0f;

    for (const auto& frame : frames)
    {
        auto minValue = clampNormalised(frame.front());
        auto maxValue = minValue;

        for (const auto value : frame)
        {
            const auto clamped = clampNormalised(value);
            minValue = std::min(minValue, clamped);
            maxValue = std::max(maxValue, clamped);
        }

        maxRange = std::max(maxRange, maxValue - minValue);
    }

    return maxRange;
}
}
