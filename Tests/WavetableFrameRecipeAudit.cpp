#include "../Source/Synth/WavetableFrameRecipes.h"

#include <cmath>
#include <iostream>

namespace
{
using Synth::WavetableFrameRecipes::ControlFrameSet;
using Synth::WavetableFrameRecipes::ControlPoints;

bool validateFrameSet(const char* label, const ControlFrameSet& frames)
{
    auto accumulatedDifference = 0.0f;

    for (size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex)
    {
        auto minValue = frames[frameIndex].front();
        auto maxValue = frames[frameIndex].front();

        for (const auto value : frames[frameIndex])
        {
            if (! std::isfinite(value) || value < 0.0f || value > 1.0f)
            {
                std::cerr << label << " frame " << frameIndex << " contains unsafe value "
                          << value << '\n';
                return false;
            }

            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
        }

        if (maxValue - minValue < 0.12f)
        {
            std::cerr << label << " frame " << frameIndex << " is too flat for a useful wavetable frame\n";
            return false;
        }

        if (frameIndex > 0)
            accumulatedDifference += Synth::WavetableFrameRecipes::meanAbsoluteDifference(frames[frameIndex - 1],
                                                                                         frames[frameIndex]);
    }

    const auto averageDifference = accumulatedDifference / static_cast<float>(frames.size() - 1);
    if (averageDifference < 0.045f)
    {
        std::cerr << label << " frames are too similar: average diff "
                  << averageDifference << '\n';
        return false;
    }

    return true;
}
}

int main()
{
    ControlPoints seed {};
    for (size_t index = 0; index < seed.size(); ++index)
        seed[index] = static_cast<float>((index * 3) % seed.size()) / static_cast<float>(seed.size() - 1);

    const auto sweep = Synth::WavetableFrameRecipes::currentSweep(seed);
    const auto house = Synth::WavetableFrameRecipes::classicHouseStack();
    const auto rave = Synth::WavetableFrameRecipes::raveSweep();

    if (! validateFrameSet("Current sweep", sweep)
        || ! validateFrameSet("Classic house", house)
        || ! validateFrameSet("Rave sweep", rave))
    {
        return 1;
    }

    if (Synth::WavetableFrameRecipes::meanAbsoluteDifference(sweep[0], sweep[7]) < 0.08f)
    {
        std::cerr << "Current sweep did not create enough movement from first to last frame\n";
        return 1;
    }

    std::cout << "Wavetable frame recipe audit passed for current, classic house, and rave frame sets.\n";
    return 0;
}
