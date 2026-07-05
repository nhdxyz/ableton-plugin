#pragma once

#include "SampleRecorderPanel.h"

#include <juce_core/juce_core.h>

class NateVSTAudioProcessor;

namespace UI::SampleRecorderState
{
juce::File resolveExportFile(const NateVSTAudioProcessor& processor);
SampleRecorderPanel::State buildPanelState(const NateVSTAudioProcessor& processor,
                                           const juce::File& exportFile,
                                           int recentTakeLimit = 12);
juce::String recordStartedStatus(const NateVSTAudioProcessor& processor);
}
