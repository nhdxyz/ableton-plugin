#include "SampleRangeControls.h"

namespace UI
{
namespace
{
juce::ModifierKeys::Flags fineDragModifierFlags()
{
    return static_cast<juce::ModifierKeys::Flags>(juce::ModifierKeys::shiftModifier
                                                  | juce::ModifierKeys::commandModifier);
}

void applyFineDragMode(juce::Slider& slider, double sensitivity)
{
    slider.setVelocityBasedMode(false);
    slider.setVelocityModeParameters(sensitivity, 1, 0.0, true, fineDragModifierFlags());
}

juce::String controlFeelTooltip(const juce::String& labelText)
{
    return labelText + ": drag to adjust, hold Shift or Cmd for fine movement, double-click to reset, or type a value.";
}

constexpr std::array<SampleRangeControls::SliderSpec, SampleRangeControls::controlCount> sliderSpecs {{
    { SampleRangeControls::Control::start, "Start", Parameters::ID::sampleStart, 12 },
    { SampleRangeControls::Control::end, "End", Parameters::ID::sampleEnd, 0 }
}};
}

SampleRangeControls::SampleRangeControls(juce::AudioProcessorValueTreeState& valueTreeState)
{
    setComponentID("SampleRangeControls");

    for (size_t index = 0; index < sliderSpecs.size(); ++index)
        configureSlider(index, sliderSpecs[index], valueTreeState);

    applyTheme(themeFor(ThemeId::darkClub));
}

void SampleRangeControls::applyTheme(const Theme& theme)
{
    for (auto& slider : sliders)
    {
        slider.setColour(juce::Slider::trackColourId, theme.accent);
        slider.setColour(juce::Slider::backgroundColourId, theme.outline);
        slider.setColour(juce::Slider::textBoxTextColourId, theme.text);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, theme.field);
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    }

    for (auto& label : labels)
        label.setColour(juce::Label::textColourId, theme.textMuted);
}

void SampleRangeControls::setModulationIndicator(Control control,
                                                 float amount,
                                                 int routeCount,
                                                 const juce::StringArray& sources)
{
    amount = juce::jlimit(-1.0f, 1.0f, amount);

    auto& slider = sliderFor(control);
    auto& properties = slider.getProperties();
    const auto previous = static_cast<float>(static_cast<double>(properties.getWithDefault("modAmount", 0.0)));
    const auto previousCount = static_cast<int>(properties.getWithDefault("modRouteCount", 0));
    const auto previousSources = properties.getWithDefault("modSourceSummary", {}).toString();
    const auto sourceSummary = sources.joinIntoString(", ");
    const auto baseTooltip = properties.getWithDefault("baseTooltip", {}).toString();
    if (std::abs(previous - amount) < 0.001f
        && previousCount == routeCount
        && previousSources == sourceSummary)
        return;

    properties.set("modAmount", amount);
    properties.set("modRouteCount", routeCount);
    properties.set("modSourceSummary", sourceSummary);
    const auto modulationTooltip = routeCount > 0
        ? "Modulated by " + sourceSummary
            + " | Sum " + (amount >= 0.0f ? "+" : "") + juce::String(juce::roundToInt(amount * 100.0f)) + "%"
        : juce::String {};
    slider.setTooltip(modulationTooltip.isNotEmpty() && baseTooltip.isNotEmpty()
                          ? baseTooltip + "\n" + modulationTooltip
                          : modulationTooltip.isNotEmpty() ? modulationTooltip : baseTooltip);
    slider.repaint();
}

juce::Slider& SampleRangeControls::sliderFor(Control control) noexcept
{
    return sliders[indexFor(control)];
}

void SampleRangeControls::resized()
{
    auto area = getLocalBounds().withTrimmedTop(6);
    sliders[indexFor(Control::start)].setBounds(area.removeFromLeft(area.getWidth() / 2).reduced(48, 6));
    sliders[indexFor(Control::end)].setBounds(area.reduced(48, 6));
}

void SampleRangeControls::configureSlider(size_t index,
                                          const SliderSpec& spec,
                                          juce::AudioProcessorValueTreeState& valueTreeState)
{
    auto& slider = sliders[index];
    auto& label = labels[index];
    const auto labelText = juce::String(spec.label);
    const auto parameterID = juce::String(spec.parameterID);

    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setMouseDragSensitivity(150);
    applyFineDragMode(slider, 0.40);
    slider.setSliderSnapsToMousePosition(false);
    slider.setScrollWheelEnabled(false);
    slider.setWantsKeyboardFocus(false);
    slider.setMouseClickGrabsKeyboardFocus(false);
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 62, 18);
    const auto tooltipText = controlFeelTooltip(labelText);
    slider.setTooltip(tooltipText);
    slider.getProperties().set("baseTooltip", tooltipText);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredLeft);
    label.attachToComponent(&slider, true);
    addAndMakeVisible(label);

    if (auto* parameter = valueTreeState.getParameter(parameterID))
        slider.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));

    slider.onDragStart = [this, labelText, destinationIndex = spec.modulationDestinationIndex]
    {
        if (onEditStarted != nullptr)
            onEditStarted(labelText);
        if (destinationIndex > 0 && onModDestinationFocused != nullptr)
            onModDestinationFocused(destinationIndex);
    };
    slider.onValueChange = [this, index, labelText, parameterID]
    {
        if (onControlChanged != nullptr)
            onControlChanged(labelText, parameterID, sliders[index].getValue());
    };
    slider.onDragEnd = [this]
    {
        if (onEditEnded != nullptr)
            onEditEnded();
    };

    attachments[index] = std::make_unique<SliderAttachment>(valueTreeState, parameterID, slider);
}

size_t SampleRangeControls::indexFor(Control control) noexcept
{
    return static_cast<size_t>(control);
}
}
