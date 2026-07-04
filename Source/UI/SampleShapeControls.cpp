#include "SampleShapeControls.h"

namespace UI
{
namespace
{
bool parameterIsOneOf(const juce::String& parameterID, std::initializer_list<const char*> ids)
{
    for (const auto* id : ids)
        if (parameterID == id)
            return true;

    return false;
}

int rotaryDragSensitivityForParameter(const juce::String& parameterID)
{
    if (parameterIsOneOf(parameterID, {
            Parameters::ID::sampleTranspose,
            Parameters::ID::samplePitchRamp
        }))
        return 64;

    return 52;
}

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

constexpr std::array<SampleShapeControls::SliderSpec, SampleShapeControls::controlCount> sliderSpecs {{
    { SampleShapeControls::Control::transpose, "Pitch", Parameters::ID::sampleTranspose, 14 },
    { SampleShapeControls::Control::pitchRamp, "Ramp", Parameters::ID::samplePitchRamp, 15 },
    { SampleShapeControls::Control::gain, "Gain", Parameters::ID::sampleGain, 0 },
    { SampleShapeControls::Control::mix, "Mix", Parameters::ID::sampleMix, 13 },
    { SampleShapeControls::Control::stutterRepeats, "Repeat", Parameters::ID::sampleStutterRepeats, 16 },
    { SampleShapeControls::Control::grainSize, "Grain", Parameters::ID::sampleGrainSize, 0 },
    { SampleShapeControls::Control::grainSpray, "Spray", Parameters::ID::sampleGrainSpray, 0 },
    { SampleShapeControls::Control::spectralFreeze, "Freeze", Parameters::ID::sampleSpectralFreeze, 0 }
}};
}

SampleShapeControls::SampleShapeControls(juce::AudioProcessorValueTreeState& valueTreeState)
{
    setComponentID("SampleShapeControls");

    for (size_t index = 0; index < sliderSpecs.size(); ++index)
        configureSlider(index, sliderSpecs[index], valueTreeState);

    applyTheme(themeFor(ThemeId::darkClub));
}

void SampleShapeControls::applyTheme(const Theme& theme)
{
    for (auto& slider : sliders)
    {
        slider.setColour(juce::Slider::textBoxTextColourId, theme.text);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, theme.field);
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    }

    for (auto& label : labels)
        label.setColour(juce::Label::textColourId, theme.textMuted);
}

void SampleShapeControls::setModulationIndicator(Control control,
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

juce::Slider& SampleShapeControls::sliderFor(Control control) noexcept
{
    return sliders[indexFor(control)];
}

std::array<SampleShapeControls::ModulationTarget, 4> SampleShapeControls::modulationTargets()
{
    return {{
        { Control::mix, 13, "Mix", Parameters::ID::sampleMix },
        { Control::transpose, 14, "Pitch", Parameters::ID::sampleTranspose },
        { Control::pitchRamp, 15, "Ramp", Parameters::ID::samplePitchRamp },
        { Control::stutterRepeats, 16, "Repeat", Parameters::ID::sampleStutterRepeats }
    }};
}

void SampleShapeControls::resized()
{
    auto area = getLocalBounds();
    if (area.getHeight() < 138)
    {
        const auto compactShapeHeight = area.getHeight();
        const auto showSecondaryShapeRow = compactShapeHeight >= 72;
        const auto rowHeight = showSecondaryShapeRow ? juce::jmax(36, compactShapeHeight / 2)
                                                     : juce::jmax(36, compactShapeHeight);
        layoutKnobRow(area.removeFromTop(juce::jmin(rowHeight, area.getHeight())).withTrimmedTop(2),
                      { Control::transpose, Control::gain, Control::mix, Control::pitchRamp });
        if (showSecondaryShapeRow)
        {
            layoutKnobRow(area.withTrimmedTop(2),
                          { Control::stutterRepeats, Control::grainSize, Control::grainSpray, Control::spectralFreeze });
        }
        else
        {
            setControlVisible(Control::stutterRepeats, false);
            setControlVisible(Control::grainSize, false);
            setControlVisible(Control::grainSpray, false);
            setControlVisible(Control::spectralFreeze, false);
        }
        return;
    }

    const auto rowHeight = juce::jlimit(46, 64, area.getHeight() / 3);
    layoutKnobRow(area.removeFromTop(rowHeight).withTrimmedTop(3),
                  { Control::transpose, Control::gain, Control::mix });
    layoutKnobRow(area.removeFromTop(rowHeight).withTrimmedTop(3),
                  { Control::pitchRamp, Control::stutterRepeats, Control::grainSize });
    layoutKnobRow(area.removeFromTop(juce::jmin(rowHeight, area.getHeight())).withTrimmedTop(3),
                  { Control::grainSpray, Control::spectralFreeze });
}

void SampleShapeControls::configureSlider(size_t index,
                                          const SliderSpec& spec,
                                          juce::AudioProcessorValueTreeState& valueTreeState)
{
    auto& slider = sliders[index];
    auto& label = labels[index];
    const auto labelText = juce::String(spec.label);
    const auto parameterID = juce::String(spec.parameterID);

    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setMouseDragSensitivity(rotaryDragSensitivityForParameter(parameterID));
    applyFineDragMode(slider, 0.32);
    slider.setSliderSnapsToMousePosition(false);
    slider.setScrollWheelEnabled(false);
    slider.setWantsKeyboardFocus(false);
    slider.setMouseClickGrabsKeyboardFocus(false);
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    const auto tooltipText = controlFeelTooltip(labelText);
    slider.setTooltip(tooltipText);
    slider.getProperties().set("baseTooltip", tooltipText);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centred);
    label.attachToComponent(&slider, false);
    addAndMakeVisible(label);

    if (auto* parameter = valueTreeState.getParameter(parameterID))
        slider.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));

    slider.onDragStart = [this, labelText, destinationIndex = spec.modulationDestinationIndex]
    {
        if (destinationIndex > 0)
        {
            if (onModDestinationFocused != nullptr)
                onModDestinationFocused(destinationIndex);
            return;
        }

        if (onEditStarted != nullptr)
            onEditStarted(labelText);
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

void SampleShapeControls::layoutKnobRow(juce::Rectangle<int> area, std::initializer_list<Control> controls)
{
    const auto count = static_cast<int>(controls.size());
    if (count == 0)
        return;

    const auto cellWidth = juce::jmax(1, area.getWidth() / count);
    const auto horizontalPadding = cellWidth < 64 ? 2 : 4;

    for (auto control : controls)
    {
        setControlVisible(control, true);
        sliderFor(control).setBounds(area.removeFromLeft(cellWidth).reduced(horizontalPadding, 0));
    }
}

void SampleShapeControls::setControlVisible(Control control, bool shouldBeVisible)
{
    const auto index = indexFor(control);
    sliders[index].setVisible(shouldBeVisible);
    labels[index].setVisible(shouldBeVisible);
}

size_t SampleShapeControls::indexFor(Control control) noexcept
{
    return static_cast<size_t>(control);
}
}
