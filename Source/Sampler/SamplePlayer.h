#pragma once

#include "../Modulation/ModulationRouting.h"
#include "../Parameters.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <array>
#include <optional>
#include <vector>

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
        float pan = 0.0f;
        float probability = 1.0f;
        float fade = 0.0f;
    };

struct SamplePeakOverview
{
    std::vector<float> minimums;
    std::vector<float> maximums;
    juce::String fileName;
    int totalSamples = 0;
    double sourceSampleRate = 44100.0;

    bool isValid() const noexcept
    {
        return totalSamples > 0 && ! minimums.empty() && minimums.size() == maximums.size();
    }
};

struct SampleContentRange
{
    float start = 0.0f;
    float end = 1.0f;
};

struct SliceMarkerRegion
{
    float start = 0.0f;
    float end = 1.0f;
    bool transient = false;
};

struct SliceDetectionResult
{
    std::array<SliceMarkerRegion, 8> regions {};
    int transientCount = 0;
    bool valid = false;
};

class SamplePlayer
{
public:
    explicit SamplePlayer(Parameters::APVTS& parameters);

    void prepare(double sampleRate);
    void clear();
    void stopAllVoices();

    bool loadFile(const juce::File& file);
    bool loadBuffer(const juce::AudioBuffer<float>& buffer, double sourceSampleRate, const juce::String& name);
    bool hasSample() const;
    juce::String getLoadedFileName() const;
    SamplePeakOverview createPeakOverview(int pointCount) const;
    std::optional<SampleContentRange> findContentRange(float threshold, double paddingMs) const;
    SliceDetectionResult detectTransientSliceRegions() const;
    std::vector<float> createMonoSnapshot(float normalisedStart, float normalisedEnd, size_t maxSamples) const;
    SampleRegion getRegion() const;
    void setRegion(SampleRegion newRegion);
    bool triggerAudition(int midiNoteNumber, float velocity, double bpm);
    bool triggerSliceAudition(int sliceIndex, int midiNoteNumber, float velocity, double bpm);
    void render(juce::AudioBuffer<float>& outputBuffer,
                const juce::MidiBuffer& midi,
                double bpm,
                std::optional<double> ppqPosition);

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
        int fadeInTotalSamples = 0;
        double stutterIntervalSamples = 1.0;
        double samplesUntilStutter = 0.0;
        double grainIntervalSamples = 1.0;
        double samplesUntilGrain = 0.0;
        float gain = 1.0f;
        float pan = 0.0f;
        float grainSpray = 0.0f;
        float spectralFreeze = 0.0f;
        int sliceIndex = -1;
        int fadeOutTotalSamples = 0;
        int engineMode = 0;
        int grainSizeSamples = 0;
        int grainResetsRemaining = 0;
    };

    Parameters::APVTS& parameters;
    juce::AudioFormatManager formatManager;
    std::shared_ptr<SampleData> sampleData;
    mutable juce::SpinLock sampleLock;
    std::array<Voice, 8> voices {};
    SampleRegion region;
    double playbackSampleRate = 44100.0;
    float sampleModLfoPhase = 0.0f;
    float sampleModLfoStepValue = 0.0f;
    float sampleModSmoothRandomStartValue = 0.0f;
    float sampleModSmoothRandomValue = 0.0f;
    float sampleModChaosValue = 0.0f;
    float sampleModLfo2Phase = 0.0f;
    float sampleModLfo2StepValue = 0.0f;
    float sampleModStepLfoPhase = 0.0f;
    float sampleModStepLfoSmoothedValue = 0.0f;
    std::array<Modulation::RouteRuntimeState, 8> sampleModRouteStates {};
    juce::ADSR sampleModEnvelope;
    juce::ADSR::Parameters sampleModEnvelopeParameters;
    float sampleModEnvelopeValue = 0.0f;
    float sampleModVelocity = 0.0f;
    float sampleModWheel = 0.0f;
    float sampleModAftertouch = 0.0f;
    float sampleModPitchBend = 0.0f;
    float sampleModNote = 0.0f;
    int sampleModActiveNotes = 0;

    struct SampleModulationOffsets
    {
        float start = 0.0f;
        float mix = 0.0f;
        float pitch = 0.0f;
        float ramp = 0.0f;
        float stutter = 0.0f;
    };

    struct SlicePlaybackSettings
    {
        bool reverse = false;
        float gainDb = -6.0f;
        float transposeSemitones = 0.0f;
        float pan = 0.0f;
        float probability = 1.0f;
        bool stutter = false;
        bool choke = false;
        int stutterRepeats = 3;
        float nudgePercent = 0.0f;
        float fade = 0.0f;
    };

    SampleModulationOffsets sampleModulation;
    juce::Random sampleModulationRandom;
    juce::Random sliceTriggerRandom;

    std::atomic<float>* sampleEnabled = nullptr;
    std::atomic<float>* sampleStart = nullptr;
    std::atomic<float>* sampleEnd = nullptr;
    std::atomic<float>* sampleReverse = nullptr;
    std::atomic<float>* samplePlaybackMode = nullptr;
    std::atomic<float>* sampleEngineMode = nullptr;
    std::atomic<float>* sampleGrainSize = nullptr;
    std::atomic<float>* sampleGrainSpray = nullptr;
    std::atomic<float>* sampleSpectralFreeze = nullptr;
    std::atomic<float>* sampleTranspose = nullptr;
    std::atomic<float>* samplePitchRamp = nullptr;
    std::atomic<float>* sampleGain = nullptr;
    std::atomic<float>* sampleMix = nullptr;
    std::atomic<float>* sampleStutterEnabled = nullptr;
    std::atomic<float>* sampleStutterRate = nullptr;
    std::atomic<float>* sampleStutterRepeats = nullptr;
    std::atomic<float>* sampleSliceStyle = nullptr;
    std::array<std::atomic<float>*, 8> sampleSliceCustom {};
    std::array<std::atomic<float>*, 8> sampleSliceStart {};
    std::array<std::atomic<float>*, 8> sampleSliceEnd {};
    std::array<std::atomic<float>*, 8> sampleSliceReverse {};
    std::array<std::atomic<float>*, 8> sampleSliceTranspose {};
    std::array<std::atomic<float>*, 8> sampleSliceGain {};
    std::array<std::atomic<float>*, 8> sampleSlicePan {};
    std::array<std::atomic<float>*, 8> sampleSliceProbability {};
    std::array<std::atomic<float>*, 8> sampleSliceStutter {};
    std::array<std::atomic<float>*, 8> sampleSliceChoke {};
    std::array<std::atomic<float>*, 8> sampleSliceStutterRepeats {};
    std::array<std::atomic<float>*, 8> sampleSliceNudge {};
    std::array<std::atomic<float>*, 8> sampleSliceFade {};
    std::array<std::atomic<float>*, 8> modMatrixSources {};
    std::array<std::atomic<float>*, 8> modMatrixDestinations {};
    std::array<std::atomic<float>*, 8> modMatrixAmounts {};
    std::array<std::atomic<float>*, 8> modMatrixEnabled {};
    std::array<std::atomic<float>*, 8> modMatrixPolarities {};
    std::array<std::atomic<float>*, 8> modMatrixCurves {};
    std::array<std::atomic<float>*, 8> modMatrixRangeMins {};
    std::array<std::atomic<float>*, 8> modMatrixRangeMaxes {};
    std::array<std::atomic<float>*, 8> modMatrixSlews {};
    std::atomic<float>* macroTone = nullptr;
    std::atomic<float>* macroDirt = nullptr;
    std::atomic<float>* macroMotion = nullptr;
    std::atomic<float>* macroSpace = nullptr;
    std::atomic<float>* macroWeight = nullptr;
    std::atomic<float>* macroBounce = nullptr;
    std::atomic<float>* macroWarp = nullptr;
    std::atomic<float>* macroThrow = nullptr;
    std::atomic<float>* lfo1Rate = nullptr;
    std::atomic<float>* lfo1Sync = nullptr;
    std::atomic<float>* lfo1SyncRate = nullptr;
    std::atomic<float>* lfo1Shape = nullptr;
    std::atomic<float>* lfo1Depth = nullptr;
    std::atomic<float>* lfo1Phase = nullptr;
    std::atomic<float>* lfo1Retrigger = nullptr;
    std::array<std::atomic<float>*, 8> lfo1CurvePoints {};
    std::atomic<float>* lfo2Rate = nullptr;
    std::atomic<float>* lfo2Sync = nullptr;
    std::atomic<float>* lfo2SyncRate = nullptr;
    std::atomic<float>* lfo2Shape = nullptr;
    std::atomic<float>* lfo2Depth = nullptr;
    std::atomic<float>* lfo2Phase = nullptr;
    std::atomic<float>* lfo2Retrigger = nullptr;
    std::atomic<float>* stepLfoSync = nullptr;
    std::atomic<float>* stepLfoSyncRate = nullptr;
    std::atomic<float>* stepLfoRate = nullptr;
    std::atomic<float>* stepLfoDepth = nullptr;
    std::atomic<float>* stepLfoSlew = nullptr;
    std::array<std::atomic<float>*, 8> stepLfoValues {};
    std::atomic<float>* modEnv1Attack = nullptr;
    std::atomic<float>* modEnv1Decay = nullptr;
    std::atomic<float>* modEnv1Sustain = nullptr;
    std::atomic<float>* modEnv1Release = nullptr;
    std::atomic<float>* modEnv1Depth = nullptr;

    void handleSampleModulationMidi(const juce::MidiBuffer& midi);
    void triggerSampleModulationNoteOn(float velocity);
    void releaseSampleModulationNote();
    float processSampleModulationEnvelope(int numSamples);
    void updateSampleModulation(int numSamples, double bpm, std::optional<double> ppqPosition);
    float processSampleModulationLfo(int numSamples, double bpm, std::optional<double> ppqPosition);
    float processSampleModulationLfo2(int numSamples, double bpm, std::optional<double> ppqPosition);
    float processSampleStepLfo(int numSamples, double bpm, std::optional<double> ppqPosition);
    float evaluateSampleModulationSource(int sourceIndex, float lfoValue, float lfo2Value, float stepLfoValue, float modEnvelopeValue) const;
    int sliceIndexForMidiNote(int midiNoteNumber) const;
    void startVoice(const SampleData& data, int midiNoteNumber, float velocity, double bpm, bool forceOneShot, int forcedSliceIndex = -1, bool ignoreSliceProbability = false);
    void stopVoicesForNote(int midiNoteNumber);
    void renderActiveVoices(const SampleData& data, juce::AudioBuffer<float>& outputBuffer, int startSampleInBlock, int numSamples);
    void renderVoice(Voice& voice, const SampleData& data, juce::AudioBuffer<float>& outputBuffer, int startSampleInBlock, int numSamples);
    void resetVoiceToEngineGrain(Voice& voice);
    SampleRegion currentRegionFor(const SampleData& data, int sliceIndex) const;
    SlicePlaybackSettings slicePlaybackSettings(int sliceIndex) const;
    SlicePlaybackSettings defaultSlicePlaybackSettings(int sliceIndex) const;
    bool sliceHasCustomSettings(int sliceIndex) const;
    bool sliceChokeEnabled(int sliceIndex) const;
    bool sliceStutterEnabled(int sliceIndex) const;
    int sliceStutterRepeats(int sliceIndex) const;
    int fadeSamplesForSpan(int sourceSpan, float fadeAmount) const;
    double incrementForVoice(const Voice& voice) const;
    double rampProgressForVoice(const Voice& voice) const;
    float readParameter(std::atomic<float>* parameter, float fallback) const;
};
}
