#pragma once

#include "../Parameters.h"
#include "Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>
#include <memory>

namespace UI
{
class SampleShapeControls final : public juce::Component
{
public:
    enum class Control
    {
        transpose = 0,
        pitchRamp,
        gain,
        mix,
        stutterRepeats,
        grainSize,
        grainSpray,
        spectralFreeze
    };

    struct ModulationTarget
    {
        Control control;
        int destinationIndex = 0;
        juce::String label;
        juce::String parameterID;
    };

    struct SliderSpec
    {
        Control control;
        const char* label;
        const char* parameterID;
        int modulationDestinationIndex = 0;
    };

    static constexpr size_t controlCount = 8;

    explicit SampleShapeControls(juce::AudioProcessorValueTreeState& valueTreeState);

    std::function<void(const juce::String& label)> onEditStarted;
    std::function<void(const juce::String& label, const juce::String& parameterID, double value)> onControlChanged;
    std::function<void()> onEditEnded;
    std::function<void(int destinationIndex)> onModDestinationFocused;

    void applyTheme(const Theme& theme);
    void setModulationIndicator(Control control, float amount, int routeCount, const juce::StringArray& sources);
    juce::Slider& sliderFor(Control control) noexcept;

    static std::array<ModulationTarget, 4> modulationTargets();

    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void configureSlider(size_t index, const SliderSpec& spec, juce::AudioProcessorValueTreeState& valueTreeState);
    void layoutKnobRow(juce::Rectangle<int> area, std::initializer_list<Control> controls);
    void setControlVisible(Control control, bool shouldBeVisible);
    static size_t indexFor(Control control) noexcept;

    std::array<juce::Slider, controlCount> sliders;
    std::array<juce::Label, controlCount> labels;
    std::array<std::unique_ptr<SliderAttachment>, controlCount> attachments;
};
}
