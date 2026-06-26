#pragma once

#include "../Parameters.h"
#include "Voice.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace Synth
{
class SynthEngine
{
public:
    explicit SynthEngine(Parameters::APVTS& parameters);

    void prepare(double sampleRate, int maximumBlockSize);
    void render(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

private:
    juce::Synthesiser synthesiser;
    std::atomic<float>* monoMode = nullptr;

    juce::MidiBuffer enforceMonoIfNeeded(const juce::MidiBuffer& midi);
};
}
