#pragma once

#include <juce_core/juce_core.h>

namespace Synth
{
class Distortion
{
public:
    float process(float sample, float amount) const;
};
}

