#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace Parameters
{
using APVTS = juce::AudioProcessorValueTreeState;

namespace ID
{
inline constexpr auto oscWave = "osc_wave";
inline constexpr auto oscOctave = "osc_octave";
inline constexpr auto oscTune = "osc_tune";
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
inline constexpr auto randomAmount = "random_amount";
inline constexpr auto randomChaos = "random_chaos";
inline constexpr auto randomBrightnessBias = "random_brightness_bias";
inline constexpr auto randomDriveBias = "random_drive_bias";
inline constexpr auto randomMotionBias = "random_motion_bias";
inline constexpr auto randomRecipe = "random_recipe";
inline constexpr auto sampleEnabled = "sample_enabled";
inline constexpr auto sampleStart = "sample_start";
inline constexpr auto sampleEnd = "sample_end";
inline constexpr auto sampleReverse = "sample_reverse";
inline constexpr auto sampleTranspose = "sample_transpose";
inline constexpr auto sampleGain = "sample_gain";
inline constexpr auto sampleMix = "sample_mix";
inline constexpr auto sequencerEnabled = "sequencer_enabled";
inline constexpr auto sequencerRate = "sequencer_rate";
inline constexpr auto sequencerRoot = "sequencer_root";
inline constexpr auto sequencerGate = "sequencer_gate";
inline constexpr auto sequencerRandomAmount = "sequencer_random_amount";
inline constexpr auto fxDistortionEnabled = "fx_distortion_enabled";
inline constexpr auto fxDistortionAmount = "fx_distortion_amount";
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
}

juce::StringArray waveformChoices();
juce::StringArray filterModeChoices();
juce::StringArray randomRecipeChoices();
juce::StringArray sequencerRateChoices();
APVTS::ParameterLayout createLayout();
}
