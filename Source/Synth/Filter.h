#pragma once

#include <juce_dsp/juce_dsp.h>

namespace Synth
{
class Filter
{
public:
    void prepare(double sampleRate, int maximumBlockSize);
    void reset();
    void setCutoffAndResonance(float cutoffHz, float resonance);
    float process(float sample);

private:
    juce::dsp::StateVariableTPTFilter<float> filter;
};
}

