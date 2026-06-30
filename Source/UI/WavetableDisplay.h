#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class WavetableDisplay final : public juce::Component,
                               public juce::TooltipClient
{
public:
    static constexpr size_t customPointCount = 16;
    using CustomPointArray = std::array<float, customPointCount>;
    enum class CustomDrawMode
    {
        point,
        line,
        smooth,
        step,
        erase
    };

    std::function<void()> onEditStart;
    std::function<void(float)> onOsc1PositionChange;
    std::function<void(float)> onOsc2PositionChange;
    std::function<void(float)> onWarpChange;
    std::function<void(int, size_t, float)> onCustomPointChange;

    WavetableDisplay();

    void setState(float newOsc1Position,
                  float newOsc2Position,
                  bool newOsc1Active,
                  bool newOsc2Active,
                  float newOsc1ModAmount = 0.0f,
                  float newOsc2ModAmount = 0.0f,
                  int newModRouteCount = 0,
                  juce::String newModSourceSummary = {},
                  float newWarp = 0.0f,
                  CustomPointArray newOsc1CustomPoints = {},
                  CustomPointArray newOsc2CustomPoints = {},
                  bool newOsc1CustomActive = false,
                  bool newOsc2CustomActive = false);
    void setCustomDrawMode(CustomDrawMode newMode);
    juce::String getTooltip() override;
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    float osc1Position = 0.0f;
    float osc2Position = 0.35f;
    float warp = 0.0f;
    CustomPointArray osc1CustomPoints {
        0.5f,
        0.691342f,
        0.853553f,
        0.961940f,
        1.0f,
        0.961940f,
        0.853553f,
        0.691342f,
        0.5f,
        0.308658f,
        0.146447f,
        0.038060f,
        0.0f,
        0.038060f,
        0.146447f,
        0.308658f
    };
    CustomPointArray osc2CustomPoints {
        0.5f,
        0.691342f,
        0.853553f,
        0.961940f,
        1.0f,
        0.961940f,
        0.853553f,
        0.691342f,
        0.5f,
        0.308658f,
        0.146447f,
        0.038060f,
        0.0f,
        0.038060f,
        0.146447f,
        0.308658f
    };
    float osc1ModAmount = 0.0f;
    float osc2ModAmount = 0.0f;
    int modRouteCount = 0;
    juce::String modSourceSummary;
    bool osc1Active = false;
    bool osc2Active = false;
    bool osc1CustomActive = false;
    bool osc2CustomActive = false;
    int editingOscillator = 1;
    int editingCustomPoint = -1;
    int editingPartial = -1;
    int lastDrawCustomPoint = -1;
    float lastDrawCustomValue = 0.5f;
    bool editGestureActive = false;
    CustomDrawMode customDrawMode = CustomDrawMode::point;

    void beginEdit(const juce::MouseEvent& event);
    void applyMousePosition(const juce::MouseEvent& event);
    void nudgePosition(const juce::MouseEvent& event, float delta);
    int oscillatorForEvent(const juce::MouseEvent& event) const noexcept;
    int customPointForEvent(const juce::MouseEvent& event) const noexcept;
    void editCustomPoint(const juce::MouseEvent& event, int pointIndex);
    void editPartial(const juce::MouseEvent& event, int partialIndex);
    void setCustomPointValue(int oscillator, size_t pointIndex, float value);
    juce::Rectangle<float> editorPlotBounds() const;
    juce::Rectangle<float> partialBarsBounds() const;
    int partialForEvent(const juce::MouseEvent& event) const noexcept;
    static juce::Path makeCustomPath(juce::Rectangle<float> bounds, const CustomPointArray& points);
    static float sampleFrame(float phase, float position);
    static juce::Path makePath(juce::Rectangle<float> bounds, float position);
    static juce::String modulationText(float amount);
};
}
