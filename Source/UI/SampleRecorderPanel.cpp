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
    progress.setTextToDisplay("0 / 8s");
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

    applyTheme(themeFor(ThemeId::darkClub));
}

void SampleRecorderPanel::applyTheme(const Theme& theme)
{
    recordLabel.setColour(juce::Label::textColourId, theme.accent);
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
    const auto progressAmount = juce::jlimit(0.0, 1.0, static_cast<double>(seconds / capacitySeconds));
    const auto captureIsRolling = progressAmount >= 0.995;
    const auto captureSourceShortName = state.captureSourceIndex == 1 ? juce::String("Host In")
                                                                      : juce::String("Post-FX");
    const auto captureSourceDescription = state.captureSourceIndex == 1
        ? juce::String("host input routed by Ableton into the plugin")
        : juce::String("post-FX plugin output");

    recordButton.setButtonText(state.isRecording ? "Stop" : "Record");
    recordButton.setColour(juce::TextButton::buttonColourId,
                           state.isRecording ? juce::Colour(0xff4a2725) : juce::Colour(0xff141d20));
    recordButton.setColour(juce::TextButton::textColourOffId,
                           state.isRecording ? juce::Colour(0xffff9a8a) : juce::Colour(0xffdce7e4));

    const auto capacityText = juce::String(capacitySeconds, 0) + "s";
    const auto durationText = hasCapture ? juce::String(seconds, 1) + " / " + capacityText
                                         : juce::String("empty / ") + capacityText;
    progressValue = progressAmount;
    progress.setTextToDisplay(durationText);
    progress.setColour(juce::ProgressBar::foregroundColourId,
                       state.isRecording ? juce::Colour(0xffff9a8a)
                                         : hasCapture ? juce::Colour(0xff8ee6c9)
                                                      : juce::Colour(0xff40535a));
    progress.setTooltip("Recorder rolling-buffer fill: " + durationText
                        + (captureIsRolling ? ". New audio is replacing the oldest audio." : "."));

    const auto statusText = state.isRecording
        ? (captureIsRolling ? juce::String("Recording ") + captureSourceShortName + " | Rolling"
                            : juce::String("Recording ") + captureSourceShortName + " | " + durationText)
        : hasCapture ? (captureIsRolling ? juce::String("Ready ") + captureSourceShortName + " | Last " + capacityText
                                         : juce::String("Ready ") + captureSourceShortName + " | " + durationText)
                     : state.hasLoadedSample ? juce::String("Loaded sample ready")
                                             : juce::String("Record or Load a sample");
    statusLabel.setText(statusText, juce::dontSendNotification);
    statusLabel.setTooltip(state.captureSourceIndex == 1
        ? "Recorder source: Host Input. Route a mic, track, or sidechain source to the plugin input in Ableton."
        : "Recorder source: Post-FX Output. Captures synth + sampler through this plugin's FX and output gain.");
    statusLabel.setColour(juce::Label::textColourId,
                          state.isRecording ? juce::Colour(0xffff9a8a)
                                            : hasCapture ? juce::Colour(0xff8ee6c9)
                                                         : state.hasLoadedSample ? juce::Colour(0xffa8d8ff)
                                                                                 : juce::Colour(0xffa8b6b8));

    const std::array<bool, 4> stepActive {
        state.isRecording,
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
        state.isRecording ? juce::String("Recording ") + captureSourceDescription + " into the rolling buffer"
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
    commitButton.setTooltip(hasCapture ? "Commit the captured " + durationText + " snippet into the sampler and auto-trim silence"
                                       : "Record audio before committing a sampler snippet");
    recordButton.setTooltip(state.isRecording ? "Stop recording " + captureSourceDescription
                                              : "Start recording " + captureSourceDescription);
    sourceBox.setTooltip(state.captureSourceIndex == 1
        ? "Host Input records audio Ableton routes into the plugin input. Select the mic or source in Ableton, then route it to this plugin."
        : "Post-FX Output records this plugin after synth, sampler, FX rack, and output gain.");
    autoTrimButton.setTooltip(state.hasLoadedSample ? "Trim the current sample range to the audible part of the recording"
                                                    : "Load or commit a sample before trimming");
    spliceButton.setTooltip(state.hasLoadedSample ? "Detect transients or split the recording into eight playable slice keys"
                                                  : "Load or commit a sample before slicing");
    mangleButton.setTooltip(state.hasLoadedSample ? "Randomize trim, slices, pitch, stutter, pan, probability, and safe FX movement"
                                                  : "Load or commit a sample before mangling");
}

void SampleRecorderPanel::resized()
{
    auto area = getLocalBounds();
    auto recorderHeaderRow = area.removeFromTop(30);
    recordLabel.setBounds(recorderHeaderRow.removeFromLeft(82).withTrimmedLeft(4).reduced(0, 5));
    sourceBox.setBounds(recorderHeaderRow.reduced(3, 4));

    auto recorderStepArea = area.removeFromTop(24).reduced(4, 3);
    const auto recorderStepWidth = recorderStepArea.getWidth() / static_cast<int>(stepLabels.size());
    for (auto& label : stepLabels)
        label.setBounds(recorderStepArea.removeFromLeft(recorderStepWidth).reduced(2, 0));

    auto recorderStatusArea = area.removeFromTop(34).reduced(5, 2);
    statusLabel.setBounds(recorderStatusArea.removeFromTop(14));
    progress.setBounds(recorderStatusArea);

    auto recordRow = area.removeFromTop(38).withTrimmedTop(2);
    const auto recordButtonWidth = juce::jmax(86, recordRow.getWidth() * 2 / 5);
    const auto commitButtonWidth = juce::jmax(70, recordRow.getWidth() * 3 / 10);
    recordButton.setBounds(recordRow.removeFromLeft(recordButtonWidth).reduced(3, 4));
    commitButton.setBounds(recordRow.removeFromLeft(commitButtonWidth).reduced(3, 4));
    auditionButton.setBounds(recordRow.reduced(3, 4));

    auto recordToolRow = area.removeFromTop(32).withTrimmedTop(1);
    const auto recordToolWidth = recordToolRow.getWidth() / 3;
    autoTrimButton.setBounds(recordToolRow.removeFromLeft(recordToolWidth).reduced(3, 4));
    spliceButton.setBounds(recordToolRow.removeFromLeft(recordToolWidth).reduced(3, 4));
    mangleButton.setBounds(recordToolRow.reduced(3, 4));
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
}
