#include "PresetSaveSummary.h"

#include <array>

namespace UI
{
void PresetSaveSummary::setState(const State& newState)
{
    state = newState;

    if (state.name.trim().isEmpty())
        state.name = "Untitled patch";
    if (state.category.trim().isEmpty())
        state.category = "User";
    if (state.author.trim().isEmpty())
        state.author = "User";
    if (state.pack.trim().isEmpty())
        state.pack = "User Pack";
    if (state.key.trim().isEmpty())
        state.key = "Any Key";
    if (state.bpm.trim().isEmpty())
        state.bpm = "Any Tempo";
    state.notesCharacters = juce::jmax(0, state.notesCharacters);

    repaint();
}

void PresetSaveSummary::drawPill(juce::Graphics& g,
                                 juce::Rectangle<float> area,
                                 const juce::String& label,
                                 const juce::String& value,
                                 juce::Colour accent)
{
    g.setColour(juce::Colour(0xff121b1f));
    g.fillRoundedRectangle(area, 4.0f);
    g.setColour(accent.withAlpha(0.42f));
    g.drawRoundedRectangle(area, 4.0f, 1.0f);

    auto content = area.reduced(6.0f, 4.0f);
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff73848a));
    g.drawFittedText(label, content.removeFromTop(9.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.68f);

    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffdce7e4));
    g.drawFittedText(value, content.toNearestInt(), juce::Justification::centredLeft, 1, 0.58f);
}

void PresetSaveSummary::drawReadinessSegment(juce::Graphics& g,
                                             juce::Rectangle<float> area,
                                             const juce::String& label,
                                             bool ready,
                                             juce::Colour accent)
{
    const auto fill = ready ? accent.withAlpha(0.22f) : juce::Colour(0xff182126);
    const auto outline = ready ? accent.withAlpha(0.7f) : juce::Colour(0xff2a363c);
    const auto text = ready ? juce::Colour(0xffe7f2ef) : juce::Colour(0xff73848a);

    g.setColour(fill);
    g.fillRoundedRectangle(area, 3.0f);
    g.setColour(outline);
    g.drawRoundedRectangle(area, 3.0f, 1.0f);

    auto content = area.reduced(3.0f, 1.0f);
    g.setFont(juce::FontOptions(6.8f, juce::Font::bold));
    g.setColour(text);
    g.drawFittedText(label, content.removeFromTop(8.0f).toNearestInt(), juce::Justification::centred, 1, 0.55f);
    g.setFont(juce::FontOptions(6.4f, juce::Font::bold));
    g.setColour(ready ? accent : juce::Colour(0xff4f5e64));
    g.drawFittedText(ready ? "OK" : "--", content.toNearestInt(), juce::Justification::centred, 1, 0.55f);
}

void PresetSaveSummary::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto compact = bounds.getHeight() < 124.0f;
    const auto accent = state.overwriteArmed ? juce::Colour(0xffff8f78)
                      : state.overwriteExists ? juce::Colour(0xffffc36b)
                      : state.generated ? juce::Colour(0xffc4a7ff)
                      : juce::Colour(0xff8ee6c9);

    g.setColour(juce::Colour(0xff0d1316));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour(0xff2a363c));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
    g.setColour(accent.withAlpha(0.7f));
    g.fillRoundedRectangle(bounds.withHeight(3.0f), 2.0f);

    auto content = bounds.reduced(compact ? 8.0f : 10.0f, compact ? 6.0f : 8.0f);
    auto header = content.removeFromTop(compact ? 14.0f : 16.0f);
    g.setFont(juce::FontOptions(compact ? 8.2f : 9.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff73848a));
    g.drawFittedText("SAVE PLAN", header.removeFromLeft(header.getWidth() * 0.42f).toNearestInt(), juce::Justification::centredLeft, 1);

    const auto status = state.overwriteArmed ? juce::String("OVERWRITE ARMED")
                      : state.overwriteExists ? juce::String("EXISTS")
                      : ! state.hasName ? juce::String("NEEDS NAME")
                      : state.generated ? juce::String("GENERATED")
                      : juce::String("NEW PATCH");
    g.setColour(accent);
    g.drawFittedText(status, header.toNearestInt(), juce::Justification::centredRight, 1, 0.68f);

    auto title = content.removeFromTop(compact ? 17.0f : 19.0f);
    g.setFont(juce::FontOptions(compact ? 13.0f : 15.0f, juce::Font::bold));
    g.setColour(state.hasName ? juce::Colour(0xffedf7f4) : juce::Colour(0xff85949a));
    g.drawFittedText(state.name, title.toNearestInt(), juce::Justification::centredLeft, 1, 0.58f);

    auto legalName = juce::File::createLegalFileName(state.name.trim());
    if (legalName.isEmpty())
        legalName = "Untitled";

    auto trail = content.removeFromTop(compact ? 15.0f : 17.0f);
    g.setColour(juce::Colour(0xff162126));
    g.fillRoundedRectangle(trail, 4.0f);
    g.setColour(juce::Colour(0xff26343a));
    g.drawRoundedRectangle(trail, 4.0f, 1.0f);
    g.setFont(juce::FontOptions(compact ? 8.0f : 9.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff9dafb3));
    const auto folderText = "Presets / " + state.category + " / " + legalName;
    g.drawFittedText(folderText,
                     trail.reduced(7.0f, 2.0f).toNearestInt(),
                     juce::Justification::centredLeft,
                     1,
                     0.48f);

    content.removeFromTop(compact ? 3.0f : 5.0f);
    auto readiness = content.removeFromTop(compact ? 18.0f : 22.0f);
    const std::array<bool, 5> readyItems {
        state.hasName,
        state.category.trim().isNotEmpty(),
        state.pack.trim().isNotEmpty(),
        state.key.trim().isNotEmpty() || state.bpm.trim().isNotEmpty(),
        state.notesCharacters > 0
    };
    const std::array<const char*, 5> readyLabels { "NAME", "FOLDER", "PACK", "CUE", "NOTES" };

    const auto segmentGap = 3.0f;
    const auto segmentWidth = (readiness.getWidth() - (segmentGap * 4.0f)) / 5.0f;
    for (size_t index = 0; index < readyItems.size(); ++index)
    {
        auto segment = readiness.removeFromLeft(index + 1 == readyItems.size() ? readiness.getWidth() : segmentWidth).reduced(0.0f, compact ? 1.0f : 0.0f);
        readiness.removeFromLeft(segmentGap);
        drawReadinessSegment(g, segment, readyLabels[index], readyItems[index], accent);
    }

    if (content.getHeight() > (compact ? 22.0f : 30.0f))
    {
        content.removeFromTop(compact ? 4.0f : 5.0f);
        auto pills = content.removeFromTop(compact ? 24.0f : 30.0f);
        const auto gap = 5.0f;
        const auto pillWidth = (pills.getWidth() - (gap * 2.0f)) / 3.0f;
        drawPill(g, pills.removeFromLeft(pillWidth), "PACK", state.pack, juce::Colour(0xff8ee6c9));
        pills.removeFromLeft(gap);
        drawPill(g, pills.removeFromLeft(pillWidth), "KEY", state.key, juce::Colour(0xffffc36b));
        pills.removeFromLeft(gap);
        drawPill(g, pills, "BPM", state.bpm, juce::Colour(0xff7bb7ff));
    }

    if (content.getHeight() > 12.0f)
    {
        content.removeFromTop(compact ? 3.0f : 5.0f);
        auto footer = content.removeFromTop(compact ? 13.0f : 15.0f);
        g.setColour(juce::Colour(0xff151d21));
        g.fillRoundedRectangle(footer, 4.0f);
        g.setColour(juce::Colour(0xff26343a));
        g.drawRoundedRectangle(footer, 4.0f, 1.0f);
        g.setFont(juce::FontOptions(compact ? 7.6f : 8.5f, juce::Font::bold));
        g.setColour(juce::Colour(0xff9dafb3));
        g.drawFittedText("AUTHOR  " + state.author + "     NOTES  " + juce::String(state.notesCharacters),
                         footer.reduced(7.0f, 2.0f).toNearestInt(),
                         juce::Justification::centredLeft,
                         1,
                         0.55f);
    }
}
}
