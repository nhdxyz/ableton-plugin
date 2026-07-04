#pragma once

#include "SequencerSceneChainControls.h"
#include "SequencerSceneControls.h"
#include "SequencerPatternActions.h"
#include "SequencerRateControls.h"
#include "SequencerRootControls.h"
#include "SequencerStepEditor.h"
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
    juce::ToggleButton& chordMemoryButton;

    SequencerRateControls& rateControls;
    SequencerPatternActions& patternActions;
    SequencerUtilityActions& utilityActions;
    SequencerSceneChainControls& sceneChainControls;
    juce::TextButton& applyGrooveTransformButton;
    juce::TextButton& expandButton;
    SequencerRootControls& rootControls;

    juce::ComboBox& grooveBox;
    juce::ComboBox& scaleBox;
    juce::ComboBox& chordBox;
    juce::ComboBox& voicingBox;
    juce::ComboBox& patternBox;
    juce::ComboBox& grooveTransformBox;
    juce::ComboBox& laneViewBox;
    juce::ComboBox& lockDestinationBox;

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
