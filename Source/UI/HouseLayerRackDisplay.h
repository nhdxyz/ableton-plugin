#pragma once

#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class HouseLayerRackDisplay final : public juce::Component,
                                    public juce::TooltipClient
{
public:
    static constexpr size_t layerCount = 5;

    struct Layer
    {
        juce::String role;
        juce::String source;
        juce::String hint;
        float level = 0.0f;
        bool active = false;
        bool lowSafe = false;
        bool wide = false;
    };

    struct State
    {
        std::array<Layer, layerCount> layers;
        juce::String summary;
    };

    struct LayoutMetrics
    {
        int visibleCards = 0;
        float minCardWidth = 0.0f;
        float minCardHeight = 0.0f;
        bool readable = false;
    };

    std::function<void(size_t)> onLayerSelected;
    std::function<void(size_t)> onLayerEditStarted;
    std::function<void(size_t, float)> onLayerLevelChanged;

    HouseLayerRackDisplay();

    void setTheme(const Theme& newTheme);
    void setState(State newState);
    LayoutMetrics getLayoutMetricsForAudit() const;
    juce::String getTooltip() override;
    void paint(juce::Graphics& g) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    Theme theme = themeFor(ThemeId::darkClub);
    State state;
    juce::String tooltipText;
    int hoveredLayerIndex = -1;
    int editingLayerIndex = -1;

    static bool layersEqual(const Layer& left, const Layer& right) noexcept;
    static juce::Colour colourForLayer(const Theme& theme, const Layer& layer, size_t index);
    std::array<juce::Rectangle<float>, layerCount> layerBoundsForArea(juce::Rectangle<float> bounds) const;
    int layerIndexAt(juce::Point<float> position) const;
    void updateLayerLevelAt(juce::Point<float> position);
};
}
