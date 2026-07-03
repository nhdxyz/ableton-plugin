#pragma once

#include <array>
#include <cstddef>

namespace Synth
{
class WavetableModel
{
public:
    static constexpr size_t frameSize = 256;
    static constexpr size_t maxFrameCount = 64;
    static constexpr size_t defaultControlPointCount = 16;

    using Frame = std::array<float, frameSize>;
    using ControlPoints = std::array<float, defaultControlPointCount>;

    WavetableModel();

    size_t getFrameCount() const noexcept { return frameCount; }
    const Frame& getFrame(size_t index) const noexcept;

    bool setFrameCount(size_t newFrameCount) noexcept;
    bool setFrame(size_t index, const Frame& frame) noexcept;
    bool insertFrame(size_t index, const Frame& frame) noexcept;
    bool removeFrame(size_t index) noexcept;

    float sample(float phase, float position) const noexcept;

    static Frame silentFrame() noexcept;
    static Frame sineFrame() noexcept;

    template <size_t pointCount>
    static Frame frameFromNormalisedControlPoints(const std::array<float, pointCount>& points) noexcept;

    template <size_t sourceFrameCount, size_t pointCount>
    static WavetableModel fromNormalisedControlFrames(
        const std::array<std::array<float, pointCount>, sourceFrameCount>& controlFrames) noexcept;

private:
    std::array<Frame, maxFrameCount> frames {};
    size_t frameCount = 1;

    static float clampBipolar(float value) noexcept;
    static float clampNormalised(float value) noexcept;
    static float wrapUnit(float value) noexcept;
    static float smoothMix(float value) noexcept;
    static float sampleFrame(const Frame& frame, float phase) noexcept;
};

template <size_t pointCount>
WavetableModel::Frame WavetableModel::frameFromNormalisedControlPoints(
    const std::array<float, pointCount>& points) noexcept
{
    Frame frame {};

    if constexpr (pointCount == 0)
    {
        return frame;
    }
    else
    {
        for (size_t sampleIndex = 0; sampleIndex < frame.size(); ++sampleIndex)
        {
            const auto phase = static_cast<float>(sampleIndex) / static_cast<float>(frame.size());
            const auto scaledPoint = phase * static_cast<float>(pointCount);
            const auto leftIndex = static_cast<size_t>(scaledPoint) % pointCount;
            const auto rightIndex = (leftIndex + 1) % pointCount;
            const auto mix = smoothMix(scaledPoint - static_cast<float>(static_cast<size_t>(scaledPoint)));
            const auto left = (clampNormalised(points[leftIndex]) * 2.0f) - 1.0f;
            const auto right = (clampNormalised(points[rightIndex]) * 2.0f) - 1.0f;

            frame[sampleIndex] = clampBipolar(left + ((right - left) * mix));
        }

        return frame;
    }
}

template <size_t sourceFrameCount, size_t pointCount>
WavetableModel WavetableModel::fromNormalisedControlFrames(
    const std::array<std::array<float, pointCount>, sourceFrameCount>& controlFrames) noexcept
{
    WavetableModel model;
    const auto safeFrameCount = sourceFrameCount > maxFrameCount ? maxFrameCount : sourceFrameCount;
    model.setFrameCount(safeFrameCount == 0 ? 1 : safeFrameCount);

    for (size_t frameIndex = 0; frameIndex < safeFrameCount; ++frameIndex)
        model.setFrame(frameIndex, frameFromNormalisedControlPoints(controlFrames[frameIndex]));

    return model;
}
}
