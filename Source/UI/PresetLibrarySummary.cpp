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

void PresetLibrarySummary::drawPill(juce::Graphics& g,
                                    juce::Rectangle<float> area,
                                    const juce::String& text,
                                    juce::Colour accent)
{
    if (area.getWidth() < 22.0f || area.getHeight() < 10.0f)
        return;

    g.setColour(accent.withAlpha(0.18f));
    g.fillRoundedRectangle(area, 4.0f);
    g.setColour(accent.withAlpha(0.52f));
    g.drawRoundedRectangle(area, 4.0f, 1.0f);
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.setColour(accent.brighter(0.18f));
    g.drawFittedText(text, area.reduced(5.0f, 1.0f).toNearestInt(), juce::Justification::centred, 1, 0.58f);
}

void PresetLibrarySummary::drawProfileMeter(juce::Graphics& g,
                                            juce::Rectangle<float> area,
                                            const juce::String& label,
                                            float value,
                                            juce::Colour accent)
{
    if (area.getWidth() < 24.0f || area.getHeight() < 18.0f)
        return;

    value = clamp01(value);

    g.setColour(juce::Colour(0xff151d21));
    g.fillRoundedRectangle(area, 4.0f);
    g.setColour(accent.withAlpha(0.28f));
    g.drawRoundedRectangle(area, 4.0f, 1.0f);

    auto content = area.reduced(5.0f, 3.0f);
    g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
    g.setColour(juce::Colour(0xff75858a));
    g.drawFittedText(label, content.removeFromTop(9.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.58f);

    auto bar = content.withTrimmedTop(2.0f).withHeight(4.0f);
    g.setColour(juce::Colour(0xff243036));
    g.fillRoundedRectangle(bar, 2.0f);
    g.setColour(accent);
    g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * value), 2.0f);
}

void PresetLibrarySummary::setState(const State& newState)
{
    state = newState;
    for (auto& value : state.macroValues)
        value = clamp01(value);
    for (auto& value : state.profileValues)
        value = clamp01(value);

    if (state.selectedRole.trim().isEmpty())
        state.selectedRole = "Patch";
    if (state.selectedTraits.trim().isEmpty())
        state.selectedTraits = "No traits";
    if (state.selectedCue.trim().isEmpty())
        state.selectedCue = "Choose a preset";

    repaint();
}

void PresetLibrarySummary::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto compact = bounds.getHeight() < 178.0f;
    const auto accent = state.selectedFavorite ? juce::Colour(0xffffd27a)
                      : state.selectedFactory ? juce::Colour(0xff7bb7ff)
                      : juce::Colour(0xff8ee6c9);

    g.setColour(juce::Colour(0xff0d1316));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour(0xff2a363c));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
    g.setColour(accent.withAlpha(0.66f));
    g.fillRoundedRectangle(bounds.withHeight(3.0f), 2.0f);

    auto content = bounds.reduced(compact ? 9.0f : 12.0f, compact ? 6.0f : 10.0f);
    auto top = content.removeFromTop(compact ? 14.0f : 16.0f);
    g.setFont(juce::FontOptions(compact ? 8.8f : 10.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff75858a));
    g.drawFittedText(state.hasSelection ? "SELECTED PRESET" : "LIBRARY",
                     top.removeFromLeft(top.getWidth() * 0.42f).toNearestInt(),
                     juce::Justification::centredLeft,
                     1);

    drawPill(g,
             top.removeFromRight(compact ? 58.0f : 66.0f),
             state.selectedRating,
             juce::Colour(0xffffc36b));
    top.removeFromRight(4.0f);
    drawPill(g,
             top,
             state.selectedFactory ? "FACTORY" : (state.hasSelection ? "USER" : juce::String(state.totalCount) + " TOTAL"),
             accent);

    auto title = content.removeFromTop(compact ? 19.0f : 25.0f);
    g.setFont(juce::FontOptions(compact ? 13.0f : 17.0f, juce::Font::bold));
    g.setColour(state.selectedFavorite ? juce::Colour(0xffffd27a) : juce::Colour(0xffedf7f4));
    g.drawFittedText(state.selectedFavorite ? "* " + state.selectedName : state.selectedName,
                     title.toNearestInt(),
                     juce::Justification::centredLeft,
                     1,
                     0.62f);

    auto chipRow = content.removeFromTop(compact ? 18.0f : 22.0f).withTrimmedTop(2.0f);
    const std::array<juce::String, 3> chipText {
        state.selectedRole,
        state.selectedCategory,
        state.selectedKey + " " + state.selectedBpm
    };
    const std::array<juce::uint32, 3> chipColours { 0xff8ee6c9, 0xff7bb7ff, 0xffffc36b };
    const auto chipGap = 4.0f;
    const auto chipWidth = (chipRow.getWidth() - (chipGap * 2.0f)) / 3.0f;
    for (size_t index = 0; index < chipText.size(); ++index)
    {
        auto chip = chipRow.removeFromLeft(index + 1 == chipText.size() ? chipRow.getWidth() : chipWidth);
        chipRow.removeFromLeft(chipGap);
        drawPill(g, chip, chipText[index], juce::Colour(chipColours[index]));
    }

    content.removeFromTop(compact ? 3.0f : 7.0f);
    auto profileArea = content.removeFromTop(compact ? 28.0f : 38.0f);
    const auto profileGap = 4.0f;
    const auto profileWidth = (profileArea.getWidth() - (profileGap * 3.0f)) / 4.0f;
    for (size_t index = 0; index < state.profileValues.size(); ++index)
    {
        auto meter = profileArea.removeFromLeft(index + 1 == state.profileValues.size() ? profileArea.getWidth() : profileWidth);
        profileArea.removeFromLeft(profileGap);
        drawProfileMeter(g, meter, state.profileLabels[index], state.profileValues[index], accentForIndex(index));
    }

    if (content.getHeight() > 14.0f)
    {
        content.removeFromTop(compact ? 4.0f : 7.0f);
        auto macroArea = content.removeFromTop(compact ? 15.0f : 25.0f);
        if (! compact)
        {
            g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
            g.setColour(juce::Colour(0xff75858a));
            g.drawFittedText("MACRO SHAPE", macroArea.removeFromTop(10.0f).toNearestInt(), juce::Justification::centredLeft, 1);
        }

        auto macroStrip = macroArea.reduced(0.0f, 2.0f);
        const auto macroGap = 3.0f;
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
    }

    if (! compact && content.getHeight() > 44.0f)
    {
        content.removeFromTop(8.0f);
        auto stats = content.removeFromTop(40.0f);
        const auto statGap = 6.0f;
        const auto statWidth = (stats.getWidth() - (statGap * 2.0f)) / 3.0f;
        drawStat(g, stats.removeFromLeft(statWidth), "VISIBLE", juce::String(state.visibleCount), juce::Colour(0xff8ee6c9));
        stats.removeFromLeft(statGap);
        drawStat(g, stats.removeFromLeft(statWidth), "RATED", juce::String(state.ratedCount), juce::Colour(0xffffc36b));
        stats.removeFromLeft(statGap);
        drawStat(g, stats, "FACTORY", juce::String(state.factoryCount), juce::Colour(0xff7bb7ff));
    }

    if (content.getHeight() > 16.0f)
    {
        content.removeFromTop(5.0f);
        g.setFont(juce::FontOptions(compact ? 8.0f : 9.5f, juce::Font::plain));
        g.setColour(juce::Colour(0xff9dafb2));
        const auto summary = state.hasSelection
            ? state.selectedTraits + " | " + state.selectedCue
            : state.selectedCue + " | " + juce::String(state.favoriteCount) + " favorites | " + juce::String(state.userCount) + " user";
        g.drawFittedText(summary, content.toNearestInt(), juce::Justification::topLeft, compact ? 1 : 2, 0.58f);
    }
}
}
