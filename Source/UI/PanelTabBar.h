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
    };

    static constexpr std::array<TabSpec, 9> tabSpecs {{
        { "HOME", 64, Tab::home },
        { "SYNTH", 64, Tab::synth },
        { "LAB", 52, Tab::lab },
        { "MOD", 52, Tab::mod },
        { "SAMPLE", 72, Tab::sample },
        { "SEQ", 54, Tab::sequencer },
        { "FX", 48, Tab::effects },
        { "LIBRARY", 82, Tab::library },
        { "INFO", 54, Tab::info }
    }};

    std::array<juce::TextButton, tabSpecs.size()> tabButtons;
    Tab activeTab = Tab::home;
};
}
