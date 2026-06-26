#include "Distortion.h"

#include <cmath>

namespace Synth
{
float Distortion::process(float sample, float amount) const
{
    const auto drive = juce::jmap(juce::jlimit(0.0f, 1.0f, amount), 1.0f, 18.0f);
    const auto makeup = 1.0f / std::sqrt(drive);
    return std::tanh(sample * drive) * makeup;
}
}
