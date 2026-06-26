#pragma once

#include <juce_dsp/juce_dsp.h>

namespace Synth
{
class Filter
{
public:
    enum class Mode
    {
        lowpass = 0,
        bandpass,
        highpass
    };

    void prepare(double sampleRate, int maximumBlockSize);
    void reset();
    void setMode(Mode mode);
    void setCutoffAndResonance(float cutoffHz, float resonance);
    float process(float sample);

private:
    juce::dsp::StateVariableTPTFilter<float> filter;
};
}
