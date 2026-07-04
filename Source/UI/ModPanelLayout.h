#pragma once

#include "MacroAssignmentPad.h"
#include "MacroPerformanceMap.h"
#include "ModCurveDisplay.h"
#include "ModMatrixRow.h"
#include "ModRouteMapDisplay.h"
#include "ModSourceMeter.h"
#include "PageButtonStrip.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace UI::ModPanelLayout
{
enum class Page
{
    sources = 0,
    matrix,
    macros,
    curves
};

struct SliderSlot
{
    juce::Slider& slider;
    juce::Label& label;
};

struct SourceControls
{
    juce::Label& sourceLabel;
    std::array<ModSourceMeter, 20>& sourceRows;
};

struct GeneratorControls
{
    juce::Label& lfo2Label;
    juce::Label& envelopeLabel;
    juce::ComboBox& lfo2ShapeBox;
    juce::ComboBox& lfo2SyncRateBox;
    juce::ToggleButton& lfo2SyncButton;
    juce::ToggleButton& lfo2RetriggerButton;
    SliderSlot lfo2Rate;
    SliderSlot lfo2Depth;
    SliderSlot lfo2Phase;
    SliderSlot envAttack;
    SliderSlot envDecay;
    SliderSlot envSustain;
    SliderSlot envRelease;
    SliderSlot envDepth;
};

struct MacroControls
{
    juce::Label& macroLabel;
    juce::Label& assignLabel;
    juce::Label& assignStatusLabel;
    juce::TextButton& expandButton;
    MacroAssignmentPad& assignmentPad;
    MacroPerformanceMap& performanceMap;
};

struct CurveControls
{
    juce::Label& lfoLabel;
    juce::ComboBox& lfo1ShapeBox;
    juce::ComboBox& lfo1SyncRateBox;
    juce::ComboBox& presetBox;
    juce::ComboBox& actionBox;
    juce::ToggleButton& syncButton;
    juce::ToggleButton& retriggerButton;
    ModCurveDisplay& curveDisplay;
    SliderSlot rate;
    SliderSlot depth;
    SliderSlot phase;
};

struct MatrixControls
{
    juce::Label& matrixLabel;
    juce::Label& statusLabel;
    juce::Label& inspectorLabel;
    juce::Label& inspectorStatusLabel;
    juce::Label& sourceHeader;
    juce::Label& destinationHeader;
    juce::Label& amountHeader;
    juce::Label& sourceHeaderB;
    juce::Label& destinationHeaderB;
    juce::Label& amountHeaderB;

    juce::ComboBox& destinationBox;
    juce::ComboBox& sourceBox;
    juce::TextButton& addButton;
    juce::TextButton& clearButton;
    ModRouteMapDisplay& routeMapDisplay;

    std::array<ModMatrixRow, 8>& matrixRows;
    std::array<juce::Label, 8>& slotRows;
    std::array<juce::ComboBox, 8>& sourceBoxes;
    std::array<juce::ComboBox, 8>& destinationBoxes;
    std::array<juce::Slider, 8>& amountSliders;
    std::array<juce::ToggleButton, 8>& enabledButtons;
    std::array<juce::TextButton, 8>& duplicateButtons;
    std::array<juce::TextButton, 8>& deleteButtons;
};

struct Components
{
    juce::Label& sectionLabel;
    PageButtonStrip& workflowStrip;
    Page activePage = Page::matrix;

    SourceControls sources;
    GeneratorControls generators;
    MacroControls macros;
    CurveControls curves;
    MatrixControls matrix;
};

void layout(juce::Rectangle<int> content, Components components);
}
