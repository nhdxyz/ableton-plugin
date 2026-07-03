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

bool framesAreSafe(const ControlFrameSet& frames)
{
    for (const auto& frame : frames)
        for (const auto value : frame)
            if (! std::isfinite(value) || value < 0.0f || value > 1.0f)
                return false;

    return true;
}

float adjacentDifference(const ControlFrameSet& frames)
{
    auto sum = 0.0f;
    for (size_t index = 1; index < frames.size(); ++index)
        sum += Synth::WavetableFrameRecipes::meanAbsoluteDifference(frames[index - 1], frames[index]);

    return sum / static_cast<float>(frames.size() - 1);
}

float averageFrameRange(const ControlFrameSet& frames)
{
    auto sum = 0.0f;
    for (const auto& frame : frames)
    {
        auto minValue = frame.front();
        auto maxValue = frame.front();
        for (const auto value : frame)
        {
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
        }

        sum += maxValue - minValue;
    }

    return sum / static_cast<float>(frames.size());
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
    const auto acid = Synth::WavetableFrameRecipes::acidStack();
    const auto rubber = Synth::WavetableFrameRecipes::rubberBassStack();
    const auto reese = Synth::WavetableFrameRecipes::reeseStack();

    if (! validateFrameSet("Current sweep", sweep)
        || ! validateFrameSet("Classic house", house)
        || ! validateFrameSet("Rave sweep", rave)
        || ! validateFrameSet("Acid stack", acid)
        || ! validateFrameSet("Rubber bass stack", rubber)
        || ! validateFrameSet("Reese stack", reese))
    {
        return 1;
    }

    if (Synth::WavetableFrameRecipes::meanAbsoluteDifference(sweep[0], sweep[7]) < 0.08f)
    {
        std::cerr << "Current sweep did not create enough movement from first to last frame\n";
        return 1;
    }

    if (Synth::WavetableFrameRecipes::meanAbsoluteDifference(acid[4], rubber[4]) < 0.06f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(acid[4], reese[4]) < 0.06f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(rubber[4], reese[4]) < 0.06f)
    {
        std::cerr << "Genre wavetable stacks are not distinct enough from each other\n";
        return 1;
    }

    const auto reversed = Synth::WavetableFrameRecipes::reverseFrameOrder(rave);
    const auto rotatedLeft = Synth::WavetableFrameRecipes::rotateFrameOrder(rave, 1);
    const auto rotatedRight = Synth::WavetableFrameRecipes::rotateFrameOrder(rave, -1);
    const auto smoothed = Synth::WavetableFrameRecipes::smoothFrameMotion(rave);
    const auto emphasised = Synth::WavetableFrameRecipes::emphasiseFrameMotion(rave);
    const auto blended = Synth::WavetableFrameRecipes::blendFrameStacks(house, rave, 0.5f);
    const auto morphed = Synth::WavetableFrameRecipes::morphBetweenFrameStacks(house, rave);
    const auto spliced = Synth::WavetableFrameRecipes::spliceFrameStacks(house, rave);
    const auto duplicated = Synth::WavetableFrameRecipes::duplicateFrameSlot(rave, 2);
    const auto duplicateAtEnd = Synth::WavetableFrameRecipes::duplicateFrameSlot(rave, 7);
    const auto deleted = Synth::WavetableFrameRecipes::deleteFrameSlot(rave, 2);
    const auto deletedAtEnd = Synth::WavetableFrameRecipes::deleteFrameSlot(rave, 7);
    const auto movedLeft = Synth::WavetableFrameRecipes::moveFrameSlot(rave, 3, 2);
    const auto movedRight = Synth::WavetableFrameRecipes::moveFrameSlot(rave, 2, 4);

    if (! framesAreSafe(reversed)
        || ! framesAreSafe(rotatedLeft)
        || ! framesAreSafe(rotatedRight)
        || ! framesAreSafe(smoothed)
        || ! framesAreSafe(emphasised)
        || ! framesAreSafe(duplicated)
        || ! framesAreSafe(duplicateAtEnd)
        || ! framesAreSafe(deleted)
        || ! framesAreSafe(deletedAtEnd)
        || ! framesAreSafe(movedLeft)
        || ! framesAreSafe(movedRight)
        || ! validateFrameSet("Blended stack", blended)
        || ! validateFrameSet("Morphed stack", morphed)
        || ! validateFrameSet("Spliced stack", spliced))
    {
        std::cerr << "Frame-stack transform produced unsafe values\n";
        return 1;
    }

    if (Synth::WavetableFrameRecipes::meanAbsoluteDifference(reversed.front(), rave.back()) > 0.0001f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(rotatedLeft.front(), rave[1]) > 0.0001f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(rotatedRight.front(), rave.back()) > 0.0001f)
    {
        std::cerr << "Frame-stack reverse/rotate transform did not preserve expected frame order\n";
        return 1;
    }

    if (Synth::WavetableFrameRecipes::meanAbsoluteDifference(duplicated[0], rave[0]) > 0.0001f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(duplicated[2], rave[2]) > 0.0001f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(duplicated[3], rave[2]) > 0.0001f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(duplicated[4], rave[3]) > 0.0001f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(duplicated[7], rave[6]) > 0.0001f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(duplicateAtEnd[7], rave[7]) > 0.0001f)
    {
        std::cerr << "Frame-slot duplicate transform did not insert the active frame correctly\n";
        return 1;
    }

    if (Synth::WavetableFrameRecipes::meanAbsoluteDifference(deleted[0], rave[0]) > 0.0001f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(deleted[2], rave[3]) > 0.0001f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(deleted[6], rave[7]) > 0.0001f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(deleted[7], rave[7]) > 0.0001f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(deletedAtEnd[7], rave[6]) > 0.0001f)
    {
        std::cerr << "Frame-slot delete transform did not close the frame gap correctly\n";
        return 1;
    }

    if (Synth::WavetableFrameRecipes::meanAbsoluteDifference(movedLeft[2], rave[3]) > 0.0001f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(movedLeft[3], rave[2]) > 0.0001f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(movedRight[2], rave[3]) > 0.0001f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(movedRight[3], rave[4]) > 0.0001f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(movedRight[4], rave[2]) > 0.0001f)
    {
        std::cerr << "Frame-slot move transform did not preserve frame order correctly\n";
        return 1;
    }

    if (Synth::WavetableFrameRecipes::meanAbsoluteDifference(morphed.front(), house.front()) > 0.0001f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(morphed.back(), rave.back()) > 0.0001f)
    {
        std::cerr << "Frame-stack morph did not preserve first/last source frames\n";
        return 1;
    }

    if (Synth::WavetableFrameRecipes::meanAbsoluteDifference(blended[3], house[3]) < 0.025f
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(blended[3], rave[3]) < 0.025f)
    {
        std::cerr << "Frame-stack blend landed too close to only one source stack\n";
        return 1;
    }

    if (Synth::WavetableFrameRecipes::meanAbsoluteDifference(spliced[0], house[0])
            >= Synth::WavetableFrameRecipes::meanAbsoluteDifference(spliced[0], rave[0])
        || Synth::WavetableFrameRecipes::meanAbsoluteDifference(spliced[1], rave[1])
            >= Synth::WavetableFrameRecipes::meanAbsoluteDifference(spliced[1], house[1]))
    {
        std::cerr << "Frame-stack splice did not alternate source-stack dominance\n";
        return 1;
    }

    if (adjacentDifference(smoothed) >= adjacentDifference(rave))
    {
        std::cerr << "Frame-stack smooth transform did not reduce adjacent frame motion\n";
        return 1;
    }

    if (averageFrameRange(emphasised) <= averageFrameRange(rave))
    {
        std::cerr << "Frame-stack emphasis transform did not increase average frame range\n";
        return 1;
    }

    std::cout << "Wavetable frame recipe audit passed for generated frame sets, genre stacks, stack transforms, frame-slot edits, and O1/O2 stack-combine tools.\n";
    return 0;
}
