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
    if (state.roleName.trim().isEmpty())
        state.roleName = "Patch";
    if (state.sourceName.trim().isEmpty())
        state.sourceName = "Synth";
    if (state.safetyName.trim().isEmpty())
        state.safetyName = "SAFE";
    state.visibleCount = juce::jmax(0, state.visibleCount);
    state.totalCount = juce::jmax(0, state.totalCount);
    state.rating = juce::jlimit(0, 5, state.rating);
    for (auto& value : state.performanceValues)
        value = juce::jlimit(0.0f, 1.0f, value);

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

void HomeSessionDisplay::drawPerformanceMeter(juce::Graphics& g,
                                              juce::Rectangle<float> area,
                                              const juce::String& label,
                                              float value,
                                              juce::Colour accent)
{
    if (area.getWidth() < 20.0f || area.getHeight() < 16.0f)
        return;

    value = juce::jlimit(0.0f, 1.0f, value);

    g.setColour(juce::Colour(0xff151d21));
    g.fillRoundedRectangle(area, 4.0f);
    g.setColour(accent.withAlpha(0.3f));
    g.drawRoundedRectangle(area, 4.0f, 1.0f);

    auto content = area.reduced(5.0f, 3.0f);
    g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
    g.setColour(juce::Colour(0xff71848a));
    g.drawFittedText(label, content.removeFromTop(9.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.64f);

    auto bar = content.withTrimmedTop(2.0f).withHeight(4.0f);
    g.setColour(juce::Colour(0xff253138));
    g.fillRoundedRectangle(bar, 2.0f);
    g.setColour(accent);
    g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * value), 2.0f);
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
    g.drawFittedText("PLAY VIEW", top.removeFromLeft(64.0f).toNearestInt(), juce::Justification::centredLeft, 1);

    drawStatusPill(g, top.removeFromRight(50.0f), state.safetyName, state.safetyName == "SAFE" ? juce::Colour(0xff8ee6c9) : juce::Colour(0xffff8f78));
    top.removeFromRight(4.0f);
    drawStatusPill(g, top.removeFromRight(60.0f), state.compareReady ? "A/B" : state.sourceName, juce::Colour(0xff7bb7ff));

    auto nameRow = content.removeFromTop(16.0f);
    g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    g.setColour(state.favorite ? juce::Colour(0xffffd27a) : juce::Colour(0xffedf7f4));
    g.drawFittedText(state.favorite ? "* " + state.selectedName : state.selectedName,
                     nameRow.toNearestInt(),
                     juce::Justification::centredLeft,
                     1,
                     0.62f);

    auto metaRow = content.removeFromTop(11.0f);
    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.setColour(juce::Colour(0xff9dafb3));
    g.drawFittedText(state.roleName + " | " + state.selectedMeta, metaRow.toNearestInt(), juce::Justification::centredLeft, 1, 0.54f);

    auto meters = content.removeFromTop(22.0f).withTrimmedTop(2.0f);
    static const std::array<const char*, 6> labels { "TONE", "DIRT", "MOVE", "SPACE", "WT", "BNC" };
    static const std::array<juce::uint32, 6> colours {
        0xff8ee6c9,
        0xffff8f78,
        0xff7bb7ff,
        0xffc4a7ff,
        0xffffc36b,
        0xffd7e37b
    };
    const auto gap = 4.0f;
    const auto meterWidth = (meters.getWidth() - (gap * 5.0f)) / 6.0f;
    for (size_t index = 0; index < labels.size(); ++index)
    {
        auto meter = meters.removeFromLeft(index + 1 == labels.size() ? meters.getWidth() : meterWidth);
        meters.removeFromLeft(gap);
        drawPerformanceMeter(g, meter, labels[index], state.performanceValues[index], juce::Colour(colours[index]));
    }

    if (content.getHeight() > 10.0f)
    {
        auto footer = content.removeFromTop(12.0f).withTrimmedTop(1.0f);
        const auto ratingText = state.rating > 0 ? juce::String(state.rating) + "/5" : juce::String("UNRATED");
        const auto sourceText = state.generated ? juce::String("GEN")
                             : state.factory ? juce::String("FACTORY")
                             : juce::String("USER");
        drawStatusPill(g, footer.removeFromLeft(54.0f), ratingText, juce::Colour(0xffffc36b));
        footer.removeFromLeft(4.0f);
        drawStatusPill(g, footer.removeFromLeft(58.0f), sourceText, accent);
        footer.removeFromLeft(4.0f);
        drawStatusPill(g, footer.removeFromLeft(44.0f), state.sequencerActive ? "SEQ" : "PLAY", juce::Colour(0xff8ee6c9));
        footer.removeFromLeft(4.0f);
        drawStatusPill(g, footer.removeFromLeft(52.0f), state.pumpActive ? "PUMP" : "DRY", juce::Colour(0xffd7e37b));
        footer.removeFromLeft(4.0f);
        drawStatusPill(g, footer, state.candidateReady ? state.candidateName : state.filterName + " " + juce::String(state.visibleCount) + "/" + juce::String(state.totalCount), juce::Colour(0xffc4a7ff));
    }
}
}
