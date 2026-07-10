#pragma once

#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class SampleChopPanel final : public juce::Component
{
public:
    struct ActionState
    {
        bool reverse = false;
        bool choke = false;
        bool ghost = false;
        bool nudged = false;
        bool faded = false;
        bool recallEnabled = false;
    };

    SampleChopPanel();

    std::function<void(size_t)> onSliceSelected;
    std::function<void()> onStoreClicked;
    std::function<void()> onRecallClicked;
    std::function<void()> onGridClicked;
    std::function<void()> onDetectClicked;
    std::function<void()> onClearClicked;
    std::function<void()> onDiceClicked;
    std::function<void()> onReverseClicked;
    std::function<void()> onChokeClicked;
    std::function<void()> onPanClicked;
    std::function<void()> onGhostClicked;
    std::function<void()> onNudgeClicked;
    std::function<void()> onFadeClicked;
    std::function<void()> onSeqClicked;
    std::function<void()> onWtClicked;

    void applyTheme(const Theme& theme);
    void setSliceButtonState(size_t sliceIndex,
                             const juce::String& text,
                             const juce::String& tooltip,
                             bool selected);
    void setStatus(const juce::String& text, const juce::String& tooltip);
    void setActionState(const ActionState& state);
    void resized() override;

    size_t getSliceCount() const noexcept { return sliceButtons.size(); }
    int compactHeight() const noexcept { return 140; }
    int focusHeight() const noexcept { return 112; }

private:
    juce::Label statusLabel;
    std::array<juce::TextButton, 8> sliceButtons;
    juce::TextButton storeButton { "Store" };
    juce::TextButton recallButton { "Recall" };
    juce::TextButton gridButton { "Grid" };
    juce::TextButton detectButton { "Detect" };
    juce::TextButton clearButton { "Clear" };
    juce::TextButton diceButton { "Dice" };
    juce::TextButton reverseButton { "Reverse" };
    juce::TextButton chokeButton { "Choke" };
    juce::TextButton panButton { "Pan" };
    juce::TextButton ghostButton { "Ghost" };
    juce::TextButton nudgeButton { "Nudge" };
    juce::TextButton fadeButton { "Fade" };
    juce::TextButton seqButton { "To Seq" };
    juce::TextButton wtButton { "To WT" };

    std::array<juce::TextButton*, 14> actionButtons() noexcept;
    std::array<const juce::TextButton*, 14> actionButtons() const noexcept;
    std::array<juce::TextButton*, 7> setupButtons() noexcept;
    std::array<juce::TextButton*, 7> performanceButtons() noexcept;
};
}
