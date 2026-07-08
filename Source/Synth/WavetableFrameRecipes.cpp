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

struct Spectrum
{
    ControlPoints real {};
    ControlPoints imaginary {};
};

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

float normalisedToBipolar(float value) noexcept
{
    return (clampNormalised(value) * 2.0f) - 1.0f;
}

template <typename ShapeForPhase>
ControlPoints makeShape(ShapeForPhase shapeForPhase) noexcept
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

ControlPoints makePulse(float width) noexcept
{
    width = std::clamp(width, 0.08f, 0.92f);
    return makeShape([width] (float phase) noexcept
    {
        return phase < width ? 1.0f : 0.0f;
    });
}

ControlPoints makeDetunedSaw(float spread, float phaseOffset) noexcept
{
    spread = std::clamp(spread, 0.0f, 0.08f);
    return makeShape([spread, phaseOffset] (float phase) noexcept
    {
        auto wave = 0.0f;
        for (auto harmonic = 1; harmonic <= 14; ++harmonic)
        {
            const auto h = static_cast<float>(harmonic);
            const auto tilt = 1.0f / std::pow(h, 0.86f);
            wave += std::sin(twoPi * ((phase * h) + phaseOffset + (spread * h))) * tilt;
            wave += std::sin(twoPi * ((phase * h) - phaseOffset - (spread * h * 0.82f))) * tilt * 0.92f;
        }

        return bipolarToNormalised(wave * 0.24f);
    });
}

ControlPoints blend(const ControlPoints& first, const ControlPoints& second, float amount) noexcept
{
    ControlPoints result {};
    const auto mix = std::clamp(amount, 0.0f, 1.0f);

    for (size_t index = 0; index < result.size(); ++index)
        result[index] = clampNormalised(first[index] + ((second[index] - first[index]) * mix));

    return result;
}

Spectrum analyseSpectrum(const ControlPoints& source) noexcept
{
    Spectrum spectrum {};
    const auto count = static_cast<float>(source.size());

    for (size_t bin = 0; bin < source.size(); ++bin)
    {
        const auto harmonic = static_cast<float>(bin);
        auto real = 0.0f;
        auto imaginary = 0.0f;

        for (size_t sampleIndex = 0; sampleIndex < source.size(); ++sampleIndex)
        {
            const auto sample = normalisedToBipolar(source[sampleIndex]);
            const auto phase = twoPi * harmonic * static_cast<float>(sampleIndex) / count;
            real += sample * std::cos(phase);
            imaginary -= sample * std::sin(phase);
        }

        spectrum.real[bin] = real / count;
        spectrum.imaginary[bin] = imaginary / count;
    }

    return spectrum;
}

Spectrum blendSpectra(const Spectrum& first, const Spectrum& second, float amount) noexcept
{
    Spectrum result {};
    const auto mix = std::clamp(amount, 0.0f, 1.0f);

    for (size_t bin = 0; bin < result.real.size(); ++bin)
    {
        const auto firstMagnitude = std::sqrt((first.real[bin] * first.real[bin])
                                              + (first.imaginary[bin] * first.imaginary[bin]));
        const auto secondMagnitude = std::sqrt((second.real[bin] * second.real[bin])
                                               + (second.imaginary[bin] * second.imaginary[bin]));
        const auto magnitude = firstMagnitude + ((secondMagnitude - firstMagnitude) * mix);

        const auto firstPhase = std::atan2(first.imaginary[bin], first.real[bin]);
        const auto secondPhase = std::atan2(second.imaginary[bin], second.real[bin]);
        auto phaseX = (std::cos(firstPhase) * (1.0f - mix)) + (std::cos(secondPhase) * mix);
        auto phaseY = (std::sin(firstPhase) * (1.0f - mix)) + (std::sin(secondPhase) * mix);
        const auto phaseLength = std::sqrt((phaseX * phaseX) + (phaseY * phaseY));

        if (phaseLength <= 0.000001f)
        {
            result.real[bin] = first.real[bin] + ((second.real[bin] - first.real[bin]) * mix);
            result.imaginary[bin] = first.imaginary[bin] + ((second.imaginary[bin] - first.imaginary[bin]) * mix);
            continue;
        }

        phaseX /= phaseLength;
        phaseY /= phaseLength;
        result.real[bin] = magnitude * phaseX;
        result.imaginary[bin] = magnitude * phaseY;
    }

    return result;
}

ControlPoints renderSpectrum(const Spectrum& spectrum) noexcept
{
    ControlPoints points {};
    const auto count = static_cast<float>(points.size());

    for (size_t sampleIndex = 0; sampleIndex < points.size(); ++sampleIndex)
    {
        auto sample = 0.0f;
        for (size_t bin = 0; bin < points.size(); ++bin)
        {
            const auto phase = twoPi * static_cast<float>(bin) * static_cast<float>(sampleIndex) / count;
            sample += (spectrum.real[bin] * std::cos(phase)) - (spectrum.imaginary[bin] * std::sin(phase));
        }

        points[sampleIndex] = bipolarToNormalised(sample);
    }

    return normalise(points);
}

size_t foldedHarmonicIndex(size_t bin, size_t count) noexcept
{
    return bin == 0 ? size_t { 0 } : std::min(bin, count - bin);
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

ControlFrameSet acidStack() noexcept
{
    const auto saw = makeShape(sawShape);
    const auto pulse28 = makePulse(0.28f);
    const auto pulse42 = makePulse(0.42f);
    const auto bright = makeShape(brightPartialsShape);
    const auto resonant = makeShape([] (float phase) noexcept
    {
        const auto edge = (phase * 2.0f) - 1.0f;
        const auto squelch = (std::sin(twoPi * phase * 3.0f + 0.18f) * 0.34f)
            + (std::sin(twoPi * phase * 7.0f + 0.41f) * 0.15f)
            + (std::sin(twoPi * phase * 11.0f + 0.24f) * 0.06f);
        return bipolarToNormalised(std::tanh((edge * 0.86f) + squelch) * 0.98f);
    });

    return {
        saw,
        normalise(blend(saw, pulse42, 0.35f)),
        pulse42,
        normalise(blend(pulse42, resonant, 0.52f)),
        resonant,
        fold(normalise(blend(resonant, bright, 0.38f)), 0.36f),
        softClip(normalise(blend(pulse28, resonant, 0.62f)), 0.62f),
        windowEnds(fold(normalise(blend(pulse28, bright, 0.42f)), 0.58f), 0.22f)
    };
}

ControlFrameSet rubberBassStack() noexcept
{
    const auto sine = makeShape(sineShape);
    const auto triangle = makeShape(triangleShape);
    const auto square = makeShape(squareShape);
    const auto hollow = makeShape(hollowShape);
    const auto pulse58 = makePulse(0.58f);
    const auto rubber = makeShape([] (float phase) noexcept
    {
        const auto wave = (std::sin(twoPi * phase) * 0.95f)
            + (std::sin(twoPi * phase * 2.0f + 0.36f) * 0.34f)
            - (std::sin(twoPi * phase * 3.0f + 0.08f) * 0.18f)
            + (std::sin(twoPi * phase * 5.0f + 0.62f) * 0.08f);
        return bipolarToNormalised(std::tanh(wave * 1.18f) * 0.84f);
    });

    return {
        sine,
        normalise(blend(sine, triangle, 0.52f)),
        triangle,
        rubber,
        normalise(blend(rubber, pulse58, 0.38f)),
        normalise(blend(square, rubber, 0.48f)),
        softClip(normalise(blend(hollow, rubber, 0.64f)), 0.52f),
        windowEnds(fold(normalise(blend(rubber, square, 0.56f)), 0.34f), 0.34f)
    };
}

ControlFrameSet reeseStack() noexcept
{
    const auto sawA = makeDetunedSaw(0.006f, 0.00f);
    const auto sawB = makeDetunedSaw(0.014f, 0.04f);
    const auto sawC = makeDetunedSaw(0.026f, 0.09f);
    const auto sawD = makeDetunedSaw(0.042f, 0.15f);
    const auto bright = makeShape(brightPartialsShape);
    const auto hollow = makeShape(hollowShape);
    const auto pulse = makePulse(0.46f);

    return {
        smooth(sawA, 0.18f),
        sawA,
        normalise(blend(sawA, sawB, 0.52f)),
        sawB,
        normalise(blend(sawB, sawC, 0.58f)),
        softClip(normalise(blend(sawC, hollow, 0.32f)), 0.42f),
        fold(normalise(blend(sawD, bright, 0.28f)), 0.28f),
        windowEnds(normalise(blend(sawD, pulse, 0.30f)), 0.18f)
    };
}

ControlFrameSet reverseFrameOrder(ControlFrameSet frames) noexcept
{
    std::reverse(frames.begin(), frames.end());
    return frames;
}

ControlFrameSet rotateFrameOrder(ControlFrameSet frames, int offset) noexcept
{
    if (frames.empty())
        return frames;

    const auto count = static_cast<int>(frames.size());
    auto safeOffset = offset % count;
    if (safeOffset < 0)
        safeOffset += count;

    std::rotate(frames.begin(), frames.begin() + safeOffset, frames.end());
    return frames;
}

ControlFrameSet duplicateFrameSlot(ControlFrameSet frames, size_t frameIndex) noexcept
{
    if (frames.empty())
        return frames;

    const auto lastIndex = frames.size() - 1;
    const auto safeIndex = std::min(frameIndex, lastIndex);
    if (safeIndex >= lastIndex)
        return frames;

    for (auto index = lastIndex; index > safeIndex + 1; --index)
        frames[index] = frames[index - 1];

    frames[safeIndex + 1] = frames[safeIndex];
    return frames;
}

ControlFrameSet deleteFrameSlot(ControlFrameSet frames, size_t frameIndex) noexcept
{
    if (frames.size() <= 1)
        return frames;

    const auto lastIndex = frames.size() - 1;
    const auto safeIndex = std::min(frameIndex, lastIndex);

    for (auto index = safeIndex; index < lastIndex; ++index)
        frames[index] = frames[index + 1];

    frames[lastIndex] = frames[lastIndex - 1];
    return frames;
}

ControlFrameSet moveFrameSlot(ControlFrameSet frames, size_t fromFrameIndex, size_t toFrameIndex) noexcept
{
    if (frames.empty())
        return frames;

    const auto lastIndex = frames.size() - 1;
    const auto fromIndex = std::min(fromFrameIndex, lastIndex);
    const auto toIndex = std::min(toFrameIndex, lastIndex);
    if (fromIndex == toIndex)
        return frames;

    const auto movingFrame = frames[fromIndex];
    if (fromIndex < toIndex)
    {
        for (auto index = fromIndex; index < toIndex; ++index)
            frames[index] = frames[index + 1];
    }
    else
    {
        for (auto index = fromIndex; index > toIndex; --index)
            frames[index] = frames[index - 1];
    }

    frames[toIndex] = movingFrame;
    return frames;
}

ControlFrameSet smoothFrameMotion(const ControlFrameSet& frames) noexcept
{
    ControlFrameSet result {};

    for (size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex)
    {
        const auto previousIndex = frameIndex == 0 ? frameIndex : frameIndex - 1;
        const auto nextIndex = frameIndex + 1 >= frames.size() ? frameIndex : frameIndex + 1;

        for (size_t pointIndex = 0; pointIndex < frames[frameIndex].size(); ++pointIndex)
        {
            result[frameIndex][pointIndex] = clampNormalised(
                (frames[previousIndex][pointIndex] * 0.24f)
                + (frames[frameIndex][pointIndex] * 0.52f)
                + (frames[nextIndex][pointIndex] * 0.24f));
        }
    }

    return result;
}

ControlFrameSet emphasiseFrameMotion(const ControlFrameSet& frames) noexcept
{
    ControlFrameSet result {};

    for (size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex)
    {
        const auto previousIndex = frameIndex == 0 ? frameIndex : frameIndex - 1;
        const auto nextIndex = frameIndex + 1 >= frames.size() ? frameIndex : frameIndex + 1;

        for (size_t pointIndex = 0; pointIndex < frames[frameIndex].size(); ++pointIndex)
        {
            const auto neighbourAverage = (frames[previousIndex][pointIndex] + frames[nextIndex][pointIndex]) * 0.5f;
            const auto localContrast = frames[frameIndex][pointIndex] - neighbourAverage;
            const auto centerContrast = frames[frameIndex][pointIndex] - 0.5f;
            result[frameIndex][pointIndex] = clampNormalised(frames[frameIndex][pointIndex]
                                                             + (localContrast * 0.32f)
                                                             + (centerContrast * 0.16f));
        }

        result[frameIndex] = normalise(result[frameIndex]);
    }

    return result;
}

ControlFrameSet blendFrameStacks(const ControlFrameSet& first, const ControlFrameSet& second, float amount) noexcept
{
    ControlFrameSet result {};
    const auto mix = std::clamp(amount, 0.0f, 1.0f);

    for (size_t frameIndex = 0; frameIndex < result.size(); ++frameIndex)
        result[frameIndex] = blend(first[frameIndex], second[frameIndex], mix);

    return result;
}

ControlFrameSet morphBetweenFrameStacks(const ControlFrameSet& first, const ControlFrameSet& second) noexcept
{
    ControlFrameSet result {};
    const auto lastIndex = std::max<size_t>(1, result.size() - 1);

    for (size_t frameIndex = 0; frameIndex < result.size(); ++frameIndex)
    {
        const auto mix = static_cast<float>(frameIndex) / static_cast<float>(lastIndex);
        result[frameIndex] = blend(first[frameIndex], second[frameIndex], mix);
    }

    return result;
}

ControlFrameSet spliceFrameStacks(const ControlFrameSet& first, const ControlFrameSet& second) noexcept
{
    ControlFrameSet result {};

    for (size_t frameIndex = 0; frameIndex < result.size(); ++frameIndex)
    {
        const auto& primary = (frameIndex % 2 == 0) ? first[frameIndex] : second[frameIndex];
        const auto& secondary = (frameIndex % 2 == 0) ? second[frameIndex] : first[frameIndex];
        const auto edgeBlend = frameIndex == 0 || frameIndex + 1 >= result.size() ? 0.18f : 0.30f;
        result[frameIndex] = blend(primary, secondary, edgeBlend);
    }

    return result;
}

ControlFrameSet spectralBlendFrameStacks(const ControlFrameSet& first, const ControlFrameSet& second, float amount) noexcept
{
    ControlFrameSet result {};
    const auto mix = std::clamp(amount, 0.0f, 1.0f);

    for (size_t frameIndex = 0; frameIndex < result.size(); ++frameIndex)
    {
        const auto firstSpectrum = analyseSpectrum(first[frameIndex]);
        const auto secondSpectrum = analyseSpectrum(second[frameIndex]);
        result[frameIndex] = renderSpectrum(blendSpectra(firstSpectrum, secondSpectrum, mix));
    }

    return result;
}

ControlFrameSet interleaveHarmonicsFrameStacks(const ControlFrameSet& primary, const ControlFrameSet& secondary) noexcept
{
    ControlFrameSet result {};

    for (size_t frameIndex = 0; frameIndex < result.size(); ++frameIndex)
    {
        const auto primarySpectrum = analyseSpectrum(primary[frameIndex]);
        const auto secondarySpectrum = analyseSpectrum(secondary[frameIndex]);
        const auto hybridSpectrum = blendSpectra(primarySpectrum, secondarySpectrum, 0.42f);
        Spectrum interleaved {};

        for (size_t bin = 0; bin < interleaved.real.size(); ++bin)
        {
            const auto harmonic = foldedHarmonicIndex(bin, interleaved.real.size());
            const auto usePrimary = harmonic <= 2;
            const auto useSecondary = harmonic > 2 && harmonic % 2 == 0;

            const auto& sourceSpectrum = usePrimary ? primarySpectrum : (useSecondary ? secondarySpectrum : hybridSpectrum);
            interleaved.real[bin] = sourceSpectrum.real[bin];
            interleaved.imaginary[bin] = sourceSpectrum.imaginary[bin];
        }

        result[frameIndex] = renderSpectrum(interleaved);
    }

    return result;
}

float meanAbsoluteDifference(const ControlPoints& first, const ControlPoints& second) noexcept
{
    auto sum = 0.0f;
    for (size_t index = 0; index < first.size(); ++index)
        sum += std::abs(clampNormalised(first[index]) - clampNormalised(second[index]));

    return sum / static_cast<float>(first.size());
}
}
