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
        "UKG Bell Pluck",
        "UKG Dred Bass"
    };
}

juce::StringArray sequencerRateChoices()
{
    return { "1/8", "1/16", "1/32" };
}

juce::StringArray sequencerGrooveModeChoices()
{
    return { "Classic", "Selective", "UKG Push", "Tight" };
}

juce::StringArray sequencerScaleChoices()
{
    return { "Off", "Major", "Minor", "Dorian", "Minor Pent" };
}

juce::StringArray sequencerChordModeChoices()
{
    return { "Off", "5th", "Minor", "Minor 7", "Major", "Minor 9" };
}

juce::StringArray sequencerChordVoicingChoices()
{
    return { "Close", "Inv 1", "Inv 2", "Open", "Drop 2" };
}

juce::StringArray lfoShapeChoices()
{
    return { "Sine", "Triangle", "Saw", "Square", "Step", "Curve" };
}

juce::StringArray lfoSyncRateChoices()
{
    return { "1/4", "1/8", "1/8T", "1/16" };
}

juce::StringArray modulationSourceChoices()
{
    return { "Off", "LFO 1", "Mod Env 1", "Velocity", "Tone", "Dirt", "Motion", "Space" };
}

juce::StringArray modulationDestinationChoices()
{
    return { "Off", "Filter Cutoff", "Filter Res", "Filter Env", "Drive", "Osc 2 Tune", "Osc 2 Level" };
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
        ID::lfo1Rate,
        "LFO 1 Rate",
        skewedRange(0.05f, 20.0f, 1.0f),
        1.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::lfo1Sync,
        "LFO 1 Sync",
        true));

    add(std::make_unique<juce::AudioParameterChoice>(
        ID::lfo1SyncRate,
        "LFO 1 Sync Rate",
        lfoSyncRateChoices(),
        1));

    add(std::make_unique<juce::AudioParameterChoice>(
        ID::lfo1Shape,
        "LFO 1 Shape",
        lfoShapeChoices(),
        0));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::lfo1Depth,
        "LFO 1 Depth",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.45f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::lfo1Phase,
        "LFO 1 Phase",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::lfo1Retrigger,
        "LFO 1 Retrigger",
        true));

    constexpr std::array<float, 8> defaultLfoCurve {
        0.0f, 0.58f, 1.0f, 0.42f, -0.18f, -0.72f, -1.0f, -0.36f
    };

    for (size_t index = 0; index < ID::lfo1Curve.size(); ++index)
    {
        add(std::make_unique<juce::AudioParameterFloat>(
            ID::lfo1Curve[index],
            "LFO 1 Curve " + juce::String(static_cast<int>(index + 1)),
            juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f },
            defaultLfoCurve[index]));
    }

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::modEnv1Attack,
        "Mod Env 1 Attack",
        skewedRange(0.001f, 5.0f, 0.03f),
        0.01f,
        juce::AudioParameterFloatAttributes().withLabel("s")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::modEnv1Decay,
        "Mod Env 1 Decay",
        skewedRange(0.001f, 5.0f, 0.2f),
        0.22f,
        juce::AudioParameterFloatAttributes().withLabel("s")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::modEnv1Sustain,
        "Mod Env 1 Sustain",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::modEnv1Release,
        "Mod Env 1 Release",
        skewedRange(0.001f, 8.0f, 0.2f),
        0.12f,
        juce::AudioParameterFloatAttributes().withLabel("s")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::modEnv1Depth,
        "Mod Env 1 Depth",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.5f));

    for (size_t index = 0; index < ID::modMatrixSource.size(); ++index)
    {
        add(std::make_unique<juce::AudioParameterChoice>(
            ID::modMatrixSource[index],
            "Mod Slot " + juce::String(static_cast<int>(index + 1)) + " Source",
            modulationSourceChoices(),
            0));

        add(std::make_unique<juce::AudioParameterChoice>(
            ID::modMatrixDestination[index],
            "Mod Slot " + juce::String(static_cast<int>(index + 1)) + " Destination",
            modulationDestinationChoices(),
            0));

        add(std::make_unique<juce::AudioParameterFloat>(
            ID::modMatrixAmount[index],
            "Mod Slot " + juce::String(static_cast<int>(index + 1)) + " Amount",
            juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f },
            0.0f));
    }

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

    add(std::make_unique<juce::AudioParameterChoice>(
        ID::samplePlaybackMode,
        "Sample Playback Mode",
        juce::StringArray { "Gate", "One Shot" },
        1));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::sampleTranspose,
        "Sample Transpose",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.01f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("st")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::samplePitchRamp,
        "Sample Pitch Ramp",
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
        ID::sampleStutterEnabled,
        "Sample Stutter Enabled",
        false));

    add(std::make_unique<juce::AudioParameterChoice>(
        ID::sampleStutterRate,
        "Sample Stutter Rate",
        juce::StringArray { "1/8", "1/16", "1/32" },
        1));

    add(std::make_unique<juce::AudioParameterInt>(
        ID::sampleStutterRepeats,
        "Sample Stutter Repeats",
        1,
        8,
        3));

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

    add(std::make_unique<juce::AudioParameterChoice>(
        ID::sequencerGrooveMode,
        "Sequencer Groove Mode",
        sequencerGrooveModeChoices(),
        0));

    add(std::make_unique<juce::AudioParameterChoice>(
        ID::sequencerScale,
        "Sequencer Scale",
        sequencerScaleChoices(),
        0));

    add(std::make_unique<juce::AudioParameterChoice>(
        ID::sequencerChordMode,
        "Sequencer Chord Mode",
        sequencerChordModeChoices(),
        0));

    add(std::make_unique<juce::AudioParameterChoice>(
        ID::sequencerChordVoicing,
        "Sequencer Chord Voicing",
        sequencerChordVoicingChoices(),
        0));

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

    add(std::make_unique<juce::AudioParameterBool>(
        ID::fxFlangerEnabled,
        "FX Flanger Enabled",
        false));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxFlangerRate,
        "FX Flanger Rate",
        skewedRange(0.03f, 6.0f, 0.35f),
        0.22f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxFlangerDepth,
        "FX Flanger Depth",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.32f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxFlangerFeedback,
        "FX Flanger Feedback",
        juce::NormalisableRange<float> { -0.85f, 0.85f, 0.001f },
        0.18f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxFlangerMix,
        "FX Flanger Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.18f));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::fxEqEnabled,
        "FX EQ Enabled",
        false));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxEqLowGain,
        "FX EQ Low Gain",
        juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxEqMidGain,
        "FX EQ Mid Gain",
        juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxEqHighGain,
        "FX EQ High Gain",
        juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxEqTrim,
        "FX EQ Trim",
        juce::NormalisableRange<float> { -12.0f, 6.0f, 0.01f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::fxTremoloEnabled,
        "FX Tremolo Enabled",
        false));

    add(std::make_unique<juce::AudioParameterChoice>(
        ID::fxTremoloRate,
        "FX Tremolo Rate",
        juce::StringArray { "1/4", "1/8", "1/8T", "1/16" },
        1));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxTremoloDepth,
        "FX Tremolo Depth",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.28f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxTremoloPan,
        "FX Tremolo Pan",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.25f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxTremoloShape,
        "FX Tremolo Shape",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.45f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxTremoloPhase,
        "FX Tremolo Phase",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.0f));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::fxRingEnabled,
        "FX Ring Mod Enabled",
        false));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxRingFrequency,
        "FX Ring Mod Frequency",
        skewedRange(0.25f, 2500.0f, 85.0f),
        72.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxRingDepth,
        "FX Ring Mod Depth",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.35f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxRingMix,
        "FX Ring Mod Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.18f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxRingBias,
        "FX Ring Mod Bias",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.45f));

    add(std::make_unique<juce::AudioParameterBool>(
        ID::fxCombEnabled,
        "FX Comb Enabled",
        false));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxCombFrequency,
        "FX Comb Frequency",
        skewedRange(25.0f, 2400.0f, 180.0f),
        180.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxCombFeedback,
        "FX Comb Feedback",
        juce::NormalisableRange<float> { -0.82f, 0.82f, 0.001f },
        0.28f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxCombDamping,
        "FX Comb Damping",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.35f));

    add(std::make_unique<juce::AudioParameterFloat>(
        ID::fxCombMix,
        "FX Comb Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.16f));

    for (size_t index = 0; index < ID::fxOrder.size(); ++index)
    {
        add(std::make_unique<juce::AudioParameterInt>(
            ID::fxOrder[index],
            "FX Order " + juce::String(static_cast<int>(index + 1)),
            0,
            static_cast<int>(ID::fxOrder.size() - 1),
            static_cast<int>(index)));
    }

    return { params.begin(), params.end() };
}
}
