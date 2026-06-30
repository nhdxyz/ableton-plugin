#pragma once

#include "../Sampler/SamplePlayer.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class SampleWaveformDisplay final : public juce::Component,
                                    public juce::TooltipClient
{
public:
    struct SliceMarker
    {
        float start = 0.0f;
        float end = 1.0f;
        float pitch = 0.0f;
        float gain = -6.0f;
        float pan = 0.0f;
        float probability = 1.0f;
        float nudgePercent = 0.0f;
        float fade = 0.0f;
        int repeats = 3;
        bool custom = false;
        bool selected = false;
        bool reverse = false;
        bool stutter = false;
        bool choke = false;
    };

    std::function<void(float, float)> onRangeChange;
    std::function<void(size_t)> onSliceSelected;

    SampleWaveformDisplay();

    void setOverview(Sampler::SamplePeakOverview newOverview);
    void setRange(float newStart, float newEnd);
    void setSliceMarkers(const std::array<SliceMarker, 8>& newMarkers);
    void setModulationState(float startAmount,
                            float mixAmount,
                            float pitchAmount,
                            float rampAmount,
                            float stutterAmount,
                            int routeCount,
                            juce::String sourceSummary);
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
    float startModAmount = 0.0f;
    float mixModAmount = 0.0f;
    float pitchModAmount = 0.0f;
    float rampModAmount = 0.0f;
    float stutterModAmount = 0.0f;
    int modRouteCount = 0;
    juce::String modSourceSummary;
    std::array<SliceMarker, 8> sliceMarkers {};
    float dragAnchor = 0.0f;
    DragMode dragMode = DragMode::none;

    juce::Rectangle<float> plotBounds() const;
    juce::Rectangle<float> sliceLaneBounds() const;
    float positionToNormalised(float xPosition) const;
    int sliceIndexAtPosition(float xPosition) const;
    void applyRange(float start, float end);
    static juce::String modulationText(float amount);
};
}
