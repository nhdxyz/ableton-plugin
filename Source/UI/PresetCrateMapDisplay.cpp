#include "PresetCrateMapDisplay.h"

namespace UI
{
void PresetCrateMapDisplay::setState(const State& newState)
{
    state = newState;
    state.visibleCount = juce::jmax(0, state.visibleCount);
    state.totalCount = juce::jmax(0, state.totalCount);
    state.favoriteCount = juce::jmax(0, state.favoriteCount);
    state.ratedCount = juce::jmax(0, state.ratedCount);
    state.factoryCount = juce::jmax(0, state.factoryCount);
    state.userCount = juce::jmax(0, state.userCount);
    state.folderCount = juce::jmax(0, state.folderCount);
    state.packCount = juce::jmax(0, state.packCount);
    state.generatedCount = juce::jmax(0, state.generatedCount);
    state.macroRichCount = juce::jmax(0, state.macroRichCount);
    state.styleCount = juce::jmax(0, state.styleCount);

    if (state.filterName.trim().isEmpty())
        state.filterName = "All";
    if (state.tagName.trim().isEmpty())
        state.tagName = "All Tags";

    repaint();
}

float PresetCrateMapDisplay::proportion(int value, int total) noexcept
{
    if (total <= 0)
        return 0.0f;

    return juce::jlimit(0.0f, 1.0f, static_cast<float>(value) / static_cast<float>(total));
}

void PresetCrateMapDisplay::drawMetric(juce::Graphics& g,
                                       juce::Rectangle<float> area,
                                       const juce::String& label,
                                       int value,
                                       int total,
                                       juce::Colour accent)
{
    g.setColour(juce::Colour(0xff151d21));
    g.fillRoundedRectangle(area, 4.0f);
    g.setColour(accent.withAlpha(0.34f));
    g.drawRoundedRectangle(area, 4.0f, 1.0f);

    auto content = area.reduced(5.0f, 3.0f);
    auto textRow = content.removeFromTop(10.0f);
    g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
    g.setColour(juce::Colour(0xff73848a));
    g.drawFittedText(label, textRow.removeFromLeft(textRow.getWidth() * 0.55f).toNearestInt(), juce::Justification::centredLeft, 1, 0.64f);
    g.setColour(juce::Colour(0xffedf7f4));
    g.drawFittedText(juce::String(value), textRow.toNearestInt(), juce::Justification::centredRight, 1);

    auto bar = content.withHeight(4.0f).withTrimmedTop(2.0f);
    g.setColour(juce::Colour(0xff202a2f));
    g.fillRoundedRectangle(bar, 2.0f);
    g.setColour(accent);
    g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * proportion(value, total)), 2.0f);
}

void PresetCrateMapDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xff0d1316));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour(0xff2a363c));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    auto content = bounds.reduced(9.0f, 7.0f);
    auto header = content.removeFromTop(15.0f);
    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.setColour(juce::Colour(0xff73848a));
    g.drawFittedText("CRATES", header.removeFromLeft(header.getWidth() * 0.44f).toNearestInt(), juce::Justification::centredLeft, 1);

    auto countPill = header.reduced(0.0f, 1.0f);
    g.setColour(juce::Colour(0xff162126));
    g.fillRoundedRectangle(countPill, 4.0f);
    g.setColour(juce::Colour(0xff8ee6c9).withAlpha(0.5f));
    g.drawRoundedRectangle(countPill, 4.0f, 1.0f);
    g.setColour(juce::Colour(0xff8ee6c9));
    g.drawFittedText(juce::String(state.visibleCount) + " / " + juce::String(state.totalCount),
                     countPill.reduced(6.0f, 1.0f).toNearestInt(),
                     juce::Justification::centredRight,
                     1);

    auto context = content.removeFromTop(13.0f);
    auto activeText = state.filterName;
    if (state.tagName != "All Tags")
        activeText += " | " + state.tagName;
    if (state.searchText.trim().isNotEmpty())
        activeText += " | " + state.searchText.trim();

    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffa8b6b8));
    g.drawFittedText(activeText, context.toNearestInt(), juce::Justification::centredLeft, 1, 0.58f);

    content.removeFromTop(4.0f);
    auto primary = content.removeFromTop(22.0f);
    const auto visibleProportion = proportion(state.visibleCount, state.totalCount);
    g.setColour(juce::Colour(0xff151d21));
    g.fillRoundedRectangle(primary, 4.0f);
    g.setColour(juce::Colour(0xff8ee6c9).withAlpha(0.36f));
    g.drawRoundedRectangle(primary, 4.0f, 1.0f);
    auto primaryContent = primary.reduced(6.0f, 3.0f);
    auto primaryLabel = primaryContent.removeFromTop(8.0f);
    g.setFont(juce::FontOptions(7.2f, juce::Font::bold));
    g.setColour(juce::Colour(0xff73848a));
    g.drawFittedText("VISIBLE LIBRARY", primaryLabel.toNearestInt(), juce::Justification::centredLeft, 1, 0.66f);
    auto visibleTrack = primaryContent.withTrimmedTop(2.0f).withHeight(4.0f);
    g.setColour(juce::Colour(0xff202a2f));
    g.fillRoundedRectangle(visibleTrack, 2.0f);
    g.setColour(juce::Colour(0xff8ee6c9));
    g.fillRoundedRectangle(visibleTrack.withWidth(visibleTrack.getWidth() * visibleProportion), 2.0f);

    content.removeFromTop(5.0f);
    const auto gap = 4.0f;
    auto metrics = content;
    const auto metricHeight = metrics.getHeight() >= 78.0f ? 22.0f : 20.0f;
    const auto metricWidth = (metrics.getWidth() - gap) / 2.0f;

    auto drawMetricPair = [&] (const juce::String& leftLabel,
                               int leftValue,
                               juce::Colour leftAccent,
                               const juce::String& rightLabel,
                               int rightValue,
                               juce::Colour rightAccent)
    {
        if (metrics.getHeight() < metricHeight)
            return;

        auto row = metrics.removeFromTop(metricHeight);
        drawMetric(g, row.removeFromLeft(metricWidth), leftLabel, leftValue, state.totalCount, leftAccent);
        row.removeFromLeft(gap);
        drawMetric(g, row, rightLabel, rightValue, state.totalCount, rightAccent);
        metrics.removeFromTop(4.0f);
    };

    drawMetricPair("USER", state.userCount, juce::Colour(0xff8ee6c9),
                   "FAC", state.factoryCount, juce::Colour(0xff7bb7ff));
    drawMetricPair("FOLD", state.folderCount, juce::Colour(0xffd7e37b),
                   "PACK", state.packCount, juce::Colour(0xffffc36b));
    drawMetricPair("GEN", state.generatedCount, juce::Colour(0xffc4a7ff),
                   "STYLE", state.styleCount, juce::Colour(0xffff8f78));

    if (metrics.getHeight() >= 18.0f)
    {
        auto footer = metrics.removeFromTop(18.0f);
        const auto rich = state.macroRichCount > 0 ? juce::String(state.macroRichCount) + " macro-rich" : juce::String("No macro-rich");
        const auto rated = state.ratedCount > 0 ? juce::String(state.ratedCount) + " rated" : juce::String("No ratings");
        g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff9dafb3));
        g.drawFittedText(rich + " | " + rated + " | " + juce::String(state.favoriteCount) + " fav",
                         footer.toNearestInt(),
                         juce::Justification::centredLeft,
                         1,
                         0.58f);
    }
}
}
