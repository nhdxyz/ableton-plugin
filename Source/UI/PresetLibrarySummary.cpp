#include "PresetLibrarySummary.h"

namespace UI
{
float PresetLibrarySummary::clamp01(float value) noexcept
{
    return juce::jlimit(0.0f, 1.0f, value);
}

juce::Colour PresetLibrarySummary::accentForIndex(size_t index)
{
    static constexpr std::array<juce::uint32, 8> colours {
        0xff8ee6c9,
        0xffffc36b,
        0xff7bb7ff,
        0xffff8f78,
        0xffd7e37b,
        0xff7fd0e6,
        0xfff0a86e,
        0xffc4a7ff
    };

    return juce::Colour(colours[index % colours.size()]);
}

void PresetLibrarySummary::drawStat(juce::Graphics& g,
                                    juce::Rectangle<float> area,
                                    const juce::String& label,
                                    const juce::String& value,
                                    juce::Colour accent)
{
    g.setColour(juce::Colour(0xff151d21));
    g.fillRoundedRectangle(area, 5.0f);
    g.setColour(accent.withAlpha(0.45f));
    g.drawRoundedRectangle(area, 5.0f, 1.0f);

    auto content = area.reduced(8.0f, 5.0f);
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff75858a));
    g.drawFittedText(label, content.removeFromTop(11.0f).toNearestInt(), juce::Justification::centredLeft, 1);
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffedf7f4));
    g.drawFittedText(value, content.toNearestInt(), juce::Justification::centredLeft, 1);
}

void PresetLibrarySummary::setState(const State& newState)
{
    state = newState;
    for (auto& value : state.macroValues)
        value = clamp01(value);

    repaint();
}

void PresetLibrarySummary::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xff0d1316));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour(0xff2a363c));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    auto content = bounds.reduced(12.0f, 10.0f);
    auto stats = content.removeFromTop(48.0f);
    const auto statGap = 6.0f;
    const auto statWidth = (stats.getWidth() - (statGap * 2.0f)) / 3.0f;
    drawStat(g, stats.removeFromLeft(statWidth), "VISIBLE", juce::String(state.visibleCount), juce::Colour(0xff8ee6c9));
    stats.removeFromLeft(statGap);
    drawStat(g, stats.removeFromLeft(statWidth), "RATED", juce::String(state.ratedCount), juce::Colour(0xffffc36b));
    stats.removeFromLeft(statGap);
    drawStat(g, stats, "FACTORY", juce::String(state.factoryCount), juce::Colour(0xff7bb7ff));

    content.removeFromTop(10.0f);
    auto title = content.removeFromTop(54.0f);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff75858a));
    g.drawFittedText(state.hasSelection ? "SELECTED PRESET" : "LIBRARY", title.removeFromTop(14.0f).toNearestInt(), juce::Justification::centredLeft, 1);

    g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    g.setColour(state.selectedFavorite ? juce::Colour(0xffffd27a) : juce::Colour(0xffedf7f4));
    g.drawFittedText(state.selectedFavorite ? "* " + state.selectedName : state.selectedName,
                     title.removeFromTop(22.0f).toNearestInt(),
                     juce::Justification::centredLeft,
                     1);

    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.setColour(juce::Colour(0xff9daeb2));
    const auto sourceText = state.selectedFactory ? "Factory" : (state.hasSelection ? "User" : juce::String(state.userCount) + " user");
    g.drawFittedText(sourceText + " | " + state.selectedCategory + " | " + state.selectedPack,
                     title.toNearestInt(),
                     juce::Justification::centredLeft,
                     1,
                     0.72f);

    auto meta = content.removeFromTop(38.0f);
    const std::array<juce::String, 3> metaText {
        state.selectedKey,
        state.selectedBpm,
        state.selectedRating
    };
    const std::array<juce::String, 3> metaLabels { "KEY", "BPM", "RATE" };
    const auto metaGap = 5.0f;
    const auto metaWidth = (meta.getWidth() - (metaGap * 2.0f)) / 3.0f;
    for (size_t index = 0; index < metaText.size(); ++index)
    {
        auto chip = meta.removeFromLeft(index + 1 == metaText.size() ? meta.getWidth() : metaWidth);
        meta.removeFromLeft(metaGap);
        g.setColour(juce::Colour(0xff172024));
        g.fillRoundedRectangle(chip, 4.0f);
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.setColour(juce::Colour(0xff75858a));
        g.drawFittedText(metaLabels[index], chip.reduced(7.0f, 3.0f).toNearestInt(), juce::Justification::centredLeft, 1);
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.setColour(juce::Colour(0xffdce7e4));
        g.drawFittedText(metaText[index],
                         chip.withTrimmedTop(12.0f).reduced(7.0f, 3.0f).toNearestInt(),
                         juce::Justification::centredLeft,
                         1,
                         0.7f);
    }

    content.removeFromTop(10.0f);
    auto macroArea = content.removeFromTop(44.0f);
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff75858a));
    g.drawFittedText("MACRO SHAPE", macroArea.removeFromTop(12.0f).toNearestInt(), juce::Justification::centredLeft, 1);

    auto macroStrip = macroArea.reduced(0.0f, 4.0f);
    const auto macroGap = 4.0f;
    const auto macroWidth = (macroStrip.getWidth() - (macroGap * 7.0f)) / 8.0f;
    for (size_t index = 0; index < state.macroValues.size(); ++index)
    {
        auto cell = macroStrip.removeFromLeft(index + 1 == state.macroValues.size() ? macroStrip.getWidth() : macroWidth);
        macroStrip.removeFromLeft(macroGap);
        const auto value = state.hasSelection ? clamp01(state.macroValues[index]) : 0.0f;
        g.setColour(juce::Colour(0xff172024));
        g.fillRoundedRectangle(cell, 3.0f);
        g.setColour(accentForIndex(index).withAlpha(state.hasSelection ? 0.82f : 0.22f));
        g.fillRoundedRectangle(cell.withWidth(cell.getWidth() * value), 3.0f);
    }

    if (content.getHeight() > 28.0f)
    {
        content.removeFromTop(8.0f);
        g.setFont(juce::FontOptions(10.0f));
        g.setColour(juce::Colour(0xff8b9ca0));
        const auto notes = state.selectedNotes.trim().isNotEmpty()
            ? state.selectedNotes.replaceCharacter('\n', ' ')
            : juce::String("Use notes to store role, macro intent, Ableton use, and mix-safety reminders.");
        g.drawFittedText(notes, content.toNearestInt(), juce::Justification::topLeft, 3, 0.62f);
    }
}
}
