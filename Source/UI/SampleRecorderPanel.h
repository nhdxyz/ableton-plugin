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
        float capacitySeconds = 16.0f;
        bool hasLoadedSample = false;
        int captureSourceIndex = 0;
        int captureStartModeIndex = 0;
        int captureLengthModeIndex = 0;
        int capturePreRollModeIndex = 0;
        float captureSourcePeak = 0.0f;
        float targetSeconds = 0.0f;
        float preRollSeconds = 0.0f;
        bool waitingForThreshold = false;
        bool hasExportableTake = false;
        int takeCount = 0;
        int selectedTakeIndex = 0;
        juce::String exportTakeName;
        juce::StringArray recentTakeNames;
    };

    explicit SampleRecorderPanel(juce::AudioProcessorValueTreeState& valueTreeState);

    std::function<void()> onRecordClicked;
    std::function<void()> onCommitClicked;
    std::function<void()> onAuditionClicked;
    std::function<void()> onAutoTrimClicked;
    std::function<void()> onSpliceClicked;
    std::function<void()> onMangleClicked;
    std::function<void()> onExportClicked;
    std::function<bool(juce::Component&)> onExportDragged;
    std::function<void(int)> onTakeSelected;

    void applyTheme(const Theme& theme);
    void setState(const State& state);
    void resized() override;

    int preferredHeight() const noexcept { return 212; }
    juce::StringArray runLayoutAudit(const juce::String& panelName,
                                     bool hasCapture,
                                     bool hasLoadedSample) const;

private:
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    class TakeExportButton final : public juce::TextButton
    {
    public:
        using juce::TextButton::TextButton;

        std::function<bool(juce::Component&)> onExternalDrag;

        void mouseDown(const juce::MouseEvent& event) override
        {
            externalDragStarted = false;
            juce::TextButton::mouseDown(event);
        }

        void mouseDrag(const juce::MouseEvent& event) override
        {
            if (! externalDragStarted && event.getDistanceFromDragStart() >= 5)
            {
                externalDragStarted = true;
                if (onExternalDrag != nullptr && onExternalDrag(*this))
                    return;
            }

            if (! externalDragStarted)
                juce::TextButton::mouseDrag(event);
        }

        void mouseUp(const juce::MouseEvent& event) override
        {
            const auto consumedByExternalDrag = externalDragStarted;
            externalDragStarted = false;
            if (! consumedByExternalDrag)
                juce::TextButton::mouseUp(event);
        }

    private:
        bool externalDragStarted = false;
    };

    juce::Label recordLabel;
    juce::ComboBox sourceBox;
    juce::ComboBox startBox;
    juce::ComboBox lengthBox;
    juce::ComboBox preRollBox;
    juce::ComboBox takeBox;
    juce::Label routeHintLabel;
    juce::Label statusLabel;
    juce::Label takeHistoryLabel;
    std::array<juce::Label, 4> stepLabels;
    double progressValue = 0.0;
    juce::ProgressBar progress { progressValue };
    juce::TextButton recordButton { "Record" };
    juce::TextButton commitButton { "Commit" };
    juce::TextButton auditionButton { "Play" };
    juce::TextButton autoTrimButton { "Trim" };
    juce::TextButton spliceButton { "Splice" };
    juce::TextButton mangleButton { "Mangle" };
    TakeExportButton exportButton { "Export" };
    std::unique_ptr<ComboBoxAttachment> sourceAttachment;
    std::unique_ptr<ComboBoxAttachment> startAttachment;
    std::unique_ptr<ComboBoxAttachment> lengthAttachment;
    std::unique_ptr<ComboBoxAttachment> preRollAttachment;
    juce::StringArray displayedTakeNames;
    bool updatingTakeBox = false;

    static juce::String componentAuditName(const juce::Component& component,
                                           const juce::String& fallback);
    static juce::String formatPeakLabel(float peak);
    static bool hasAudiblePeak(float peak) noexcept;
    static juce::String startModeShortName(int modeIndex);
    static juce::String lengthModeShortName(int modeIndex);
    static juce::String preRollModeShortName(int modeIndex);
};
}
