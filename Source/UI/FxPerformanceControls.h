#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class FxPerformanceControls final : public juce::Component
{
public:
    enum class Action
    {
        delayThrow,
        spaceThrow,
        pumpDrop,
        dryReset,
        holdDelay,
        holdSpace,
        holdPump,
        muteDrop
    };

    FxPerformanceControls();

    std::function<void(Action)> onActionClicked;
    std::function<void(Action, bool)> onMomentaryActionChanged;

    void resized() override;

private:
    juce::TextButton delayThrowButton { "Delay Throw" };
    juce::TextButton spaceThrowButton { "Space Throw" };
    juce::TextButton pumpDropButton { "Pump Drop" };
    juce::TextButton dryResetButton { "Dry Reset" };
    juce::TextButton holdDelayButton { "Hold Dly" };
    juce::TextButton holdSpaceButton { "Hold Spc" };
    juce::TextButton holdPumpButton { "Hold Pump" };
    juce::TextButton muteDropButton { "Mute Drop" };

    void configureClickButton(juce::TextButton& button, Action action, const juce::String& tooltip);
    void configureHoldButton(juce::TextButton& button, Action action, const juce::String& tooltip);
};
}
