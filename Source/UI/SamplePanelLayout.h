#pragma once

#include "SampleChopPanel.h"
#include "SampleFileActions.h"
#include "SamplePlaybackControls.h"
#include "SampleRecorderPanel.h"
#include "SampleRecipeActions.h"
#include "SampleWaveformDisplay.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI::SamplePanelLayout
{
struct SliderSlot
{
    juce::Slider& slider;
    juce::Label& label;
};

struct Components
{
    juce::Label& sectionLabel;
    juce::Label& sourceLabel;
    juce::Label& chopLabel;
    juce::Label& shapeLabel;
    juce::Label& sampleNameLabel;

    SampleFileActions& fileActions;
    SampleRecipeActions& recipeActions;
    juce::TextButton& chopExpandButton;

    juce::ToggleButton& enabledButton;
    juce::ToggleButton& reverseButton;
    SamplePlaybackControls& playbackControls;

    SampleRecorderPanel& recorderPanel;
    SampleWaveformDisplay& waveformDisplay;
    SampleChopPanel& chopPanel;

    SliderSlot start;
    SliderSlot end;
    SliderSlot transpose;
    SliderSlot pitchRamp;
    SliderSlot gain;
    SliderSlot mix;
    SliderSlot stutterRepeats;
    SliderSlot grainSize;
    SliderSlot grainSpray;
    SliderSlot spectralFreeze;
};

void layout(juce::Rectangle<int> content, Components components);
}
