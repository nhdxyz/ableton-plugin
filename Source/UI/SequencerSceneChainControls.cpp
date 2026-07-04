#include "SequencerSceneChainControls.h"

namespace UI
{
SequencerSceneChainControls::SequencerSceneChainControls()
{
    setComponentID("SequencerSceneChainControls");

    liveButton.setClickingTogglesState(true);
    liveButton.setTooltip("Play captured A/B/Fill/Drop scene steps as a live 2/4-bar chain");
    liveButton.setWantsKeyboardFocus(false);
    liveButton.setMouseClickGrabsKeyboardFocus(false);
    liveButton.onClick = [this]
    {
        if (onLiveToggled != nullptr)
            onLiveToggled(liveButton.getToggleState());
    };
    addAndMakeVisible(liveButton);

    lengthButton.setTooltip("Set scene-chain MIDI/live length: Auto, forced 2 bars, or forced 4 bars");
    lengthButton.setWantsKeyboardFocus(false);
    lengthButton.setMouseClickGrabsKeyboardFocus(false);
    lengthButton.onClick = [this]
    {
        if (onLengthCycle != nullptr)
            onLengthCycle();
    };
    addAndMakeVisible(lengthButton);

    refreshButtonState();
}

void SequencerSceneChainControls::setChainState(bool liveEnabled, int liveLengthBars, int forcedLengthBars)
{
    liveChainEnabled = liveEnabled;
    liveChainLength = juce::jmax(1, liveLengthBars);
    forcedChainBars = forcedLengthBars;
    refreshButtonState();
}

void SequencerSceneChainControls::resized()
{
    refreshButtonState();

    auto area = getLocalBounds();
    const auto liveWidth = area.getWidth() < 118 ? juce::jmax(1, area.getWidth() * 58 / 100)
                                                 : juce::jmax(1, area.getWidth() / 2);
    liveButton.setBounds(area.removeFromLeft(liveWidth).reduced(2, 3));
    lengthButton.setBounds(area.reduced(2, 3));
}

void SequencerSceneChainControls::refreshButtonState()
{
    const auto compact = getWidth() > 0 && getWidth() < 118;

    liveButton.setToggleState(liveChainEnabled, juce::dontSendNotification);
    liveButton.setButtonText(liveChainEnabled
                                 ? (compact ? "L" + juce::String(liveChainLength)
                                            : "Live " + juce::String(liveChainLength))
                                 : "Live");
    liveButton.setTooltip(liveChainEnabled
                              ? "Live scene-chain playback is following captured scene steps"
                              : "Play captured A/B/Fill/Drop scene steps as a live 2/4-bar chain");

    lengthButton.setButtonText(forcedChainBars == 0
                                   ? "Auto"
                                   : (compact ? juce::String(forcedChainBars) + "B"
                                              : juce::String(forcedChainBars) + " Bar"));
    lengthButton.setTooltip(forcedChainBars == 0
                                ? "Auto chain length follows captured scenes; click for forced 2 bars"
                                : "Forced " + juce::String(forcedChainBars) + "-bar scene-chain MIDI/live length; click to cycle");
}
}
