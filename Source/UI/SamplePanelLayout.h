#pragma once

#include "SampleChopHeader.h"
#include "SampleChopPanel.h"
#include "SampleFileActions.h"
#include "SamplePlaybackControls.h"
#include "SampleRecorderPanel.h"
#include "SampleRecipeActions.h"
#include "SampleShapeControls.h"
#include "SampleSourceControls.h"
#include "SampleStatusLabel.h"
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
    juce::Label& shapeLabel;
    SampleStatusLabel& sampleStatusLabel;

    SampleFileActions& fileActions;
    SampleRecipeActions& recipeActions;
    SampleChopHeader& chopHeader;
    SampleShapeControls& shapeControls;

    SampleSourceControls& sourceControls;
    SamplePlaybackControls& playbackControls;

    SampleRecorderPanel& recorderPanel;
    SampleWaveformDisplay& waveformDisplay;
    SampleChopPanel& chopPanel;

    SliderSlot start;
    SliderSlot end;
};

void layout(juce::Rectangle<int> content, Components components);
}
