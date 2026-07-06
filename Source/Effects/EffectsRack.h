#pragma once

#include "../Modulation/ModulationRouting.h"
#include "../Parameters.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>
#include <optional>
#include <vector>

namespace Effects
{
class EffectsRack
{
public:
    explicit EffectsRack(Parameters::APVTS& parameters);

    void prepare(double sampleRate, int maximumBlockSize, int numChannels);
    void reset();
    void setSequencerLock(int destinationIndex, float amount) noexcept;
    void process(juce::AudioBuffer<float>& buffer,
                 const juce::MidiBuffer& midi,
                 float outputGainDb,
                 double bpm,
                 std::optional<double> ppqPosition);
    void getPumpMeterLevels(float& phase, float& gain, float& reduction, bool& active) const noexcept;
    void getGuardMeterLevels(float& drive, float& reduction, bool& active) const noexcept;
    void requestSendTailKill() noexcept;

private:
    static constexpr size_t fxModuleCount = 15;
    static constexpr int guardModuleIndex = 14;

    Parameters::APVTS& parameters;
    juce::dsp::Phaser<float> phaser;
    juce::dsp::Chorus<float> flanger;
    juce::dsp::Chorus<float> chorus;
    juce::Reverb reverb;
    juce::Reverb sendReverb;
    juce::AudioBuffer<float> delayBuffer;
    juce::AudioBuffer<float> sendDelayBuffer;
    juce::AudioBuffer<float> sendInputBuffer;
    juce::AudioBuffer<float> sendWorkBuffer;
    juce::AudioBuffer<float> combBuffer;
    std::vector<float> toneLowCutState;
    std::vector<float> toneTiltState;
    std::vector<float> eqLowState;
    std::vector<float> eqHighState;
    std::vector<float> distortionLowState;
    std::vector<float> combDampingState;
    std::vector<float> bitcrushHeldSample;
    std::vector<int> bitcrushHoldCounter;
    std::vector<float> widthLowState;
    std::vector<float> guardEnvelopeState;
    std::vector<float> guardCompressionGainState;
    std::vector<float> guardTransientFastState;
    std::vector<float> guardTransientSlowState;

    double currentSampleRate = 44100.0;
    double pumpPhase = 0.0;
    double tremoloPhase = 0.0;
    double ringPhase = 0.0;
    float pumpSmoothedGain = 1.0f;
    std::atomic<float> pumpMeterPhase { 0.0f };
    std::atomic<float> pumpMeterGain { 1.0f };
    std::atomic<float> pumpMeterReduction { 0.0f };
    std::atomic<bool> pumpMeterActive { false };
    std::atomic<float> guardMeterDrive { 0.0f };
    std::atomic<float> guardMeterReduction { 0.0f };
    std::atomic<bool> guardMeterActive { false };
    std::atomic<bool> sendTailKillRequested { false };
    float fxModLfoPhase = 0.0f;
    float fxModLfoStepValue = 0.0f;
    float fxModSmoothRandomStartValue = 0.0f;
    float fxModSmoothRandomValue = 0.0f;
    float fxModChaosValue = 0.0f;
    float fxModLfo2Phase = 0.0f;
    float fxModLfo2StepValue = 0.0f;
    float fxModStepLfoPhase = 0.0f;
    float fxModStepLfoSmoothedValue = 0.0f;
    std::array<Modulation::RouteRuntimeState, 8> fxModRouteStates {};
    juce::ADSR fxModEnvelope;
    juce::ADSR::Parameters fxModEnvelopeParameters;
    float fxModEnvelopeValue = 0.0f;
    float fxModVelocity = 0.0f;
    float fxModWheel = 0.0f;
    float fxModAftertouch = 0.0f;
    float fxModPitchBend = 0.0f;
    float fxModNote = 0.0f;
    int fxModActiveNotes = 0;
    int sequencerLockDestination = 0;
    float sequencerLockAmount = 0.0f;
    int delayWritePosition = 0;
    int sendDelayWritePosition = 0;
    int combWritePosition = 0;
    int preparedChannels = 2;

    struct FxModulationOffsets
    {
        float drive = 0.0f;
        float pumpDepth = 0.0f;
        float delayMix = 0.0f;
        float reverbMix = 0.0f;
        float width = 0.0f;
        float sendDelay = 0.0f;
        float sendReverb = 0.0f;
    };

    FxModulationOffsets fxModulation;
    juce::Random fxModulationRandom;

    std::atomic<float>* fxDistortionEnabled = nullptr;
    std::atomic<float>* fxDistortionAmount = nullptr;
    std::atomic<float>* fxDistortionBassSafe = nullptr;
    std::atomic<float>* fxBitcrushEnabled = nullptr;
    std::atomic<float>* fxBitcrushBits = nullptr;
    std::atomic<float>* fxBitcrushDownsample = nullptr;
    std::atomic<float>* fxBitcrushMix = nullptr;
    std::atomic<float>* fxPumpEnabled = nullptr;
    std::atomic<float>* fxPumpRate = nullptr;
    std::atomic<float>* fxPumpCurve = nullptr;
    std::array<std::atomic<float>*, 8> fxPumpCustomCurve {};
    std::atomic<float>* fxPumpDepth = nullptr;
    std::atomic<float>* fxPumpShape = nullptr;
    std::atomic<float>* fxPumpPhase = nullptr;
    std::atomic<float>* fxTremoloEnabled = nullptr;
    std::atomic<float>* fxTremoloRate = nullptr;
    std::atomic<float>* fxTremoloDepth = nullptr;
    std::atomic<float>* fxTremoloPan = nullptr;
    std::atomic<float>* fxTremoloShape = nullptr;
    std::atomic<float>* fxTremoloPhase = nullptr;
    std::atomic<float>* fxRingEnabled = nullptr;
    std::atomic<float>* fxRingFrequency = nullptr;
    std::atomic<float>* fxRingDepth = nullptr;
    std::atomic<float>* fxRingMix = nullptr;
    std::atomic<float>* fxRingBias = nullptr;
    std::atomic<float>* fxCombEnabled = nullptr;
    std::atomic<float>* fxCombFrequency = nullptr;
    std::atomic<float>* fxCombFeedback = nullptr;
    std::atomic<float>* fxCombDamping = nullptr;
    std::atomic<float>* fxCombMix = nullptr;
    std::atomic<float>* fxChorusEnabled = nullptr;
    std::atomic<float>* fxChorusRate = nullptr;
    std::atomic<float>* fxChorusDepth = nullptr;
    std::atomic<float>* fxChorusMix = nullptr;
    std::atomic<float>* fxDelayEnabled = nullptr;
    std::atomic<float>* fxDelaySync = nullptr;
    std::atomic<float>* fxDelayRate = nullptr;
    std::atomic<float>* fxDelayTime = nullptr;
    std::atomic<float>* fxDelayFeedback = nullptr;
    std::atomic<float>* fxDelayMix = nullptr;
    std::atomic<float>* fxSendDelay = nullptr;
    std::atomic<float>* fxReverbEnabled = nullptr;
    std::atomic<float>* fxReverbSize = nullptr;
    std::atomic<float>* fxReverbDamping = nullptr;
    std::atomic<float>* fxReverbMix = nullptr;
    std::atomic<float>* fxSendReverb = nullptr;
    std::atomic<float>* fxSendTailKill = nullptr;
    std::atomic<float>* fxWidthEnabled = nullptr;
    std::atomic<float>* fxWidthAmount = nullptr;
    std::atomic<float>* fxWidthMonoCutoff = nullptr;
    std::atomic<float>* fxToneEnabled = nullptr;
    std::atomic<float>* fxToneTilt = nullptr;
    std::atomic<float>* fxToneLowCut = nullptr;
    std::atomic<float>* fxEqEnabled = nullptr;
    std::atomic<float>* fxEqLowGain = nullptr;
    std::atomic<float>* fxEqMidGain = nullptr;
    std::atomic<float>* fxEqHighGain = nullptr;
    std::atomic<float>* fxEqTrim = nullptr;
    std::atomic<float>* fxPhaserEnabled = nullptr;
    std::atomic<float>* fxPhaserRate = nullptr;
    std::atomic<float>* fxPhaserDepth = nullptr;
    std::atomic<float>* fxPhaserMix = nullptr;
    std::atomic<float>* fxGuardEnabled = nullptr;
    std::atomic<float>* fxGuardPush = nullptr;
    std::atomic<float>* fxGuardCeiling = nullptr;
    std::atomic<float>* fxGuardGlue = nullptr;
    std::atomic<float>* fxGuardPunch = nullptr;
    std::atomic<float>* fxGuardClipMix = nullptr;
    std::atomic<float>* fxFlangerEnabled = nullptr;
    std::atomic<float>* fxFlangerRate = nullptr;
    std::atomic<float>* fxFlangerDepth = nullptr;
    std::atomic<float>* fxFlangerFeedback = nullptr;
    std::atomic<float>* fxFlangerMix = nullptr;
    std::array<std::atomic<float>*, fxModuleCount> fxOrder {};
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

    std::array<int, fxModuleCount> orderedModuleIndices() const;
    void handleFxModulationMidi(const juce::MidiBuffer& midi);
    void triggerFxModulationNoteOn(float velocity);
    void releaseFxModulationNote();
    float processFxModulationEnvelope(int numSamples);
    void updateFxModulation(int numSamples, double bpm, std::optional<double> ppqPosition);
    float processFxModulationLfo(int numSamples, double bpm, std::optional<double> ppqPosition);
    float processFxModulationLfo2(int numSamples, double bpm, std::optional<double> ppqPosition);
    float processFxStepLfo(int numSamples, double bpm, std::optional<double> ppqPosition);
    float evaluateFxModulationSource(int sourceIndex, float lfoValue, float lfo2Value, float stepLfoValue, float modEnvelopeValue) const;
    void processModule(int moduleIndex,
                       juce::AudioBuffer<float>& buffer,
                       double bpm,
                       std::optional<double> ppqPosition);
    void processTone(juce::AudioBuffer<float>& buffer);
    void processEq(juce::AudioBuffer<float>& buffer);
    void processDistortion(juce::AudioBuffer<float>& buffer);
    void processBitcrush(juce::AudioBuffer<float>& buffer);
    void processPump(juce::AudioBuffer<float>& buffer, double bpm, std::optional<double> ppqPosition);
    void processTremolo(juce::AudioBuffer<float>& buffer, double bpm, std::optional<double> ppqPosition);
    void processRingMod(juce::AudioBuffer<float>& buffer);
    void processComb(juce::AudioBuffer<float>& buffer);
    void processPhaser(juce::AudioBuffer<float>& buffer);
    void processFlanger(juce::AudioBuffer<float>& buffer);
    void processChorus(juce::AudioBuffer<float>& buffer);
    void processDelay(juce::AudioBuffer<float>& buffer, double bpm);
    void processReverb(juce::AudioBuffer<float>& buffer);
    void captureSendInput(const juce::AudioBuffer<float>& buffer);
    void clearSendTails();
    void processSendBus(juce::AudioBuffer<float>& buffer, double bpm);
    void processDelaySend(juce::AudioBuffer<float>& buffer, double bpm, float sendAmount, float throwAmount);
    void processReverbSend(juce::AudioBuffer<float>& buffer, float sendAmount, float throwAmount);
    void processWidth(juce::AudioBuffer<float>& buffer);
    void applyOutputGainAndSafety(juce::AudioBuffer<float>& buffer, float outputGainDb);
    float softClip(float sample) const;
    float readParameter(std::atomic<float>* parameter, float fallback) const;
};
}
