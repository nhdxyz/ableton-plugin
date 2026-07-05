#include "SampleRecorderState.h"

#include "../PluginProcessor.h"

namespace UI::SampleRecorderState
{
juce::File resolveExportFile(const NateVSTAudioProcessor& processor)
{
    const auto selectedTakeFile = juce::File(processor.getSelectedSampleCaptureTakePath());
    if (selectedTakeFile.existsAsFile())
        return selectedTakeFile;

    const auto latestTakeFile = juce::File(processor.getLatestSampleCaptureTakePath());
    if (latestTakeFile.existsAsFile())
        return latestTakeFile;

    const auto loadedSampleFile = juce::File(processor.getLoadedSamplePath());
    if (loadedSampleFile.existsAsFile())
        return loadedSampleFile;

    return {};
}

SampleRecorderPanel::State buildPanelState(const NateVSTAudioProcessor& processor,
                                           const juce::File& exportFile,
                                           int recentTakeLimit)
{
    return {
        processor.isSampleCaptureEnabled(),
        processor.getSampleCaptureDurationSeconds(),
        processor.getSampleCaptureCapacitySeconds(),
        processor.hasLoadedSample(),
        processor.getSampleCaptureSourceIndex(),
        processor.getSampleCaptureStartModeIndex(),
        processor.getSampleCaptureLengthModeIndex(),
        processor.getSampleCapturePreRollModeIndex(),
        processor.getSampleCaptureSourcePeak(),
        processor.getSampleCaptureTargetDurationSeconds(),
        processor.getSampleCapturePreRollDurationSeconds(),
        processor.isSampleCaptureWaitingForThreshold(),
        exportFile.existsAsFile(),
        processor.getSampleCaptureTakeCount(),
        processor.getSelectedSampleCaptureTakeIndex(),
        exportFile.getFileName(),
        processor.getSampleCaptureTakeNames(recentTakeLimit)
    };
}

juce::String recordStartedStatus(const NateVSTAudioProcessor& processor)
{
    const auto lengthStatus = processor.getSampleCaptureLengthModeIndex() > 0
        ? juce::String(" | ") + processor.getSampleCaptureLengthModeName()
        : juce::String();
    const auto preRollStatus = processor.getSampleCapturePreRollModeIndex() > 0
        ? juce::String(" | ") + processor.getSampleCapturePreRollModeName()
        : juce::String();

    if (processor.isSampleCaptureWaitingForThreshold())
        return "Recorder armed " + processor.getSampleCaptureStartModeName()
            + lengthStatus + preRollStatus;

    return "Recording " + processor.getSampleCaptureSourceName()
        + lengthStatus + preRollStatus;
}
}
