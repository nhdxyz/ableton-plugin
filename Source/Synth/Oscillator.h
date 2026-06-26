#pragma once

#include <juce_dsp/juce_dsp.h>

namespace Synth
{
enum class Waveform
{
    sine = 0,
    saw,
    square,
    triangle
};

class Oscillator
{
public:
    void prepare(double newSampleRate);
    void reset();

    void setFrequency(float newFrequencyHz);
    void setWaveform(Waveform newWaveform);

    float process();

private:
    double sampleRate = 44100.0;
    float frequencyHz = 440.0f;
    float phase = 0.0f;
    float phaseDelta = 0.0f;
    Waveform waveform = Waveform::saw;

    void updatePhaseDelta();
};
}

