#include "WavetableModel.h"

#include <algorithm>
#include <cmath>

namespace Synth
{
WavetableModel::WavetableModel()
{
    frames[0] = sineFrame();
}

const WavetableModel::Frame& WavetableModel::getFrame(size_t index) const noexcept
{
    return frames[std::min(index, frameCount - 1)];
}

bool WavetableModel::setFrameCount(size_t newFrameCount) noexcept
{
    if (newFrameCount == 0 || newFrameCount > maxFrameCount)
        return false;

    const auto oldFrameCount = frameCount;
    frameCount = newFrameCount;

    if (frameCount > oldFrameCount)
    {
        const auto fillFrame = oldFrameCount > 0 ? frames[oldFrameCount - 1] : sineFrame();
        for (auto index = oldFrameCount; index < frameCount; ++index)
            frames[index] = fillFrame;
    }

    return true;
}

bool WavetableModel::setFrame(size_t index, const Frame& frame) noexcept
{
    if (index >= frameCount)
        return false;

    for (size_t sampleIndex = 0; sampleIndex < frame.size(); ++sampleIndex)
        frames[index][sampleIndex] = clampBipolar(frame[sampleIndex]);

    return true;
}

bool WavetableModel::insertFrame(size_t index, const Frame& frame) noexcept
{
    if (index > frameCount || frameCount >= maxFrameCount)
        return false;

    for (auto moveIndex = frameCount; moveIndex > index; --moveIndex)
        frames[moveIndex] = frames[moveIndex - 1];

    ++frameCount;
    return setFrame(index, frame);
}

bool WavetableModel::removeFrame(size_t index) noexcept
{
    if (frameCount <= 1 || index >= frameCount)
        return false;

    for (auto moveIndex = index; moveIndex + 1 < frameCount; ++moveIndex)
        frames[moveIndex] = frames[moveIndex + 1];

    --frameCount;
    return true;
}

float WavetableModel::sample(float phase, float position) const noexcept
{
    if (frameCount <= 1)
        return sampleFrame(frames[0], phase);

    const auto scaledFrame = clampNormalised(position) * static_cast<float>(frameCount - 1);
    const auto lowerFrame = std::min(static_cast<size_t>(scaledFrame), frameCount - 1);
    const auto upperFrame = std::min(lowerFrame + 1, frameCount - 1);
    const auto mix = smoothMix(scaledFrame - static_cast<float>(lowerFrame));
    const auto lower = sampleFrame(frames[lowerFrame], phase);
    const auto upper = sampleFrame(frames[upperFrame], phase);

    return clampBipolar(lower + ((upper - lower) * mix));
}

WavetableModel::Frame WavetableModel::silentFrame() noexcept
{
    Frame frame {};
    frame.fill(0.0f);
    return frame;
}

WavetableModel::Frame WavetableModel::sineFrame() noexcept
{
    Frame frame {};
    for (size_t sampleIndex = 0; sampleIndex < frame.size(); ++sampleIndex)
    {
        const auto phase = static_cast<float>(sampleIndex) / static_cast<float>(frame.size());
        frame[sampleIndex] = std::sin(phase * 2.0f * 3.14159265358979323846f);
    }

    return frame;
}

float WavetableModel::clampBipolar(float value) noexcept
{
    if (! std::isfinite(value))
        return 0.0f;

    return std::clamp(value, -1.0f, 1.0f);
}

float WavetableModel::clampNormalised(float value) noexcept
{
    if (! std::isfinite(value))
        return 0.0f;

    return std::clamp(value, 0.0f, 1.0f);
}

float WavetableModel::wrapUnit(float value) noexcept
{
    if (! std::isfinite(value))
        return 0.0f;

    return value - std::floor(value);
}

float WavetableModel::smoothMix(float value) noexcept
{
    const auto clamped = clampNormalised(value);
    return clamped * clamped * (3.0f - (2.0f * clamped));
}

float WavetableModel::sampleFrame(const Frame& frame, float phase) noexcept
{
    const auto wrappedPhase = wrapUnit(phase);
    const auto scaledSample = wrappedPhase * static_cast<float>(frame.size());
    const auto leftIndex = static_cast<size_t>(scaledSample) % frame.size();
    const auto rightIndex = (leftIndex + 1) % frame.size();
    const auto mix = smoothMix(scaledSample - static_cast<float>(leftIndex));

    return clampBipolar(frame[leftIndex] + ((frame[rightIndex] - frame[leftIndex]) * mix));
}
}
