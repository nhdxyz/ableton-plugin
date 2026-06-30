#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class ModSourceMeter final : public juce::Component,
                             public juce::TooltipClient
{
public:
    std::function<void(int, juce::Component&)> onDragStart;

    ModSourceMeter();

    void setSourceIndex(int newSourceIndex);
    void setState(const juce::String& newSummary,
                  int newRouteCount,
                  float newDepth,
                  float newActivity,
                  const juce::String& newTooltip);

    void paint(juce::Graphics& g) override;
    juce::String getTooltip() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    juce::String summary;
    juce::String tooltipText;
    int sourceIndex = 0;
    int routeCount = 0;
    float depth = 0.0f;
    float activity = 0.0f;
    bool dragStarted = false;
};
}
