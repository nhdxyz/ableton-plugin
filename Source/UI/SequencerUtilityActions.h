#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class SequencerUtilityActions final : public juce::Component
{
public:
    enum class Action
    {
        copy,
        rotateLeft,
        rotateRight,
        exportMidi,
        exportChain
    };

    SequencerUtilityActions();

    std::function<void(Action)> onActionClicked;
    std::function<bool(Action, juce::Component&)> onExternalDrag;

    static constexpr int preferredHeight = 68;

    void resized() override;

private:
    class ExternalFileDragButton final : public juce::TextButton
    {
    public:
        using juce::TextButton::TextButton;

        std::function<bool(juce::Component&)> onExternalDrag;

        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;

    private:
        bool externalDragStarted = false;
    };

    juce::TextButton copyButton { "Copy" };
    juce::TextButton rotateLeftButton { "Rot <" };
    juce::TextButton rotateRightButton { "Rot >" };
    ExternalFileDragButton exportMidiButton { "MIDI" };
    ExternalFileDragButton exportChainButton { "Chain" };

    void configureButton(juce::TextButton& button, Action action, const juce::String& tooltip);
    void configureDragButton(ExternalFileDragButton& button, Action action, const juce::String& tooltip);
};
}
