#include "Filter.h"

namespace Synth
{
void Filter::prepare(double sampleRate, int maximumBlockSize)
{
    juce::dsp::ProcessSpec spec {
        sampleRate,
        static_cast<juce::uint32>(juce::jmax(1, maximumBlockSize)),
        1
    };

    filter.prepare(spec);
    setMode(Mode::lowpass);
    reset();
}

void Filter::reset()
{
    filter.reset();
}

void Filter::setMode(Mode mode)
{
    switch (mode)
    {
        case Mode::lowpass:
            filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            break;
        case Mode::bandpass:
            filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            break;
        case Mode::highpass:
            filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
            break;
    }
}

void Filter::setCutoffAndResonance(float cutoffHz, float resonance)
{
    filter.setCutoffFrequency(juce::jlimit(20.0f, 20000.0f, cutoffHz));
    filter.setResonance(juce::jlimit(0.1f, 1.4f, resonance));
}

float Filter::process(float sample)
{
    return filter.processSample(0, sample);
}
}
