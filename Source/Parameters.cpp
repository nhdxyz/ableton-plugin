#include "Parameters.h"

namespace
{
juce::NormalisableRange<float> skewedRange(float min, float max, float centre)
{
    juce::NormalisableRange<float> range { min, max };
    range.setSkewForCentre(centre);
    return range;
}
}

namespace Parameters
{
juce::StringArray waveformChoices()
{
    return { "Sine", "Saw", "Square", "Triangle" };
}

juce::StringArray filterModeChoices()
{
    return { "LP", "BP", "HP" };
}

juce::StringArray randomRecipeChoices()
{
    return {
        "Deep House Bass",
        "Rolling Tech Bass",
        "Acid Line",
        "Minimal Blip",
        "Dark Stab",
        "Noise FX",
        "UKG 2-Step Bass",
        "UKG Organ Stab",
        "UKG Chord Stab",
        "UKG Bell Pluck"
    };
}

juce::StringArray sequencerRateChoices()
{
    return { "1/8", "1/16", "1/32" };
}

APVTS::ParameterLayout createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto add = [&params] (std::unique_ptr<juce::RangedAudioParameter> param)
    {
        params.push_back(std::move(param));
    };

    add(std::make_unique<juce::AudioParameterChoice>(
        ID::oscWave,
        "Osc Wave",
        waveformChoices(),
        1));

    add(std::make_unique<juce::AudioParameterInt>(
        ID::oscOctave,
        "Osc Octave",
        -2,
        2,
        0));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::oscTune,
        "Osc Tune",
        juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("st")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::osc1Level,
        "Osc 1 Level",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        1.0f));

    add(std::make_unique<juce::AudioParameterChoice>(
        ID::osc2Wave,
        "Osc 2 Wave",
        waveformChoices(),
        1));

    add(std::make_unique<juce::AudioParameterInt>(
        ID::osc2Octave,
        "Osc 2 Octave",
        -2,
        2,
        0));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::osc2Tune,
        "Osc 2 Tune",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.01f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("st")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::osc2Level,
        "Osc 2 Level",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::subLevel,
        "Sub Level",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::noiseLevel,
        "Noise Level",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::ampAttack,
        "Amp Attack",
        skewedRange(0.001f, 5.0f, 0.05f),
        0.01f,
        juce::AudioParameterFloatAttributes().withLabel("s")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::ampDecay,
        "Amp Decay",
        skewedRange(0.001f, 5.0f, 0.25f),
        0.18f,
        juce::AudioParameterFloatAttributes().withLabel("s")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::ampSustain,
        "Amp Sustain",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.65f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::ampRelease,
        "Amp Release",
        skewedRange(0.001f, 8.0f, 0.35f),
        0.22f,
        juce::AudioParameterFloatAttributes().withLabel("s")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::filterCutoff,
        "Filter Cutoff",
        skewedRange(20.0f, 20000.0f, 1000.0f),
        1800.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::filterResonance,
        "Filter Resonance",
        juce::NormalisableRange<float> { 0.1f, 1.4f, 0.001f },
        0.45f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::filterEnvAmount,
        "Filter Env Amount",
        juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f },
        0.15f));

    add(std::make_unique<juce::AudioParameterChoice>(
        ID::filterMode,
        "Filter Mode",
        filterModeChoices(),
        0));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::driveAmount,
        "Drive",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.18f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::outputGain,
        "Output Gain",
        juce::NormalisableRange<float> { -24.0f, 6.0f, 0.01f },
        -8.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::monoMode,
        "Mono",
        false));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::glideTime,
        "Glide",
        skewedRange(0.0f, 1.0f, 0.08f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("s")));

    add(std::make_unique<juce::AudioParameterInt>(
        ID::unisonVoices,
        "Unison Voices",
        1,
        7,
        1));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::unisonDetune,
        "Unison Detune",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::unisonBlend,
        "Unison Blend",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.65f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::unisonSpread,
        "Unison Spread",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::macroTone,
        "Macro 1 Tone",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::macroDirt,
        "Macro 2 Dirt",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::macroMotion,
        "Macro 3 Motion",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::macroSpace,
        "Macro 4 Space",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::randomAmount,
        "Random Amount",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.45f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::randomChaos,
        "Random Chaos",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.25f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::randomBrightnessBias,
        "Brightness Bias",
        juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::randomDriveBias,
        "Drive Bias",
        juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::randomMotionBias,
        "Motion Bias",
        juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterChoice>(
        ID::randomRecipe,
        "Random Recipe",
        randomRecipeChoices(),
        1));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::randomLockPitch,
        "Random Lock Pitch",
        false));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::randomLockEnvelope,
        "Random Lock Envelope",
        false));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::randomLockFilter,
        "Random Lock Filter",
        false));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::randomLockSource,
        "Random Lock Source",
        false));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::randomLockSample,
        "Random Lock Sample",
        false));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::randomLockFx,
        "Random Lock FX",
        false));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::randomLockOutput,
        "Random Lock Output",
        false));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::randomLockSequencer,
        "Random Lock Sequencer",
        false));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::sampleEnabled,
        "Sample Enabled",
        false));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::sampleStart,
        "Sample Start",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::sampleEnd,
        "Sample End",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        1.0f));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::sampleReverse,
        "Sample Reverse",
        false));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::sampleTranspose,
        "Sample Transpose",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.01f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("st")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::sampleGain,
        "Sample Gain",
        juce::NormalisableRange<float> { -24.0f, 12.0f, 0.01f },
        -6.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::sampleMix,
        "Sample Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.75f));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::sequencerEnabled,
        "Sequencer Enabled",
        false));

    add(std::make_unique<juce::AudioParameterChoice>(
        ID::sequencerRate,
        "Sequencer Rate",
        sequencerRateChoices(),
        1));

    add(std::make_unique<juce::AudioParameterInt>(
        ID::sequencerRoot,
        "Sequencer Root",
        24,
        84,
        36));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::sequencerGate,
        "Sequencer Gate",
        juce::NormalisableRange<float> { 0.05f, 0.95f, 0.001f },
        0.55f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::sequencerSwing,
        "Sequencer Swing",
        juce::NormalisableRange<float> { 0.0f, 0.65f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::sequencerAccent,
        "Sequencer Accent",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.35f));

    add(std::make_unique<juce::AudioParameterInt>(
        ID::sequencerOctave,
        "Sequencer Octave",
        -2,
        2,
        0));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::sequencerProbability,
        "Sequencer Probability",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        1.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::sequencerRandomAmount,
        "Sequencer Random Amount",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.55f));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::fxDistortionEnabled,
        "FX Distortion Enabled",
        false));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxDistortionAmount,
        "FX Distortion Amount",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.2f));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::fxBitcrushEnabled,
        "FX Bitcrush Enabled",
        false));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxBitcrushBits,
        "FX Bitcrush Bits",
        juce::NormalisableRange<float> { 4.0f, 16.0f, 1.0f },
        12.0f,
        juce::AudioParameterFloatAttributes().withLabel("bit")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxBitcrushDownsample,
        "FX Bitcrush Downsample",
        juce::NormalisableRange<float> { 1.0f, 32.0f, 1.0f },
        1.0f,
        juce::AudioParameterFloatAttributes().withLabel("x")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxBitcrushMix,
        "FX Bitcrush Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.25f));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::fxPumpEnabled,
        "FX Pump Enabled",
        false));

    add(std::make_unique<juce::AudioParameterChoice>(
        ID::fxPumpRate,
        "FX Pump Rate",
        juce::StringArray { "1/4", "1/8", "1/8T", "1/16" },
        0));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxPumpDepth,
        "FX Pump Depth",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.35f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxPumpShape,
        "FX Pump Shape",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.45f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxPumpPhase,
        "FX Pump Phase",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::fxChorusEnabled,
        "FX Chorus Enabled",
        false));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxChorusRate,
        "FX Chorus Rate",
        skewedRange(0.05f, 8.0f, 0.7f),
        0.35f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxChorusDepth,
        "FX Chorus Depth",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.35f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxChorusMix,
        "FX Chorus Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.25f));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::fxDelayEnabled,
        "FX Delay Enabled",
        false));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxDelayTime,
        "FX Delay Time",
        skewedRange(0.02f, 1.0f, 0.25f),
        0.25f,
        juce::AudioParameterFloatAttributes().withLabel("s")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxDelayFeedback,
        "FX Delay Feedback",
        juce::NormalisableRange<float> { 0.0f, 0.85f, 0.001f },
        0.25f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxDelayMix,
        "FX Delay Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.2f));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::fxReverbEnabled,
        "FX Reverb Enabled",
        false));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxReverbSize,
        "FX Reverb Size",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.35f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxReverbDamping,
        "FX Reverb Damping",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.45f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxReverbMix,
        "FX Reverb Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.2f));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::fxWidthEnabled,
        "FX Width Enabled",
        false));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxWidthAmount,
        "FX Width Amount",
        juce::NormalisableRange<float> { 0.0f, 1.6f, 0.001f },
        1.15f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxWidthMonoCutoff,
        "FX Width Mono Bass",
        skewedRange(40.0f, 260.0f, 120.0f),
        120.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::fxToneEnabled,
        "FX Tone Enabled",
        false));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxToneTilt,
        "FX Tone Tilt",
        juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxToneLowCut,
        "FX Tone Low Cut",
        skewedRange(20.0f, 250.0f, 45.0f),
        30.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::fxPhaserEnabled,
        "FX Phaser Enabled",
        false));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxPhaserRate,
        "FX Phaser Rate",
        skewedRange(0.05f, 8.0f, 0.45f),
        0.32f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxPhaserDepth,
        "FX Phaser Depth",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.42f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxPhaserMix,
        "FX Phaser Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.22f));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::fxGuardEnabled,
        "FX Guard Enabled",
        false));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxGuardPush,
        "FX Guard Push",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxGuardCeiling,
        "FX Guard Ceiling",
        juce::NormalisableRange<float> { 0.65f, 0.98f, 0.001f },
        0.92f));

    return { params.begin(), params.end() };
}
}
