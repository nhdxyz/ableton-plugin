#include "../Source/Parameters.h"
#include "../Source/Synth/Oscillator.h"

#include <cmath>
#include <iostream>
#include <vector>

namespace
{
struct RenderStats
{
    float rms = 0.0f;
    float peak = 0.0f;
    std::vector<float> samples;
};

RenderStats renderWaveform(Synth::Waveform waveform,
                           int warpMode = 0,
                           float warpAmount = 0.0f,
                           int warpBMode = 0,
                           float warpBAmount = 0.0f)
{
    Synth::Oscillator oscillator;
    oscillator.prepare(44100.0);
    oscillator.reset();
    oscillator.setWaveform(waveform);
    oscillator.setFrequency(110.0f);
    oscillator.setWarp(warpAmount);
    oscillator.setWarpMode(warpMode);
    oscillator.setWarpB(warpBAmount);
    oscillator.setWarpBMode(warpBMode);
    oscillator.setWavetablePosition(0.42f);

    RenderStats stats;
    stats.samples.reserve(4096);

    double sumSquares = 0.0;
    for (auto index = 0; index < 4096; ++index)
    {
        const auto sample = oscillator.process();
        if (! std::isfinite(sample))
        {
            stats.rms = -1.0f;
            return stats;
        }

        stats.samples.push_back(sample);
        stats.peak = std::max(stats.peak, std::abs(sample));
        sumSquares += static_cast<double>(sample) * static_cast<double>(sample);
    }

    stats.rms = static_cast<float>(std::sqrt(sumSquares / static_cast<double>(stats.samples.size())));
    return stats;
}

float meanAbsoluteDifference(const std::vector<float>& first, const std::vector<float>& second)
{
    const auto count = std::min(first.size(), second.size());
    if (count == 0)
        return 0.0f;

    double sum = 0.0;
    for (size_t index = 0; index < count; ++index)
        sum += std::abs(first[index] - second[index]);

    return static_cast<float>(sum / static_cast<double>(count));
}
}

int main()
{
    const auto choices = Parameters::waveformChoices();
    if (choices.size() != 8
        || choices[0] != "Sine"
        || choices[1] != "Saw"
        || choices[2] != "Square"
        || choices[3] != "Triangle"
        || choices[4] != "Wavetable"
        || choices[5] != "Organ"
        || choices[6] != "House Piano"
        || choices[7] != "Custom")
    {
        std::cerr << "Waveform choices were reordered or missing appended custom/house source modes\n";
        return 1;
    }

    const auto warpModeChoices = Parameters::oscWarpModeChoices();
    if (warpModeChoices.size() != 4
        || warpModeChoices[0] != "Harmonic"
        || warpModeChoices[1] != "Fold"
        || warpModeChoices[2] != "Bend"
        || warpModeChoices[3] != "Sync")
    {
        std::cerr << "Osc warp mode choices were reordered or missing\n";
        return 1;
    }

    const auto organStats = renderWaveform(Synth::Waveform::organ);
    const auto pianoStats = renderWaveform(Synth::Waveform::housePiano);
    const auto customStats = renderWaveform(Synth::Waveform::custom);

    if (organStats.rms <= 0.02f || organStats.peak <= 0.05f || organStats.peak > 1.05f)
    {
        std::cerr << "Organ source rendered outside the expected audible/safe range: rms "
                  << organStats.rms << " peak " << organStats.peak << '\n';
        return 1;
    }

    if (pianoStats.rms <= 0.02f || pianoStats.peak <= 0.05f || pianoStats.peak > 1.05f)
    {
        std::cerr << "House Piano source rendered outside the expected audible/safe range: rms "
                  << pianoStats.rms << " peak " << pianoStats.peak << '\n';
        return 1;
    }

    if (customStats.rms <= 0.02f || customStats.peak <= 0.05f || customStats.peak > 1.05f)
    {
        std::cerr << "Custom source rendered outside the expected audible/safe range: rms "
                  << customStats.rms << " peak " << customStats.peak << '\n';
        return 1;
    }

    const auto harmonicWarp = renderWaveform(Synth::Waveform::saw, 0, 0.68f);
    const auto foldWarp = renderWaveform(Synth::Waveform::saw, 1, 0.68f);
    const auto bendWarp = renderWaveform(Synth::Waveform::saw, 2, 0.68f);
    const auto syncWarp = renderWaveform(Synth::Waveform::saw, 3, 0.68f);
    const auto dualWarp = renderWaveform(Synth::Waveform::saw, 1, 0.42f, 2, 0.34f);

    for (const auto* stats : { &harmonicWarp, &foldWarp, &bendWarp, &syncWarp, &dualWarp })
    {
        if (stats->rms <= 0.02f || stats->peak <= 0.05f || stats->peak > 1.05f)
        {
            std::cerr << "Warp mode rendered outside the expected audible/safe range: rms "
                      << stats->rms << " peak " << stats->peak << '\n';
            return 1;
        }
    }

    if (meanAbsoluteDifference(harmonicWarp.samples, foldWarp.samples) <= 0.025f
        || meanAbsoluteDifference(harmonicWarp.samples, bendWarp.samples) <= 0.025f
        || meanAbsoluteDifference(harmonicWarp.samples, syncWarp.samples) <= 0.025f)
    {
        std::cerr << "Osc warp modes rendered too similarly to harmonic mode\n";
        return 1;
    }

    if (meanAbsoluteDifference(foldWarp.samples, dualWarp.samples) <= 0.018f)
    {
        std::cerr << "Dual warp stack rendered too similarly to a single warp lane\n";
        return 1;
    }

    const auto difference = meanAbsoluteDifference(organStats.samples, pianoStats.samples);
    if (difference <= 0.04f)
    {
        std::cerr << "Organ and House Piano source modes rendered too similarly: mean diff "
                  << difference << '\n';
        return 1;
    }

    std::cout << "Source character audit passed for appended waveform choices, warp mode rendering, custom source rendering, and house source rendering.\n";
    return 0;
}
