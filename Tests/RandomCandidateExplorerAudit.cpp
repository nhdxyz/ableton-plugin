#include "../Source/UI/RandomCandidateExplorer.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <iostream>

namespace
{
juce::MouseEvent makeMouseEvent(UI::RandomCandidateExplorer& explorer, float x, float y)
{
    const auto now = juce::Time::getCurrentTime();
    return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),
                            { x, y },
                            juce::ModifierKeys::leftButtonModifier,
                            1.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            &explorer,
                            &explorer,
                            now,
                            { x, y },
                            now,
                            1,
                            false);
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    UI::RandomCandidateExplorer explorer;
    explorer.setBounds(0, 0, 520, 92);

    UI::RandomCandidateExplorer::State state;
    state.rootName = "Warehouse Seed";
    for (size_t index = 0; index < state.candidates.size(); ++index)
    {
        auto& candidate = state.candidates[index];
        candidate.title = "Variation " + juce::String(static_cast<int>(index + 1));
        candidate.traits = index % 2 == 0 ? "Source, Filter" : "FX, Motion";
        candidate.changedSections = static_cast<int>(index + 1);
        candidate.fingerprint = candidate.title.hashCode64();
        candidate.ready = true;
        candidate.active = index == 1;
        candidate.cueing = index == 2;
    }
    explorer.setState(state);

    const auto metrics = explorer.getLayoutMetricsForAudit();
    if (! metrics.readable || metrics.visibleCards != 4)
    {
        std::cerr << "Random candidate explorer layout is not readable: "
                  << metrics.visibleCards << " cards, "
                  << metrics.minCardWidth << "x" << metrics.minCardHeight << '\n';
        return 1;
    }

    juce::Image image(juce::Image::ARGB, explorer.getWidth(), explorer.getHeight(), true);
    juce::Graphics graphics(image);
    explorer.paint(graphics);
    auto visiblePixels = 0;
    for (auto y = 0; y < image.getHeight(); y += 4)
        for (auto x = 0; x < image.getWidth(); x += 4)
            if (image.getPixelAt(x, y).getAlpha() > 0)
                ++visiblePixels;
    if (visiblePixels < 1200)
    {
        std::cerr << "Random candidate explorer render appears sparse: " << visiblePixels << '\n';
        return 1;
    }

    auto recalled = -1;
    auto cued = -1;
    explorer.onRecall = [&recalled] (size_t index) { recalled = static_cast<int>(index); };
    explorer.onCue = [&cued] (size_t index) { cued = static_cast<int>(index); };

    explorer.mouseDown(makeMouseEvent(explorer, 125.0f, 34.0f));
    if (recalled != 0)
    {
        std::cerr << "Candidate card click did not recall the first branch\n";
        return 1;
    }

    explorer.mouseDown(makeMouseEvent(explorer, 148.0f, 77.0f));
    if (cued != 0)
    {
        std::cerr << "Candidate Cue target did not audition the first branch\n";
        return 1;
    }

    std::cout << "Random candidate explorer render and interaction audit passed.\n";
    return 0;
}
