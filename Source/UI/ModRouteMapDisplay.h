#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace UI
{
class ModRouteMapDisplay final : public juce::Component,
                                 public juce::TooltipClient
{
public:
    struct Route
    {
        juce::String source;
        juce::String destination;
        float amount = 0.0f;
        bool enabled = true;
    };

    void setRoutes(std::vector<Route> newRoutes);
    juce::String getTooltip() override;
    void paint(juce::Graphics& g) override;

private:
    std::vector<Route> routes;
    juce::String tooltip { "No modulation routes" };

    static juce::Colour colourForAmount(float amount, bool enabled) noexcept;
    static juce::String compactName(const juce::String& name);
    void rebuildTooltip();
};
}
