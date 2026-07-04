#pragma once

#include "FxPerformanceControls.h"
#include "FxRackOrderControls.h"
#include "FxRackRow.h"
#include "PumpCurveDisplay.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace UI::FxRackPanelLayout
{
inline constexpr auto moduleCount = 15;

struct SliderSlot
{
    juce::Slider& slider;
    juce::Label& label;
};

struct RackSlot
{
    FxRackRow* row = nullptr;
    bool visible = false;
};

struct ToneControls
{
    juce::ToggleButton& enabledButton;
    SliderSlot tilt;
    SliderSlot lowCut;
};

struct EqControls
{
    juce::ToggleButton& enabledButton;
    SliderSlot lowGain;
    SliderSlot midGain;
    SliderSlot highGain;
    SliderSlot trim;
};

struct DistortionControls
{
    juce::ToggleButton& enabledButton;
    SliderSlot amount;
    SliderSlot bassSafe;
};

struct BitcrushControls
{
    juce::ToggleButton& enabledButton;
    SliderSlot bits;
    SliderSlot downsample;
    SliderSlot mix;
};

struct PumpControls
{
    juce::ToggleButton& enabledButton;
    juce::ComboBox& rateBox;
    juce::ComboBox& curveBox;
    PumpCurveDisplay& curveDisplay;
    SliderSlot depth;
    SliderSlot shape;
    SliderSlot phase;
};

struct TremoloControls
{
    juce::ToggleButton& enabledButton;
    juce::ComboBox& rateBox;
    SliderSlot depth;
    SliderSlot pan;
    SliderSlot shape;
    SliderSlot phase;
};

struct RingControls
{
    juce::ToggleButton& enabledButton;
    SliderSlot frequency;
    SliderSlot depth;
    SliderSlot mix;
    SliderSlot bias;
};

struct CombControls
{
    juce::ToggleButton& enabledButton;
    SliderSlot frequency;
    SliderSlot feedback;
    SliderSlot damping;
    SliderSlot mix;
};

struct PhaserControls
{
    juce::ToggleButton& enabledButton;
    SliderSlot rate;
    SliderSlot depth;
    SliderSlot mix;
};

struct FlangerControls
{
    juce::ToggleButton& enabledButton;
    SliderSlot rate;
    SliderSlot depth;
    SliderSlot feedback;
    SliderSlot mix;
};

struct ChorusControls
{
    juce::ToggleButton& enabledButton;
    SliderSlot rate;
    SliderSlot depth;
    SliderSlot mix;
};

struct DelayControls
{
    juce::ToggleButton& enabledButton;
    juce::ToggleButton& syncButton;
    juce::ComboBox& rateBox;
    SliderSlot time;
    SliderSlot feedback;
    SliderSlot mix;
    SliderSlot send;
};

struct ReverbControls
{
    juce::ToggleButton& enabledButton;
    SliderSlot size;
    SliderSlot damping;
    SliderSlot mix;
    SliderSlot send;
};

struct WidthControls
{
    juce::ToggleButton& enabledButton;
    SliderSlot amount;
    SliderSlot monoCutoff;
};

struct GuardControls
{
    juce::ToggleButton& enabledButton;
    SliderSlot push;
    SliderSlot glue;
    SliderSlot punch;
    SliderSlot clipMix;
    SliderSlot ceiling;
};

struct Components
{
    juce::Label& sectionLabel;
    juce::Label& hostSyncStatusLabel;
    juce::Label& statusLabel;

    juce::ComboBox& addBox;
    juce::ComboBox& presetBox;

    FxRackOrderControls& orderControls;
    FxPerformanceControls& performanceControls;
    juce::TextButton& applyPresetButton;

    std::array<RackSlot, moduleCount> rackSlots;
    int selectedModuleIndex = 0;

    ToneControls tone;
    EqControls eq;
    DistortionControls distortion;
    BitcrushControls bitcrush;
    PumpControls pump;
    TremoloControls tremolo;
    RingControls ring;
    CombControls comb;
    PhaserControls phaser;
    FlangerControls flanger;
    ChorusControls chorus;
    DelayControls delay;
    ReverbControls reverb;
    WidthControls width;
    GuardControls guard;
};

void layout(juce::Rectangle<int> content, Components components);
}
