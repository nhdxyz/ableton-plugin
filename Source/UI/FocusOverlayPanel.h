#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class FocusOverlayPanel final : public juce::Component
{
public:
    FocusOverlayPanel();

    juce::Rectangle<int> getPanelBounds() const;
    juce::Rectangle<int> getContentBounds() const;
    void paint(juce::Graphics& g) override;
};
}
