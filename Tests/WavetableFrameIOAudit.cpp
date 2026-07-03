#include "../Source/Synth/WavetableFrameIO.h"
#include "../Source/Synth/WavetableFrameRecipes.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
bool framesAreSafe(const Synth::WavetableFrameIO::ControlFrameSet& frames)
{
    for (const auto& frame : frames)
        for (const auto value : frame)
            if (! std::isfinite(value) || value < 0.0f || value > 1.0f)
                return false;

    return true;
}
}

int main()
{
    constexpr auto samplesPerFrame = 128;
    std::vector<float> samples;
    samples.resize(Synth::Oscillator::customWaveFrameCount * samplesPerFrame);

    for (size_t frameIndex = 0; frameIndex < Synth::Oscillator::customWaveFrameCount; ++frameIndex)
    {
        const auto harmonic = static_cast<float>(frameIndex + 1);
        const auto brightness = 0.2f + (static_cast<float>(frameIndex) * 0.07f);
        for (auto sampleIndex = 0; sampleIndex < samplesPerFrame; ++sampleIndex)
        {
            const auto phase = static_cast<float>(sampleIndex) / static_cast<float>(samplesPerFrame);
            const auto base = std::sin(phase * 2.0f * 3.14159265358979323846f);
            const auto overtone = std::sin(phase * harmonic * 2.0f * 3.14159265358979323846f) * brightness;
            samples[(frameIndex * samplesPerFrame) + static_cast<size_t>(sampleIndex)] = (base * 0.72f) + overtone;
        }
    }

    samples[5] = std::numeric_limits<float>::quiet_NaN();
    samples[111] = 4.0f;
    samples[222] = -4.0f;

    const auto imported = Synth::WavetableFrameIO::importContiguousSamples(samples.data(), samples.size());
    if (! framesAreSafe(imported))
    {
        std::cerr << "Imported frame stack contained unsafe control values\n";
        return 1;
    }

    if (Synth::WavetableFrameIO::maxFrameRange(imported) < 0.5f)
    {
        std::cerr << "Imported frame stack was too flat\n";
        return 1;
    }

    const auto firstLastDifference = Synth::WavetableFrameRecipes::meanAbsoluteDifference(imported.front(),
                                                                                         imported.back());
    if (firstLastDifference < 0.03f)
    {
        std::cerr << "Imported frame stack did not preserve meaningful frame movement: "
                  << firstLastDifference << '\n';
        return 1;
    }

    const auto exported = Synth::WavetableFrameIO::renderContiguousFrames(imported, 64);
    if (exported.size() != Synth::Oscillator::customWaveFrameCount * 64)
    {
        std::cerr << "Rendered frame stack length was unexpected: " << exported.size() << '\n';
        return 1;
    }

    auto exportedPeak = 0.0f;
    for (const auto sample : exported)
    {
        if (! std::isfinite(sample) || sample < -1.0f || sample > 1.0f)
        {
            std::cerr << "Rendered frame stack contained unsafe sample " << sample << '\n';
            return 1;
        }

        exportedPeak = std::max(exportedPeak, std::abs(sample));
    }

    if (exportedPeak < 0.25f)
    {
        std::cerr << "Rendered frame stack was unexpectedly quiet: " << exportedPeak << '\n';
        return 1;
    }

    std::vector<float> silence(samples.size(), 0.0f);
    const auto silentFrames = Synth::WavetableFrameIO::importContiguousSamples(silence.data(), silence.size());
    if (! framesAreSafe(silentFrames) || Synth::WavetableFrameIO::maxFrameRange(silentFrames) > 0.001f)
    {
        std::cerr << "Silent frame-stack import did not return neutral frames\n";
        return 1;
    }

    std::cout << "Wavetable frame IO audit passed for import safety, frame movement, export bounds, and silence handling.\n";
    return 0;
}
