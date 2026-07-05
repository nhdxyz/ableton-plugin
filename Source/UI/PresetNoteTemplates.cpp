#include "PresetNoteTemplates.h"

namespace UI::PresetNoteTemplates
{
void configureComboBox(juce::ComboBox& box)
{
    box.addItem("Note Template", 1);
    box.addItem("Macro Intent", 2);
    box.addItem("Ableton Use", 3);
    box.addItem("UKG Variation", 4);
    box.addItem("Mix Safety", 5);
    box.addItem("Pack Notes", 6);
    box.setSelectedId(1, juce::dontSendNotification);
    box.setTextWhenNothingSelected("Note Template");
    box.setTooltip("Insert a reusable notes scaffold into the generated preset notes");
}

juce::String textForId(int templateId, const Context& context)
{
    switch (templateId)
    {
        case 2:
            return "Macro Intent:\n"
                "Tone: brighten/darken core harmonics\n"
                "Dirt: drive edge without clipping the sub\n"
                "Motion: filter/warp movement for groove\n"
                "Space: delay/reverb throw amount\n"
                "Weight: mono low-end focus\n"
                "Bounce: pump or swing feel\n"
                "Warp: source movement amount\n"
                "Throw: momentary FX send";

        case 3:
            return "Ableton Use:\n"
                "Role: " + context.suggestedUse + "\n"
                "Tempo: " + context.bpm + "\n"
                "Clip idea: 4-bar loop, automate Motion and Throw\n"
                "Arrangement: duplicate for breakdown and mute low end before drop";

        case 4:
            return "UKG Variation:\n"
                "Groove: 2-step push with swung 16ths\n"
                "Bass: mono sub, moving upper layer, short glide\n"
                "Chop: pitched vocal or organ response on offbeats\n"
                "FX: short delay throw, guarded reverb tail\n"
                "Recipe source: " + context.recipe;

        case 5:
            return "Mix Safety:\n"
                "Sub: mono below 120 Hz\n"
                "Peak: leave headroom before Ableton group processing\n"
                "Drive: check harshness after Dirt/Warp automation\n"
                "Space: keep reverb low for bass presets\n"
                "Compare: A/B against saved candidate before overwriting";

        case 6:
            return "Pack Notes:\n"
                "Category: " + context.category + "\n"
                "Recipe: " + context.recipe + "\n"
                "Pack role: starter, variation, or performance macro patch\n"
                "Tags to add: Generated, Random Lab, house, UKG, tech-house, minimal, techno";

        default:
            break;
    }

    return {};
}
}
