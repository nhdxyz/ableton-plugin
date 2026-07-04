#pragma once

#include "SequencerSceneChainControls.h"
#include "SequencerSceneControls.h"
#include "SequencerPatternControls.h"
#include "SequencerGrooveControls.h"
#include "SequencerLaneViewControls.h"
#include "SequencerRateControls.h"
#include "SequencerRootControls.h"
#include "SequencerStepEditor.h"
#include "SequencerTransformControls.h"
#include "SequencerUtilityActions.h"
#include "StepSequencerGrid.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace UI::SequencerPanelLayout
{
struct SliderSlot
{
    juce::Slider& slider;
    juce::Label& label;
};

struct Components
{
    juce::Label& sectionLabel;
    juce::Label& hostSyncStatusLabel;
    SequencerStepEditor& stepEditor;

    juce::ToggleButton& enabledButton;

    SequencerRateControls& rateControls;
    SequencerPatternControls& patternControls;
    SequencerUtilityActions& utilityActions;
    SequencerSceneChainControls& sceneChainControls;
    juce::TextButton& expandButton;
    SequencerRootControls& rootControls;

    SequencerGrooveControls& grooveControls;
    SequencerLaneViewControls& laneViewControls;
    SequencerTransformControls& transformControls;

    SequencerSceneControls& sceneControls;

    StepSequencerGrid& grid;

    SliderSlot root;
    SliderSlot gate;
    SliderSlot swing;
    SliderSlot chordStrum;
    SliderSlot accent;
    SliderSlot octave;
    SliderSlot probability;
    SliderSlot random;
    SliderSlot lockDepth;
};

void layout(juce::Rectangle<int> content, Components components);
}
