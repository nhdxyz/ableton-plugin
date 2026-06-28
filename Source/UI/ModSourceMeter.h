#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI
{
class ModSourceMeter final : public juce::Component,
                             public juce::TooltipClient
{
public:
    void setState(const juce::String& newSummary,
                  int newRouteCount,
                  float newDepth,
                  float newActivity,
                  const juce::String& newTooltip);

    void paint(juce::Graphics& g) override;
    juce::String getTooltip() override;

private:
    juce::String summary;
    juce::String tooltipText;
    int routeCount = 0;
    float depth = 0.0f;
    float activity = 0.0f;
};
}
