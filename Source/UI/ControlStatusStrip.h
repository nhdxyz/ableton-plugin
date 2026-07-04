#pragma once

#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class ControlStatusStrip final : public juce::Component
{
public:
    ControlStatusStrip();

    void resized() override;
    void applyTheme(const Theme& theme);

    void setStatus(const juce::String& text,
                   const juce::String& tooltip,
                   juce::Colour textColour,
                   juce::Colour backgroundColour);
    void setModActionState(bool enabled,
                           const juce::String& addTooltip,
                           const juce::String& openTooltip);
    void setHistoryActionState(bool canUndo,
                               bool canRedo,
                               const juce::String& undoTooltip,
                               const juce::String& redoTooltip);

    std::function<void()> onUndoClicked;
    std::function<void()> onRedoClicked;
    std::function<void()> onAddModClicked;
    std::function<void()> onOpenModClicked;

private:
    juce::Label headerLabel;
    juce::Label statusLabel;
    juce::TextButton undoButton { "Undo Edit" };
    juce::TextButton redoButton { "Redo Edit" };
    juce::TextButton addModButton { "MOD+" };
    juce::TextButton openModButton { "MOD" };
};
}
