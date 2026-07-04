#pragma once

#include "SequencerStepEditor.h"
#include "StepSequencerGrid.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

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
    juce::Label& rootValueLabel;
    SequencerStepEditor& stepEditor;

    juce::ToggleButton& enabledButton;
    juce::ToggleButton& chordMemoryButton;

    juce::TextButton& rateEighthButton;
    juce::TextButton& rateSixteenthButton;
    juce::TextButton& rateThirtySecondButton;
    juce::TextButton& applyPatternButton;
    juce::TextButton& copyButton;
    juce::TextButton& rotateLeftButton;
    juce::TextButton& rotateRightButton;
    juce::TextButton& exportMidiButton;
    juce::TextButton& exportChainButton;
    juce::TextButton& sceneChainLiveButton;
    juce::TextButton& applyGrooveTransformButton;
    juce::TextButton& randomButton;
    juce::TextButton& mutateButton;
    juce::TextButton& undoButton;
    juce::TextButton& clearButton;
    juce::TextButton& expandButton;
    juce::TextButton& rootDownButton;
    juce::TextButton& rootUpButton;

    juce::ComboBox& grooveBox;
    juce::ComboBox& scaleBox;
    juce::ComboBox& chordBox;
    juce::ComboBox& voicingBox;
    juce::ComboBox& patternBox;
    juce::ComboBox& grooveTransformBox;
    juce::ComboBox& laneViewBox;
    juce::ComboBox& lockDestinationBox;

    std::array<juce::TextButton, 4>& sceneRecallButtons;
    std::array<juce::TextButton, 4>& sceneCaptureButtons;

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
