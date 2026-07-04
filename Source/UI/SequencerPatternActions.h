#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class SequencerPatternActions final : public juce::Component
{
public:
    enum class Action
    {
        apply,
        random,
        mutate,
        undo,
        clear
    };

    SequencerPatternActions();

    std::function<void(Action)> onActionClicked;

    static constexpr int preferredHeight = 68;

    void resized() override;

private:
    juce::TextButton applyButton { "Apply" };
    juce::TextButton randomButton { "Rand Seq" };
    juce::TextButton mutateButton { "Vary" };
    juce::TextButton undoButton { "Undo" };
    juce::TextButton clearButton { "Clear" };

    void configureButton(juce::TextButton& button, Action action, const juce::String& tooltip);
};
}
