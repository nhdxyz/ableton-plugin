#include "HomeSessionDisplay.h"

namespace UI
{
void HomeSessionDisplay::setState(const State& newState)
{
    state = newState;
    if (state.selectedName.trim().isEmpty())
        state.selectedName = "No preset selected";
    if (state.selectedMeta.trim().isEmpty())
        state.selectedMeta = "Choose a sound";
    if (state.filterName.trim().isEmpty())
        state.filterName = "All";
    if (state.candidateName.trim().isEmpty())
        state.candidateName = "No candidate";
    state.visibleCount = juce::jmax(0, state.visibleCount);
    state.totalCount = juce::jmax(0, state.totalCount);
    state.rating = juce::jlimit(0, 5, state.rating);

    repaint();
}

void HomeSessionDisplay::drawStatusPill(juce::Graphics& g,
                                        juce::Rectangle<float> area,
                                        const juce::String& text,
                                        juce::Colour colour)
{
    if (area.getWidth() < 14.0f || area.getHeight() < 8.0f)
        return;

    g.setColour(colour.withAlpha(0.22f));
    g.fillRoundedRectangle(area, 4.0f);
    g.setColour(colour.withAlpha(0.66f));
    g.drawRoundedRectangle(area, 4.0f, 1.0f);
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.setColour(colour.brighter(0.22f));
    g.drawFittedText(text, area.reduced(5.0f, 1.0f).toNearestInt(), juce::Justification::centred, 1, 0.62f);
}

void HomeSessionDisplay::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto accent = state.generated ? juce::Colour(0xffc4a7ff)
                      : state.factory ? juce::Colour(0xff7bb7ff)
                      : juce::Colour(0xff8ee6c9);

    g.setColour(juce::Colour(0xff0d1316));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour(0xff2a363c));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
    g.setColour(accent.withAlpha(0.66f));
    g.fillRoundedRectangle(bounds.withHeight(3.0f), 2.0f);

    auto content = bounds.reduced(9.0f, 5.0f);
    auto top = content.removeFromTop(14.0f);
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff73848a));
    g.drawFittedText("SESSION", top.removeFromLeft(54.0f).toNearestInt(), juce::Justification::centredLeft, 1);

    const auto countText = juce::String(state.visibleCount) + "/" + juce::String(state.totalCount);
    drawStatusPill(g, top.removeFromRight(52.0f), countText, juce::Colour(0xff8ee6c9));
    top.removeFromRight(4.0f);
    drawStatusPill(g, top.removeFromRight(70.0f), state.filterName, juce::Colour(0xffffc36b));

    auto nameRow = content.removeFromTop(18.0f);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.setColour(state.favorite ? juce::Colour(0xffffd27a) : juce::Colour(0xffedf7f4));
    g.drawFittedText(state.favorite ? "* " + state.selectedName : state.selectedName,
                     nameRow.toNearestInt(),
                     juce::Justification::centredLeft,
                     1,
                     0.68f);

    auto metaRow = content.removeFromTop(11.0f);
    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.setColour(juce::Colour(0xff9dafb3));
    g.drawFittedText(state.selectedMeta, metaRow.toNearestInt(), juce::Justification::centredLeft, 1, 0.62f);

    auto footer = content.removeFromTop(12.0f).withTrimmedTop(1.0f);
    const auto ratingText = state.rating > 0 ? juce::String(state.rating) + "/5" : juce::String("UNRATED");
    const auto sourceText = state.generated ? juce::String("GEN")
                         : state.factory ? juce::String("FACTORY")
                         : juce::String("USER");
    drawStatusPill(g, footer.removeFromLeft(54.0f), ratingText, juce::Colour(0xffffc36b));
    footer.removeFromLeft(4.0f);
    drawStatusPill(g, footer.removeFromLeft(58.0f), sourceText, accent);
    footer.removeFromLeft(4.0f);
    drawStatusPill(g, footer.removeFromLeft(62.0f), state.compareReady ? "COMPARE" : "NO COMP", juce::Colour(0xff7bb7ff));
    footer.removeFromLeft(4.0f);
    drawStatusPill(g, footer, state.candidateReady ? state.candidateName : "NO CAND", juce::Colour(0xffc4a7ff));
}
}
