#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class SequencerStepEditor final : public juce::Component
{
public:
    enum class Action
    {
        noteDown,
        noteUp,
        octaveDown,
        octaveUp,
        velocityDown,
        velocityUp,
        probabilityDown,
        probabilityUp,
        lengthDown,
        lengthUp,
        ratchet,
        slide,
        lock
    };

    SequencerStepEditor();

    std::function<void(Action)> onAction;

    void setNoSelection();
    void setSelectedStep(int stepIndex,
                         const juce::String& noteName,
                         bool stepEnabled,
                         int velocityPercent,
                         int probabilityPercent,
                         int lengthPercent,
                         int ratchet,
                         bool slide,
                         int lockPercent);
    void resized() override;

    int preferredHeight() const noexcept { return 104; }

private:
    static constexpr size_t buttonCount = 13;

    juce::Label statusLabel;
    std::array<juce::TextButton, buttonCount> buttons;

    static Action actionForIndex(size_t index) noexcept;
};
}
