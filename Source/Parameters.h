#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

namespace Parameters
{
using APVTS = juce::AudioProcessorValueTreeState;

namespace ID
{
inline constexpr auto oscWave = "osc_wave";
inline constexpr auto oscOctave = "osc_octave";
inline constexpr auto oscTune = "osc_tune";
inline constexpr auto osc1Level = "osc1_level";
inline constexpr auto osc2Wave = "osc2_wave";
inline constexpr auto osc2Octave = "osc2_octave";
inline constexpr auto osc2Tune = "osc2_tune";
inline constexpr auto osc2Level = "osc2_level";
inline constexpr auto subLevel = "sub_level";
inline constexpr auto noiseLevel = "noise_level";
inline constexpr auto ampAttack = "amp_attack";
inline constexpr auto ampDecay = "amp_decay";
inline constexpr auto ampSustain = "amp_sustain";
inline constexpr auto ampRelease = "amp_release";
inline constexpr auto filterCutoff = "filter_cutoff";
inline constexpr auto filterResonance = "filter_resonance";
inline constexpr auto filterEnvAmount = "filter_env_amount";
inline constexpr auto filterMode = "filter_mode";
inline constexpr auto driveAmount = "drive_amount";
inline constexpr auto outputGain = "output_gain";
inline constexpr auto monoMode = "mono_mode";
inline constexpr auto glideTime = "glide_time";
inline constexpr auto unisonVoices = "unison_voices";
inline constexpr auto unisonDetune = "unison_detune";
inline constexpr auto unisonBlend = "unison_blend";
inline constexpr auto unisonSpread = "unison_spread";
inline constexpr auto macroTone = "macro_1";
inline constexpr auto macroDirt = "macro_2";
inline constexpr auto macroMotion = "macro_3";
inline constexpr auto macroSpace = "macro_4";
inline constexpr auto lfo1Rate = "lfo1_rate";
inline constexpr auto lfo1Sync = "lfo1_sync";
inline constexpr auto lfo1SyncRate = "lfo1_sync_rate";
inline constexpr auto lfo1Shape = "lfo1_shape";
inline constexpr auto lfo1Depth = "lfo1_depth";
inline constexpr auto lfo1Phase = "lfo1_phase";
inline constexpr auto lfo1Retrigger = "lfo1_retrigger";
inline constexpr auto modEnv1Attack = "mod_env1_attack";
inline constexpr auto modEnv1Decay = "mod_env1_decay";
inline constexpr auto modEnv1Sustain = "mod_env1_sustain";
inline constexpr auto modEnv1Release = "mod_env1_release";
inline constexpr auto modEnv1Depth = "mod_env1_depth";
inline constexpr std::array<const char*, 8> modMatrixSource {
    "mod_slot_1_source",
    "mod_slot_2_source",
    "mod_slot_3_source",
    "mod_slot_4_source",
    "mod_slot_5_source",
    "mod_slot_6_source",
    "mod_slot_7_source",
    "mod_slot_8_source"
};
inline constexpr std::array<const char*, 8> modMatrixDestination {
    "mod_slot_1_destination",
    "mod_slot_2_destination",
    "mod_slot_3_destination",
    "mod_slot_4_destination",
    "mod_slot_5_destination",
    "mod_slot_6_destination",
    "mod_slot_7_destination",
    "mod_slot_8_destination"
};
inline constexpr std::array<const char*, 8> modMatrixAmount {
    "mod_slot_1_amount",
    "mod_slot_2_amount",
    "mod_slot_3_amount",
    "mod_slot_4_amount",
    "mod_slot_5_amount",
    "mod_slot_6_amount",
    "mod_slot_7_amount",
    "mod_slot_8_amount"
};
inline constexpr auto randomAmount = "random_amount";
inline constexpr auto randomChaos = "random_chaos";
inline constexpr auto randomBrightnessBias = "random_brightness_bias";
inline constexpr auto randomDriveBias = "random_drive_bias";
inline constexpr auto randomMotionBias = "random_motion_bias";
inline constexpr auto randomRecipe = "random_recipe";
inline constexpr auto randomLockPitch = "random_lock_pitch";
inline constexpr auto randomLockEnvelope = "random_lock_envelope";
inline constexpr auto randomLockFilter = "random_lock_filter";
inline constexpr auto randomLockSource = "random_lock_source";
inline constexpr auto randomLockSample = "random_lock_sample";
inline constexpr auto randomLockFx = "random_lock_fx";
inline constexpr auto randomLockOutput = "random_lock_output";
inline constexpr auto randomLockSequencer = "random_lock_sequencer";
inline constexpr auto sampleEnabled = "sample_enabled";
inline constexpr auto sampleStart = "sample_start";
inline constexpr auto sampleEnd = "sample_end";
inline constexpr auto sampleReverse = "sample_reverse";
inline constexpr auto samplePlaybackMode = "sample_playback_mode";
inline constexpr auto sampleTranspose = "sample_transpose";
inline constexpr auto samplePitchRamp = "sample_pitch_ramp";
inline constexpr auto sampleGain = "sample_gain";
inline constexpr auto sampleMix = "sample_mix";
inline constexpr auto sampleStutterEnabled = "sample_stutter_enabled";
inline constexpr auto sampleStutterRate = "sample_stutter_rate";
inline constexpr auto sampleStutterRepeats = "sample_stutter_repeats";
inline constexpr auto sequencerEnabled = "sequencer_enabled";
inline constexpr auto sequencerRate = "sequencer_rate";
inline constexpr auto sequencerRoot = "sequencer_root";
inline constexpr auto sequencerGate = "sequencer_gate";
inline constexpr auto sequencerSwing = "sequencer_swing";
inline constexpr auto sequencerGrooveMode = "sequencer_groove_mode";
inline constexpr auto sequencerAccent = "sequencer_accent";
inline constexpr auto sequencerOctave = "sequencer_octave";
inline constexpr auto sequencerProbability = "sequencer_probability";
inline constexpr auto sequencerRandomAmount = "sequencer_random_amount";
inline constexpr auto fxDistortionEnabled = "fx_distortion_enabled";
inline constexpr auto fxDistortionAmount = "fx_distortion_amount";
inline constexpr auto fxBitcrushEnabled = "fx_bitcrush_enabled";
inline constexpr auto fxBitcrushBits = "fx_bitcrush_bits";
inline constexpr auto fxBitcrushDownsample = "fx_bitcrush_downsample";
inline constexpr auto fxBitcrushMix = "fx_bitcrush_mix";
inline constexpr auto fxPumpEnabled = "fx_pump_enabled";
inline constexpr auto fxPumpRate = "fx_pump_rate";
inline constexpr auto fxPumpDepth = "fx_pump_depth";
inline constexpr auto fxPumpShape = "fx_pump_shape";
inline constexpr auto fxPumpPhase = "fx_pump_phase";
inline constexpr auto fxChorusEnabled = "fx_chorus_enabled";
inline constexpr auto fxChorusRate = "fx_chorus_rate";
inline constexpr auto fxChorusDepth = "fx_chorus_depth";
inline constexpr auto fxChorusMix = "fx_chorus_mix";
inline constexpr auto fxDelayEnabled = "fx_delay_enabled";
inline constexpr auto fxDelayTime = "fx_delay_time";
inline constexpr auto fxDelayFeedback = "fx_delay_feedback";
inline constexpr auto fxDelayMix = "fx_delay_mix";
inline constexpr auto fxReverbEnabled = "fx_reverb_enabled";
inline constexpr auto fxReverbSize = "fx_reverb_size";
inline constexpr auto fxReverbDamping = "fx_reverb_damping";
inline constexpr auto fxReverbMix = "fx_reverb_mix";
inline constexpr auto fxWidthEnabled = "fx_width_enabled";
inline constexpr auto fxWidthAmount = "fx_width_amount";
inline constexpr auto fxWidthMonoCutoff = "fx_width_mono_cutoff";
inline constexpr auto fxToneEnabled = "fx_tone_enabled";
inline constexpr auto fxToneTilt = "fx_tone_tilt";
inline constexpr auto fxToneLowCut = "fx_tone_low_cut";
inline constexpr auto fxPhaserEnabled = "fx_phaser_enabled";
inline constexpr auto fxPhaserRate = "fx_phaser_rate";
inline constexpr auto fxPhaserDepth = "fx_phaser_depth";
inline constexpr auto fxPhaserMix = "fx_phaser_mix";
inline constexpr auto fxGuardEnabled = "fx_guard_enabled";
inline constexpr auto fxGuardPush = "fx_guard_push";
inline constexpr auto fxGuardCeiling = "fx_guard_ceiling";
inline constexpr auto fxFlangerEnabled = "fx_flanger_enabled";
inline constexpr auto fxFlangerRate = "fx_flanger_rate";
inline constexpr auto fxFlangerDepth = "fx_flanger_depth";
inline constexpr auto fxFlangerFeedback = "fx_flanger_feedback";
inline constexpr auto fxFlangerMix = "fx_flanger_mix";
}

juce::StringArray waveformChoices();
juce::StringArray filterModeChoices();
juce::StringArray randomRecipeChoices();
juce::StringArray sequencerRateChoices();
juce::StringArray sequencerGrooveModeChoices();
juce::StringArray lfoShapeChoices();
juce::StringArray lfoSyncRateChoices();
juce::StringArray modulationSourceChoices();
juce::StringArray modulationDestinationChoices();
APVTS::ParameterLayout createLayout();
}
