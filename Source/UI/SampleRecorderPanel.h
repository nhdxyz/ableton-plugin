#pragma once

#include "Theme.h"
#include "../Parameters.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>
#include <memory>

namespace UI
{
class SampleRecorderPanel final : public juce::Component
{
public:
    struct State
    {
        bool isRecording = false;
        float seconds = 0.0f;
        float capacitySeconds = 8.0f;
        bool hasLoadedSample = false;
        int captureSourceIndex = 0;
        int captureStartModeIndex = 0;
        float captureSourcePeak = 0.0f;
        bool waitingForThreshold = false;
    };

    explicit SampleRecorderPanel(juce::AudioProcessorValueTreeState& valueTreeState);

    std::function<void()> onRecordClicked;
    std::function<void()> onCommitClicked;
    std::function<void()> onAuditionClicked;
    std::function<void()> onAutoTrimClicked;
    std::function<void()> onSpliceClicked;
    std::function<void()> onMangleClicked;

    void applyTheme(const Theme& theme);
    void setState(const State& state);
    void resized() override;

    int preferredHeight() const noexcept { return 158; }
    juce::StringArray runLayoutAudit(const juce::String& panelName,
                                     bool hasCapture,
                                     bool hasLoadedSample) const;

private:
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    juce::Label recordLabel;
    juce::ComboBox sourceBox;
    juce::ComboBox startBox;
    juce::Label statusLabel;
    std::array<juce::Label, 4> stepLabels;
    double progressValue = 0.0;
    juce::ProgressBar progress { progressValue };
    juce::TextButton recordButton { "Record" };
    juce::TextButton commitButton { "Commit" };
    juce::TextButton auditionButton { "Play" };
    juce::TextButton autoTrimButton { "Trim" };
    juce::TextButton spliceButton { "Splice" };
    juce::TextButton mangleButton { "Mangle" };
    std::unique_ptr<ComboBoxAttachment> sourceAttachment;
    std::unique_ptr<ComboBoxAttachment> startAttachment;

    static juce::String componentAuditName(const juce::Component& component,
                                           const juce::String& fallback);
    static juce::String formatPeakLabel(float peak);
    static juce::String startModeShortName(int modeIndex);
};
}
