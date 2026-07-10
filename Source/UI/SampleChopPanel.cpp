#include "SampleChopPanel.h"

namespace UI
{
SampleChopPanel::SampleChopPanel()
{
    statusLabel.setText("S1 default | 0-13% | pitch 0 st | gain -6 dB", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setFont(juce::FontOptions(11.0f));
    statusLabel.setMinimumHorizontalScale(0.62f);
    statusLabel.setTooltip("Selected slice memory: store or recall region, pitch, gain, reverse, choke, and stutter edits");
    addAndMakeVisible(statusLabel);

    for (size_t index = 0; index < sliceButtons.size(); ++index)
    {
        auto& button = sliceButtons[index];
        button.setButtonText(juce::String(static_cast<int>(index + 1)));
        button.setTooltip("Select and audition sample slice " + juce::String(static_cast<int>(index + 1)));
        button.onClick = [this, index]
        {
            if (onSliceSelected != nullptr)
                onSliceSelected(index);
        };
        addAndMakeVisible(button);
    }

    storeButton.setTooltip("Store the current sample region, pitch, gain, pan, chance, reverse, choke, stutter, nudge, and fade settings into this slice");
    storeButton.onClick = [this]
    {
        if (onStoreClicked != nullptr)
            onStoreClicked();
    };

    recallButton.setTooltip("Recall this slice's stored region, pitch, gain, pan, chance, reverse, choke, stutter, nudge, and fade settings");
    recallButton.onClick = [this]
    {
        if (onRecallClicked != nullptr)
            onRecallClicked();
    };

    gridButton.setTooltip("Force the loaded sample into eight equal beat-grid Slice Keys regions");
    gridButton.onClick = [this]
    {
        if (onGridClicked != nullptr)
            onGridClicked();
    };

    detectButton.setTooltip("Detect transient starts in the loaded sample and write eight custom slice regions");
    detectButton.onClick = [this]
    {
        if (onDetectClicked != nullptr)
            onDetectClicked();
    };

    clearButton.setTooltip("Clear this slice's custom marker and reset it to the default beat-grid region");
    clearButton.onClick = [this]
    {
        if (onClearClicked != nullptr)
            onClearClicked();
    };

    diceButton.setTooltip("Create a UKG-style random edit for this slice and store it");
    diceButton.onClick = [this]
    {
        if (onDiceClicked != nullptr)
            onDiceClicked();
    };

    reverseButton.setTooltip("Toggle reverse for the selected slice and store it");
    reverseButton.onClick = [this]
    {
        if (onReverseClicked != nullptr)
            onReverseClicked();
    };

    chokeButton.setTooltip("Toggle slice-key choke for this slice, so choked slices cut each other off in Slice Keys mode");
    chokeButton.onClick = [this]
    {
        if (onChokeClicked != nullptr)
            onChokeClicked();
    };

    panButton.setTooltip("Cycle this slice between center, left, and right pan for Slice Keys playback");
    panButton.onClick = [this]
    {
        if (onPanClicked != nullptr)
            onPanClicked();
    };

    ghostButton.setTooltip("Toggle ghost probability for this slice so Slice Keys can skip it sometimes");
    ghostButton.onClick = [this]
    {
        if (onGhostClicked != nullptr)
            onGhostClicked();
    };

    nudgeButton.setTooltip("Cycle this slice timing between centered, early, and late for tighter vocal-chop placement");
    nudgeButton.onClick = [this]
    {
        if (onNudgeClicked != nullptr)
            onNudgeClicked();
    };

    fadeButton.setTooltip("Cycle this slice boundary fade between tight, soft, and smooth envelopes");
    fadeButton.onClick = [this]
    {
        if (onFadeClicked != nullptr)
            onFadeClicked();
    };

    seqButton.setTooltip("Write stored sample slices into the 16-step sequencer as slice-key notes");
    seqButton.onClick = [this]
    {
        if (onSeqClicked != nullptr)
            onSeqClicked();
    };

    wtButton.setTooltip("Convert the active sample region into the selected oscillator custom wavetable stack");
    wtButton.onClick = [this]
    {
        if (onWtClicked != nullptr)
            onWtClicked();
    };

    for (auto* button : actionButtons())
        addAndMakeVisible(*button);

    applyTheme(themeFor(ThemeId::darkClub));
}

void SampleChopPanel::applyTheme(const Theme& theme)
{
    statusLabel.setColour(juce::Label::textColourId, theme.textMuted);
}

void SampleChopPanel::setSliceButtonState(size_t sliceIndex,
                                          const juce::String& text,
                                          const juce::String& tooltip,
                                          bool selected)
{
    if (! juce::isPositiveAndBelow(sliceIndex, sliceButtons.size()))
        return;

    auto& button = sliceButtons[sliceIndex];
    button.setButtonText(text);
    button.setTooltip(tooltip);
    button.setToggleState(selected, juce::dontSendNotification);
}

void SampleChopPanel::setStatus(const juce::String& text, const juce::String& tooltip)
{
    statusLabel.setText(text, juce::dontSendNotification);
    statusLabel.setTooltip(tooltip);
}

void SampleChopPanel::setActionState(const ActionState& state)
{
    reverseButton.setToggleState(state.reverse, juce::dontSendNotification);
    chokeButton.setToggleState(state.choke, juce::dontSendNotification);
    panButton.setButtonText("Pan");
    ghostButton.setButtonText("Ghost");
    ghostButton.setToggleState(state.ghost, juce::dontSendNotification);
    nudgeButton.setButtonText("Nudge");
    nudgeButton.setToggleState(state.nudged, juce::dontSendNotification);
    fadeButton.setButtonText("Fade");
    fadeButton.setToggleState(state.faded, juce::dontSendNotification);
    recallButton.setEnabled(state.recallEnabled);
    clearButton.setEnabled(state.recallEnabled);
}

void SampleChopPanel::resized()
{
    auto area = getLocalBounds();
    const auto focusLayout = area.getHeight() <= 120 && area.getWidth() >= 780;

    auto sliceRow = area.removeFromTop(focusLayout ? 38 : 36).withTrimmedTop(2);
    const auto sliceWidth = sliceRow.getWidth() / static_cast<int>(sliceButtons.size());
    for (auto& button : sliceButtons)
        button.setBounds(sliceRow.removeFromLeft(sliceWidth).reduced(4));

    statusLabel.setBounds(area.removeFromTop(focusLayout ? 30 : 28).reduced(8, 3));

    if (focusLayout)
    {
        auto actionRow = area.removeFromTop(42).withTrimmedTop(2);
        const auto width = juce::jmax(1, actionRow.getWidth() / static_cast<int>(actionButtons().size()));
        for (auto* button : actionButtons())
            button->setBounds(actionRow.removeFromLeft(width).reduced(4, 3));
        return;
    }

    auto layoutActionRow = [] (juce::Rectangle<int> row, const auto& buttons)
    {
        const auto width = juce::jmax(1, row.getWidth() / static_cast<int>(buttons.size()));
        for (auto* button : buttons)
            button->setBounds(row.removeFromLeft(width).reduced(4, 3));
    };

    constexpr auto actionRowHeight = 38;
    layoutActionRow(area.removeFromTop(actionRowHeight).withTrimmedTop(2), setupButtons());
    layoutActionRow(area.removeFromTop(actionRowHeight).withTrimmedTop(1), performanceButtons());
}

std::array<juce::TextButton*, 14> SampleChopPanel::actionButtons() noexcept
{
    return {
        &storeButton,
        &recallButton,
        &gridButton,
        &detectButton,
        &clearButton,
        &seqButton,
        &wtButton,
        &diceButton,
        &reverseButton,
        &chokeButton,
        &panButton,
        &ghostButton,
        &nudgeButton,
        &fadeButton
    };
}

std::array<const juce::TextButton*, 14> SampleChopPanel::actionButtons() const noexcept
{
    return {
        &storeButton,
        &recallButton,
        &gridButton,
        &detectButton,
        &clearButton,
        &seqButton,
        &wtButton,
        &diceButton,
        &reverseButton,
        &chokeButton,
        &panButton,
        &ghostButton,
        &nudgeButton,
        &fadeButton
    };
}

std::array<juce::TextButton*, 7> SampleChopPanel::setupButtons() noexcept
{
    return {
        &storeButton,
        &recallButton,
        &gridButton,
        &detectButton,
        &clearButton,
        &seqButton,
        &wtButton
    };
}

std::array<juce::TextButton*, 7> SampleChopPanel::performanceButtons() noexcept
{
    return {
        &diceButton,
        &reverseButton,
        &chokeButton,
        &panButton,
        &ghostButton,
        &nudgeButton,
        &fadeButton
    };
}
}
