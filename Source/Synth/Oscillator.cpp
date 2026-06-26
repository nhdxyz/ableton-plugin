#include "Oscillator.h"

#include <cmath>

namespace Synth
{
void Oscillator::prepare(double newSampleRate)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    updatePhaseDelta();
}

void Oscillator::reset()
{
    phase = 0.0f;
}

void Oscillator::setFrequency(float newFrequencyHz)
{
    frequencyHz = juce::jlimit(8.0f, 24000.0f, newFrequencyHz);
    updatePhaseDelta();
}

void Oscillator::setWaveform(Waveform newWaveform)
{
    waveform = newWaveform;
}

float Oscillator::process()
{
    float sample = 0.0f;

    switch (waveform)
    {
        case Waveform::sine:
            sample = std::sin(juce::MathConstants<float>::twoPi * phase);
            break;

        case Waveform::saw:
            sample = (2.0f * phase) - 1.0f;
            break;

        case Waveform::square:
            sample = phase < 0.5f ? 1.0f : -1.0f;
            break;

        case Waveform::triangle:
            sample = 1.0f - (4.0f * std::abs(phase - 0.5f));
            break;
    }

    phase += phaseDelta;
    if (phase >= 1.0f)
        phase -= std::floor(phase);

    return sample;
}

void Oscillator::updatePhaseDelta()
{
    phaseDelta = static_cast<float>(frequencyHz / sampleRate);
}
}
