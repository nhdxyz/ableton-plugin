#pragma once

#include "../Parameters.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <array>

namespace Sampler
{
struct SampleData
{
    juce::AudioBuffer<float> buffer;
    double sourceSampleRate = 44100.0;
    juce::String fileName;
};

struct SampleRegion
{
    int startSample = 0;
    int endSample = 0;
    bool reverse = false;
    float gain = 1.0f;
    float transposeSemitones = 0.0f;
};

class SamplePlayer
{
public:
    explicit SamplePlayer(Parameters::APVTS& parameters);

    void prepare(double sampleRate);
    void clear();

    bool loadFile(const juce::File& file);
    bool hasSample() const;
    juce::String getLoadedFileName() const;
    SampleRegion getRegion() const;
    void setRegion(SampleRegion newRegion);
    bool triggerAudition(int midiNoteNumber, float velocity, double bpm);
    void render(juce::AudioBuffer<float>& outputBuffer, const juce::MidiBuffer& midi, double bpm);

private:
    struct Voice
    {
        bool active = false;
        double position = 0.0;
        double increment = 1.0;
        int startSample = 0;
        int endSample = 0;
        int midiNoteNumber = -1;
        float velocity = 0.0f;
        bool reverse = false;
        bool gated = false;
        double sourceRatio = 1.0;
        float baseTransposeSemitones = 0.0f;
        float pitchRampSemitones = 0.0f;
        bool stutterEnabled = false;
        int stutterRepeatsRemaining = 0;
        int fadeInSamplesRemaining = 0;
        double stutterIntervalSamples = 1.0;
        double samplesUntilStutter = 0.0;
    };

    Parameters::APVTS& parameters;
    juce::AudioFormatManager formatManager;
    std::shared_ptr<SampleData> sampleData;
    mutable juce::SpinLock sampleLock;
    std::array<Voice, 8> voices {};
    SampleRegion region;
    double playbackSampleRate = 44100.0;

    std::atomic<float>* sampleEnabled = nullptr;
    std::atomic<float>* sampleStart = nullptr;
    std::atomic<float>* sampleEnd = nullptr;
    std::atomic<float>* sampleReverse = nullptr;
    std::atomic<float>* samplePlaybackMode = nullptr;
    std::atomic<float>* sampleTranspose = nullptr;
    std::atomic<float>* samplePitchRamp = nullptr;
    std::atomic<float>* sampleGain = nullptr;
    std::atomic<float>* sampleMix = nullptr;
    std::atomic<float>* sampleStutterEnabled = nullptr;
    std::atomic<float>* sampleStutterRate = nullptr;
    std::atomic<float>* sampleStutterRepeats = nullptr;

    void startVoice(const SampleData& data, int midiNoteNumber, float velocity, double bpm, bool forceOneShot);
    void stopVoicesForNote(int midiNoteNumber);
    void renderActiveVoices(const SampleData& data, juce::AudioBuffer<float>& outputBuffer, int startSampleInBlock, int numSamples);
    void renderVoice(Voice& voice, const SampleData& data, juce::AudioBuffer<float>& outputBuffer, int startSampleInBlock, int numSamples);
    SampleRegion currentRegionFor(const SampleData& data) const;
    double incrementForVoice(const Voice& voice) const;
    double rampProgressForVoice(const Voice& voice) const;
    float readParameter(std::atomic<float>* parameter, float fallback) const;
};
}
