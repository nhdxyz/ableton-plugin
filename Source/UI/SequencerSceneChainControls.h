#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class SequencerSceneChainControls final : public juce::Component
{
public:
    SequencerSceneChainControls();

    std::function<void(bool)> onLiveToggled;
    std::function<void()> onLengthCycle;

    void setChainState(bool liveEnabled, int liveLengthBars, int forcedLengthBars);
    void resized() override;

private:
    juce::TextButton liveButton { "Live" };
    juce::TextButton lengthButton { "Auto" };

    bool liveChainEnabled = false;
    int liveChainLength = 1;
    int forcedChainBars = 0;

    void refreshButtonState();
};
}
