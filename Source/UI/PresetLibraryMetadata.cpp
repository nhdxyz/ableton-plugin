#include "PresetLibraryMetadata.h"

#include "PresetLibraryChoices.h"

#include <cmath>
#include <initializer_list>

namespace UI::PresetLibraryMetadata
{
namespace
{
bool textContainsAny(const juce::String& text, std::initializer_list<const char*> terms)
{
    for (const auto* term : terms)
        if (text.containsIgnoreCase(term))
            return true;

    return false;
}
}

juce::String searchText(const NateVSTAudioProcessor::PresetInfo& preset)
{
    const auto sourceText = preset.isFactory ? juce::String("Factory") : juce::String("User");
    const auto ratingText = preset.rating > 0 ? juce::String(preset.rating) + " Star" : juce::String("Unrated");

    return preset.name + " " + preset.category + " " + preset.source + " " + preset.tags + " "
        + preset.folder + " " + sourceText + " " + ratingText + " " + preset.author + " "
        + preset.pack + " " + preset.key + " " + PresetLibraryChoices::formatBpm(preset.bpm) + " "
        + preset.macroSummary + " " + preset.notes
        + (preset.isFavorite ? " Favorite" : "");
}

bool matchesSmartCrate(const NateVSTAudioProcessor::PresetInfo& preset, const juce::String& crate)
{
    const auto text = searchText(preset);
    const auto isUkg = textContainsAny(text, { "UKG", "Garage", "2-Step", "Two Step", "Dred", "Reese" });
    const auto isHouse = textContainsAny(text, { "House", "Deep House", "Detroit House", "Lo-Fi", "Lo-Fi House", "Indie Dance", "Italo Disco", "Balearic House", "Afro", "Afro Melodic", "French House", "Soulful House", "Garage House", "Breaks House", "Nu Disco", "Progressive House", "Acid House", "Hard House" })
        && ! textContainsAny(text, { "Tech House", "Tech-House" });
    const auto isTechHouse = textContainsAny(text, { "Tech House", "Tech-House", "Deep Tech", "Tribal Tech House", "Afro Tech" });
    const auto isMinimal = textContainsAny(text, { "Minimal", "Hypno", "Click", "Microhouse", "Minimal FM", "Deep Minimal", "Bubble" });
    const auto isTechno = textContainsAny(text, { "Techno", "Warehouse", "Melodic Techno", "Acid Techno", "Raw Techno", "Hypnotic Techno", "Peak Time Techno", "Detroit Techno" });
    const auto isBass = textContainsAny(text, { "Bass", "Sub", "Dred", "Reese", "Rubber", "Low End" });
    const auto isChordOrStab = textContainsAny(text, { "Chord", "Stab", "Organ", "Keys", "Dub", "Seventh", "Mellow" });
    const auto isChop = textContainsAny(text, { "Chop", "Vocal", "Sample", "Slice" });
    const auto isPluck = textContainsAny(text, { "Pluck", "Bell", "Ping", "Marimba", "Bubble" });
    const auto isDirty = textContainsAny(text, { "Dirty", "Dirt", "Drive", "Distortion", "Acid", "Warehouse", "Metallic" })
        || preset.macroValues[1] >= 0.45f;
    const auto isMonoSafe = textContainsAny(text, { "Mono Safe", "Mono-Safe", "Sub Safe" })
        || (isBass && preset.macroValues[4] >= 0.35f && preset.macroValues[3] <= 0.45f);
    const auto isSequenced = textContainsAny(text, { "Sequence", "Sequenced", "Pattern", "Groove", "Arp", "Pulse", "Roll" });

    if (crate == "Five Star Crate")
        return preset.rating == 5;
    if (crate == "Club Bass")
        return isBass;
    if (crate == "Chord Stabs")
        return isChordOrStab;
    if (crate == "Dub Stabs")
        return textContainsAny(text, { "Dub", "Chord", "Stab" }) && (isHouse || isTechno || isMinimal || isUkg);
    if (crate == "Project Pack")
        return preset.pack.equalsIgnoreCase("Project Pack") || (! preset.isFactory && ! preset.source.equalsIgnoreCase("Generated"));
    if (crate == "Factory Pack")
        return preset.isFactory || preset.pack.equalsIgnoreCase("Factory Pack");
    if (crate == "UKG Essentials")
        return isUkg;
    if (crate == "UKG Basslines")
        return isUkg && isBass;
    if (crate == "Garage Chops")
        return isUkg && isChop;
    if (crate == "House Chords")
        return isHouse && isChordOrStab;
    if (crate == "House Tools")
        return isHouse;
    if (crate == "Tech House Tools")
        return isTechHouse;
    if (crate == "Minimal Plucks")
        return isMinimal && isPluck;
    if (crate == "Minimal Tools")
        return isMinimal;
    if (crate == "Techno Stabs")
        return isTechno && isChordOrStab;
    if (crate == "Techno Tools")
        return isTechno;
    if (crate == "Dirty Tools")
        return isDirty;
    if (crate == "Mono Safe")
        return isMonoSafe;
    if (crate == "Sequenced Grooves")
        return isSequenced;

    return false;
}

juce::String auditionRoleName(PresetAuditionRole role)
{
    switch (role)
    {
        case PresetAuditionRole::bass: return "Bass phrase";
        case PresetAuditionRole::chord: return "Chord stab phrase";
        case PresetAuditionRole::chop: return "Garage chop phrase";
        case PresetAuditionRole::sequence: return "Groove phrase";
        case PresetAuditionRole::fx: return "FX sweep phrase";
        case PresetAuditionRole::lead: return "Lead phrase";
        case PresetAuditionRole::pluck: return "Pluck phrase";
        case PresetAuditionRole::general:
        default: return "Patch phrase";
    }
}

PresetAuditionRole inferAuditionRole(const NateVSTAudioProcessor::PresetInfo* preset)
{
    if (preset == nullptr)
        return PresetAuditionRole::general;

    const auto text = searchText(*preset);
    if (textContainsAny(text, { "Chop", "Vocal", "Slice", "Sample" }))
        return PresetAuditionRole::chop;
    if (textContainsAny(text, { "Bass", "Sub", "Dred", "Reese", "Rubber", "Low End" }))
        return PresetAuditionRole::bass;
    if (textContainsAny(text, { "Chord", "Stab", "Organ", "Keys", "Dub", "Seventh", "Mellow" }))
        return PresetAuditionRole::chord;
    if (textContainsAny(text, { "Sequence", "Sequenced", "Pattern", "Groove", "Arp", "Pulse", "Roll" }))
        return PresetAuditionRole::sequence;
    if (textContainsAny(text, { "FX", "Noise", "Riser", "Drop", "Throw", "Sweep" }))
        return PresetAuditionRole::fx;
    if (textContainsAny(text, { "Lead" }))
        return PresetAuditionRole::lead;
    if (textContainsAny(text, { "Pluck", "Bell", "Ping", "Marimba" }))
        return PresetAuditionRole::pluck;

    return PresetAuditionRole::general;
}

juce::String inspectorRoleName(PresetAuditionRole role)
{
    switch (role)
    {
        case PresetAuditionRole::bass: return "Bass";
        case PresetAuditionRole::chord: return "Chord/Stab";
        case PresetAuditionRole::chop: return "Chop";
        case PresetAuditionRole::sequence: return "Groove";
        case PresetAuditionRole::fx: return "FX";
        case PresetAuditionRole::lead: return "Lead";
        case PresetAuditionRole::pluck: return "Pluck";
        case PresetAuditionRole::general:
        default: return "Patch";
    }
}

juce::String tempoBand(int bpm)
{
    if (bpm < 20)
        return "Any tempo";
    if (bpm <= 124)
        return "Deep / warm";
    if (bpm <= 128)
        return "House drive";
    if (bpm <= 132)
        return "Tech pressure";

    return "Garage pace";
}

juce::String inspectorTraits(const NateVSTAudioProcessor::PresetInfo& preset)
{
    const auto text = searchText(preset);
    juce::StringArray traits;

    if (textContainsAny(text, { "Mono Safe", "Mono-Safe", "Sub Safe" }))
        traits.add("Mono safe");
    if (textContainsAny(text, { "Sequence", "Sequenced", "Pattern", "Groove", "Roll", "Pulse" }))
        traits.add("Sequenced");
    if (textContainsAny(text, { "Pump", "Sidechain", "Duck" }))
        traits.add("Pump");
    if (textContainsAny(text, { "Guard", "Clip", "Limiter" }))
        traits.add("Guard");
    if (textContainsAny(text, { "Wide", "Space", "Pad", "Wash" }))
        traits.add("Wide");
    if (textContainsAny(text, { "Dirty", "Drive", "Acid", "Raw", "Warehouse", "Metallic" })
        || preset.macroValues[1] >= 0.48f)
    {
        traits.add("Dirt");
    }
    if (preset.source.equalsIgnoreCase("Generated") || textContainsAny(text, { "Generated", "Random Lab" }))
        traits.add("Generated");

    while (traits.size() > 4)
        traits.remove(traits.size() - 1);

    return traits.isEmpty() ? juce::String("Clean starter") : traits.joinIntoString(" | ");
}

juce::String inspectorCue(const NateVSTAudioProcessor::PresetInfo& preset)
{
    const auto role = inferAuditionRole(&preset);
    const auto tempo = tempoBand(preset.bpm);
    const auto key = preset.key.trim().isNotEmpty() ? preset.key : juce::String("Any Key");

    switch (role)
    {
        case PresetAuditionRole::bass:
            return tempo + " bassline | " + key;
        case PresetAuditionRole::chord:
            return tempo + " stab layer | " + key;
        case PresetAuditionRole::chop:
            return tempo + " chop idea | " + key;
        case PresetAuditionRole::sequence:
            return tempo + " pattern | " + key;
        case PresetAuditionRole::fx:
            return tempo + " transition | " + key;
        case PresetAuditionRole::lead:
            return tempo + " hook | " + key;
        case PresetAuditionRole::pluck:
            return tempo + " pluck | " + key;
        case PresetAuditionRole::general:
        default:
            return tempo + " patch | " + key;
    }
}

std::array<float, 4> inspectorProfile(const NateVSTAudioProcessor::PresetInfo& preset)
{
    const auto tone = juce::jlimit(0.0f, 1.0f, preset.macroValues[0]);
    const auto dirt = juce::jlimit(0.0f, 1.0f, preset.macroValues[1]);
    const auto motion = juce::jlimit(0.0f, 1.0f, (preset.macroValues[2] * 0.58f)
                                                  + (preset.macroValues[5] * 0.24f)
                                                  + (preset.macroValues[6] * 0.18f));
    const auto space = juce::jlimit(0.0f, 1.0f, (preset.macroValues[3] * 0.72f)
                                                 + (preset.macroValues[7] * 0.28f));

    return { tone, dirt, motion, space };
}

juce::String macroPreviewText(const NateVSTAudioProcessor::PresetInfo& preset)
{
    const auto summary = preset.macroSummary.trim();
    const auto values = [&preset]
    {
        static constexpr std::array<const char*, 8> labels { "T", "D", "M", "S", "W", "B", "Wr", "Th" };
        juce::StringArray valueText;

        for (size_t index = 0; index < labels.size(); ++index)
            valueText.add(juce::String(labels[index]) + juce::String(juce::roundToInt(preset.macroValues[index] * 100.0f)));

        return valueText.joinIntoString(" ");
    }();

    return (summary.isNotEmpty() ? "Macros: " + summary : juce::String("Macros: flat"))
        + " | " + values;
}

juce::String previewSummaryText(const NateVSTAudioProcessor::PresetInfo& preset)
{
    if (! preset.previewAvailable)
        return "Preview renders on audition";

    if (preset.previewStale)
        return "Preview stale";

    if (preset.previewPeak > 0.0f && preset.previewRms > 0.0f)
        return "Preview ready | pk "
            + juce::String(juce::Decibels::gainToDecibels(preset.previewPeak, -100.0f), 1)
            + " dB";

    return "Preview ready";
}

juce::String previewBadgeText(const NateVSTAudioProcessor::PresetInfo& preset)
{
    if (! preset.previewAvailable)
        return "RENDER";

    if (preset.previewStale)
        return "STALE";

    if (preset.previewPeak > 0.0f && std::isfinite(preset.previewPeak))
        return "PK " + juce::String(juce::Decibels::gainToDecibels(preset.previewPeak, -100.0f), 0) + " dB";

    return "READY";
}

float previewLevelNormalised(const NateVSTAudioProcessor::PresetInfo& preset)
{
    if (! preset.previewAvailable || preset.previewStale || preset.previewPeak <= 0.0f || ! std::isfinite(preset.previewPeak))
        return 0.0f;

    const auto peakDb = juce::Decibels::gainToDecibels(preset.previewPeak, -100.0f);
    return juce::jlimit(0.0f, 1.0f, juce::jmap(peakDb, -60.0f, -6.0f, 0.0f, 1.0f));
}

void drawPreviewLevelBadge(juce::Graphics& g,
                           juce::Rectangle<int> area,
                           const NateVSTAudioProcessor::PresetInfo& preset)
{
    if (area.getWidth() <= 0 || area.getHeight() <= 0)
        return;

    const auto ready = preset.previewAvailable && ! preset.previewStale;
    const auto stale = preset.previewStale;
    const auto accent = ready ? juce::Colour(0xff8ee6c9)
                              : (stale ? juce::Colour(0xffffc36b) : juce::Colour(0xff617078));
    const auto bounds = area.toFloat().reduced(1.0f, 1.0f);
    g.setColour(juce::Colour(0xff151e22));
    g.fillRoundedRectangle(bounds, 4.0f);

    const auto levelWidth = bounds.getWidth() * previewLevelNormalised(preset);
    if (levelWidth > 1.0f)
    {
        g.setColour(accent.withAlpha(0.28f));
        g.fillRoundedRectangle(bounds.withWidth(levelWidth), 4.0f);
    }

    g.setColour(accent.withAlpha(ready ? 0.78f : 0.52f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.setColour(ready ? juce::Colour(0xffdff8f0) : accent);
    g.drawFittedText(previewBadgeText(preset),
                     area.reduced(3, 0),
                     juce::Justification::centred,
                     1,
                     0.58f);
}
}
