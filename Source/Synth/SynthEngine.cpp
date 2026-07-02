#include "SynthEngine.h"

namespace Synth
{
SynthEngine::SynthEngine(Parameters::APVTS& parameters)
{
    monoMode = parameters.getRawParameterValue(Parameters::ID::monoMode);
    synthesiser.setNoteStealingEnabled(true);
    synthesiser.addSound(new Sound());

    for (auto i = 0; i < 8; ++i)
        synthesiser.addVoice(new Voice(parameters));
}

void SynthEngine::prepare(double sampleRate, int maximumBlockSize)
{
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);
    monoMidiScratch.clear();
    monoMidiScratch.ensureSize(static_cast<size_t>(juce::jmax(4096, maximumBlockSize * 4)));

    for (auto voiceIndex = 0; voiceIndex < synthesiser.getNumVoices(); ++voiceIndex)
        if (auto* voice = dynamic_cast<Voice*>(synthesiser.getVoice(voiceIndex)))
            voice->prepare(sampleRate, maximumBlockSize);
}

void SynthEngine::setSequencerLock(int destinationIndex, float amount)
{
    for (auto voiceIndex = 0; voiceIndex < synthesiser.getNumVoices(); ++voiceIndex)
        if (auto* voice = dynamic_cast<Voice*>(synthesiser.getVoice(voiceIndex)))
            voice->setSequencerLock(destinationIndex, amount);
}

void SynthEngine::render(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, double bpm)
{
    auto estimatedActiveVoiceCount = 0;
    for (auto voiceIndex = 0; voiceIndex < synthesiser.getNumVoices(); ++voiceIndex)
    {
        if (auto* voice = dynamic_cast<Voice*>(synthesiser.getVoice(voiceIndex)))
        {
            if (voice->isVoiceActive())
                ++estimatedActiveVoiceCount;

            voice->setHostBpm(bpm);
        }
    }

    for (const auto metadata : midi)
        if (metadata.getMessage().isNoteOn())
            ++estimatedActiveVoiceCount;

    estimatedActiveVoiceCount = juce::jlimit(1, synthesiser.getNumVoices(), estimatedActiveVoiceCount);
    for (auto voiceIndex = 0; voiceIndex < synthesiser.getNumVoices(); ++voiceIndex)
        if (auto* voice = dynamic_cast<Voice*>(synthesiser.getVoice(voiceIndex)))
            voice->setActiveVoiceLoad(estimatedActiveVoiceCount);

    if (monoMode != nullptr && monoMode->load() > 0.5f)
    {
        enforceMonoIfNeeded(midi);
        synthesiser.renderNextBlock(buffer, monoMidiScratch, 0, buffer.getNumSamples());
        return;
    }

    synthesiser.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
}

void SynthEngine::allNotesOff()
{
    synthesiser.allNotesOff(0, false);
}

void SynthEngine::enforceMonoIfNeeded(const juce::MidiBuffer& midi)
{
    monoMidiScratch.clear();

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        const auto samplePosition = metadata.samplePosition;

        if (message.isNoteOn())
            monoMidiScratch.addEvent(juce::MidiMessage::allNotesOff(message.getChannel()), samplePosition);

        monoMidiScratch.addEvent(message, samplePosition);
    }
}
}
