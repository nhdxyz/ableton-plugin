#include "SynthEngine.h"

namespace Synth
{
SynthEngine::SynthEngine(Parameters::APVTS& parameters)
{
    monoMode = parameters.getRawParameterValue(Parameters::ID::monoMode);
    synthesiser.addSound(new Sound());

    for (auto i = 0; i < 8; ++i)
        synthesiser.addVoice(new Voice(parameters));
}

void SynthEngine::prepare(double sampleRate, int maximumBlockSize)
{
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);

    for (auto voiceIndex = 0; voiceIndex < synthesiser.getNumVoices(); ++voiceIndex)
        if (auto* voice = dynamic_cast<Voice*>(synthesiser.getVoice(voiceIndex)))
            voice->prepare(sampleRate, maximumBlockSize);
}

void SynthEngine::render(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    if (monoMode != nullptr && monoMode->load() > 0.5f)
    {
        auto monoMidi = enforceMonoIfNeeded(midi);
        synthesiser.renderNextBlock(buffer, monoMidi, 0, buffer.getNumSamples());
        return;
    }

    synthesiser.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
}

juce::MidiBuffer SynthEngine::enforceMonoIfNeeded(const juce::MidiBuffer& midi)
{
    juce::MidiBuffer monoMidi;

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        const auto samplePosition = metadata.samplePosition;

        if (message.isNoteOn())
            monoMidi.addEvent(juce::MidiMessage::allNotesOff(message.getChannel()), samplePosition);

        monoMidi.addEvent(message, samplePosition);
    }

    return monoMidi;
}
}
