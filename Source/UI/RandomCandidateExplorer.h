#pragma once

#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace UI
{
class RandomCandidateExplorer final : public juce::Component,
                                      public juce::TooltipClient
{
public:
    static constexpr size_t candidateCount = 4;

    struct Candidate
    {
        juce::String title = "Empty";
        juce::String traits;
        int changedSections = 0;
        juce::int64 fingerprint = 0;
        bool ready = false;
        bool active = false;
        bool cueing = false;
    };

    struct State
    {
        juce::String rootName = "Current";
        std::array<Candidate, candidateCount> candidates;
    };

    struct LayoutMetrics
    {
        int visibleCards = 0;
        float minCardWidth = 0.0f;
        float minCardHeight = 0.0f;
        bool readable = false;
    };

    std::function<void(size_t)> onRecall;
    std::function<void(size_t)> onCue;

    RandomCandidateExplorer();
    void setTheme(const Theme& newTheme);
    void setState(State newState);
    LayoutMetrics getLayoutMetricsForAudit() const;
    juce::String getTooltip() override;

    void paint(juce::Graphics& g) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    Theme theme = themeFor(ThemeId::darkClub);
    State state;
    int hoveredCandidate = -1;

    std::array<juce::Rectangle<float>, candidateCount> cardBounds() const;
    juce::Rectangle<float> cueBounds(size_t index) const;
    int candidateAt(juce::Point<float> point) const;
    static float fingerprintValue(juce::int64 seed, int index) noexcept;
};
}
