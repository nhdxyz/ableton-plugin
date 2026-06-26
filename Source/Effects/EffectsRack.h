#pragma once

#include "../Parameters.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <vector>

namespace Effects
{
class EffectsRack
{
public:
    explicit EffectsRack(Parameters::APVTS& parameters);

    void prepare(double sampleRate, int maximumBlockSize, int numChannels);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, float outputGainDb);

private:
    Parameters::APVTS& parameters;
    juce::dsp::Phaser<float> phaser;
    juce::dsp::Chorus<float> chorus;
    juce::Reverb reverb;
    juce::AudioBuffer<float> delayBuffer;
    std::vector<float> toneLowCutState;
    std::vector<float> toneTiltState;
    std::vector<float> bitcrushHeldSample;
    std::vector<int> bitcrushHoldCounter;

    double currentSampleRate = 44100.0;
    int delayWritePosition = 0;
    int preparedChannels = 2;

    std::atomic<float>* fxDistortionEnabled = nullptr;
    std::atomic<float>* fxDistortionAmount = nullptr;
    std::atomic<float>* fxBitcrushEnabled = nullptr;
    std::atomic<float>* fxBitcrushBits = nullptr;
    std::atomic<float>* fxBitcrushDownsample = nullptr;
    std::atomic<float>* fxBitcrushMix = nullptr;
    std::atomic<float>* fxChorusEnabled = nullptr;
    std::atomic<float>* fxChorusRate = nullptr;
    std::atomic<float>* fxChorusDepth = nullptr;
    std::atomic<float>* fxChorusMix = nullptr;
    std::atomic<float>* fxDelayEnabled = nullptr;
    std::atomic<float>* fxDelayTime = nullptr;
    std::atomic<float>* fxDelayFeedback = nullptr;
    std::atomic<float>* fxDelayMix = nullptr;
    std::atomic<float>* fxReverbEnabled = nullptr;
    std::atomic<float>* fxReverbSize = nullptr;
    std::atomic<float>* fxReverbDamping = nullptr;
    std::atomic<float>* fxReverbMix = nullptr;
    std::atomic<float>* fxToneEnabled = nullptr;
    std::atomic<float>* fxToneTilt = nullptr;
    std::atomic<float>* fxToneLowCut = nullptr;
    std::atomic<float>* fxPhaserEnabled = nullptr;
    std::atomic<float>* fxPhaserRate = nullptr;
    std::atomic<float>* fxPhaserDepth = nullptr;
    std::atomic<float>* fxPhaserMix = nullptr;
    std::atomic<float>* fxGuardEnabled = nullptr;
    std::atomic<float>* fxGuardPush = nullptr;
    std::atomic<float>* fxGuardCeiling = nullptr;
    std::atomic<float>* macroDirt = nullptr;
    std::atomic<float>* macroSpace = nullptr;

    void processTone(juce::AudioBuffer<float>& buffer);
    void processDistortion(juce::AudioBuffer<float>& buffer);
    void processBitcrush(juce::AudioBuffer<float>& buffer);
    void processPhaser(juce::AudioBuffer<float>& buffer);
    void processChorus(juce::AudioBuffer<float>& buffer);
    void processDelay(juce::AudioBuffer<float>& buffer);
    void processReverb(juce::AudioBuffer<float>& buffer);
    void applyOutputGainAndSafety(juce::AudioBuffer<float>& buffer, float outputGainDb);
    float softClip(float sample) const;
    float readParameter(std::atomic<float>* parameter, float fallback) const;
};
}
