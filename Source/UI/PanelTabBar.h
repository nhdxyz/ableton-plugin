#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class PanelTabBar final : public juce::Component
{
public:
    enum class Tab
    {
        home = 0,
        synth,
        lab,
        mod,
        sample,
        sequencer,
        effects,
        library,
        info
    };

    PanelTabBar();

    void resized() override;
    void setActiveTab(Tab tab);

    std::function<void(Tab)> onTabSelected;

private:
    struct TabSpec
    {
        const char* label;
        int width;
        Tab tab;
        const char* tooltip;
    };

    static constexpr std::array<TabSpec, 9> tabSpecs {{
        { "HOME", 64, Tab::home, "Performance macros, snapshots, signal flow, and output monitoring" },
        { "SYNTH", 64, Tab::synth, "Oscillators, wavetable layers, voice controls, filter, and envelopes" },
        { "LAB", 52, Tab::lab, "Generate, mutate, compare, and save recipe-based sound candidates" },
        { "MOD", 52, Tab::mod, "LFOs, envelopes, macros, modulation routes, and destination inspector" },
        { "SAMPLE", 72, Tab::sample, "Load, record, trim, slice, shape, and export sample material" },
        { "SEQ", 54, Tab::sequencer, "Pattern steps, scenes, groove, chord memory, and MIDI export" },
        { "FX", 48, Tab::effects, "Reorderable effects rack, sends, pump, width, and output guard" },
        { "LIBRARY", 82, Tab::library, "Search, preview, rate, organize, and save presets" },
        { "INFO", 54, Tab::info, "Plugin workflow, control behavior, and current feature notes" }
    }};

    std::array<juce::TextButton, tabSpecs.size()> tabButtons;
    Tab activeTab = Tab::home;
};
}
