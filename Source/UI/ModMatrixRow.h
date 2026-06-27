#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class ModMatrixRow final : public juce::Component,
                           public juce::TooltipClient
{
public:
    ModMatrixRow();

    void setState(int newSlotNumber,
                  const juce::String& newSourceText,
                  const juce::String& newDestinationText,
                  float newAmount);
    juce::String getTooltip() override;
    void paint(juce::Graphics& g) override;

private:
    int slotNumber = 1;
    juce::String sourceText { "Off" };
    juce::String destinationText { "Off" };
    float amount = 0.0f;

    bool isActive() const noexcept;
    juce::Colour accentColour() const noexcept;
};
}
