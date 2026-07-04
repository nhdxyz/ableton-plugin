#include "SampleRecorderPanel.h"

namespace UI
{
SampleRecorderPanel::SampleRecorderPanel(juce::AudioProcessorValueTreeState& valueTreeState)
{
    recordLabel.setText("RECORDER", juce::dontSendNotification);
    recordLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    recordLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(recordLabel);

    sourceBox.addItemList(Parameters::sampleRecordSourceChoices(), 1);
    sourceBox.setTextWhenNothingSelected("Source");
    sourceBox.setTooltip("Choose what Record captures: Post-FX Output records this plugin after FX/output gain; Host Input records audio Ableton routes into the plugin input, such as a mic or sidechain source");
    addAndMakeVisible(sourceBox);
    sourceAttachment = std::make_unique<ComboBoxAttachment>(valueTreeState,
                                                            Parameters::ID::sampleRecordSource,
                                                            sourceBox);

    startBox.addItemList(Parameters::sampleRecordStartChoices(), 1);
    startBox.setTextWhenNothingSelected("Start");
    startBox.setTooltip("Choose when recording begins: Immediate records at once; Detect waits until the selected source crosses the dB threshold");
    addAndMakeVisible(startBox);
    startAttachment = std::make_unique<ComboBoxAttachment>(valueTreeState,
                                                           Parameters::ID::sampleRecordStart,
                                                           startBox);

    lengthBox.addItemList(Parameters::sampleRecordLengthChoices(), 1);
    lengthBox.setTextWhenNothingSelected("Length");
    lengthBox.setTooltip("Choose fixed capture length. Free records until you stop; bar lengths auto-stop from the host tempo.");
    addAndMakeVisible(lengthBox);
    lengthAttachment = std::make_unique<ComboBoxAttachment>(valueTreeState,
                                                            Parameters::ID::sampleRecordLength,
                                                            lengthBox);

    preRollBox.addItemList(Parameters::sampleRecordPreRollChoices(), 1);
    preRollBox.setTextWhenNothingSelected("Pre");
    preRollBox.setTooltip("Keep a short lookback while threshold-armed so the captured sample includes the attack before the trigger.");
    addAndMakeVisible(preRollBox);
    preRollAttachment = std::make_unique<ComboBoxAttachment>(valueTreeState,
                                                             Parameters::ID::sampleRecordPreRoll,
                                                             preRollBox);

    routeHintLabel.setText("Post-FX internal capture", juce::dontSendNotification);
    routeHintLabel.setJustificationType(juce::Justification::centredLeft);
    routeHintLabel.setFont(juce::FontOptions(10.5f));
    routeHintLabel.setMinimumHorizontalScale(0.58f);
    routeHintLabel.setTooltip("Recorder route and monitor safety hint");
    addAndMakeVisible(routeHintLabel);

    statusLabel.setText("Recorder idle", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setFont(juce::FontOptions(11.0f));
    statusLabel.setMinimumHorizontalScale(0.62f);
    statusLabel.setTooltip("Rolling sampler recorder status and captured duration");
    addAndMakeVisible(statusLabel);

    const std::array<juce::String, 4> recorderStepTexts { "REC", "READY", "USE", "PLAY" };
    for (size_t index = 0; index < stepLabels.size(); ++index)
    {
        auto& label = stepLabels[index];
        label.setText(recorderStepTexts[index], juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        label.setMinimumHorizontalScale(0.72f);
        label.setTooltip("Recorder flow state");
        addAndMakeVisible(label);
    }

    progress.setPercentageDisplay(false);
    progress.setTextToDisplay("0 / 16s");
    progress.setTooltip("Recorder rolling-buffer fill. When it reaches the end, the newest audio replaces the oldest audio.");
    addAndMakeVisible(progress);

    recordButton.setButtonText("Record");
    recordButton.setTooltip("Start or stop recording the selected recorder source into a short rolling sampler buffer");
    recordButton.onClick = [this]
    {
        if (onRecordClicked != nullptr)
            onRecordClicked();
    };
    addAndMakeVisible(recordButton);

    commitButton.setButtonText("Commit");
    commitButton.setTooltip("Commit the recorded snippet into the sampler waveform and auto-trim silence");
    commitButton.onClick = [this]
    {
        if (onCommitClicked != nullptr)
            onCommitClicked();
    };
    addAndMakeVisible(commitButton);

    auditionButton.setTooltip("Audition the loaded or recorded sampler source");
    auditionButton.onClick = [this]
    {
        if (onAuditionClicked != nullptr)
            onAuditionClicked();
    };
    addAndMakeVisible(auditionButton);

    autoTrimButton.setTooltip("Trim the current sample range to the audible part of the recording");
    autoTrimButton.onClick = [this]
    {
        if (onAutoTrimClicked != nullptr)
            onAutoTrimClicked();
    };
    addAndMakeVisible(autoTrimButton);

    spliceButton.setTooltip("Detect transients or split the recording into eight playable slice keys");
    spliceButton.onClick = [this]
    {
        if (onSpliceClicked != nullptr)
            onSpliceClicked();
    };
    addAndMakeVisible(spliceButton);

    mangleButton.setTooltip("Randomize the recorded sample's trim, slices, pitch, stutter, pan, probability, and safe FX movement");
    mangleButton.onClick = [this]
    {
        if (onMangleClicked != nullptr)
            onMangleClicked();
    };
    addAndMakeVisible(mangleButton);

    exportButton.setButtonText("Export");
    exportButton.setTooltip("Click to reveal the latest committed recorder take, or drag the WAV into Ableton");
    exportButton.onClick = [this]
    {
        if (onExportClicked != nullptr)
            onExportClicked();
    };
    exportButton.onExternalDrag = [this] (juce::Component& sourceComponent)
    {
        return onExportDragged != nullptr && onExportDragged(sourceComponent);
    };
    addAndMakeVisible(exportButton);

    applyTheme(themeFor(ThemeId::darkClub));
}

void SampleRecorderPanel::applyTheme(const Theme& theme)
{
    recordLabel.setColour(juce::Label::textColourId, theme.accent);
    routeHintLabel.setColour(juce::Label::textColourId, theme.textDim);
    routeHintLabel.setColour(juce::Label::backgroundColourId, theme.field.withAlpha(0.78f));
    routeHintLabel.setColour(juce::Label::outlineColourId, theme.outline);
    statusLabel.setColour(juce::Label::textColourId, theme.textMuted);
    progress.setColour(juce::ProgressBar::backgroundColourId, theme.field);
    progress.setColour(juce::TextButton::textColourOffId, theme.textMuted);

    for (auto& label : stepLabels)
    {
        label.setColour(juce::Label::textColourId, theme.textDim);
        label.setColour(juce::Label::backgroundColourId, theme.field);
        label.setColour(juce::Label::outlineColourId, theme.outline);
    }
}

void SampleRecorderPanel::setState(const State& state)
{
    const auto capacitySeconds = juce::jmax(0.1f, state.capacitySeconds);
    const auto seconds = juce::jmax(0.0f, state.seconds);
    const auto hasCapture = seconds >= 0.05f;
    const auto captureSourceShortName = state.captureSourceIndex == 1 ? juce::String("Host In")
                                                                      : juce::String("Post-FX");
    const auto captureSourceDescription = state.captureSourceIndex == 1
        ? juce::String("host input routed by Ableton into the plugin")
        : juce::String("post-FX plugin output");
    const auto captureStartShortName = startModeShortName(state.captureStartModeIndex);
    const auto captureLengthShortName = lengthModeShortName(state.captureLengthModeIndex);
    const auto capturePreRollShortName = preRollModeShortName(state.capturePreRollModeIndex);
    const auto sourceLevelText = formatPeakLabel(state.captureSourcePeak);
    const auto takeSummary = state.takeCount > 0 ? juce::String(" | Take ") + juce::String(state.takeCount)
                                                 : juce::String();
    const auto fixedTarget = state.targetSeconds > 0.05f;
    const auto hasPreRoll = state.preRollSeconds > 0.001f;
    const auto progressTargetSeconds = fixedTarget ? juce::jmin(capacitySeconds, juce::jmax(0.1f, state.targetSeconds))
                                                   : capacitySeconds;
    const auto rollingProgressAmount = juce::jlimit(0.0, 1.0, static_cast<double>(seconds / capacitySeconds));
    const auto captureIsRolling = ! fixedTarget && rollingProgressAmount >= 0.995;

    recordButton.setButtonText(state.waitingForThreshold ? "Cancel" : state.isRecording ? "Stop" : "Record");
    recordButton.setColour(juce::TextButton::buttonColourId,
                           state.isRecording ? juce::Colour(0xff4a2725) : juce::Colour(0xff141d20));
    recordButton.setColour(juce::TextButton::textColourOffId,
                           state.isRecording ? juce::Colour(0xffff9a8a) : juce::Colour(0xffdce7e4));

    const auto capacityText = juce::String(capacitySeconds, 0) + "s";
    const auto targetText = fixedTarget ? juce::String(progressTargetSeconds, 1) + "s"
                                        : capacityText;
    const auto durationText = hasCapture ? juce::String(seconds, 1) + " / " + targetText
                                         : juce::String("empty / ") + targetText;
    const auto targetDescription = fixedTarget ? juce::String(" | ") + captureLengthShortName
                                               : juce::String();
    const auto preRollDescription = hasPreRoll ? juce::String(" | ") + capturePreRollShortName
                                               : juce::String();
    progressValue = juce::jlimit(0.0, 1.0, static_cast<double>(seconds / progressTargetSeconds));
    progress.setTextToDisplay(durationText);
    progress.setColour(juce::ProgressBar::foregroundColourId,
                       state.isRecording ? juce::Colour(0xffff9a8a)
                                         : hasCapture ? juce::Colour(0xff8ee6c9)
                                                      : juce::Colour(0xff40535a));
    progress.setTooltip((fixedTarget ? juce::String("Recorder fixed-length progress: ")
                                     : juce::String("Recorder rolling-buffer fill: "))
                        + durationText
                        + (fixedTarget ? ". Recorder auto-stops at " + captureLengthShortName + "."
                                       : captureIsRolling ? ". New audio is replacing the oldest audio." : "."));

    const auto sourceHasSignal = hasAudiblePeak(state.captureSourcePeak);
    const auto routeHintText = state.captureSourceIndex == 1
        ? sourceHasSignal ? juce::String("Host Input active | Live routes track/mic into plugin")
                          : juce::String("Host Input idle | Choose sidechain/input route in Live")
        : juce::String("Post-FX internal | captures synth + sampler + FX");
    const auto monitorSafetyText = state.captureSourceIndex == 1
        ? juce::String("No input thru-monitor here; use Ableton monitoring if needed.")
        : juce::String("No external mic/input needed for this mode.");
    routeHintLabel.setText(routeHintText, juce::dontSendNotification);
    routeHintLabel.setTooltip(routeHintText + " | " + monitorSafetyText);
    routeHintLabel.setColour(juce::Label::textColourId,
                             state.captureSourceIndex == 1
                                 ? sourceHasSignal ? juce::Colour(0xff8ee6c9)
                                                   : juce::Colour(0xffffc36b)
                                 : juce::Colour(0xff9fc7ff));
    routeHintLabel.setColour(juce::Label::outlineColourId,
                             state.captureSourceIndex == 1
                                 ? sourceHasSignal ? juce::Colour(0xff3a7a68)
                                                   : juce::Colour(0xff7a5730)
                                 : juce::Colour(0xff365b7d));

    const auto statusText = state.waitingForThreshold
        ? juce::String("Armed ") + captureSourceShortName + " " + sourceLevelText + " | " + captureStartShortName + targetDescription + preRollDescription
        : state.isRecording
        ? (captureIsRolling ? juce::String("Rec ") + captureSourceShortName + " " + sourceLevelText + " | Rolling"
                            : juce::String("Rec ") + captureSourceShortName + " " + sourceLevelText + targetDescription + preRollDescription + " | " + durationText)
        : hasCapture ? (captureIsRolling ? juce::String("Ready ") + captureSourceShortName + " " + sourceLevelText + " | Last " + capacityText
                                         : juce::String("Ready ") + captureSourceShortName + " " + sourceLevelText + " | " + durationText)
                     : state.hasLoadedSample ? juce::String("Loaded | ") + captureSourceShortName + " " + sourceLevelText + takeSummary
                                             : captureSourceShortName + " " + sourceLevelText + " | Record or Load";
    statusLabel.setText(statusText, juce::dontSendNotification);
    statusLabel.setTooltip(state.captureSourceIndex == 1
        ? "Recorder source: Host Input. Route a mic, track, or sidechain source to the plugin input in Ableton. Current peak: " + sourceLevelText
        : "Recorder source: Post-FX Output. Captures synth + sampler through this plugin's FX and output gain. Current peak: " + sourceLevelText);
    statusLabel.setColour(juce::Label::textColourId,
                          state.isRecording ? juce::Colour(0xffff9a8a)
                                            : hasCapture ? juce::Colour(0xff8ee6c9)
                                                         : state.hasLoadedSample ? juce::Colour(0xffa8d8ff)
                                                                                 : juce::Colour(0xffa8b6b8));

    const std::array<juce::String, 4> stepTexts {
        state.waitingForThreshold ? juce::String("WAIT") : juce::String("REC"),
        "READY",
        "USE",
        "PLAY"
    };
    const std::array<bool, 4> stepActive {
        state.isRecording || state.waitingForThreshold,
        hasCapture,
        hasCapture,
        state.hasLoadedSample
    };
    const std::array<juce::Colour, 4> stepColours {
        juce::Colour(0xffff9a8a),
        juce::Colour(0xff8ee6c9),
        juce::Colour(0xff9fc7ff),
        juce::Colour(0xffd6bcff)
    };
    const std::array<juce::String, 4> stepTooltips {
        state.waitingForThreshold ? juce::String("Armed for ") + captureStartShortName + " on " + captureSourceDescription + preRollDescription
        : state.isRecording ? juce::String("Recording ") + captureSourceDescription + " into the rolling buffer"
                          : juce::String("Start recording ") + captureSourceDescription,
        hasCapture ? juce::String("Captured ") + durationText
                   : juce::String("Record audio to fill the capture buffer"),
        hasCapture ? "Commit the captured snippet into the sampler"
                   : juce::String("Commit becomes available after recording"),
        state.hasLoadedSample ? "Loaded sample can be auditioned and edited"
                              : juce::String("Load or commit audio before playback")
    };

    for (size_t index = 0; index < stepLabels.size(); ++index)
    {
        auto& label = stepLabels[index];
        const auto active = stepActive[index];
        const auto colour = stepColours[index];
        label.setText(stepTexts[index], juce::dontSendNotification);
        label.setColour(juce::Label::backgroundColourId,
                        active ? colour.withAlpha(0.24f) : juce::Colour(0xff101619));
        label.setColour(juce::Label::outlineColourId,
                        active ? colour.withAlpha(0.78f) : juce::Colour(0xff263238));
        label.setColour(juce::Label::textColourId,
                        active ? colour : juce::Colour(0xff7f9196));
        label.setTooltip(stepTooltips[index]);
    }

    commitButton.setEnabled(hasCapture);
    auditionButton.setEnabled(state.hasLoadedSample);
    autoTrimButton.setEnabled(state.hasLoadedSample);
    spliceButton.setEnabled(state.hasLoadedSample);
    mangleButton.setEnabled(state.hasLoadedSample);
    exportButton.setEnabled(state.hasExportableTake);
    commitButton.setTooltip(hasCapture ? "Commit the captured " + durationText + " snippet into the sampler and auto-trim silence"
                                       : "Record audio before committing a sampler snippet");
    recordButton.setTooltip(state.waitingForThreshold ? "Cancel threshold-armed recording"
                                                      : state.isRecording ? "Stop recording " + captureSourceDescription
                                                                          : "Start recording " + captureSourceDescription);
    sourceBox.setTooltip(state.captureSourceIndex == 1
        ? "Host Input records audio Ableton routes into the plugin input. Select the mic or source in Ableton, route it to this plugin, then watch the Host In level in the recorder status."
        : "Post-FX Output records this plugin after synth, sampler, FX rack, and output gain. Watch the Post-FX level in the recorder status.");
    startBox.setTooltip(state.captureStartModeIndex <= 0
        ? "Immediate starts recording as soon as you press Record."
        : "Detect arms the recorder and starts only when " + captureSourceShortName + " crosses " + captureStartShortName + ".");
    lengthBox.setTooltip(state.captureLengthModeIndex <= 0
        ? "Free records until you stop or the rolling buffer fills."
        : fixedTarget ? captureLengthShortName + " auto-stops recording from the host tempo. Current target is " + targetText + "."
                      : captureLengthShortName + " auto-stops recording from the host tempo when Record starts.");
    preRollBox.setTooltip(state.capturePreRollModeIndex <= 0
        ? "Pre-roll is off. Threshold recording starts with the trigger block."
        : capturePreRollShortName + " keeps a threshold lookback so the committed sample includes audio before the trigger.");
    autoTrimButton.setTooltip(state.hasLoadedSample ? "Trim the current sample range to the audible part of the recording"
                                                    : "Load or commit a sample before trimming");
    spliceButton.setTooltip(state.hasLoadedSample ? "Detect transients or split the recording into eight playable slice keys"
                                                  : "Load or commit a sample before slicing");
    mangleButton.setTooltip(state.hasLoadedSample ? "Randomize trim, slices, pitch, stutter, pan, probability, and safe FX movement"
                                                  : "Load or commit a sample before mangling");
    const auto exportName = state.exportTakeName.isNotEmpty() ? state.exportTakeName : juce::String("latest take");
    exportButton.setTooltip(state.hasExportableTake ? "Click to reveal " + exportName + ", or drag the WAV into Ableton"
                                                    : "Commit a recorded take before dragging/exporting a WAV");
}

void SampleRecorderPanel::resized()
{
    auto area = getLocalBounds();
    auto recorderHeaderRow = area.removeFromTop(24);
    recordLabel.setBounds(recorderHeaderRow.removeFromLeft(82).withTrimmedLeft(4).reduced(0, 5));
    sourceBox.setBounds(recorderHeaderRow.reduced(3, 2));

    auto recorderSettingsRow = area.removeFromTop(24);
    startBox.setBounds(recorderSettingsRow.removeFromLeft(recorderSettingsRow.getWidth() / 2).reduced(3, 2));
    lengthBox.setBounds(recorderSettingsRow.reduced(3, 2));

    auto recorderPreRollRow = area.removeFromTop(24);
    preRollBox.setBounds(recorderPreRollRow.reduced(3, 2));

    auto recorderRouteHintRow = area.removeFromTop(16);
    routeHintLabel.setBounds(recorderRouteHintRow.reduced(5, 1));

    auto recorderStepArea = area.removeFromTop(18).reduced(4, 2);
    const auto recorderStepWidth = recorderStepArea.getWidth() / static_cast<int>(stepLabels.size());
    for (auto& label : stepLabels)
        label.setBounds(recorderStepArea.removeFromLeft(recorderStepWidth).reduced(2, 0));

    auto recorderStatusArea = area.removeFromTop(26).reduced(5, 1);
    statusLabel.setBounds(recorderStatusArea.removeFromTop(10));
    progress.setBounds(recorderStatusArea);

    auto recordRow = area.removeFromTop(31).withTrimmedTop(1);
    const auto recordButtonWidth = juce::jmax(86, recordRow.getWidth() * 2 / 5);
    const auto commitButtonWidth = juce::jmax(70, recordRow.getWidth() * 3 / 10);
    recordButton.setBounds(recordRow.removeFromLeft(recordButtonWidth).reduced(3, 3));
    commitButton.setBounds(recordRow.removeFromLeft(commitButtonWidth).reduced(3, 3));
    auditionButton.setBounds(recordRow.reduced(3, 3));

    auto recordToolRow = area.removeFromTop(25);
    const auto recordToolWidth = recordToolRow.getWidth() / 4;
    autoTrimButton.setBounds(recordToolRow.removeFromLeft(recordToolWidth).reduced(3, 2));
    spliceButton.setBounds(recordToolRow.removeFromLeft(recordToolWidth).reduced(3, 2));
    mangleButton.setBounds(recordToolRow.removeFromLeft(recordToolWidth).reduced(3, 2));
    exportButton.setBounds(recordToolRow.reduced(3, 2));
}

juce::StringArray SampleRecorderPanel::runLayoutAudit(const juce::String& panelName,
                                                      bool hasCapture,
                                                      bool hasLoadedSample) const
{
    juce::StringArray issues;

    for (const auto& stepLabel : stepLabels)
    {
        if (! stepLabel.isVisible())
        {
            issues.add(panelName + ": recorder state rail label is hidden");
            continue;
        }

        const auto stepBounds = stepLabel.getBounds();
        if (stepBounds.getWidth() < 34 || stepBounds.getHeight() < 14)
            issues.add(panelName + ": recorder state rail label is too compressed "
                       + stepBounds.toString());
    }

    if (! progress.isVisible())
    {
        issues.add(panelName + ": recorder progress meter is hidden");
    }
    else
    {
        const auto progressBounds = progress.getBounds();
        if (progressBounds.getWidth() < 120 || progressBounds.getHeight() < 8)
            issues.add(panelName + ": recorder progress meter is too small "
                       + progressBounds.toString());
    }

    for (const auto* settingBox : {
             static_cast<const juce::ComboBox*>(&sourceBox),
             static_cast<const juce::ComboBox*>(&startBox),
             static_cast<const juce::ComboBox*>(&lengthBox),
             static_cast<const juce::ComboBox*>(&preRollBox) })
    {
        const auto bounds = settingBox->getBounds();
        if (bounds.getWidth() < 72 || bounds.getHeight() < 20)
            issues.add(panelName + ": recorder setting "
                       + componentAuditName(*settingBox, "box")
                       + " is too compressed "
                       + bounds.toString());
    }

    if (! routeHintLabel.isVisible())
    {
        issues.add(panelName + ": recorder route hint is hidden");
    }
    else
    {
        const auto routeHintBounds = routeHintLabel.getBounds();
        if (routeHintBounds.getWidth() < 120 || routeHintBounds.getHeight() < 14)
            issues.add(panelName + ": recorder route hint is too compressed "
                       + routeHintBounds.toString());
    }

    for (const auto* recorderButton : {
             static_cast<const juce::Button*>(&recordButton),
             static_cast<const juce::Button*>(&commitButton),
             static_cast<const juce::Button*>(&auditionButton) })
    {
        const auto buttonBounds = recorderButton->getBounds();
        if (buttonBounds.getWidth() < 58 || buttonBounds.getHeight() < 24)
        {
            issues.add(panelName + ": recorder primary button "
                       + componentAuditName(*recorderButton, recorderButton->getButtonText())
                       + " is too compressed "
                       + buttonBounds.toString());
        }
    }

    if (! hasCapture && commitButton.isEnabled())
        issues.add(panelName + ": recorder Commit is enabled without captured audio");

    if (! hasLoadedSample)
    {
        for (const auto* sampleEditButton : {
                 static_cast<const juce::Button*>(&auditionButton),
                 static_cast<const juce::Button*>(&autoTrimButton),
                 static_cast<const juce::Button*>(&spliceButton),
                 static_cast<const juce::Button*>(&mangleButton) })
        {
            if (sampleEditButton->isVisible() && sampleEditButton->isEnabled())
            {
                issues.add(panelName + ": sample edit action "
                           + componentAuditName(*sampleEditButton, sampleEditButton->getButtonText())
                           + " is enabled before a sample is loaded");
            }
        }
    }

    const auto exportButtonBounds = exportButton.getBounds();
    if (exportButton.isVisible()
        && (exportButtonBounds.getWidth() < 50 || exportButtonBounds.getHeight() < 21))
    {
        issues.add(panelName + ": recorder export button is too compressed "
                   + exportButtonBounds.toString());
    }

    return issues;
}

juce::String SampleRecorderPanel::componentAuditName(const juce::Component& component,
                                                     const juce::String& fallback)
{
    if (component.getName().isNotEmpty())
        return component.getName();

    if (component.getComponentID().isNotEmpty())
        return component.getComponentID();

    return fallback;
}

juce::String SampleRecorderPanel::formatPeakLabel(float peak)
{
    if (peak <= 0.000001f)
        return "-inf dB";

    const auto db = juce::Decibels::gainToDecibels(juce::jmax(peak, 0.000001f));
    if (db < -60.0f)
        return "<-60 dB";

    return juce::String(db, 0) + " dB";
}

bool SampleRecorderPanel::hasAudiblePeak(float peak) noexcept
{
    return peak > 0.001f;
}

juce::String SampleRecorderPanel::startModeShortName(int modeIndex)
{
    const auto choices = Parameters::sampleRecordStartChoices();
    if (juce::isPositiveAndBelow(modeIndex, choices.size()))
        return choices[modeIndex];

    return "Immediate";
}

juce::String SampleRecorderPanel::lengthModeShortName(int modeIndex)
{
    const auto choices = Parameters::sampleRecordLengthChoices();
    if (juce::isPositiveAndBelow(modeIndex, choices.size()))
        return choices[modeIndex];

    return "Free";
}

juce::String SampleRecorderPanel::preRollModeShortName(int modeIndex)
{
    const auto choices = Parameters::sampleRecordPreRollChoices();
    if (juce::isPositiveAndBelow(modeIndex, choices.size()))
        return choices[modeIndex];

    return "Pre Off";
}
}
