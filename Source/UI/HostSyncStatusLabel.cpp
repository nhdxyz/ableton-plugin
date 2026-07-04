#include "HostSyncStatusLabel.h"

namespace UI
{
HostSyncStatusLabel::HostSyncStatusLabel()
{
    setComponentID("HostSyncStatusLabel");
    setFont(juce::FontOptions(11.0f, juce::Font::bold));
    setJustificationType(juce::Justification::centred);
    setMinimumHorizontalScale(0.72f);
    applyTheme(themeFor(ThemeId::darkClub));
}

void HostSyncStatusLabel::applyTheme(const Theme& theme)
{
    setColour(juce::Label::textColourId, theme.textDim);
    setColour(juce::Label::backgroundColourId, theme.panel.withAlpha(0.13f));
    setColour(juce::Label::outlineColourId, theme.outline);
    setHostSyncStatus(currentState, currentBpm, currentPpqPosition);
}

void HostSyncStatusLabel::setHostSyncStatus(State state, int bpm, double ppqPosition)
{
    currentState = state;
    currentBpm = bpm;
    currentPpqPosition = ppqPosition;

    juce::String text;
    juce::String tooltip;
    auto textColour = juce::Colour(0xff7d8b90);
    auto background = juce::Colour(0x22141a1d);
    auto outline = juce::Colour(0xff263035);

    switch (state)
    {
        case State::lockedPlaying:
            text = "LOCK " + juce::String(bpm) + " | PLAY";
            tooltip = "Host BPM and PPQ are available. SEQ, Pump, Tremolo, and synced Delay can follow Ableton transport phase. PPQ "
                + juce::String(ppqPosition, 2);
            textColour = juce::Colour(0xff8ee6c9);
            background = juce::Colour(0x243bcfa7);
            outline = juce::Colour(0xff3bcfa7);
            break;

        case State::hostNoPpq:
            text = "HOST " + juce::String(bpm) + " | NO PPQ";
            tooltip = "Host tempo is available, but PPQ phase is not. Tempo-synced movement uses internal phase fallback.";
            textColour = juce::Colour(0xffffc29a);
            background = juce::Colour(0x22ff8a4d);
            outline = juce::Colour(0xff705846);
            break;

        case State::hostStopped:
            text = "HOST " + juce::String(bpm) + " | STOP";
            tooltip = "Host tempo is available and transport is stopped. The sequencer waits; tempo-synced FX can audition from internal phase.";
            textColour = juce::Colour(0xffffc29a);
            background = juce::Colour(0x22ff8a4d);
            outline = juce::Colour(0xff705846);
            break;

        case State::free:
        default:
            text = "INT " + juce::String(bpm) + " | FREE";
            tooltip = "No host tempo or transport position has reached the audio engine yet. Nate VST is using its internal fallback tempo.";
            break;
    }

    setText(text, juce::dontSendNotification);
    setTooltip(tooltip);
    setColour(juce::Label::textColourId, textColour);
    setColour(juce::Label::backgroundColourId, background);
    setColour(juce::Label::outlineColourId, outline);
}
}
