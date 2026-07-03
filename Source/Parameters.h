#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

namespace Parameters
{
using APVTS = juce::AudioProcessorValueTreeState;
inline constexpr size_t customWaveMorphFrameCount = 8;

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
inline constexpr auto noiseType = "noise_type";
inline constexpr auto noiseDecay = "noise_decay";
inline constexpr auto oscWarp = "osc_warp";
inline constexpr auto oscWarpB = "osc_warp_b";
inline constexpr auto osc2Warp = "osc2_warp";
inline constexpr auto osc2WarpB = "osc2_warp_b";
inline constexpr auto oscWarpMode = "osc_warp_mode";
inline constexpr auto oscWarpBMode = "osc_warp_b_mode";
inline constexpr auto osc2WarpMode = "osc2_warp_mode";
inline constexpr auto osc2WarpBMode = "osc2_warp_b_mode";
inline constexpr auto oscWavetablePosition = "osc_wavetable_position";
inline constexpr auto osc2WavetablePosition = "osc2_wavetable_position";
inline constexpr std::array<const char*, 16> oscCustomWave {
    "osc_custom_wave_1",
    "osc_custom_wave_2",
    "osc_custom_wave_3",
    "osc_custom_wave_4",
    "osc_custom_wave_5",
    "osc_custom_wave_6",
    "osc_custom_wave_7",
    "osc_custom_wave_8",
    "osc_custom_wave_9",
    "osc_custom_wave_10",
    "osc_custom_wave_11",
    "osc_custom_wave_12",
    "osc_custom_wave_13",
    "osc_custom_wave_14",
    "osc_custom_wave_15",
    "osc_custom_wave_16"
};
inline constexpr std::array<const char*, 16> osc2CustomWave {
    "osc2_custom_wave_1",
    "osc2_custom_wave_2",
    "osc2_custom_wave_3",
    "osc2_custom_wave_4",
    "osc2_custom_wave_5",
    "osc2_custom_wave_6",
    "osc2_custom_wave_7",
    "osc2_custom_wave_8",
    "osc2_custom_wave_9",
    "osc2_custom_wave_10",
    "osc2_custom_wave_11",
    "osc2_custom_wave_12",
    "osc2_custom_wave_13",
    "osc2_custom_wave_14",
    "osc2_custom_wave_15",
    "osc2_custom_wave_16"
};
inline constexpr auto ampAttack = "amp_attack";
inline constexpr auto ampDecay = "amp_decay";
inline constexpr auto ampSustain = "amp_sustain";
inline constexpr auto ampRelease = "amp_release";
inline constexpr auto filterCutoff = "filter_cutoff";
inline constexpr auto filterResonance = "filter_resonance";
inline constexpr auto filterEnvAmount = "filter_env_amount";
inline constexpr auto filterMode = "filter_mode";
inline constexpr auto filterCharacter = "filter_character";
inline constexpr auto filterSlope = "filter_slope";
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
inline constexpr auto macroWeight = "macro_5";
inline constexpr auto macroBounce = "macro_6";
inline constexpr auto macroWarp = "macro_7";
inline constexpr auto macroThrow = "macro_8";
inline constexpr auto lfo1Rate = "lfo1_rate";
inline constexpr auto lfo1Sync = "lfo1_sync";
inline constexpr auto lfo1SyncRate = "lfo1_sync_rate";
inline constexpr auto lfo1Shape = "lfo1_shape";
inline constexpr auto lfo1Depth = "lfo1_depth";
inline constexpr auto lfo1Phase = "lfo1_phase";
inline constexpr auto lfo1Retrigger = "lfo1_retrigger";
inline constexpr std::array<const char*, 8> lfo1Curve {
    "lfo1_curve_1",
    "lfo1_curve_2",
    "lfo1_curve_3",
    "lfo1_curve_4",
    "lfo1_curve_5",
    "lfo1_curve_6",
    "lfo1_curve_7",
    "lfo1_curve_8"
};
inline constexpr auto lfo2Rate = "lfo2_rate";
inline constexpr auto lfo2Sync = "lfo2_sync";
inline constexpr auto lfo2SyncRate = "lfo2_sync_rate";
inline constexpr auto lfo2Shape = "lfo2_shape";
inline constexpr auto lfo2Depth = "lfo2_depth";
inline constexpr auto lfo2Phase = "lfo2_phase";
inline constexpr auto lfo2Retrigger = "lfo2_retrigger";
inline constexpr auto stepLfoSync = "step_lfo_sync";
inline constexpr auto stepLfoSyncRate = "step_lfo_sync_rate";
inline constexpr auto stepLfoRate = "step_lfo_rate";
inline constexpr auto stepLfoDepth = "step_lfo_depth";
inline constexpr auto stepLfoSlew = "step_lfo_slew";
inline constexpr std::array<const char*, 8> stepLfoValue {
    "step_lfo_value_1",
    "step_lfo_value_2",
    "step_lfo_value_3",
    "step_lfo_value_4",
    "step_lfo_value_5",
    "step_lfo_value_6",
    "step_lfo_value_7",
    "step_lfo_value_8"
};
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
inline constexpr std::array<const char*, 8> modMatrixEnabled {
    "mod_slot_1_enabled",
    "mod_slot_2_enabled",
    "mod_slot_3_enabled",
    "mod_slot_4_enabled",
    "mod_slot_5_enabled",
    "mod_slot_6_enabled",
    "mod_slot_7_enabled",
    "mod_slot_8_enabled"
};
inline constexpr std::array<const char*, 8> modMatrixPolarity {
    "mod_slot_1_polarity",
    "mod_slot_2_polarity",
    "mod_slot_3_polarity",
    "mod_slot_4_polarity",
    "mod_slot_5_polarity",
    "mod_slot_6_polarity",
    "mod_slot_7_polarity",
    "mod_slot_8_polarity"
};
inline constexpr std::array<const char*, 8> modMatrixCurve {
    "mod_slot_1_curve",
    "mod_slot_2_curve",
    "mod_slot_3_curve",
    "mod_slot_4_curve",
    "mod_slot_5_curve",
    "mod_slot_6_curve",
    "mod_slot_7_curve",
    "mod_slot_8_curve"
};
inline constexpr std::array<const char*, 8> modMatrixRangeMin {
    "mod_slot_1_range_min",
    "mod_slot_2_range_min",
    "mod_slot_3_range_min",
    "mod_slot_4_range_min",
    "mod_slot_5_range_min",
    "mod_slot_6_range_min",
    "mod_slot_7_range_min",
    "mod_slot_8_range_min"
};
inline constexpr std::array<const char*, 8> modMatrixRangeMax {
    "mod_slot_1_range_max",
    "mod_slot_2_range_max",
    "mod_slot_3_range_max",
    "mod_slot_4_range_max",
    "mod_slot_5_range_max",
    "mod_slot_6_range_max",
    "mod_slot_7_range_max",
    "mod_slot_8_range_max"
};
inline constexpr std::array<const char*, 8> modMatrixSlew {
    "mod_slot_1_slew",
    "mod_slot_2_slew",
    "mod_slot_3_slew",
    "mod_slot_4_slew",
    "mod_slot_5_slew",
    "mod_slot_6_slew",
    "mod_slot_7_slew",
    "mod_slot_8_slew"
};
inline constexpr auto randomAmount = "random_amount";
inline constexpr auto randomChaos = "random_chaos";
inline constexpr auto randomBrightnessBias = "random_brightness_bias";
inline constexpr auto randomDriveBias = "random_drive_bias";
inline constexpr auto randomMotionBias = "random_motion_bias";
inline constexpr auto randomSourceIntensity = "random_source_intensity";
inline constexpr auto randomEnvelopeIntensity = "random_envelope_intensity";
inline constexpr auto randomFilterIntensity = "random_filter_intensity";
inline constexpr auto randomSampleIntensity = "random_sample_intensity";
inline constexpr auto randomFxIntensity = "random_fx_intensity";
inline constexpr auto randomSequencerIntensity = "random_sequencer_intensity";
inline constexpr auto randomMacroIntensity = "random_macro_intensity";
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
inline constexpr auto sampleRecordSource = "sample_record_source";
inline constexpr auto sampleRecordStart = "sample_record_start";
inline constexpr auto sampleRecordLength = "sample_record_length";
inline constexpr auto sampleRecordPreRoll = "sample_record_pre_roll";
inline constexpr auto samplePlaybackMode = "sample_playback_mode";
inline constexpr auto sampleEngineMode = "sample_engine_mode";
inline constexpr auto sampleGrainSize = "sample_grain_size";
inline constexpr auto sampleGrainSpray = "sample_grain_spray";
inline constexpr auto sampleSpectralFreeze = "sample_spectral_freeze";
inline constexpr auto sampleTranspose = "sample_transpose";
inline constexpr auto samplePitchRamp = "sample_pitch_ramp";
inline constexpr auto sampleGain = "sample_gain";
inline constexpr auto sampleMix = "sample_mix";
inline constexpr auto sampleStutterEnabled = "sample_stutter_enabled";
inline constexpr auto sampleStutterRate = "sample_stutter_rate";
inline constexpr auto sampleStutterRepeats = "sample_stutter_repeats";
inline constexpr auto sampleSliceStyle = "sample_slice_style";
inline constexpr std::array<const char*, 8> sampleSliceCustom {
    "sample_slice_1_custom",
    "sample_slice_2_custom",
    "sample_slice_3_custom",
    "sample_slice_4_custom",
    "sample_slice_5_custom",
    "sample_slice_6_custom",
    "sample_slice_7_custom",
    "sample_slice_8_custom"
};
inline constexpr std::array<const char*, 8> sampleSliceStart {
    "sample_slice_1_start",
    "sample_slice_2_start",
    "sample_slice_3_start",
    "sample_slice_4_start",
    "sample_slice_5_start",
    "sample_slice_6_start",
    "sample_slice_7_start",
    "sample_slice_8_start"
};
inline constexpr std::array<const char*, 8> sampleSliceEnd {
    "sample_slice_1_end",
    "sample_slice_2_end",
    "sample_slice_3_end",
    "sample_slice_4_end",
    "sample_slice_5_end",
    "sample_slice_6_end",
    "sample_slice_7_end",
    "sample_slice_8_end"
};
inline constexpr std::array<const char*, 8> sampleSliceReverse {
    "sample_slice_1_reverse",
    "sample_slice_2_reverse",
    "sample_slice_3_reverse",
    "sample_slice_4_reverse",
    "sample_slice_5_reverse",
    "sample_slice_6_reverse",
    "sample_slice_7_reverse",
    "sample_slice_8_reverse"
};
inline constexpr std::array<const char*, 8> sampleSliceTranspose {
    "sample_slice_1_transpose",
    "sample_slice_2_transpose",
    "sample_slice_3_transpose",
    "sample_slice_4_transpose",
    "sample_slice_5_transpose",
    "sample_slice_6_transpose",
    "sample_slice_7_transpose",
    "sample_slice_8_transpose"
};
inline constexpr std::array<const char*, 8> sampleSliceGain {
    "sample_slice_1_gain",
    "sample_slice_2_gain",
    "sample_slice_3_gain",
    "sample_slice_4_gain",
    "sample_slice_5_gain",
    "sample_slice_6_gain",
    "sample_slice_7_gain",
    "sample_slice_8_gain"
};
inline constexpr std::array<const char*, 8> sampleSlicePan {
    "sample_slice_1_pan",
    "sample_slice_2_pan",
    "sample_slice_3_pan",
    "sample_slice_4_pan",
    "sample_slice_5_pan",
    "sample_slice_6_pan",
    "sample_slice_7_pan",
    "sample_slice_8_pan"
};
inline constexpr std::array<const char*, 8> sampleSliceProbability {
    "sample_slice_1_probability",
    "sample_slice_2_probability",
    "sample_slice_3_probability",
    "sample_slice_4_probability",
    "sample_slice_5_probability",
    "sample_slice_6_probability",
    "sample_slice_7_probability",
    "sample_slice_8_probability"
};
inline constexpr std::array<const char*, 8> sampleSliceStutter {
    "sample_slice_1_stutter",
    "sample_slice_2_stutter",
    "sample_slice_3_stutter",
    "sample_slice_4_stutter",
    "sample_slice_5_stutter",
    "sample_slice_6_stutter",
    "sample_slice_7_stutter",
    "sample_slice_8_stutter"
};
inline constexpr std::array<const char*, 8> sampleSliceChoke {
    "sample_slice_1_choke",
    "sample_slice_2_choke",
    "sample_slice_3_choke",
    "sample_slice_4_choke",
    "sample_slice_5_choke",
    "sample_slice_6_choke",
    "sample_slice_7_choke",
    "sample_slice_8_choke"
};
    inline constexpr std::array<const char*, 8> sampleSliceStutterRepeats {
        "sample_slice_1_stutter_repeats",
        "sample_slice_2_stutter_repeats",
        "sample_slice_3_stutter_repeats",
        "sample_slice_4_stutter_repeats",
        "sample_slice_5_stutter_repeats",
        "sample_slice_6_stutter_repeats",
        "sample_slice_7_stutter_repeats",
        "sample_slice_8_stutter_repeats"
    };
    inline constexpr std::array<const char*, 8> sampleSliceNudge {
        "sample_slice_1_nudge",
        "sample_slice_2_nudge",
        "sample_slice_3_nudge",
        "sample_slice_4_nudge",
        "sample_slice_5_nudge",
        "sample_slice_6_nudge",
        "sample_slice_7_nudge",
        "sample_slice_8_nudge"
    };
    inline constexpr std::array<const char*, 8> sampleSliceFade {
        "sample_slice_1_fade",
        "sample_slice_2_fade",
        "sample_slice_3_fade",
        "sample_slice_4_fade",
        "sample_slice_5_fade",
        "sample_slice_6_fade",
        "sample_slice_7_fade",
        "sample_slice_8_fade"
    };
    inline constexpr auto sequencerEnabled = "sequencer_enabled";
inline constexpr auto sequencerRate = "sequencer_rate";
inline constexpr auto sequencerRoot = "sequencer_root";
inline constexpr auto sequencerGate = "sequencer_gate";
inline constexpr auto sequencerSwing = "sequencer_swing";
inline constexpr auto sequencerGrooveMode = "sequencer_groove_mode";
inline constexpr auto sequencerScale = "sequencer_scale";
inline constexpr auto sequencerChordMode = "sequencer_chord_mode";
inline constexpr auto sequencerChordVoicing = "sequencer_chord_voicing";
inline constexpr auto sequencerChordStrum = "sequencer_chord_strum";
inline constexpr auto sequencerChordMemory = "sequencer_chord_memory";
inline constexpr auto sequencerAccent = "sequencer_accent";
inline constexpr auto sequencerOctave = "sequencer_octave";
inline constexpr auto sequencerProbability = "sequencer_probability";
inline constexpr auto sequencerRandomAmount = "sequencer_random_amount";
inline constexpr auto sequencerLockDestination = "sequencer_lock_destination";
inline constexpr auto sequencerLockDepth = "sequencer_lock_depth";
inline constexpr auto fxDistortionEnabled = "fx_distortion_enabled";
inline constexpr auto fxDistortionAmount = "fx_distortion_amount";
inline constexpr auto fxDistortionBassSafe = "fx_distortion_bass_safe";
inline constexpr auto fxBitcrushEnabled = "fx_bitcrush_enabled";
inline constexpr auto fxBitcrushBits = "fx_bitcrush_bits";
inline constexpr auto fxBitcrushDownsample = "fx_bitcrush_downsample";
inline constexpr auto fxBitcrushMix = "fx_bitcrush_mix";
inline constexpr auto fxPumpEnabled = "fx_pump_enabled";
inline constexpr auto fxPumpRate = "fx_pump_rate";
inline constexpr auto fxPumpCurve = "fx_pump_curve";
inline constexpr std::array<const char*, 8> fxPumpCustomCurve {
    "fx_pump_curve_1",
    "fx_pump_curve_2",
    "fx_pump_curve_3",
    "fx_pump_curve_4",
    "fx_pump_curve_5",
    "fx_pump_curve_6",
    "fx_pump_curve_7",
    "fx_pump_curve_8"
};
inline constexpr auto fxPumpDepth = "fx_pump_depth";
inline constexpr auto fxPumpShape = "fx_pump_shape";
inline constexpr auto fxPumpPhase = "fx_pump_phase";
inline constexpr auto fxTremoloEnabled = "fx_tremolo_enabled";
inline constexpr auto fxTremoloRate = "fx_tremolo_rate";
inline constexpr auto fxTremoloDepth = "fx_tremolo_depth";
inline constexpr auto fxTremoloPan = "fx_tremolo_pan";
inline constexpr auto fxTremoloShape = "fx_tremolo_shape";
inline constexpr auto fxTremoloPhase = "fx_tremolo_phase";
inline constexpr auto fxRingEnabled = "fx_ring_enabled";
inline constexpr auto fxRingFrequency = "fx_ring_frequency";
inline constexpr auto fxRingDepth = "fx_ring_depth";
inline constexpr auto fxRingMix = "fx_ring_mix";
inline constexpr auto fxRingBias = "fx_ring_bias";
inline constexpr auto fxCombEnabled = "fx_comb_enabled";
inline constexpr auto fxCombFrequency = "fx_comb_frequency";
inline constexpr auto fxCombFeedback = "fx_comb_feedback";
inline constexpr auto fxCombDamping = "fx_comb_damping";
inline constexpr auto fxCombMix = "fx_comb_mix";
inline constexpr auto fxChorusEnabled = "fx_chorus_enabled";
inline constexpr auto fxChorusRate = "fx_chorus_rate";
inline constexpr auto fxChorusDepth = "fx_chorus_depth";
inline constexpr auto fxChorusMix = "fx_chorus_mix";
inline constexpr auto fxDelayEnabled = "fx_delay_enabled";
inline constexpr auto fxDelaySync = "fx_delay_sync";
inline constexpr auto fxDelayRate = "fx_delay_rate";
inline constexpr auto fxDelayTime = "fx_delay_time";
inline constexpr auto fxDelayFeedback = "fx_delay_feedback";
inline constexpr auto fxDelayMix = "fx_delay_mix";
inline constexpr auto fxSendDelay = "fx_send_delay";
inline constexpr auto fxReverbEnabled = "fx_reverb_enabled";
inline constexpr auto fxReverbSize = "fx_reverb_size";
inline constexpr auto fxReverbDamping = "fx_reverb_damping";
inline constexpr auto fxReverbMix = "fx_reverb_mix";
inline constexpr auto fxSendReverb = "fx_send_reverb";
inline constexpr auto fxSendTailKill = "fx_send_tail_kill";
inline constexpr auto fxWidthEnabled = "fx_width_enabled";
inline constexpr auto fxWidthAmount = "fx_width_amount";
inline constexpr auto fxWidthMonoCutoff = "fx_width_mono_cutoff";
inline constexpr auto fxToneEnabled = "fx_tone_enabled";
inline constexpr auto fxToneTilt = "fx_tone_tilt";
inline constexpr auto fxToneLowCut = "fx_tone_low_cut";
inline constexpr auto fxEqEnabled = "fx_eq_enabled";
inline constexpr auto fxEqLowGain = "fx_eq_low_gain";
inline constexpr auto fxEqMidGain = "fx_eq_mid_gain";
inline constexpr auto fxEqHighGain = "fx_eq_high_gain";
inline constexpr auto fxEqTrim = "fx_eq_trim";
inline constexpr auto fxPhaserEnabled = "fx_phaser_enabled";
inline constexpr auto fxPhaserRate = "fx_phaser_rate";
inline constexpr auto fxPhaserDepth = "fx_phaser_depth";
inline constexpr auto fxPhaserMix = "fx_phaser_mix";
inline constexpr auto fxGuardEnabled = "fx_guard_enabled";
inline constexpr auto fxGuardPush = "fx_guard_push";
inline constexpr auto fxGuardCeiling = "fx_guard_ceiling";
inline constexpr auto fxGuardGlue = "fx_guard_glue";
inline constexpr auto fxGuardPunch = "fx_guard_punch";
inline constexpr auto fxGuardClipMix = "fx_guard_clip_mix";
inline constexpr auto fxFlangerEnabled = "fx_flanger_enabled";
inline constexpr auto fxFlangerRate = "fx_flanger_rate";
inline constexpr auto fxFlangerDepth = "fx_flanger_depth";
inline constexpr auto fxFlangerFeedback = "fx_flanger_feedback";
inline constexpr auto fxFlangerMix = "fx_flanger_mix";
inline constexpr std::array<const char*, 15> fxOrder {
    "fx_order_1",
    "fx_order_2",
    "fx_order_3",
    "fx_order_4",
    "fx_order_5",
    "fx_order_6",
    "fx_order_7",
    "fx_order_8",
    "fx_order_9",
    "fx_order_10",
    "fx_order_11",
    "fx_order_12",
    "fx_order_13",
    "fx_order_14",
    "fx_order_15"
};
}

juce::StringArray waveformChoices();
juce::String customWaveMorphFrameParameterID(bool oscillator2, size_t frameIndex, size_t pointIndex);
juce::StringArray filterModeChoices();
juce::StringArray filterCharacterChoices();
juce::StringArray filterSlopeChoices();
juce::StringArray noiseTypeChoices();
juce::StringArray oscWarpModeChoices();
juce::StringArray randomRecipeChoices();
juce::StringArray sequencerRateChoices();
juce::StringArray sequencerGrooveModeChoices();
juce::StringArray sequencerScaleChoices();
juce::StringArray sequencerChordModeChoices();
juce::StringArray sequencerChordVoicingChoices();
juce::StringArray sequencerLockDestinationChoices();
juce::StringArray sampleRecordSourceChoices();
juce::StringArray sampleRecordStartChoices();
juce::StringArray sampleRecordLengthChoices();
juce::StringArray sampleRecordPreRollChoices();
juce::StringArray sampleEngineModeChoices();
juce::StringArray sampleSliceStyleChoices();
juce::StringArray lfoShapeChoices();
juce::StringArray lfo2ShapeChoices();
juce::StringArray lfoSyncRateChoices();
juce::StringArray delayRateChoices();
juce::StringArray pumpCurveChoices();
juce::StringArray modulationSourceChoices();
juce::StringArray modulationDestinationChoices();
juce::StringArray modulationRoutePolarityChoices();
juce::StringArray modulationRouteCurveChoices();
APVTS::ParameterLayout createLayout();
}
