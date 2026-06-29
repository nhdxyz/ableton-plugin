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

void PresetSaveSummary::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
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

    auto content = bounds.reduced(10.0f, 8.0f);
    auto header = content.removeFromTop(16.0f);
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff73848a));
    g.drawFittedText("SAVE TARGET", header.removeFromLeft(header.getWidth() * 0.42f).toNearestInt(), juce::Justification::centredLeft, 1);

    const auto status = state.overwriteArmed ? juce::String("OVERWRITE ARMED")
                      : state.overwriteExists ? juce::String("EXISTS")
                      : ! state.hasName ? juce::String("NEEDS NAME")
                      : state.generated ? juce::String("GENERATED")
                      : juce::String("NEW PATCH");
    g.setColour(accent);
    g.drawFittedText(status, header.toNearestInt(), juce::Justification::centredRight, 1, 0.68f);

    auto title = content.removeFromTop(19.0f);
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.setColour(state.hasName ? juce::Colour(0xffedf7f4) : juce::Colour(0xff85949a));
    g.drawFittedText(state.name, title.toNearestInt(), juce::Justification::centredLeft, 1, 0.58f);

    auto legalName = juce::File::createLegalFileName(state.name.trim());
    if (legalName.isEmpty())
        legalName = "Untitled";

    auto trail = content.removeFromTop(17.0f);
    g.setColour(juce::Colour(0xff162126));
    g.fillRoundedRectangle(trail, 4.0f);
    g.setColour(juce::Colour(0xff26343a));
    g.drawRoundedRectangle(trail, 4.0f, 1.0f);
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff9dafb3));
    g.drawFittedText("User Presets > " + state.category + " > " + legalName,
                     trail.reduced(7.0f, 2.0f).toNearestInt(),
                     juce::Justification::centredLeft,
                     1,
                     0.48f);

    content.removeFromTop(5.0f);
    auto readiness = content.removeFromTop(10.0f);
    const std::array<bool, 5> readyItems {
        state.hasName,
        state.category.trim().isNotEmpty(),
        state.pack.trim().isNotEmpty(),
        state.key.trim().isNotEmpty() || state.bpm.trim().isNotEmpty(),
        state.notesCharacters > 0
    };

    g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
    g.setColour(juce::Colour(0xff73848a));
    g.drawFittedText("READY", readiness.removeFromLeft(38.0f).toNearestInt(), juce::Justification::centredLeft, 1);
    const auto dotGap = 4.0f;
    const auto dotWidth = (readiness.getWidth() - (dotGap * 4.0f)) / 5.0f;
    for (size_t index = 0; index < readyItems.size(); ++index)
    {
        auto dot = readiness.removeFromLeft(index + 1 == readyItems.size() ? readiness.getWidth() : dotWidth).reduced(0.0f, 2.0f);
        readiness.removeFromLeft(dotGap);
        g.setColour(readyItems[index] ? accent : juce::Colour(0xff233036));
        g.fillRoundedRectangle(dot, 2.0f);
    }

    content.removeFromTop(5.0f);
    auto pills = content.removeFromTop(30.0f);
    const auto gap = 5.0f;
    const auto pillWidth = (pills.getWidth() - (gap * 2.0f)) / 3.0f;
    drawPill(g, pills.removeFromLeft(pillWidth), "PACK", state.pack, juce::Colour(0xff8ee6c9));
    pills.removeFromLeft(gap);
    drawPill(g, pills.removeFromLeft(pillWidth), "KEY", state.key, juce::Colour(0xffffc36b));
    pills.removeFromLeft(gap);
    drawPill(g, pills, "BPM", state.bpm, juce::Colour(0xff7bb7ff));

    if (content.getHeight() > 12.0f)
    {
        content.removeFromTop(5.0f);
        auto footer = content.removeFromTop(15.0f);
        g.setColour(juce::Colour(0xff151d21));
        g.fillRoundedRectangle(footer, 4.0f);
        g.setColour(juce::Colour(0xff26343a));
        g.drawRoundedRectangle(footer, 4.0f, 1.0f);
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.setColour(juce::Colour(0xff9dafb3));
        g.drawFittedText("AUTHOR  " + state.author + "     NOTES  " + juce::String(state.notesCharacters),
                         footer.reduced(7.0f, 2.0f).toNearestInt(),
                         juce::Justification::centredLeft,
                         1,
                         0.55f);
    }
}
}
