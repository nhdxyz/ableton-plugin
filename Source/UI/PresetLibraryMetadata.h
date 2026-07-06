#pragma once

#include "../PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace UI::PresetLibraryMetadata
{
enum class PresetAuditionRole
{
    bass,
    chord,
    chop,
    sequence,
    fx,
    lead,
    pluck,
    general
};

juce::String searchText(const NateVSTAudioProcessor::PresetInfo& preset);
bool matchesSmartCrate(const NateVSTAudioProcessor::PresetInfo& preset, const juce::String& crate);

juce::String auditionRoleName(PresetAuditionRole role);
PresetAuditionRole inferAuditionRole(const NateVSTAudioProcessor::PresetInfo* preset);
juce::String inspectorRoleName(PresetAuditionRole role);
juce::String tempoBand(int bpm);
juce::String inspectorTraits(const NateVSTAudioProcessor::PresetInfo& preset);
juce::String inspectorCue(const NateVSTAudioProcessor::PresetInfo& preset);
std::array<float, 4> inspectorProfile(const NateVSTAudioProcessor::PresetInfo& preset);

juce::String macroPreviewText(const NateVSTAudioProcessor::PresetInfo& preset);
juce::String previewSummaryText(const NateVSTAudioProcessor::PresetInfo& preset);
juce::String previewBadgeText(const NateVSTAudioProcessor::PresetInfo& preset);
float previewLevelNormalised(const NateVSTAudioProcessor::PresetInfo& preset);
void drawPreviewLevelBadge(juce::Graphics& g,
                           juce::Rectangle<int> area,
                           const NateVSTAudioProcessor::PresetInfo& preset);
}
