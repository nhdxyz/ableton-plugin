#include "Envelope.h"

namespace Synth
{
void Envelope::setSampleRate(double sampleRate)
{
    adsr.setSampleRate(sampleRate);
}

void Envelope::setParameters(float attack, float decay, float sustain, float release)
{
    parameters.attack = juce::jmax(0.001f, attack);
    parameters.decay = juce::jmax(0.001f, decay);
    parameters.sustain = juce::jlimit(0.0f, 1.0f, sustain);
    parameters.release = juce::jmax(0.001f, release);
    adsr.setParameters(parameters);
}

void Envelope::noteOn()
{
    adsr.noteOn();
}

void Envelope::noteOff()
{
    adsr.noteOff();
}

void Envelope::reset()
{
    adsr.reset();
}

bool Envelope::isActive() const
{
    return adsr.isActive();
}

float Envelope::process()
{
    return adsr.getNextSample();
}
}

