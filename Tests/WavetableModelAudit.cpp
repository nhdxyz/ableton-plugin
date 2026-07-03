#include "../Source/Synth/WavetableModel.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>

namespace
{
using Synth::WavetableModel;

bool nearlyEqual(float first, float second, float tolerance)
{
    return std::abs(first - second) <= tolerance;
}

WavetableModel::Frame constantFrame(float value)
{
    auto frame = WavetableModel::Frame {};
    frame.fill(value);
    return frame;
}
}

int main()
{
    WavetableModel model;

    if (model.getFrameCount() != 1)
    {
        std::cerr << "Default wavetable did not start with one frame\n";
        return 1;
    }

    const auto defaultQuarter = model.sample(0.25f, 0.0f);
    if (! std::isfinite(defaultQuarter) || defaultQuarter < 0.85f)
    {
        std::cerr << "Default wavetable is not an audible sine-like frame: "
                  << defaultQuarter << '\n';
        return 1;
    }

    std::array<WavetableModel::ControlPoints, 2> controlFrames {};
    controlFrames[0].fill(0.0f);
    controlFrames[1].fill(1.0f);
    model = WavetableModel::fromNormalisedControlFrames(controlFrames);

    if (model.getFrameCount() != 2)
    {
        std::cerr << "Control-frame wavetable did not preserve frame count\n";
        return 1;
    }

    if (! nearlyEqual(model.sample(0.37f, 0.0f), -1.0f, 0.001f)
        || ! nearlyEqual(model.sample(0.37f, 1.0f), 1.0f, 0.001f)
        || ! nearlyEqual(model.sample(0.37f, 0.5f), 0.0f, 0.001f))
    {
        std::cerr << "Frame-position interpolation did not land at expected values: "
                  << model.sample(0.37f, 0.0f) << ' '
                  << model.sample(0.37f, 0.5f) << ' '
                  << model.sample(0.37f, 1.0f) << '\n';
        return 1;
    }

    if (! model.insertFrame(1, WavetableModel::sineFrame()) || model.getFrameCount() != 3)
    {
        std::cerr << "Wavetable did not insert a middle frame\n";
        return 1;
    }

    if (model.sample(0.25f, 0.5f) < 0.85f)
    {
        std::cerr << "Inserted frame was not sampled at the middle table position\n";
        return 1;
    }

    if (! model.removeFrame(1) || model.getFrameCount() != 2)
    {
        std::cerr << "Wavetable did not remove the inserted frame\n";
        return 1;
    }

    auto unsafeFrame = constantFrame(2.5f);
    unsafeFrame[17] = -2.5f;
    unsafeFrame[23] = std::numeric_limits<float>::quiet_NaN();
    if (! model.setFrame(0, unsafeFrame))
    {
        std::cerr << "Wavetable rejected a valid frame index\n";
        return 1;
    }

    for (const auto phase : { 0.0f, 0.066f, 0.09f, 0.5f, 0.99f })
    {
        const auto sample = model.sample(phase, 0.0f);
        if (! std::isfinite(sample) || sample < -1.0f || sample > 1.0f)
        {
            std::cerr << "Wavetable sample was not clamped/finite at phase "
                      << phase << ": " << sample << '\n';
            return 1;
        }
    }

    std::cout << "Wavetable model audit passed for default frame, interpolation, insert/remove, and sample safety.\n";
    return 0;
}
