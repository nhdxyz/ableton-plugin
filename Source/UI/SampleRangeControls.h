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
class SampleRangeControls final : public juce::Component
{
public:
    enum class Control
    {
        start = 0,
        end
    };

    struct SliderSpec
    {
        Control control;
        const char* label;
        const char* parameterID;
        int modulationDestinationIndex = 0;
    };

    static constexpr size_t controlCount = 2;

    explicit SampleRangeControls(juce::AudioProcessorValueTreeState& valueTreeState);

    std::function<void(const juce::String& label)> onEditStarted;
    std::function<void(const juce::String& label, const juce::String& parameterID, double value)> onControlChanged;
    std::function<void()> onEditEnded;
    std::function<void(int destinationIndex)> onModDestinationFocused;

    void applyTheme(const Theme& theme);
    void setModulationIndicator(Control control, float amount, int routeCount, const juce::StringArray& sources);
    juce::Slider& sliderFor(Control control) noexcept;

    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void configureSlider(size_t index, const SliderSpec& spec, juce::AudioProcessorValueTreeState& valueTreeState);
    static size_t indexFor(Control control) noexcept;

    std::array<juce::Slider, controlCount> sliders;
    std::array<juce::Label, controlCount> labels;
    std::array<std::unique_ptr<SliderAttachment>, controlCount> attachments;
};
}
