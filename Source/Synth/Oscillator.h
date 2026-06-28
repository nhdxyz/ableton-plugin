#pragma once

#include <juce_dsp/juce_dsp.h>

namespace Synth
{
enum class Waveform
{
    sine = 0,
    saw,
    square,
    triangle,
    wavetable
};

class Oscillator
{
public:
    void prepare(double newSampleRate);
    void reset();

    void setFrequency(float newFrequencyHz);
    void setWaveform(Waveform newWaveform);
    void setWarp(float newWarpAmount);
    void setWavetablePosition(float newPosition);

    float process();

private:
    double sampleRate = 44100.0;
    float frequencyHz = 440.0f;
    float phase = 0.0f;
    float phaseDelta = 0.0f;
    float triangleState = -1.0f;
    float warpAmount = 0.0f;
    float wavetablePosition = 0.0f;
    Waveform waveform = Waveform::saw;

    void updatePhaseDelta();
};
}
