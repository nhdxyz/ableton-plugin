#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>

namespace Synth
{
enum class Waveform
{
    sine = 0,
    saw,
    square,
    triangle,
    wavetable,
    organ,
    housePiano,
    custom
};

class Oscillator
{
public:
    static constexpr size_t customWavePointCount = 16;
    static constexpr size_t customWaveFrameCount = 8;
    using CustomWavePoints = std::array<float, customWavePointCount>;
    using CustomWaveFrames = std::array<CustomWavePoints, customWaveFrameCount>;

    void prepare(double newSampleRate);
    void reset();

    void setFrequency(float newFrequencyHz);
    void setWaveform(Waveform newWaveform);
    void setWarp(float newWarpAmount);
    void setWavetablePosition(float newPosition);
    void setCustomWaveform(const CustomWavePoints& points);
    void setCustomWavetableFrames(const CustomWaveFrames& frames);

    float process();

private:
    double sampleRate = 44100.0;
    float frequencyHz = 440.0f;
    float phase = 0.0f;
    float phaseDelta = 0.0f;
    float triangleState = -1.0f;
    float warpAmount = 0.0f;
    float wavetablePosition = 0.0f;
    bool customFramesInitialised = false;
    CustomWavePoints customWavePoints {
        0.5f,
        0.691342f,
        0.853553f,
        0.961940f,
        1.0f,
        0.961940f,
        0.853553f,
        0.691342f,
        0.5f,
        0.308658f,
        0.146447f,
        0.038060f,
        0.0f,
        0.038060f,
        0.146447f,
        0.308658f
    };
    CustomWaveFrames customWaveFrames {};
    Waveform waveform = Waveform::saw;

    void updatePhaseDelta();
};
}
