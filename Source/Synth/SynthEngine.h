#pragma once

#include "../Parameters.h"
#include "Voice.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <optional>

namespace Synth
{
class SynthEngine
{
public:
    explicit SynthEngine(Parameters::APVTS& parameters);

    void prepare(double sampleRate, int maximumBlockSize);
    void setSequencerLock(int destinationIndex, float amount);
    void render(juce::AudioBuffer<float>& buffer,
                juce::MidiBuffer& midi,
                double bpm,
                std::optional<double> ppqPosition);
    void allNotesOff();

private:
    juce::Synthesiser synthesiser;
    std::atomic<float>* monoMode = nullptr;
    juce::MidiBuffer monoMidiScratch;

    void enforceMonoIfNeeded(const juce::MidiBuffer& midi);
};
}
