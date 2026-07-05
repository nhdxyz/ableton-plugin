#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace UI::PresetNoteTemplates
{
struct Context
{
    juce::String category;
    juce::String recipe;
    juce::String bpm;
    juce::String suggestedUse;
};

void configureComboBox(juce::ComboBox& box);
juce::String textForId(int templateId, const Context& context);
}
