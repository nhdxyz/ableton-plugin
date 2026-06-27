#pragma once

#include "../Sampler/SamplePlayer.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace UI
{
class SampleWaveformDisplay final : public juce::Component,
                                    public juce::TooltipClient
{
public:
    std::function<void(float, float)> onRangeChange;

    SampleWaveformDisplay();

    void setOverview(Sampler::SamplePeakOverview newOverview);
    void setRange(float newStart, float newEnd);
    juce::String getTooltip() override;
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    enum class DragMode
    {
        none,
        start,
        end,
        range
    };

    Sampler::SamplePeakOverview overview;
    float startNormalised = 0.0f;
    float endNormalised = 1.0f;
    float dragAnchor = 0.0f;
    DragMode dragMode = DragMode::none;

    juce::Rectangle<float> plotBounds() const;
    float positionToNormalised(float xPosition) const;
    void applyRange(float start, float end);
};
}
