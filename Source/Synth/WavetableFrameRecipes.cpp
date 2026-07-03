#include "WavetableFrameRecipes.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace Synth::WavetableFrameRecipes
{
namespace
{
constexpr auto pi = 3.14159265358979323846f;
constexpr auto twoPi = pi * 2.0f;

float clampNormalised(float value) noexcept
{
    if (! std::isfinite(value))
        return 0.5f;

    return std::clamp(value, 0.0f, 1.0f);
}

float bipolarToNormalised(float value) noexcept
{
    return clampNormalised(0.5f + (value * 0.5f));
}

ControlPoints makeShape(float (*shapeForPhase)(float)) noexcept
{
    ControlPoints points {};
    const auto lastIndex = std::max<size_t>(1, points.size() - 1);

    for (size_t index = 0; index < points.size(); ++index)
    {
        const auto phase = static_cast<float>(index) / static_cast<float>(lastIndex);
        points[index] = clampNormalised(shapeForPhase(phase));
    }

    return points;
}

ControlPoints blend(const ControlPoints& first, const ControlPoints& second, float amount) noexcept
{
    ControlPoints result {};
    const auto mix = std::clamp(amount, 0.0f, 1.0f);

    for (size_t index = 0; index < result.size(); ++index)
        result[index] = clampNormalised(first[index] + ((second[index] - first[index]) * mix));

    return result;
}

ControlPoints smooth(const ControlPoints& source, float amount) noexcept
{
    ControlPoints result {};
    const auto mix = std::clamp(amount, 0.0f, 1.0f);

    for (size_t index = 0; index < source.size(); ++index)
    {
        const auto left = source[index == 0 ? 0 : index - 1];
        const auto right = source[index + 1 >= source.size() ? source.size() - 1 : index + 1];
        const auto smoothed = (left * 0.25f) + (source[index] * 0.5f) + (right * 0.25f);
        result[index] = clampNormalised(source[index] + ((smoothed - source[index]) * mix));
    }

    return result;
}

ControlPoints sharpen(const ControlPoints& source, float amount) noexcept
{
    ControlPoints result {};
    const auto depth = std::clamp(amount, 0.0f, 1.0f);

    for (size_t index = 0; index < source.size(); ++index)
    {
        const auto left = source[index == 0 ? 0 : index - 1];
        const auto right = source[index + 1 >= source.size() ? source.size() - 1 : index + 1];
        const auto neighbourAverage = (left + right) * 0.5f;
        result[index] = clampNormalised(source[index] + ((source[index] - neighbourAverage) * depth));
    }

    return result;
}

ControlPoints fold(const ControlPoints& source, float amount) noexcept
{
    ControlPoints result {};
    const auto depth = std::clamp(amount, 0.0f, 1.0f);

    for (size_t index = 0; index < source.size(); ++index)
    {
        const auto bipolar = (source[index] * 2.0f) - 1.0f;
        const auto folded = std::sin(bipolar * pi * (1.0f + depth));
        result[index] = bipolarToNormalised((bipolar * (1.0f - depth)) + (folded * depth));
    }

    return result;
}

ControlPoints softClip(const ControlPoints& source, float amount) noexcept
{
    ControlPoints result {};
    const auto drive = 1.0f + (std::clamp(amount, 0.0f, 1.0f) * 3.2f);
    const auto normaliser = std::tanh(drive);

    for (size_t index = 0; index < source.size(); ++index)
    {
        const auto bipolar = (source[index] * 2.0f) - 1.0f;
        result[index] = bipolarToNormalised(std::tanh(bipolar * drive) / normaliser);
    }

    return result;
}

ControlPoints windowEnds(const ControlPoints& source, float amount) noexcept
{
    ControlPoints result {};
    const auto depth = std::clamp(amount, 0.0f, 1.0f);
    const auto lastIndex = std::max<size_t>(1, source.size() - 1);

    for (size_t index = 0; index < source.size(); ++index)
    {
        const auto phase = static_cast<float>(index) / static_cast<float>(lastIndex);
        const auto window = std::sin(pi * phase);
        result[index] = clampNormalised(source[index] + (((0.5f + ((source[index] - 0.5f) * window)) - source[index]) * depth));
    }

    return result;
}

float sineShape(float phase) noexcept
{
    return bipolarToNormalised(std::sin(twoPi * phase));
}

float triangleShape(float phase) noexcept
{
    const auto bipolar = phase < 0.5f ? (phase * 4.0f) - 1.0f : 3.0f - (phase * 4.0f);
    return bipolarToNormalised(bipolar);
}

float sawShape(float phase) noexcept
{
    return clampNormalised(phase);
}

float squareShape(float phase) noexcept
{
    return phase < 0.5f ? 1.0f : 0.0f;
}

float organShape(float phase) noexcept
{
    const auto wave = (std::sin(twoPi * phase) * 0.95f)
        + (std::sin(twoPi * phase * 2.0f) * 0.42f)
        + (std::sin(twoPi * phase * 3.0f + 0.08f) * 0.25f)
        + (std::sin(twoPi * phase * 6.0f + 0.17f) * 0.11f);
    return bipolarToNormalised(wave * 0.58f);
}

float pianoStabShape(float phase) noexcept
{
    const auto wave = (std::sin(twoPi * phase) * 0.92f)
        + (std::sin(twoPi * phase * 2.0f + 0.08f) * 0.46f)
        + (std::sin(twoPi * phase * 3.0f + 0.35f) * 0.28f)
        + (std::sin(twoPi * phase * 5.0f + 0.22f) * 0.18f)
        + (std::sin(twoPi * phase * 9.0f + 0.62f) * 0.08f);
    return bipolarToNormalised(std::sin(wave * 0.86f) * 0.94f);
}

float hollowShape(float phase) noexcept
{
    const auto wave = (std::sin(twoPi * phase * 2.0f) * 0.82f)
        + (std::sin(twoPi * phase * 4.0f + 0.11f) * 0.42f)
        + (std::sin(twoPi * phase * 8.0f + 0.27f) * 0.18f);
    return bipolarToNormalised(wave * 0.72f);
}

float brightPartialsShape(float phase) noexcept
{
    auto wave = 0.0f;
    for (auto harmonic = 1; harmonic <= 14; ++harmonic)
    {
        const auto h = static_cast<float>(harmonic);
        const auto tilt = std::pow(h / 14.0f, 0.32f);
        wave += std::sin(twoPi * phase * h + (static_cast<float>(harmonic % 3) * 0.13f))
            * tilt / std::sqrt(h);
    }

    return bipolarToNormalised(wave * 0.32f);
}

float raveFoldShape(float phase) noexcept
{
    const auto base = ((sawShape(phase) * 2.0f) - 1.0f) * 1.35f;
    const auto folded = std::sin(base * pi * 1.7f);
    return bipolarToNormalised((base * 0.36f) + (folded * 0.64f));
}

float pulseWidthShape(float phase) noexcept
{
    return phase < 0.34f ? 1.0f : 0.0f;
}
}

ControlPoints normalise(ControlPoints points) noexcept
{
    auto minValue = points.front();
    auto maxValue = points.front();

    for (auto& value : points)
    {
        value = clampNormalised(value);
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }

    const auto range = maxValue - minValue;
    if (range <= 0.001f)
    {
        points.fill(0.5f);
        return points;
    }

    for (auto& value : points)
        value = clampNormalised((value - minValue) / range);

    return points;
}

ControlFrameSet currentSweep(const ControlPoints& seed) noexcept
{
    const auto base = normalise(seed);
    const auto sine = makeShape(sineShape);
    const auto saw = makeShape(sawShape);
    const auto square = makeShape(squareShape);
    const auto bright = makeShape(brightPartialsShape);

    return {
        smooth(base, 0.55f),
        base,
        sharpen(base, 0.55f),
        normalise(blend(base, saw, 0.45f)),
        normalise(blend(base, square, 0.35f)),
        fold(normalise(blend(base, bright, 0.42f)), 0.45f),
        softClip(normalise(blend(base, sine, 0.34f)), 0.68f),
        windowEnds(fold(sharpen(base, 0.45f), 0.62f), 0.42f)
    };
}

ControlFrameSet classicHouseStack() noexcept
{
    const auto sine = makeShape(sineShape);
    const auto organ = makeShape(organShape);
    const auto piano = makeShape(pianoStabShape);
    const auto saw = makeShape(sawShape);
    const auto hollow = makeShape(hollowShape);
    const auto bright = makeShape(brightPartialsShape);

    return {
        sine,
        smooth(organ, 0.34f),
        organ,
        piano,
        normalise(blend(piano, saw, 0.28f)),
        hollow,
        softClip(bright, 0.45f),
        windowEnds(fold(normalise(blend(bright, saw, 0.38f)), 0.38f), 0.35f)
    };
}

ControlFrameSet raveSweep() noexcept
{
    const auto saw = makeShape(sawShape);
    const auto square = makeShape(squareShape);
    const auto pulse = makeShape(pulseWidthShape);
    const auto triangle = makeShape(triangleShape);
    const auto bright = makeShape(brightPartialsShape);
    const auto foldShapePoints = makeShape(raveFoldShape);

    return {
        saw,
        normalise(blend(saw, square, 0.32f)),
        square,
        pulse,
        normalise(blend(pulse, triangle, 0.38f)),
        foldShapePoints,
        softClip(normalise(blend(foldShapePoints, bright, 0.48f)), 0.55f),
        windowEnds(fold(bright, 0.72f), 0.25f)
    };
}

float meanAbsoluteDifference(const ControlPoints& first, const ControlPoints& second) noexcept
{
    auto sum = 0.0f;
    for (size_t index = 0; index < first.size(); ++index)
        sum += std::abs(clampNormalised(first[index]) - clampNormalised(second[index]));

    return sum / static_cast<float>(first.size());
}
}
