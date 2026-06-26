#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace Synth
{
class Envelope
{
public:
    void setSampleRate(double sampleRate);
    void setParameters(float attack, float decay, float sustain, float release);
    void noteOn();
    void noteOff();
    void reset();
    bool isActive() const;
    float process();

private:
    juce::ADSR adsr;
    juce::ADSR::Parameters parameters;
};
}

