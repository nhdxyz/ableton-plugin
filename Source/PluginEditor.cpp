#include "PluginEditor.h"

#include "Modulation/ModulationRouting.h"
#include "Synth/WavetableFrameRecipes.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <typeinfo>
#include <utility>
#include <vector>

namespace
{
constexpr auto editorMinWidth = 1040;
constexpr auto editorMinHeight = 760;
constexpr auto editorDefaultWidth = 1280;
constexpr auto editorDefaultHeight = 820;
constexpr auto editorMaxWidth = 1680;
constexpr auto editorMaxHeight = 1040;
constexpr auto pianoKeyboardHeight = 96;
constexpr auto keyboardControlsWidth = 356;
constexpr auto keyboardMinimumWhiteKeyWidth = 20.0f;
constexpr auto keyboardMaximumWhiteKeyWidth = 96.0f;
constexpr auto keyboardLowestNote = 36;
constexpr auto keyboardHighestNote = 96;
constexpr auto keyboardVisualLowestNote = 36;
constexpr auto keyboardVisualHighestNote = 84;
constexpr auto keyboardInitialLowestNote = 72;
constexpr auto keyboardMinLowestVisibleNote = 36;
constexpr auto keyboardMaxLowestVisibleNote = 72;
constexpr auto keyboardTypingKeySpanSemitones = 16;
constexpr auto abletonMiddleCOctave = 3;
constexpr auto modMatrixTitleHeight = 28;
constexpr auto modMatrixInspectorHeight = 30;
constexpr auto modMatrixRouteMapHeight = 46;
constexpr auto modMatrixHeaderHeight = 18;
constexpr auto modMatrixMinimumRowHeight = 26;
constexpr auto modMatrixRouteMapMinimumRowHeight = 26;
constexpr auto firstMacroModSourceIndex = 4;
constexpr auto presetAuditionVelocity = 0.86f;
constexpr auto candidateAuditionDurationMs = 680.0;
constexpr auto candidateAuditionVelocity = 0.88f;
constexpr auto fxRackStatusOverrideMs = 2200.0;
constexpr auto presetOverwriteConfirmMs = 6500.0;
constexpr auto presetBrowserCompactRowBreakpoint = 420;
constexpr auto sequencerFourBarChainTransformIndex = 12;
constexpr std::array<const char*, 8> macroPerformanceParameterIDs {
    Parameters::ID::macroTone,
    Parameters::ID::macroDirt,
    Parameters::ID::macroMotion,
    Parameters::ID::macroSpace,
    Parameters::ID::macroWeight,
    Parameters::ID::macroBounce,
    Parameters::ID::macroWarp,
    Parameters::ID::macroThrow
};
constexpr std::array<const char*, 8> macroPerformanceNames {
    "Tone",
    "Dirt",
    "Motion",
    "Space",
    "Weight",
    "Bounce",
    "Warp",
    "Throw"
};

enum class PresetAuditionRole
{
    bass,
    chord,
    chop,
    sequence,
    fx,
    lead,
    pluck,
    general
};

constexpr std::array<const char*, 34> momentaryFxParameterIDs {
    Parameters::ID::fxDelayEnabled,
    Parameters::ID::fxDelaySync,
    Parameters::ID::fxDelayRate,
    Parameters::ID::fxDelayTime,
    Parameters::ID::fxDelayFeedback,
    Parameters::ID::fxDelayMix,
    Parameters::ID::fxReverbEnabled,
    Parameters::ID::fxReverbSize,
    Parameters::ID::fxReverbDamping,
    Parameters::ID::fxReverbMix,
    Parameters::ID::fxPumpEnabled,
    Parameters::ID::fxPumpRate,
    Parameters::ID::fxPumpCurve,
    Parameters::ID::fxPumpCustomCurve[0],
    Parameters::ID::fxPumpCustomCurve[1],
    Parameters::ID::fxPumpCustomCurve[2],
    Parameters::ID::fxPumpCustomCurve[3],
    Parameters::ID::fxPumpCustomCurve[4],
    Parameters::ID::fxPumpCustomCurve[5],
    Parameters::ID::fxPumpCustomCurve[6],
    Parameters::ID::fxPumpCustomCurve[7],
    Parameters::ID::fxPumpDepth,
    Parameters::ID::fxPumpShape,
    Parameters::ID::fxPumpPhase,
    Parameters::ID::fxWidthEnabled,
    Parameters::ID::fxWidthAmount,
    Parameters::ID::fxWidthMonoCutoff,
    Parameters::ID::fxGuardEnabled,
    Parameters::ID::fxGuardPush,
    Parameters::ID::fxGuardGlue,
    Parameters::ID::fxGuardPunch,
    Parameters::ID::fxGuardClipMix,
    Parameters::ID::fxGuardCeiling,
    Parameters::ID::outputGain
};
constexpr auto lastMacroModSourceIndex = 11;
constexpr std::array<float, 8> defaultSlicePitchLadder { -12.0f, -7.0f, -5.0f, 0.0f, 3.0f, 7.0f, 10.0f, 12.0f };
constexpr std::array<float, 8> defaultGarageSlicePitch { -12.0f, 0.0f, 7.0f, -5.0f, 0.0f, 12.0f, 3.0f, -7.0f };
constexpr std::array<int, 17> computerKeyboardKeyCodes {
    'a', 'w', 's', 'e', 'd', 'f', 't', 'g', 'y', 'h', 'u', 'j', 'k', 'o', 'l', 'p', ';'
};

static_assert(keyboardLowestNote <= keyboardMinLowestVisibleNote);
static_assert(keyboardMinLowestVisibleNote <= keyboardInitialLowestNote);
static_assert(keyboardInitialLowestNote <= keyboardMaxLowestVisibleNote);
static_assert(keyboardMaxLowestVisibleNote + keyboardTypingKeySpanSemitones <= keyboardHighestNote);
static_assert(keyboardLowestNote <= keyboardVisualLowestNote);
static_assert(keyboardVisualLowestNote <= keyboardVisualHighestNote);
static_assert(keyboardVisualHighestNote <= keyboardHighestNote);
static_assert(keyboardVisualLowestNote == 36);
static_assert(keyboardVisualHighestNote == 84);
static_assert(keyboardInitialLowestNote == 72);
static_assert(keyboardMinLowestVisibleNote == 36);
static_assert(keyboardMaxLowestVisibleNote == 72);

int clampedKeyboardLowestVisibleNote(int note) noexcept
{
    return juce::jlimit(keyboardMinLowestVisibleNote, keyboardMaxLowestVisibleNote, note);
}

int keyboardTypingBaseNoteForLowestNote(int note) noexcept
{
    return clampedKeyboardLowestVisibleNote(note);
}

juce::String abletonNoteName(int midiNote)
{
    return juce::MidiMessage::getMidiNoteName(midiNote, true, true, abletonMiddleCOctave);
}

int whiteKeyCountInRange(int lowestNote, int highestNote) noexcept
{
    auto count = 0;
    for (auto note = lowestNote; note <= highestNote; ++note)
        if (! juce::MidiMessage::isMidiNoteBlack(note % 12))
            ++count;

    return juce::jmax(1, count);
}

float responsiveKeyboardKeyWidthForBounds(juce::Rectangle<int> keyboardBounds, int) noexcept
{
    const auto whiteKeyCount = static_cast<float>(whiteKeyCountInRange(keyboardVisualLowestNote,
                                                                       keyboardVisualHighestNote));
    const auto fillWidth = static_cast<float>(juce::jmax(1, keyboardBounds.getWidth())) / whiteKeyCount;
    return juce::jlimit(keyboardMinimumWhiteKeyWidth, keyboardMaximumWhiteKeyWidth, fillWidth);
}

template <typename... Components>
bool anyComponentVisible(const Components&... components) noexcept
{
    return (... || components.isVisible());
}

struct PresetBrowserRowLayout
{
    int paddedWidth = 0;
    int nameWidth = 0;
    int infoWidth = 0;
    int previewWidth = 0;
    int macroWidth = 0;
    bool compact = false;
    bool showsMacroStrip = true;
};

PresetBrowserRowLayout presetBrowserRowLayoutForWidth(int width, int height)
{
    auto row = juce::Rectangle<int>(0, 0, juce::jmax(0, width), juce::jmax(0, height)).reduced(9, 4);
    PresetBrowserRowLayout layout;
    layout.paddedWidth = juce::jmax(0, row.getWidth());
    if (layout.paddedWidth <= 0)
        return layout;

    layout.compact = layout.paddedWidth < presetBrowserCompactRowBreakpoint;
    layout.compact = layout.compact || height <= 34;
    if (layout.compact)
    {
        constexpr auto gap = 4;
        layout.previewWidth = juce::jlimit(60, 92, layout.paddedWidth / 5);
        layout.infoWidth = juce::jlimit(52, 78, layout.paddedWidth / 6);
        layout.macroWidth = 0;
        layout.showsMacroStrip = false;
        layout.nameWidth = juce::jmax(0, layout.paddedWidth - layout.previewWidth - layout.infoWidth - (gap * 2));
        return layout;
    }

    auto remainingWidth = layout.paddedWidth;
    layout.infoWidth = juce::jlimit(78, 118, remainingWidth / 5);
    remainingWidth -= layout.infoWidth + 6;
    layout.previewWidth = juce::jlimit(78, 118, remainingWidth / 5);
    remainingWidth -= layout.previewWidth + 6;
    layout.macroWidth = 0;
    layout.showsMacroStrip = false;
    layout.nameWidth = juce::jmax(0, remainingWidth);
    return layout;
}

struct SampleSlicePreviewSettings
{
    float pitch = 0.0f;
    float gain = -6.0f;
    bool reverse = false;
    bool stutter = false;
    bool choke = false;
    int repeats = 3;
    float pan = 0.0f;
    float probability = 1.0f;
    float nudgePercent = 0.0f;
    float fade = 0.0f;
};

juce::String slicePanText(float pan)
{
    pan = juce::jlimit(-1.0f, 1.0f, pan);
    if (pan < -0.05f)
        return "L" + juce::String(juce::roundToInt(std::abs(pan) * 100.0f));
    if (pan > 0.05f)
        return "R" + juce::String(juce::roundToInt(pan * 100.0f));

    return "C";
}

juce::String sliceChanceText(float probability)
{
    return juce::String(juce::roundToInt(juce::jlimit(0.0f, 1.0f, probability) * 100.0f)) + "%";
}

juce::String sliceNudgeText(float nudgePercent)
{
    nudgePercent = juce::jlimit(-5.0f, 5.0f, nudgePercent);
    if (std::abs(nudgePercent) < 0.01f)
        return "N0";

    return juce::String(nudgePercent > 0.0f ? "N+" : "N") + juce::String(nudgePercent, 1) + "%";
}

juce::String sliceFadeText(float fade)
{
    fade = juce::jlimit(0.0f, 1.0f, fade);
    if (fade < 0.12f)
        return "tight";
    if (fade < 0.55f)
        return "soft";

    return "smooth";
}

juce::String sourceNameForWave(int wave)
{
    switch (wave)
    {
        case 0: return "Sine";
        case 1: return "Saw";
        case 2: return "Square";
        case 3: return "Triangle";
        case 4: return "Wavetable";
        case 5: return "Organ";
        case 6: return "House Piano";
        default: return "Synth";
    }
}

juce::String bodyRoleForWave(int wave)
{
    switch (wave)
    {
        case 4: return "WT BODY";
        case 5: return "ORGAN";
        case 6: return "KEYS";
        default: return "BODY";
    }
}

juce::String characterRoleForWave(int wave)
{
    switch (wave)
    {
        case 4: return "WT";
        case 5: return "DRAW";
        case 6: return "STAB";
        default: return "CHAR";
    }
}

SampleSlicePreviewSettings defaultSlicePreviewSettings(size_t sliceIndex, int styleIndex)
{
    const auto safeIndex = juce::jlimit<size_t>(0, defaultSlicePitchLadder.size() - 1, sliceIndex);
    const auto slicePosition = static_cast<int>(safeIndex);
    SampleSlicePreviewSettings settings;

    switch (juce::jlimit(0, 4, styleIndex))
    {
        case 1: // Pitch
            settings.pitch = defaultSlicePitchLadder[safeIndex];
            settings.gain = -7.0f + static_cast<float>(slicePosition % 3);
            break;

        case 2: // Reverse
            settings.reverse = (slicePosition % 2) != 0;
            settings.pitch = defaultSlicePitchLadder[static_cast<size_t>((slicePosition + 2) % 8)];
            settings.gain = -7.5f;
            break;

        case 3: // Stutter
            settings.pitch = defaultGarageSlicePitch[static_cast<size_t>((slicePosition + 1) % 8)] * 0.5f;
            settings.gain = -8.0f;
            settings.stutter = true;
            settings.choke = true;
            settings.repeats = 2 + (slicePosition % 4);
            break;

        case 4: // Garage
            settings.reverse = slicePosition == 2 || slicePosition == 6;
            settings.pitch = defaultGarageSlicePitch[safeIndex];
            settings.gain = -8.5f + static_cast<float>(slicePosition % 4) * 0.8f;
            settings.stutter = slicePosition == 3 || slicePosition == 7;
            settings.choke = true;
            settings.repeats = slicePosition == 7 ? 5 : 3;
            break;

        case 0: // Clean
        default:
            break;
    }

    return settings;
}

juce::String layoutAuditComponentName(const juce::Component& component, int siblingIndex)
{
    if (component.getName().isNotEmpty())
        return component.getName();

    if (component.getComponentID().isNotEmpty())
        return component.getComponentID();

    return juce::String(typeid(component).name()) + "#" + juce::String(siblingIndex);
}

void appendVisibleLayoutIssues(const juce::Component& root,
                               const juce::Component& component,
                               const juce::String& panelName,
                               const juce::String& parentPath,
                               juce::StringArray& issues)
{
    const auto editorBounds = root.getLocalBounds();

    for (auto childIndex = 0; childIndex < component.getNumChildComponents(); ++childIndex)
    {
        const auto* child = component.getChildComponent(childIndex);
        if (child == nullptr || ! child->isVisible())
            continue;

        const auto childName = layoutAuditComponentName(*child, childIndex);
        const auto childPath = parentPath.isEmpty() ? childName : parentPath + "/" + childName;
        const auto* parent = child->getParentComponent();
        const auto boundsInEditor = parent != nullptr
            ? root.getLocalArea(parent, child->getBounds())
            : child->getBounds();

        if (boundsInEditor.isEmpty())
        {
            issues.add(panelName + ": " + childPath + " has empty bounds "
                       + boundsInEditor.toString());
        }
        else if (! editorBounds.contains(boundsInEditor))
        {
            issues.add(panelName + ": " + childPath + " overflows editor bounds "
                       + boundsInEditor.toString() + " outside " + editorBounds.toString());
        }

        // Viewport contents can be virtual/clipped rows; audit the viewport shell, not its scroll contents.
        if (dynamic_cast<const juce::Viewport*>(child) == nullptr)
            appendVisibleLayoutIssues(root, *child, panelName, childPath, issues);
    }
}

juce::Colour backgroundColour(const UI::Theme& theme)
{
    return theme.background;
}

juce::Colour panelColour(const UI::Theme& theme)
{
    return theme.panel;
}

juce::StringArray presetCategoryChoices()
{
    return {
        "User",
        "Bass",
        "Stab",
        "Lead",
        "House",
        "House/Acid",
        "House/Bass",
        "House/Chords",
        "House/Keys",
        "House/Plucks",
        "House/Indie Dance",
        "House/Italo Disco",
        "House/Nu Disco",
        "House/Balearic",
        "House/Afro Melodic",
        "House/Progressive",
        "House/Hard House",
        "House/Lo-Fi",
        "Afro House",
        "Amapiano",
        "Organic House",
        "Melodic Techno",
        "Drum & Bass",
        "Bass House",
        "Hard Techno",
        "Indie Dance",
        "Trance",
        "Ambient",
        "Future Garage",
        "Latin Tech",
        "Tech House",
        "Tech House/Bass",
        "Tech House/Deep Tech",
        "Tech House/Afro Tech",
        "Techno",
        "Techno/Dub",
        "Techno/Hardgroove",
        "Techno/Peak Time",
        "Techno/Detroit",
        "Techno/Melodic",
        "Techno/Stabs",
        "Minimal",
        "Minimal/FM",
        "Minimal/Deep",
        "Minimal/Plucks",
        "Romanian Minimal",
        "UKG",
        "UKG/Bass",
        "UKG/Chops",
        "UKG/Organ",
        "UKG/Stabs",
        "UKG/Bells",
        "Electro Breaks",
        "FX",
        "Sequence",
        "Sample"
    };
}

juce::StringArray presetFilterChoices()
{
    return {
        "All",
        "Favorites",
        "Recent",
        "Rated",
        "5 Stars",
        "4+ Stars",
        "Macro Rich",
        "Generated",
        "User",
        "Factory",
        "Similar",
        "Five Star Crate",
        "Club Bass",
        "Chord Stabs",
        "Dub Stabs",
        "Bass",
        "Chord",
        "Pad",
        "Stab",
        "Lead",
        "House",
        "Tech House",
        "Techno",
        "Minimal",
        "UKG",
        "Afro House",
        "Organic House",
        "Drum & Bass",
        "Hard Techno",
        "Trance",
        "Ambient",
        "Latin Tech",
        "Bass House",
        "Amapiano",
        "Hardgroove",
        "Future Garage",
        "Speed Garage",
        "Deep Tech",
        "Indie Dance",
        "Italo Disco",
        "Balearic House",
        "Acid House",
        "Nu Disco",
        "Afro Tech",
        "Afro Melodic",
        "Progressive House",
        "Hard House",
        "Peak Time Techno",
        "Detroit Techno",
        "Melodic Techno",
        "Minimal FM",
        "Deep Minimal",
        "Lo-Fi House",
        "French House",
        "Soulful House",
        "Garage House",
        "Microhouse",
        "Raw Techno",
        "Tribal Tech House",
        "Breaks House",
        "Chicago House",
        "Classic House",
        "Funky House",
        "Melodic House",
        "Romanian Minimal",
        "Dub Techno",
        "Electro Breaks",
        "FX",
        "Sequence",
        "Sample",
        "Project Pack",
        "Factory Pack",
        "UKG Essentials",
        "UKG Basslines",
        "Garage Chops",
        "House Chords",
        "House Tools",
        "House Expansion",
        "Afro House Rituals",
        "Amapiano Log Lab",
        "Melodic Techno Motion",
        "Organic Progressive",
        "Drum & Bass Tools",
        "Bass House Weapons",
        "Hard Techno Rave",
        "Indie Dance Dark Disco",
        "Trance Progressive",
        "Ambient Electronica",
        "Future Garage Dub",
        "Latin Tech Tribal",
        "Tech House Tools",
        "Tech House Expansion",
        "Minimal Plucks",
        "Minimal Tools",
        "Minimal Expansion",
        "Techno Stabs",
        "Techno Tools",
        "Techno Expansion",
        "UKG Expansion",
        "Dirty Tools",
        "Mono Safe",
        "Sequenced Grooves",
        "120-124 BPM",
        "125-128 BPM",
        "129-132 BPM",
        "133+ BPM"
    };
}

juce::StringArray presetSortChoices()
{
    return { "Name", "Rating", "Newest", "Category", "Pack", "BPM", "Key", "Author", "Source", "Macros" };
}

juce::StringArray presetPackChoices()
{
    return {
        "User Pack",
        "Project Pack",
        "UKG Essentials",
        "UKG Basslines",
        "Garage Chops",
        "House Chords",
        "House Tools",
        "House Expansion",
        "Afro House Rituals",
        "Amapiano Log Lab",
        "Dub Stabs",
        "Tech House Tools",
        "Tech House Expansion",
        "Minimal Plucks",
        "Minimal Tools",
        "Minimal Expansion",
        "Dub Techno Tools",
        "Techno Tools",
        "Techno Expansion",
        "Melodic Techno Motion",
        "Organic Progressive",
        "Drum & Bass Tools",
        "Bass House Weapons",
        "Hard Techno Rave",
        "Indie Dance Dark Disco",
        "Trance Progressive",
        "Ambient Electronica",
        "Future Garage Dub",
        "Latin Tech Tribal",
        "UKG Expansion",
        "Electro Breaks Tools",
        "Factory Pack"
    };
}

juce::StringArray presetKeyChoices()
{
    return {
        "Any Key",
        "C Min",
        "C# Min",
        "D Min",
        "D# Min",
        "E Min",
        "F Min",
        "F# Min",
        "G Min",
        "G# Min",
        "A Min",
        "A# Min",
        "B Min",
        "C Maj",
        "C# Maj",
        "D Maj",
        "D# Maj",
        "E Maj",
        "F Maj",
        "F# Maj",
        "G Maj",
        "G# Maj",
        "A Maj",
        "A# Maj",
        "B Maj"
    };
}

juce::StringArray presetBpmChoices()
{
    return { "Any Tempo", "120 BPM", "122 BPM", "124 BPM", "125 BPM", "126 BPM", "128 BPM", "130 BPM", "132 BPM", "134 BPM", "136 BPM", "138 BPM" };
}

int parsePresetBpm(const juce::String& text)
{
    const auto bpm = text.retainCharacters("0123456789").getIntValue();
    return bpm >= 20 && bpm <= 300 ? bpm : 0;
}

juce::String formatPresetBpm(int bpm)
{
    return bpm >= 20 && bpm <= 300 ? juce::String(bpm) + " BPM" : juce::String("Any Tempo");
}

juce::String presetSearchText(const NateVSTAudioProcessor::PresetInfo& preset)
{
    const auto sourceText = preset.isFactory ? juce::String("Factory") : juce::String("User");
    const auto ratingText = preset.rating > 0 ? juce::String(preset.rating) + " Star" : juce::String("Unrated");

    return preset.name + " " + preset.category + " " + preset.source + " " + preset.tags + " "
        + preset.folder + " " + sourceText + " " + ratingText + " " + preset.author + " "
        + preset.pack + " " + preset.key + " " + formatPresetBpm(preset.bpm) + " "
        + preset.macroSummary + " " + preset.notes
        + (preset.isFavorite ? " Favorite" : "");
}

bool textContainsAny(const juce::String& text, std::initializer_list<const char*> terms)
{
    for (const auto* term : terms)
        if (text.containsIgnoreCase(term))
            return true;

    return false;
}

bool presetMatchesSmartCrate(const NateVSTAudioProcessor::PresetInfo& preset, const juce::String& crate)
{
    const auto text = presetSearchText(preset);
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

juce::String presetAuditionRoleName(PresetAuditionRole role)
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

PresetAuditionRole inferPresetAuditionRole(const NateVSTAudioProcessor::PresetInfo* preset)
{
    if (preset == nullptr)
        return PresetAuditionRole::general;

    const auto text = presetSearchText(*preset);
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

juce::String presetInspectorRoleName(PresetAuditionRole role)
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

juce::String presetTempoBand(int bpm)
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

juce::String presetInspectorTraits(const NateVSTAudioProcessor::PresetInfo& preset)
{
    const auto text = presetSearchText(preset);
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

juce::String presetInspectorCue(const NateVSTAudioProcessor::PresetInfo& preset)
{
    const auto role = inferPresetAuditionRole(&preset);
    const auto tempo = presetTempoBand(preset.bpm);
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

std::array<float, 4> presetInspectorProfile(const NateVSTAudioProcessor::PresetInfo& preset)
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

int foldMidiNoteToRange(int note, int low, int high)
{
    while (note < low)
        note += 12;
    while (note > high)
        note -= 12;

    return juce::jlimit(0, 127, note);
}

juce::String outputSafetySummary(float peak)
{
    const auto safePeak = juce::jlimit(0.0f, 2.0f, peak);
    const auto peakDb = safePeak <= 0.000001f ? -60.0f
                                              : juce::Decibels::gainToDecibels(safePeak);

    if (safePeak >= 0.995f)
        return "clip risk " + juce::String(peakDb, 1) + " dB";

    if (peakDb >= -3.0f)
        return "hot peak " + juce::String(peakDb, 1) + " dB";

    if (peakDb <= -30.0f)
        return "low peak " + juce::String(peakDb, 1) + " dB";

    return "safe peak " + juce::String(peakDb, 1) + " dB";
}

juce::String guardSafetySummary(float peak, float guardReduction, bool guardActive)
{
    if (guardActive && guardReduction > 0.005f)
        return "guard -" + juce::String(juce::roundToInt(guardReduction * 100.0f)) + "% | " + outputSafetySummary(peak);

    return outputSafetySummary(peak);
}

juce::String presetMacroPreviewText(const NateVSTAudioProcessor::PresetInfo& preset)
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

juce::String presetPreviewSummaryText(const NateVSTAudioProcessor::PresetInfo& preset)
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

juce::String presetPreviewBadgeText(const NateVSTAudioProcessor::PresetInfo& preset)
{
    if (! preset.previewAvailable)
        return "RENDER";

    if (preset.previewStale)
        return "STALE";

    if (preset.previewPeak > 0.0f && std::isfinite(preset.previewPeak))
        return "PK " + juce::String(juce::Decibels::gainToDecibels(preset.previewPeak, -100.0f), 0) + " dB";

    return "READY";
}

float presetPreviewLevelNormalised(const NateVSTAudioProcessor::PresetInfo& preset)
{
    if (! preset.previewAvailable || preset.previewStale || preset.previewPeak <= 0.0f || ! std::isfinite(preset.previewPeak))
        return 0.0f;

    const auto peakDb = juce::Decibels::gainToDecibels(preset.previewPeak, -100.0f);
    return juce::jlimit(0.0f, 1.0f, juce::jmap(peakDb, -60.0f, -6.0f, 0.0f, 1.0f));
}

void drawPresetPreviewLevelBadge(juce::Graphics& g,
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

    const auto levelWidth = bounds.getWidth() * presetPreviewLevelNormalised(preset);
    if (levelWidth > 1.0f)
    {
        g.setColour(accent.withAlpha(0.28f));
        g.fillRoundedRectangle(bounds.withWidth(levelWidth), 4.0f);
    }

    g.setColour(accent.withAlpha(ready ? 0.78f : 0.52f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.setColour(ready ? juce::Colour(0xffdff8f0) : accent);
    g.drawFittedText(presetPreviewBadgeText(preset),
                     area.reduced(3, 0),
                     juce::Justification::centred,
                     1,
                     0.58f);
}

juce::StringArray presetTagChoices()
{
    return {
        "All Tags",
        "Bass",
        "Chord",
        "Pluck",
        "Stab",
        "Sequenced",
        "Mono Safe",
        "Pump",
        "Wide",
        "FX",
        "Generated",
        "Random Lab",
        "Vocal Chop",
        "Sample",
        "Lead",
        "Organ",
        "House",
        "Tech House",
        "Techno",
        "Minimal",
        "UKG",
        "Bass House",
        "Amapiano",
        "Hardgroove",
        "Future Garage",
        "Speed Garage",
        "Deep Tech",
        "Indie Dance",
        "Italo Disco",
        "Balearic House",
        "Acid House",
        "Nu Disco",
        "Afro Tech",
        "Afro Melodic",
        "Progressive House",
        "Hard House",
        "Peak Time Techno",
        "Detroit Techno",
        "Melodic Techno",
        "Minimal FM",
        "Deep Minimal",
        "Lo-Fi House",
        "French House",
        "Soulful House",
        "Garage House",
        "Microhouse",
        "Raw Techno",
        "Tribal Tech House",
        "Breaks House",
        "Chicago House",
        "Classic House",
        "Funky House",
        "Melodic House",
        "Romanian Minimal",
        "Dub Techno",
        "Electro Breaks"
    };
}

float smoothMeterValue(float current, float target)
{
    target = juce::jlimit(0.0f, 2.0f, target);
    const auto coefficient = target > current ? 0.65f : 0.18f;
    return current + ((target - current) * coefficient);
}

constexpr std::array<float, UI::OutputSpectrumDisplay::bandCount> spectrumBandFrequencies {
    45.0f,
    70.0f,
    105.0f,
    160.0f,
    250.0f,
    390.0f,
    620.0f,
    1000.0f,
    1700.0f,
    3100.0f,
    5800.0f,
    11000.0f
};

float goertzelBandLevel(const std::array<float, NateVSTAudioProcessor::outputSpectrumSnapshotSize>& samples,
                        double sampleRate,
                        float frequencyHz)
{
    if (sampleRate <= 0.0 || frequencyHz <= 0.0f)
        return 0.0f;

    const auto omega = juce::MathConstants<double>::twoPi * static_cast<double>(frequencyHz) / sampleRate;
    const auto coefficient = 2.0 * std::cos(omega);
    auto q1 = 0.0;
    auto q2 = 0.0;

    for (const auto sample : samples)
    {
        const auto q0 = (coefficient * q1) - q2 + static_cast<double>(sample);
        q2 = q1;
        q1 = q0;
    }

    const auto real = q1 - (q2 * std::cos(omega));
    const auto imaginary = q2 * std::sin(omega);
    const auto magnitude = std::sqrt((real * real) + (imaginary * imaginary))
        / static_cast<double>(samples.size());
    const auto decibels = juce::Decibels::gainToDecibels(static_cast<float>(magnitude) + 0.000001f);

    return juce::jlimit(0.0f, 1.0f, juce::jmap(decibels, -78.0f, -18.0f, 0.0f, 1.0f));
}

bool parameterIsOneOf(const juce::String& parameterID, std::initializer_list<const char*> ids)
{
    for (const auto* id : ids)
        if (parameterID == id)
            return true;

    return false;
}

bool destinationUsesGlobalModulationSources(int destinationIndex)
{
    return (destinationIndex >= 7 && destinationIndex <= 16)
        || destinationIndex == 20
        || destinationIndex == 21;
}

int rotaryDragSensitivityForParameter(const juce::String& parameterID)
{
    if (parameterIsOneOf(parameterID, {
            Parameters::ID::oscTune,
            Parameters::ID::osc2Tune,
            Parameters::ID::filterCutoff,
            Parameters::ID::fxRingFrequency,
            Parameters::ID::fxCombFrequency,
            Parameters::ID::fxDelayTime,
            Parameters::ID::fxWidthMonoCutoff,
            Parameters::ID::fxToneLowCut
        }))
        return 86;

    if (parameterIsOneOf(parameterID, {
            Parameters::ID::filterResonance,
            Parameters::ID::filterEnvAmount,
            Parameters::ID::driveAmount,
            Parameters::ID::outputGain,
            Parameters::ID::sampleTranspose,
            Parameters::ID::samplePitchRamp,
            Parameters::ID::sequencerRoot,
            Parameters::ID::sequencerSwing,
            Parameters::ID::fxEqLowGain,
            Parameters::ID::fxEqMidGain,
            Parameters::ID::fxEqHighGain,
            Parameters::ID::fxEqTrim,
            Parameters::ID::fxGuardCeiling
        }))
        return 64;

    if (parameterIsOneOf(parameterID, {
            Parameters::ID::macroTone,
            Parameters::ID::macroDirt,
            Parameters::ID::macroMotion,
            Parameters::ID::macroSpace,
            Parameters::ID::macroWeight,
            Parameters::ID::macroBounce,
            Parameters::ID::macroWarp,
            Parameters::ID::macroThrow,
            Parameters::ID::randomAmount,
            Parameters::ID::randomChaos,
            Parameters::ID::randomBrightnessBias,
            Parameters::ID::randomDriveBias,
            Parameters::ID::randomMotionBias
        }))
        return 58;

    return 52;
}

juce::ModifierKeys::Flags fineDragModifierFlags()
{
    return static_cast<juce::ModifierKeys::Flags>(juce::ModifierKeys::shiftModifier
                                                  | juce::ModifierKeys::commandModifier);
}

void applyFineDragMode(juce::Slider& slider, double sensitivity)
{
    slider.setVelocityBasedMode(false);
    slider.setVelocityModeParameters(sensitivity, 1, 0.0, true, fineDragModifierFlags());
}

juce::String controlFeelTooltip(const juce::String& labelText)
{
    return labelText + ": drag to adjust, hold Shift or Cmd for fine movement, double-click to reset, or type a value.";
}

juce::String modSourceSummaryText(size_t index)
{
    static const std::array<const char*, 20> sourceTexts {
        "LFO 1: synced shape source",
        "Mod Env: assignable ADSR",
        "Velocity: note force",
        "Tone: cutoff + resonance",
        "Dirt: drive + output trim",
        "Motion: filter env + osc2 tune",
        "Space: delay + reverb sends",
        "Weight: sub + low-end support",
        "Bounce: pump depth + groove",
        "Warp: osc bend + harmonic edge",
        "Throw: delay + reverb push",
        "S&H: stepped random movement",
        "Smooth: slewed random drift",
        "Chaos: wandering random walk",
        "LFO 2: secondary groove motion",
        "Mod Wheel: MIDI CC1 expression",
        "Aftertouch: pressure expression",
        "Pitch Bend: bipolar wheel source",
        "Note: key position source",
        "Step LFO: eight-step motion"
    };

    if (index < sourceTexts.size())
        return sourceTexts[index];

    return {};
}

struct RandomRecipeInfo
{
    const char* name = "";
    const char* genre = "";
    const char* tempo = "";
    const char* goodFor = "";
    const char* bias = "";
};

RandomRecipeInfo randomRecipeInfoForName(const juce::String& recipeName)
{
    static const std::array recipes {
        RandomRecipeInfo { "Deep House Bass", "Deep House", "120-126 BPM", "round basslines, warm chords, low drive", "sub weight, warm filter, light pump" },
        RandomRecipeInfo { "Rolling Tech Bass", "Tech House", "124-130 BPM", "rolling bass, tight groove, mid punch", "short envelope, drive, 24 dB filter motion" },
        RandomRecipeInfo { "Acid Line", "Acid / Techno", "128-134 BPM", "rubber riffs, resonant hooks, automation", "acid character, resonance, sync movement" },
        RandomRecipeInfo { "Minimal Blip", "Minimal", "124-130 BPM", "short plucks, sparse riffs, percussive tops", "fast envelope, step motion, restrained space" },
        RandomRecipeInfo { "Dark Stab", "Techno", "128-138 BPM", "warehouse hits, chord stabs, tense movement", "dirty filter, darker WT edge, Guard safety" },
        RandomRecipeInfo { "Noise FX", "Techno / FX", "Any Tempo", "risers, impacts, fills, noisy transitions", "noise source, wide FX, stepped motion" },
        RandomRecipeInfo { "UKG 2-Step Bass", "UKG", "130-134 BPM", "swung bass, skippy garage drops, mono subs", "centered sub, garage pump, safe width" },
        RandomRecipeInfo { "UKG Organ Stab", "UKG", "130-134 BPM", "short organ hits, speed-garage stabs", "warm filter, quick amp, delay/room space" },
        RandomRecipeInfo { "UKG Chord Stab", "UKG", "130-134 BPM", "minor stabs, late chords, shuffled hooks", "chord color, strum-friendly motion, pump" },
        RandomRecipeInfo { "UKG Bell Pluck", "UKG", "130-136 BPM", "bright plucks, metallic hooks, call-response", "clean transient, WT brightness, short tails" },
        RandomRecipeInfo { "UKG Dred Bass", "UKG / Speed Garage", "132-136 BPM", "Dred/Reese bass, darker 2-step pressure", "mono sub, detuned upper bass, slow filter pull" },
        RandomRecipeInfo { "Afro House Pluck", "Afro House", "120-124 BPM", "syncopated plucks, chant hooks, warm percussion", "bright pluck, moderate motion, clean low end" },
        RandomRecipeInfo { "Amapiano Log Bass", "Amapiano", "112-116 BPM", "round log bass, sparse groove, soft transient", "warm low tone, light drive, pitch-safe movement" },
        RandomRecipeInfo { "Bass House Reece", "Bass House", "124-128 BPM", "wide upper bass, mono sub support, dirty hooks", "Reese detune, drive, clipped club safety" },
        RandomRecipeInfo { "Dub Techno Chord", "Dub Techno", "122-128 BPM", "dark chords, long tails, smoky stabs", "darker filter, softer chaos, spacious motion" },
        RandomRecipeInfo { "Detroit Tech Stab", "Detroit Techno", "126-132 BPM", "classic stab riffs, chord memory, machine soul", "mid punch, guarded drive, steady LFO" },
        RandomRecipeInfo { "Hardgroove Rumble", "Hardgroove", "132-138 BPM", "rolling rumble, percussion pressure, hypnotic bass", "low tilt, drive, sequencer and pump motion" },
        RandomRecipeInfo { "Minimal House Pluck", "Minimal House", "122-126 BPM", "small plucks, restrained hooks, dry groove", "low chaos, bright transient, tight envelope" },
        RandomRecipeInfo { "Microhouse Perc Blip", "Microhouse", "122-128 BPM", "clicky blips, micro percussion, short motifs", "bright tone, high motion, small FX throws" },
        RandomRecipeInfo { "Garage Vocal Chop", "Garage", "130-134 BPM", "vocal-like chops, shuffled call-response, hook fills", "bright bell engine, more motion, short tails" },
        RandomRecipeInfo { "Future Garage Pad", "Future Garage", "130-136 BPM", "washed chords, moody beds, atmospheric hooks", "soft drive, darker tone, wider motion" },
        RandomRecipeInfo { "Tech House Chord Tool", "Tech House", "124-128 BPM", "stab chords, groove tools, DJ-friendly hooks", "chord stab engine, mild drive, pump-ready" },
        RandomRecipeInfo { "Peak Techno Acid", "Peak Techno", "132-140 BPM", "acid lead pressure, rave hooks, automation", "bright resonance, more drive, high chaos" },
        RandomRecipeInfo { "Lo-Fi House Keys", "Lo-Fi House", "118-124 BPM", "dusty key stabs, soft chords, mellow riffs", "darker keys, low chaos, gentle motion" },
        RandomRecipeInfo { "Tribal Tech Perc", "Tribal Tech", "124-130 BPM", "percussive blips, tribal loops, call hits", "short envelope, motion bias, drive-safe FX" }
    };

    for (const auto& recipe : recipes)
        if (recipeName.equalsIgnoreCase(recipe.name))
            return recipe;

    return recipes.front();
}

juce::String randomRecipeInfoText(const juce::String& recipeName)
{
    const auto info = randomRecipeInfoForName(recipeName.trim().isNotEmpty() ? recipeName.trim()
                                                                             : juce::String("Deep House Bass"));
    return juce::String(info.genre) + " | " + info.tempo
        + " | Good for: " + info.goodFor
        + " | Bias: " + info.bias;
}

juce::StringArray lfoCurvePresetChoices()
{
    return {
        "Manual",
        "Garage Push",
        "Tight Duck",
        "Offbeat Skank",
        "Riser",
        "Fall",
        "Gate Steps",
        "Wobble",
        "UKG Swing",
        "Minimal Pulse",
        "Techno Ramp",
        "House Chug",
        "Flat"
    };
}

std::array<float, 8> lfoCurvePresetValues(int presetId)
{
    switch (presetId)
    {
        case 2: return { -0.10f, 0.38f, 0.98f, 0.54f, 0.08f, -0.30f, -0.78f, -0.22f };
        case 3: return { -1.00f, -0.65f, -0.22f, 0.34f, 0.76f, 1.00f, 0.42f, -0.12f };
        case 4: return { -0.36f, 0.14f, 0.82f, 0.26f, -0.22f, 0.58f, 1.00f, -0.48f };
        case 5: return { -1.00f, -0.72f, -0.45f, -0.12f, 0.18f, 0.48f, 0.76f, 1.00f };
        case 6: return { 1.00f, 0.72f, 0.45f, 0.12f, -0.18f, -0.48f, -0.76f, -1.00f };
        case 7: return { 1.00f, 1.00f, -0.65f, -0.65f, 0.80f, 0.80f, -1.00f, -1.00f };
        case 8: return { 0.00f, 1.00f, 0.00f, -1.00f, 0.00f, 0.72f, 0.00f, -0.72f };
        case 9: return { -0.54f, 0.82f, -0.18f, 0.58f, -0.70f, 1.00f, -0.30f, 0.34f };
        case 10: return { -1.00f, -1.00f, 0.85f, -1.00f, -1.00f, -1.00f, 0.42f, -1.00f };
        case 11: return { -0.80f, -0.58f, -0.24f, 0.18f, 0.56f, 0.88f, 1.00f, -0.20f };
        case 12: return { -0.25f, 0.48f, 0.28f, -0.18f, 0.42f, 0.10f, -0.36f, 0.18f };
        case 13: return { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f };
        default: return { 0.00f, 0.58f, 1.00f, 0.42f, -0.18f, -0.72f, -1.00f, -0.36f };
    }
}

double lfoCyclesPerBeatForUi(int rateIndex)
{
    switch (rateIndex)
    {
        case 1: return 2.0;
        case 2: return 3.0;
        case 3: return 4.0;
        default: return 1.0;
    }
}

float evaluateLfoCurveForUi(const std::array<float, 8>& values, float phase)
{
    constexpr auto pointCount = 8;
    const auto scaledPhase = juce::jlimit(0.0f, 0.999999f, phase) * static_cast<float>(pointCount);
    const auto leftIndex = static_cast<int>(std::floor(scaledPhase)) % pointCount;
    const auto rightIndex = (leftIndex + 1) % pointCount;
    const auto fraction = scaledPhase - std::floor(scaledPhase);

    return juce::jlimit(-1.0f,
                        1.0f,
                        values[static_cast<size_t>(leftIndex)]
                            + ((values[static_cast<size_t>(rightIndex)] - values[static_cast<size_t>(leftIndex)]) * fraction));
}

float lfoShapeValueForUi(int shapeIndex, float phase, const std::array<float, 8>& curveValues)
{
    phase = std::fmod(phase + 1.0f, 1.0f);

    switch (shapeIndex)
    {
        case 1:
            return phase < 0.25f ? phase * 4.0f
                : phase < 0.75f ? 2.0f - (phase * 4.0f)
                : (phase * 4.0f) - 4.0f;

        case 2:
            return (phase * 2.0f) - 1.0f;

        case 3:
            return phase < 0.5f ? 1.0f : -1.0f;

        case 4:
            return phase < 0.25f ? 1.0f
                : phase < 0.5f ? -0.35f
                : phase < 0.75f ? 0.55f
                : -1.0f;

        case 5:
            return evaluateLfoCurveForUi(curveValues, phase);

        default:
            return std::sin(juce::MathConstants<float>::twoPi * phase);
    }
}
}

NateVSTAudioProcessorEditor::NateVSTAudioProcessorEditor(NateVSTAudioProcessor& processorToUse)
    : AudioProcessorEditor(&processorToUse),
      audioProcessor(processorToUse),
      sampleRecorderPanel(processorToUse.getValueTreeState()),
      pianoKeyboard(processorToUse.getMidiKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&lookAndFeel);
    setWantsKeyboardFocus(true);
    setSize(editorDefaultWidth, editorDefaultHeight);
    setResizeLimits(editorMinWidth, editorMinHeight, editorMaxWidth, editorMaxHeight);
    setResizable(true, true);
    addMouseListener(this, true);

    titleLabel.setText("Nate VST", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffedf7f4));
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(outputMeter);
    addAndMakeVisible(outputOscilloscopeDisplay);
    addAndMakeVisible(outputSpectrumDisplay);
    addAndMakeVisible(stereoFieldDisplay);
    addAndMakeVisible(clubMonitorDisplay);
    addAndMakeVisible(homeOverviewDisplay);
    addAndMakeVisible(homeSignalFlowDisplay);
    addAndMakeVisible(homeSessionDisplay);
    addAndMakeVisible(lowEndAssistant);

    pianoKeyboard.setAvailableRange(keyboardVisualLowestNote, keyboardVisualHighestNote);
    pianoKeyboard.setLowestVisibleKey(keyboardVisualLowestNote);
    pianoKeyboard.setOctaveForMiddleC(abletonMiddleCOctave);
    keyboardTypingBaseNote = keyboardTypingBaseNoteForLowestNote(keyboardInitialLowestNote);
    syncPianoKeyboardComputerMapping();
    pianoKeyboard.setKeyWidth(keyboardMinimumWhiteKeyWidth);
    pinPianoKeyboardVisualRange();
    pianoKeyboard.setWantsKeyboardFocus(true);
    pianoKeyboard.setScrollButtonsVisible(false);
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffd9e3df));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff151b1f));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xff2a363c));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour(0x338ee6c9));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0xaa8ee6c9));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, juce::Colour(0xff253037));
    addAndMakeVisible(pianoKeyboard);

    keyboardOctaveDownButton.setTooltip("Shift laptop keys down one octave; the piano strip stays fixed from C1 to C5");
    keyboardOctaveDownButton.setWantsKeyboardFocus(false);
    keyboardOctaveDownButton.setMouseClickGrabsKeyboardFocus(false);
    keyboardOctaveDownButton.onClick = [this] { shiftKeyboardOctave(-12); };
    addAndMakeVisible(keyboardOctaveDownButton);

    keyboardOctaveUpButton.setTooltip("Shift laptop keys up one octave; the piano strip stays fixed from C1 to C5");
    keyboardOctaveUpButton.setWantsKeyboardFocus(false);
    keyboardOctaveUpButton.setMouseClickGrabsKeyboardFocus(false);
    keyboardOctaveUpButton.onClick = [this] { shiftKeyboardOctave(12); };
    addAndMakeVisible(keyboardOctaveUpButton);

    keyboardHomeButton.setTooltip("Reset laptop keys so A plays C4; the visible piano strip stays fixed from C1 to C5");
    keyboardHomeButton.setWantsKeyboardFocus(false);
    keyboardHomeButton.setMouseClickGrabsKeyboardFocus(false);
    keyboardHomeButton.onClick = [this]
    {
        releaseComputerKeyboardNotes();
        audioProcessor.getMidiKeyboardState().allNotesOff(1);
        keyboardTypingBaseNote = keyboardTypingBaseNoteForLowestNote(keyboardInitialLowestNote);
        syncPianoKeyboardComputerMapping();
        updateKeyboardRangeLabel();
        returnKeyboardFocusToPiano();
    };
    addAndMakeVisible(keyboardHomeButton);

    keyboardPanicButton.setTooltip("Stop stuck or held notes from the keyboard, chord memory, synth, and sampler");
    keyboardPanicButton.setWantsKeyboardFocus(false);
    keyboardPanicButton.setMouseClickGrabsKeyboardFocus(false);
    keyboardPanicButton.onClick = [this]
    {
        releaseComputerKeyboardNotes();
        releasePresetAuditionNote();
        audioProcessor.panicAllNotesOff();
        setRandomStatus("All Off: stopped held notes");
    };
    addAndMakeVisible(keyboardPanicButton);

    keyboardRangeLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    keyboardRangeLabel.setJustificationType(juce::Justification::centred);
    keyboardRangeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdce7e4));
    addAndMakeVisible(keyboardRangeLabel);
    updateKeyboardRangeLabel();

    sequencerRootDownButton.setTooltip("Move sequencer root note down one semitone");
    sequencerRootDownButton.setWantsKeyboardFocus(false);
    sequencerRootDownButton.setMouseClickGrabsKeyboardFocus(false);
    sequencerRootDownButton.onClick = [this] { stepSequencerRoot(-1); };
    addAndMakeVisible(sequencerRootDownButton);

    sequencerRootUpButton.setTooltip("Move sequencer root note up one semitone");
    sequencerRootUpButton.setWantsKeyboardFocus(false);
    sequencerRootUpButton.setMouseClickGrabsKeyboardFocus(false);
    sequencerRootUpButton.onClick = [this] { stepSequencerRoot(1); };
    addAndMakeVisible(sequencerRootUpButton);

    sequencerRootValueLabel.setText("C1", juce::dontSendNotification);
    sequencerRootValueLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    sequencerRootValueLabel.setJustificationType(juce::Justification::centred);
    sequencerRootValueLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdce7e4));
    sequencerRootValueLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff101619));
    sequencerRootValueLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff31434a));
    sequencerRootValueLabel.setTooltip("Sequencer root note");
    addAndMakeVisible(sequencerRootValueLabel);

    sequencerStepEditorLabel.setText("Select a step", juce::dontSendNotification);
    sequencerStepEditorLabel.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    sequencerStepEditorLabel.setJustificationType(juce::Justification::centred);
    sequencerStepEditorLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdce7e4));
    sequencerStepEditorLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff101619));
    sequencerStepEditorLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff31434a));
    sequencerStepEditorLabel.setTooltip("Focused editor for the selected piano-roll step");
    addAndMakeVisible(sequencerStepEditorLabel);

    const std::array<juce::String, 13> stepButtonLabels {
        "Note -", "Note +", "Oct -", "Oct +", "Vel -", "Vel +", "Prob -", "Prob +", "Len -", "Len +", "Rat", "Slide", "Lock"
    };
    for (size_t index = 0; index < sequencerStepEditorButtons.size(); ++index)
    {
        auto& button = sequencerStepEditorButtons[index];
        button.setButtonText(stepButtonLabels[index]);
        button.setWantsKeyboardFocus(false);
        button.setMouseClickGrabsKeyboardFocus(false);
        button.setTooltip("Edit the selected sequencer step: " + stepButtonLabels[index]);
        button.onClick = [this, index]
        {
            switch (index)
            {
                case 0: adjustSelectedSequencerStepNote(-1); break;
                case 1: adjustSelectedSequencerStepNote(1); break;
                case 2: adjustSelectedSequencerStepNote(-12); break;
                case 3: adjustSelectedSequencerStepNote(12); break;
                case 4: adjustSelectedSequencerStepValue(0, -0.05f); break;
                case 5: adjustSelectedSequencerStepValue(0, 0.05f); break;
                case 6: adjustSelectedSequencerStepValue(1, -0.05f); break;
                case 7: adjustSelectedSequencerStepValue(1, 0.05f); break;
                case 8: adjustSelectedSequencerStepValue(2, -0.05f); break;
                case 9: adjustSelectedSequencerStepValue(2, 0.05f); break;
                case 10: toggleSelectedSequencerStepFlag(0); break;
                case 11: toggleSelectedSequencerStepFlag(1); break;
                case 12: toggleSelectedSequencerStepFlag(2); break;
                default: break;
            }
        };
        addAndMakeVisible(button);
    }

    configureSectionLabel(homeSectionLabel, "HOME");
    configureSectionLabel(homeEngineLabel, "PERFORM");
    configureSectionLabel(homeShapeLabel, "MACROS");
    configureSectionLabel(homeLabLabel, "RANDOM LAB");
    configureSectionLabel(homeLibraryLabel, "PATCH SNAPSHOT");
    configureSectionLabel(synthSectionLabel, "SYNTH");
    configureSectionLabel(synthSourceLabel, "SOURCE MIX");
    configureSectionLabel(synthVoiceLabel, "PITCH + VOICE");
    configureSectionLabel(synthFilterLabel, "WAVE + FILTER");
    configureSectionLabel(synthAmpLabel, "AMP + OUTPUT");
    configureSectionLabel(randomSectionLabel, "LAB");
    configureSectionLabel(modSectionLabel, "MOD");
    configureSectionLabel(modSourceLabel, "SOURCES");
    configureSectionLabel(modMacroLabel, "MACROS");
    configureSectionLabel(modLfoLabel, "LFO 1 MSEG");
    configureSectionLabel(modLfo2Label, "LFO 2");
    configureSectionLabel(modEnvelopeLabel, "MOD ENV 1");
    configureSectionLabel(modMatrixLabel, "ROUTING");
    configureSectionLabel(sampleSectionLabel, "SAMPLE");
    configureSectionLabel(sampleSourceLabel, "SOURCE");
    configureSectionLabel(sampleChopLabel, "CHOP");
    configureSectionLabel(sampleShapeLabel, "SHAPE");
    configureSectionLabel(sequencerSectionLabel, "SEQ");
    configureSectionLabel(futureSectionLabel, "FX");
    configureSectionLabel(librarySectionLabel, "LIBRARY");
    configureSectionLabel(libraryFindLabel, "CRATES");
    configureSectionLabel(libraryBrowserLabel, "BROWSER");
    configureSectionLabel(librarySaveLabel, "SAVE TARGET");
    configureSectionLabel(libraryInspectorLabel, "PRESET PROFILE");
    configureSectionLabel(infoSectionLabel, "INFO");
    configureSectionLabel(infoAboutLabel, "ABOUT");
    configureSectionLabel(infoWorkflowLabel, "HOUSE WORKFLOW");
    configureSectionLabel(infoDetailsLabel, "DETAILS");
    configureSectionLabel(infoFocusLabel, "OPEN A WORK AREA");

    hostSyncStatusLabel.setText("INT 124 | FREE", juce::dontSendNotification);
    hostSyncStatusLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    hostSyncStatusLabel.setJustificationType(juce::Justification::centred);
    hostSyncStatusLabel.setMinimumHorizontalScale(0.72f);
    hostSyncStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7d8b90));
    hostSyncStatusLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0x22141a1d));
    hostSyncStatusLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff263035));
    hostSyncStatusLabel.setTooltip("Host tempo and transport phase status for sequencer and tempo-synced FX");
    addAndMakeVisible(hostSyncStatusLabel);

    selectedControlHeaderLabel.setText("CONTROL", juce::dontSendNotification);
    selectedControlHeaderLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    selectedControlHeaderLabel.setJustificationType(juce::Justification::centred);
    selectedControlHeaderLabel.setMinimumHorizontalScale(0.72f);
    selectedControlHeaderLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8ee6c9));
    selectedControlHeaderLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0x33141a1d));
    selectedControlHeaderLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff263035));
    selectedControlHeaderLabel.setTooltip("Last changed control, automation ID, and modulation route status");
    addAndMakeVisible(selectedControlHeaderLabel);

    selectedControlStatusLabel.setText("Touch a control for value, automation ID, and modulation routes", juce::dontSendNotification);
    selectedControlStatusLabel.setFont(juce::FontOptions(11.0f));
    selectedControlStatusLabel.setJustificationType(juce::Justification::centredLeft);
    selectedControlStatusLabel.setMinimumHorizontalScale(0.62f);
    selectedControlStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    selectedControlStatusLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xee101619));
    selectedControlStatusLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff263035));
    selectedControlStatusLabel.setTooltip("Last changed control, automation ID, value, and active modulation routes");
    addAndMakeVisible(selectedControlStatusLabel);

    sampleNameLabel.setText("No sample", juce::dontSendNotification);
    sampleNameLabel.setJustificationType(juce::Justification::centredLeft);
    sampleNameLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(sampleNameLabel);

    presetStatusLabel.setJustificationType(juce::Justification::centredLeft);
    presetStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(presetStatusLabel);

    presetBrowserHeaderLabel.setText("SOUND / PACK        PREVIEW        INFO", juce::dontSendNotification);
    presetBrowserHeaderLabel.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    presetBrowserHeaderLabel.setJustificationType(juce::Justification::centredLeft);
    presetBrowserHeaderLabel.setMinimumHorizontalScale(0.58f);
    presetBrowserHeaderLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8ee6c9));
    presetBrowserHeaderLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff101619));
    presetBrowserHeaderLabel.setTooltip("Visible preset browser columns");
    addAndMakeVisible(presetBrowserHeaderLabel);

    presetBrowserList.setModel(this);
    presetBrowserList.setRowHeight(30);
    presetBrowserList.setMultipleSelectionEnabled(false);
    presetBrowserList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff101619));
    presetBrowserList.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff344047));
    presetBrowserList.setColour(juce::ListBox::textColourId, juce::Colour(0xffdce7e4));
    presetBrowserList.setOutlineThickness(1);
    presetBrowserList.setTooltip("Click a preset row to select it. Double-click a row to load it.");
    addAndMakeVisible(presetBrowserList);
    addAndMakeVisible(presetCrateMapDisplay);
    addAndMakeVisible(presetLibrarySummary);
    addAndMakeVisible(presetSaveSummary);

    randomStatusLabel.setText("Ready", juce::dontSendNotification);
    randomStatusLabel.setJustificationType(juce::Justification::centredLeft);
    randomStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(randomStatusLabel);

    randomRecipeInfoLabel.setText(randomRecipeInfoText("Deep House Bass"), juce::dontSendNotification);
    randomRecipeInfoLabel.setJustificationType(juce::Justification::centredLeft);
    randomRecipeInfoLabel.setFont(juce::FontOptions(11.0f));
    randomRecipeInfoLabel.setMinimumHorizontalScale(0.58f);
    randomRecipeInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xffd9fff1));
    randomRecipeInfoLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xee101619));
    randomRecipeInfoLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff263035));
    randomRecipeInfoLabel.setTooltip("Selected Random Lab recipe intent, tempo range, use case, and generator bias");
    addAndMakeVisible(randomRecipeInfoLabel);

    randomMorphPad.onChange = [this] (float x, float y)
    {
        applyRandomMorphPad(x, y, false);
    };
    randomMorphPad.onCommit = [this] (float x, float y)
    {
        applyRandomMorphPad(x, y, true);
    };
    addAndMakeVisible(randomMorphPad);

    modMatrixStatusLabel.setJustificationType(juce::Justification::centredRight);
    modMatrixStatusLabel.setFont(juce::FontOptions(11.0f));
    modMatrixStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(modMatrixStatusLabel);

    auto configureMatrixHeader = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        label.setJustificationType(juce::Justification::centredLeft);
        label.setColour(juce::Label::textColourId, juce::Colour(0xff617078));
        addAndMakeVisible(label);
    };
    configureMatrixHeader(modMatrixSourceHeader, "SOURCE");
    configureMatrixHeader(modMatrixDestinationHeader, "DESTINATION");
    configureMatrixHeader(modMatrixAmountHeader, "AMOUNT");
    configureMatrixHeader(modMatrixSourceHeaderB, "SOURCE");
    configureMatrixHeader(modMatrixDestinationHeaderB, "DESTINATION");
    configureMatrixHeader(modMatrixAmountHeaderB, "AMOUNT");
    configureMatrixHeader(modInspectorLabel, "INSPECT");
    configureMatrixHeader(modMacroAssignLabel, "ASSIGN");

    modInspectorStatusLabel.setJustificationType(juce::Justification::centredLeft);
    modInspectorStatusLabel.setFont(juce::FontOptions(11.0f));
    modInspectorStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(modInspectorStatusLabel);

    modMacroAssignStatusLabel.setJustificationType(juce::Justification::centredLeft);
    modMacroAssignStatusLabel.setFont(juce::FontOptions(10.5f));
    modMacroAssignStatusLabel.setMinimumHorizontalScale(0.64f);
    modMacroAssignStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(modMacroAssignStatusLabel);

    performanceStatusLabel.setJustificationType(juce::Justification::centredLeft);
    performanceStatusLabel.setFont(juce::FontOptions(11.0f));
    performanceStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(performanceStatusLabel);

    performanceXYPad.onChange = [this] (float motion, float space)
    {
        setPlainParameterValue(Parameters::ID::macroMotion, motion);
        setPlainParameterValue(Parameters::ID::macroSpace, space);
        updateSelectedControlInspector("Motion / Space", Parameters::ID::macroMotion, motion);
        updatePerformanceXYPad();
        updateHomeOverviewDisplay();
    };
    addAndMakeVisible(performanceXYPad);

    macroPerformanceMap.onChange = [this] (size_t index, float value)
    {
        if (index >= macroPerformanceParameterIDs.size())
            return;

        const auto parameterID = juce::String(macroPerformanceParameterIDs[index]);
        setPlainParameterValue(parameterID, value);
        updateSelectedControlInspector(macroPerformanceNames[index], parameterID, value);
        updatePerformanceXYPad();
        updateHomeOverviewDisplay();
    };
    addAndMakeVisible(macroPerformanceMap);
    expandedMacroPerformanceMap.onChange = macroPerformanceMap.onChange;
    addAndMakeVisible(expandedMacroPerformanceMap);

    macroAssignmentPad.onSourceSelected = [this] (int sourceIndex)
    {
        const auto destinationIndex = selectedMacroAssignmentDestinationIndex();
        const auto amount = macroAssignmentAmountForRoute(sourceIndex,
                                                          destinationIndex,
                                                          static_cast<float>(modMacroAssignAmountSlider.getValue() / 100.0));
        modMacroAssignSourceBox.setSelectedId(sourceIndex + 1, juce::dontSendNotification);
        modMacroAssignAmountSlider.setValue(amount * 100.0f, juce::dontSendNotification);
        updateMacroAssignmentEditorStatus();
    };
    macroAssignmentPad.onDestinationSelected = [this] (int destinationIndex)
    {
        const auto sourceIndex = selectedMacroAssignmentSourceIndex();
        const auto amount = macroAssignmentAmountForRoute(sourceIndex,
                                                          destinationIndex,
                                                          static_cast<float>(modMacroAssignAmountSlider.getValue() / 100.0));
        modMacroAssignDestinationBox.setSelectedId(destinationIndex + 1, juce::dontSendNotification);
        modMacroAssignAmountSlider.setValue(amount * 100.0f, juce::dontSendNotification);
        updateMacroAssignmentEditorStatus();
    };
    macroAssignmentPad.onTargetAmountPreview = [this] (int sourceIndex, int destinationIndex, float amount)
    {
        modMacroAssignSourceBox.setSelectedId(sourceIndex + 1, juce::dontSendNotification);
        modMacroAssignDestinationBox.setSelectedId(destinationIndex + 1, juce::dontSendNotification);
        modMacroAssignAmountSlider.setValue(amount * 100.0f, juce::dontSendNotification);
        updateMacroAssignmentEditorStatus();
    };
    macroAssignmentPad.onTargetAmountCommit = [this] (int sourceIndex, int destinationIndex, float amount)
    {
        modMacroAssignSourceBox.setSelectedId(sourceIndex + 1, juce::dontSendNotification);
        modMacroAssignDestinationBox.setSelectedId(destinationIndex + 1, juce::dontSendNotification);
        modMacroAssignAmountSlider.setValue(amount * 100.0f, juce::dontSendNotification);
        addMacroAssignment(false);
    };
    macroAssignmentPad.onClearSource = [this] (int sourceIndex)
    {
        modMacroAssignSourceBox.setSelectedId(sourceIndex + 1, juce::dontSendNotification);
        clearSelectedMacroAssignments();
    };
    addAndMakeVisible(macroAssignmentPad);
    expandedMacroAssignmentPad.onSourceSelected = macroAssignmentPad.onSourceSelected;
    expandedMacroAssignmentPad.onDestinationSelected = macroAssignmentPad.onDestinationSelected;
    expandedMacroAssignmentPad.onTargetAmountPreview = macroAssignmentPad.onTargetAmountPreview;
    expandedMacroAssignmentPad.onTargetAmountCommit = macroAssignmentPad.onTargetAmountCommit;
    expandedMacroAssignmentPad.onClearSource = macroAssignmentPad.onClearSource;
    addAndMakeVisible(expandedMacroAssignmentPad);

    homeMacroExpandButton.setTooltip("Open a larger macro editor");
    homeMacroExpandButton.onClick = [this] { openMacroFocusOverlay(); };
    addAndMakeVisible(homeMacroExpandButton);

    modMacroExpandButton.setTooltip("Open a larger macro editor");
    modMacroExpandButton.onClick = [this] { openMacroFocusOverlay(); };
    addAndMakeVisible(modMacroExpandButton);

    sampleChopExpandButton.setTooltip("Open a larger sample chop editor");
    sampleChopExpandButton.onClick = [this] { openSampleChopFocusOverlay(); };
    addAndMakeVisible(sampleChopExpandButton);

    sourceLayerExpandButton.setTooltip("Open a larger house source layer rack");
    sourceLayerExpandButton.onClick = [this] { openSourceLayerFocusOverlay(); };
    addAndMakeVisible(sourceLayerExpandButton);

    sequencerExpandButton.setTooltip("Open a larger sequencer editor");
    sequencerExpandButton.onClick = [this] { openSequencerFocusOverlay(); };
    addAndMakeVisible(sequencerExpandButton);

    focusOverlayCloseButton.setTooltip("Close the expanded editor");
    focusOverlayCloseButton.onClick = [this] { closeFocusOverlay(); };
    addAndMakeVisible(focusOverlayPanel);
    focusOverlayTitleLabel.setText("EXPANDED MACRO EDITOR", juce::dontSendNotification);
    focusOverlayTitleLabel.setJustificationType(juce::Justification::centredLeft);
    focusOverlayTitleLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    focusOverlayTitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdce7e4));
    addAndMakeVisible(focusOverlayTitleLabel);
    addAndMakeVisible(focusOverlayCloseButton);

    sampleWaveformDisplay.onRangeChange = [this] (float start, float end)
    {
        setPlainParameterValue(Parameters::ID::sampleStart, start);
        setPlainParameterValue(Parameters::ID::sampleEnd, end);
        setPlainParameterValue(Parameters::ID::sampleEnabled, 1.0f);
        updateSampleSliceButtons();
    };
    sampleWaveformDisplay.onSliceSelected = [this] (size_t sliceIndex) { selectSampleSlice(sliceIndex); };
    sampleWaveformDisplay.onSliceBoundaryChange = [this] (size_t boundaryIndex, float position) { editSampleSliceBoundary(boundaryIndex, position); };
    addAndMakeVisible(sampleWaveformDisplay);
    expandedSampleWaveformDisplay.onRangeChange = sampleWaveformDisplay.onRangeChange;
    expandedSampleWaveformDisplay.onSliceSelected = sampleWaveformDisplay.onSliceSelected;
    expandedSampleWaveformDisplay.onSliceBoundaryChange = sampleWaveformDisplay.onSliceBoundaryChange;
    addAndMakeVisible(expandedSampleWaveformDisplay);
    wavetableDisplay.onEditStart = [this] { captureGlobalEdit("Edit wavetable"); };
    wavetableDisplay.onOsc1PositionChange = [this] (float position)
    {
        setPlainParameterValue(Parameters::ID::oscWave, 4.0f);
        waveformBox.setSelectedItemIndex(4, juce::dontSendNotification);
        setPlainParameterValue(Parameters::ID::oscWavetablePosition, position);
        updateSelectedControlInspector("WT 1", Parameters::ID::oscWavetablePosition, position);
        updateSegmentedSelectors();
        updateWavetableDisplay();
    };
    wavetableDisplay.onOsc2PositionChange = [this] (float position)
    {
        setPlainParameterValue(Parameters::ID::osc2Wave, 4.0f);
        osc2WaveBox.setSelectedItemIndex(4, juce::dontSendNotification);
        setPlainParameterValue(Parameters::ID::osc2WavetablePosition, position);
        updateSelectedControlInspector("WT 2", Parameters::ID::osc2WavetablePosition, position);
        updateSegmentedSelectors();
        updateWavetableDisplay();
    };
    wavetableDisplay.onWarpChange = [this] (float warp)
    {
        setPlainParameterValue(Parameters::ID::oscWarp, warp);
        updateSelectedControlInspector("Osc Warp", Parameters::ID::oscWarp, warp);
        updateWavetableDisplay();
    };
    wavetableDisplay.onCustomPointChange = [this] (int oscillator, size_t pointIndex, float value)
    {
        if (pointIndex >= Parameters::ID::oscCustomWave.size())
            return;

        const auto targetOsc2 = oscillator == 2;
        const auto& pointIDs = targetOsc2 ? Parameters::ID::osc2CustomWave : Parameters::ID::oscCustomWave;
        const auto waveID = targetOsc2 ? juce::String(Parameters::ID::osc2Wave)
                                       : juce::String(Parameters::ID::oscWave);
        auto& waveBox = targetOsc2 ? osc2WaveBox : waveformBox;

        setPlainParameterValue(waveID, 7.0f);
        waveBox.setSelectedItemIndex(7, juce::dontSendNotification);
        setPlainParameterValue(pointIDs[pointIndex], juce::jlimit(0.0f, 1.0f, value));
        updateSelectedControlInspector(targetOsc2 ? "O2 Custom" : "O1 Custom",
                                       pointIDs[pointIndex],
                                       value);
        updateSegmentedSelectors();
        updateWavetableDisplay();
    };
    addAndMakeVisible(wavetableDisplay);
    houseLayerRackDisplay.onLayerSelected = [this] (size_t layerIndex) { focusHouseLayer(layerIndex); };
    houseLayerRackDisplay.onLayerEditStarted = [this] (size_t layerIndex) { beginHouseLayerLevelEdit(layerIndex); };
    houseLayerRackDisplay.onLayerLevelChanged = [this] (size_t layerIndex, float level) { setHouseLayerLevel(layerIndex, level); };
    addAndMakeVisible(houseLayerRackDisplay);
    expandedHouseLayerRackDisplay.setComponentID("ExpandedHouseLayerRackDisplay");
    expandedHouseLayerRackDisplay.onLayerSelected = houseLayerRackDisplay.onLayerSelected;
    expandedHouseLayerRackDisplay.onLayerEditStarted = houseLayerRackDisplay.onLayerEditStarted;
    expandedHouseLayerRackDisplay.onLayerLevelChanged = houseLayerRackDisplay.onLayerLevelChanged;
    addAndMakeVisible(expandedHouseLayerRackDisplay);
    addAndMakeVisible(filterResponseDisplay);

    fxRackStatusLabel.setJustificationType(juce::Justification::centredLeft);
    fxRackStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(fxRackStatusLabel);

    for (size_t index = 0; index < modSourceRows.size(); ++index)
    {
        auto& meter = modSourceRows[index];
        meter.setComponentID("ModSource" + juce::String(static_cast<int>(index + 1)));
        meter.setSourceIndex(static_cast<int>(index + 1));
        meter.onDragStart = [this] (int sourceIndex, juce::Component& sourceComponent)
        {
            beginModSourceDrag(sourceIndex, sourceComponent);
        };
        meter.setState(modSourceSummaryText(index), 0, 0.0f, 0.0f, modSourceSummaryText(index));
        addAndMakeVisible(meter);
    }

    for (size_t index = 0; index < modSlotRows.size(); ++index)
    {
        modSlotRows[index].setText(juce::String(static_cast<int>(index + 1)), juce::dontSendNotification);
        modSlotRows[index].setFont(juce::FontOptions(12.0f, juce::Font::bold));
        modSlotRows[index].setJustificationType(juce::Justification::centred);
        modSlotRows[index].setColour(juce::Label::textColourId, juce::Colour(0xff8ee6c9));
        addAndMakeVisible(modSlotRows[index]);
    }

    for (auto& row : modMatrixRows)
    {
        addAndMakeVisible(row);
        row.toBack();
    }
    addAndMakeVisible(modRouteMapDisplay);

    presetNameEditor.setTextToShowWhenEmpty("Preset name", juce::Colour(0xff617078));
    presetNameEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff101619));
    presetNameEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff344047));
    presetNameEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff8ee6c9));
    presetNameEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xffdce7e4));
    presetNameEditor.onTextChange = [this]
    {
        presetNameIsRandomDraft = false;
        clearPresetOverwriteWarning();
        updatePresetSaveSummary();
    };
    addAndMakeVisible(presetNameEditor);

    presetSearchEditor.setTextToShowWhenEmpty("Search presets", juce::Colour(0xff617078));
    presetSearchEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff101619));
    presetSearchEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff344047));
    presetSearchEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff8ee6c9));
    presetSearchEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xffdce7e4));
    addAndMakeVisible(presetSearchEditor);

    presetAuthorEditor.setTextToShowWhenEmpty("Author", juce::Colour(0xff617078));
    presetAuthorEditor.setText("Nate", juce::dontSendNotification);
    presetAuthorEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff101619));
    presetAuthorEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff344047));
    presetAuthorEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff8ee6c9));
    presetAuthorEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xffdce7e4));
    presetAuthorEditor.onTextChange = [this] { updatePresetSaveSummary(); };
    addAndMakeVisible(presetAuthorEditor);

    presetNotesEditor.setTextToShowWhenEmpty("Generated notes, macro intent, use case, or Ableton idea", juce::Colour(0xff617078));
    presetNotesEditor.setMultiLine(true, true);
    presetNotesEditor.setReturnKeyStartsNewLine(true);
    presetNotesEditor.setScrollbarsShown(true);
    presetNotesEditor.setFont(juce::FontOptions(12.0f));
    presetNotesEditor.setIndents(8, 6);
    presetNotesEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff101619));
    presetNotesEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff344047));
    presetNotesEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff8ee6c9));
    presetNotesEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xffdce7e4));
    presetNotesEditor.setColour(juce::TextEditor::highlightColourId, juce::Colour(0x558ee6c9));
    presetNotesEditor.setTooltip("Editable notes saved into preset metadata and included in Library search");
    presetNotesEditor.onTextChange = [this]
    {
        presetNotesIsRandomDraft = false;
        updatePresetSaveSummary();
    };
    addAndMakeVisible(presetNotesEditor);

    presetNotesTemplateBox.addItem("Note Template", 1);
    presetNotesTemplateBox.addItem("Macro Intent", 2);
    presetNotesTemplateBox.addItem("Ableton Use", 3);
    presetNotesTemplateBox.addItem("UKG Variation", 4);
    presetNotesTemplateBox.addItem("Mix Safety", 5);
    presetNotesTemplateBox.addItem("Pack Notes", 6);
    presetNotesTemplateBox.setSelectedId(1, juce::dontSendNotification);
    presetNotesTemplateBox.setTextWhenNothingSelected("Note Template");
    presetNotesTemplateBox.setTooltip("Insert a reusable notes scaffold into the generated preset notes");
    presetNotesTemplateBox.onChange = [this]
    {
        const auto templateId = presetNotesTemplateBox.getSelectedId();
        if (templateId <= 1)
            return;

        const auto templateText = presetNoteTemplateForId(templateId).trim();
        if (templateText.isEmpty())
            return;

        const auto currentText = presetNotesEditor.getText().trim();
        presetNotesEditor.setText(currentText.isEmpty()
                                      ? templateText
                                      : currentText + "\n\n" + templateText,
                                  juce::dontSendNotification);
        presetNotesIsRandomDraft = false;
        presetNotesTemplateBox.setSelectedId(1, juce::dontSendNotification);
        presetStatusLabel.setText("Inserted note template", juce::dontSendNotification);
        updatePresetSaveSummary();
    };
    addAndMakeVisible(presetNotesTemplateBox);

    randomCandidateDetailEditor.setReadOnly(true);
    randomCandidateDetailEditor.setMultiLine(true, true);
    randomCandidateDetailEditor.setScrollbarsShown(false);
    randomCandidateDetailEditor.setCaretVisible(false);
    randomCandidateDetailEditor.setPopupMenuEnabled(false);
    randomCandidateDetailEditor.setJustification(juce::Justification::topLeft);
    randomCandidateDetailEditor.setFont(juce::FontOptions(12.0f));
    randomCandidateDetailEditor.setIndents(8, 6);
    randomCandidateDetailEditor.setText("No active candidate\nSections: -\nTraits: -\nDiffs: -", juce::dontSendNotification);
    randomCandidateDetailEditor.setTooltip("Active Random Lab candidate summary");
    randomCandidateDetailEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xee101619));
    randomCandidateDetailEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff263035));
    randomCandidateDetailEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff8ee6c9));
    randomCandidateDetailEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xffdce7e4));
    randomCandidateDetailEditor.setColour(juce::TextEditor::highlightColourId, juce::Colour(0x558ee6c9));
    addAndMakeVisible(randomCandidateDetailEditor);

    auto configureReadOnlyInfoEditor = [this] (juce::TextEditor& editor, const juce::String& text, const juce::String& tooltip)
    {
        editor.setReadOnly(true);
        editor.setMultiLine(true, true);
        editor.setReturnKeyStartsNewLine(false);
        editor.setScrollbarsShown(true);
        editor.setCaretVisible(false);
        editor.setPopupMenuEnabled(false);
        editor.setJustification(juce::Justification::topLeft);
        editor.setFont(juce::FontOptions(12.5f));
        editor.setIndents(10, 8);
        editor.setText(text, juce::dontSendNotification);
        editor.setTooltip(tooltip);
        editor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff101619));
        editor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff2b363c));
        editor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff8ee6c9));
        editor.setColour(juce::TextEditor::textColourId, juce::Colour(0xffdce7e4));
        editor.setColour(juce::TextEditor::highlightColourId, juce::Colour(0x558ee6c9));
        addAndMakeVisible(editor);
    };

    configureReadOnlyInfoEditor(infoAboutEditor,
                                "Nate VST is a club-focused synth and sample instrument for house, UK garage, tech house, techno, and minimal patches.\n\nThe main workflow is fast sound design: build a source, add motion, shape the groove, then save usable variations with category, pack, key, BPM, rating, and notes.",
                                "About Nate VST");
    configureReadOnlyInfoEditor(infoWorkflowEditor,
                                "1. HOME: audition the current patch, balance macros, capture A/B snapshots, and grab a preset.\n2. LAB: generate recipe-aware ideas, lock sections, mutate only one area, then save the strongest candidate.\n3. SYNTH/SAMPLE: refine the source and chop movement.\n4. MOD/SEQ/FX: add groove, motion, throws, pump, width, and mix safety.\n5. LIBRARY: tag, rate, favorite, filter, and reload patches for Ableton sessions.",
                                "Suggested sound-design workflow");
    configureReadOnlyInfoEditor(infoDetailEditor,
                                juce::String(),
                                "Topic details for the selected Nate VST area");

    infoTopicBox.addItem("Getting Started", 1);
    infoTopicBox.addItem("Random Lab", 2);
    infoTopicBox.addItem("Modulation", 3);
    infoTopicBox.addItem("FX Rack", 4);
    infoTopicBox.addItem("Preset Library", 5);
    infoTopicBox.addItem("UKG + House Targets", 6);
    infoTopicBox.addItem("Undo + Editing Feel", 7);
    infoTopicBox.addItem("Planned Additions", 8);
    infoTopicBox.setSelectedId(1, juce::dontSendNotification);
    infoTopicBox.setTextWhenNothingSelected("Choose topic");
    infoTopicBox.setTooltip("Choose a Nate VST area to explain");
    infoTopicBox.onChange = [this] { updateInfoDetail(); };
    addAndMakeVisible(infoTopicBox);
    updateInfoDetail();

    waveformBox.addItemList(Parameters::waveformChoices(), 1);
    waveformBox.setTooltip("Osc 1 waveform. Choose Custom to edit the 16-point wave in the center display.");
    addAndMakeVisible(waveformBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::oscWave, waveformBox));

    osc2WaveBox.addItemList(Parameters::waveformChoices(), 1);
    osc2WaveBox.setTooltip("Osc 2 waveform. Choose Custom to edit the 16-point wave in the center display.");
    addAndMakeVisible(osc2WaveBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::osc2Wave, osc2WaveBox));

    waveEditorFocusButton.setTooltip("Open the center custom-wave editor. If no custom oscillator is active, Osc 1 switches to Custom so drawing, partial bars, frames, and WAV import/export are immediately editable.");
    waveEditorFocusButton.setWantsKeyboardFocus(false);
    waveEditorFocusButton.setMouseClickGrabsKeyboardFocus(false);
    waveEditorFocusButton.onClick = [this]
    {
        const auto osc1IsCustom = juce::roundToInt(readPlainParameterValue(Parameters::ID::oscWave, 1.0f)) == 7;
        const auto osc2IsCustom = juce::roundToInt(readPlainParameterValue(Parameters::ID::osc2Wave, 1.0f)) == 7;
        const auto targetOsc2 = osc2IsCustom && ! osc1IsCustom;

        if (! osc1IsCustom && ! osc2IsCustom)
        {
            captureGlobalEdit("Open wave editor");
            setPlainParameterValue(Parameters::ID::oscWave, 7.0f);
            waveformBox.setSelectedItemIndex(7, juce::dontSendNotification);
            updateSegmentedSelectors();
        }

        updateWavetableDisplay();
        updateSelectedControlInspector(targetOsc2 ? "O2 Wave Editor" : "O1 Wave Editor",
                                       targetOsc2 ? Parameters::ID::osc2CustomWave[0]
                                                  : Parameters::ID::oscCustomWave[0],
                                       readPlainParameterValue(targetOsc2 ? Parameters::ID::osc2CustomWave[0]
                                                                         : Parameters::ID::oscCustomWave[0],
                                                               0.5f));
        setRandomStatus(juce::String(targetOsc2 ? "O2" : "O1")
                        + " Wave Editor ready: draw, partials, frames, or WAV import/export");
        returnKeyboardFocusToPiano();
    };
    addAndMakeVisible(waveEditorFocusButton);

    wavetableDrawModeBox.addItem("Point Draw", 1);
    wavetableDrawModeBox.addItem("Line Draw", 2);
    wavetableDrawModeBox.addItem("Smooth Brush", 3);
    wavetableDrawModeBox.addItem("Step Pencil", 4);
    wavetableDrawModeBox.addItem("Center Erase", 5);
    wavetableDrawModeBox.setSelectedId(1, juce::dontSendNotification);
    wavetableDrawModeBox.setTextWhenNothingSelected("Draw Mode");
    wavetableDrawModeBox.setTooltip("Choose how dragging edits custom wave points");
    addAndMakeVisible(wavetableDrawModeBox);

    wavetableToolBox.addItem("Wave Tools", 1);
    wavetableToolBox.addItem("Mirror", 2);
    wavetableToolBox.addItem("Normalize", 3);
    wavetableToolBox.addItem("Smooth", 4);
    wavetableToolBox.addItem("Sharpen", 5);
    wavetableToolBox.addItem("Randomize", 6);
    wavetableToolBox.addItem("Harmonics", 7);
    wavetableToolBox.addItem("Sine", 8);
    wavetableToolBox.addItem("Saw", 9);
    wavetableToolBox.addItem("Square", 10);
    wavetableToolBox.addItem("Triangle", 11);
    wavetableToolBox.addItem("Copy Points", 12);
    wavetableToolBox.addItem("Paste Points", 13);
    wavetableToolBox.addItem("Invert", 14);
    wavetableToolBox.addItem("Reverse", 15);
    wavetableToolBox.addItem("Rotate Left", 16);
    wavetableToolBox.addItem("Rotate Right", 17);
    wavetableToolBox.addItem("Quantize", 18);
    wavetableToolBox.addItem("Fold", 19);
    wavetableToolBox.addItem("Zero DC", 20);
    wavetableToolBox.addItem("Bright Partials", 21);
    wavetableToolBox.addItem("Soft Clip", 22);
    wavetableToolBox.addItem("Window Ends", 23);
    wavetableToolBox.addSectionHeading("Single-Cycle WAV");
    wavetableToolBox.addItem("Import WAV", 24);
    wavetableToolBox.addItem("Export WAV", 25);
    wavetableToolBox.addSectionHeading("Additive Partials");
    wavetableToolBox.addItem("Odd Harmonics", 26);
    wavetableToolBox.addItem("Even Harmonics", 27);
    wavetableToolBox.addItem("Tilt Bright", 28);
    wavetableToolBox.addItem("Tilt Dark", 29);
    wavetableToolBox.addItem("Hollow Partials", 30);
    wavetableToolBox.addItem("Random Partials", 31);
    wavetableToolBox.addSectionHeading("Morph Frames");
    for (auto frame = 1; frame <= 8; ++frame)
        wavetableToolBox.addItem("Store Frame " + juce::String(frame), 31 + frame);
    for (auto frame = 1; frame <= 8; ++frame)
        wavetableToolBox.addItem("Load Frame " + juce::String(frame), 39 + frame);
    wavetableToolBox.addItem("Bake WT Position", 48);
    wavetableToolBox.addItem("Current Sweep", 49);
    wavetableToolBox.addItem("Classic House Stack", 50);
    wavetableToolBox.addItem("Rave Sweep", 51);
    wavetableToolBox.setSelectedId(1, juce::dontSendNotification);
    wavetableToolBox.setTextWhenNothingSelected("Wave Tools");
    wavetableToolBox.setTooltip("Apply custom-wave edits, import/export WAV single cycles, shape additive partials, generate frame stacks, or store/load eight WT morph frames");
    addAndMakeVisible(wavetableToolBox);

    noiseTypeBox.addItemList(Parameters::noiseTypeChoices(), 1);
    noiseTypeBox.setTextWhenNothingSelected("Noise Type");
    noiseTypeBox.setTooltip("Choose the noise source color for attack ticks, air, vinyl texture, and digital grit");
    addAndMakeVisible(noiseTypeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::noiseType, noiseTypeBox));

    filterModeBox.addItemList(Parameters::filterModeChoices(), 1);
    filterModeBox.setTextWhenNothingSelected("Filter Mode");
    filterModeBox.setTooltip("Choose low-pass, band-pass, or high-pass filter mode");
    addAndMakeVisible(filterModeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::filterMode, filterModeBox));

    filterCharacterBox.addItemList(Parameters::filterCharacterChoices(), 1);
    filterCharacterBox.setTextWhenNothingSelected("Character");
    filterCharacterBox.setTooltip("Choose the filter drive character");
    addAndMakeVisible(filterCharacterBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::filterCharacter, filterCharacterBox));

    filterSlopeBox.addItemList(Parameters::filterSlopeChoices(), 1);
    filterSlopeBox.setTextWhenNothingSelected("Slope");
    filterSlopeBox.setTooltip("Choose 12 dB or 24 dB filter slope");
    addAndMakeVisible(filterSlopeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::filterSlope, filterSlopeBox));

    recipeBox.addItemList(Parameters::randomRecipeChoices(), 1);
    recipeBox.setTextWhenNothingSelected("Generator Recipe");
    recipeBox.setTooltip("Profiled Random Lab recipes: bass, pluck, chord, pad, FX, garage, house, techno, and minimal starting points");
    addAndMakeVisible(recipeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomRecipe, recipeBox));
    recipeBox.onChange = [this] { updateRandomRecipeInfo(); };

    randomScopeBox.addItemList({ "All", "Source", "Env", "Filter", "Sample", "FX", "Seq", "Macros" }, 1);
    randomScopeBox.setSelectedId(1, juce::dontSendNotification);
    randomScopeBox.setTextWhenNothingSelected("Scope");
    randomScopeBox.setTooltip("Limit Generate, Vary, Mutate, and Wild to one patch section");
    addAndMakeVisible(randomScopeBox);

    randomSectionActionBox.addItem("Random Action", 1);
    randomSectionActionBox.addItem("Source", 2);
    randomSectionActionBox.addItem("Env", 3);
    randomSectionActionBox.addItem("Filter", 4);
    randomSectionActionBox.addItem("Sample", 5);
    randomSectionActionBox.addItem("FX", 6);
    randomSectionActionBox.addItem("Seq", 7);
    randomSectionActionBox.addItem("Macros", 8);
    randomSectionActionBox.addItem("Smart Full", 9);
    randomSectionActionBox.addItem("Safe Vary", 10);
    randomSectionActionBox.addItem("Wild Chain", 11);
    randomSectionActionBox.addItem("Dice Seq", 12);
    randomSectionActionBox.addItem("Macro Motion", 13);
    randomSectionActionBox.addItem("FX Space", 14);
    randomSectionActionBox.addItem("Explore 3", 15);
    randomSectionActionBox.setSelectedId(1, juce::dontSendNotification);
    randomSectionActionBox.setTextWhenNothingSelected("Random Action");
    randomSectionActionBox.setTooltip("Run focused randomization actions: section rolls, full generation, safe variation, wild chain, sequencer dice, macro motion, FX space, or three candidate explorations");
    addAndMakeVisible(randomSectionActionBox);

    randomLockActionBox.addItem("Lock Focus", 1);
    randomLockActionBox.addItem("Pitch", 2);
    randomLockActionBox.addItem("Envelope", 3);
    randomLockActionBox.addItem("Filter", 4);
    randomLockActionBox.addItem("Source", 5);
    randomLockActionBox.addItem("Sample", 6);
    randomLockActionBox.addItem("FX", 7);
    randomLockActionBox.addItem("Output", 8);
    randomLockActionBox.addItem("Sequencer", 9);
    randomLockActionBox.setSelectedId(1, juce::dontSendNotification);
    randomLockActionBox.setTextWhenNothingSelected("Lock Focus");
    randomLockActionBox.setTooltip("Toggle one generation lock without exposing the full lock grid");
    addAndMakeVisible(randomLockActionBox);

    sequencerRateBox.addItemList(Parameters::sequencerRateChoices(), 1);
    addAndMakeVisible(sequencerRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerRate, sequencerRateBox));

    sequencerGrooveBox.addItemList(Parameters::sequencerGrooveModeChoices(), 1);
    sequencerGrooveBox.setTextWhenNothingSelected("Groove");
    addAndMakeVisible(sequencerGrooveBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerGrooveMode, sequencerGrooveBox));

    sequencerScaleBox.addItemList(Parameters::sequencerScaleChoices(), 1);
    sequencerScaleBox.setTextWhenNothingSelected("Scale");
    addAndMakeVisible(sequencerScaleBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerScale, sequencerScaleBox));

    sequencerChordBox.addItemList(Parameters::sequencerChordModeChoices(), 1);
    sequencerChordBox.setTextWhenNothingSelected("Chord");
    addAndMakeVisible(sequencerChordBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerChordMode, sequencerChordBox));

    sequencerVoicingBox.addItemList(Parameters::sequencerChordVoicingChoices(), 1);
    sequencerVoicingBox.setTextWhenNothingSelected("Voice");
    addAndMakeVisible(sequencerVoicingBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerChordVoicing, sequencerVoicingBox));

    sequencerPatternBox.addItem("Bass", 1);
    sequencerPatternBox.addItem("Stab", 2);
    sequencerPatternBox.addItem("UKG 2-Step", 3);
    sequencerPatternBox.addItem("Shuffle Bass", 4);
    sequencerPatternBox.addItem("Organ Skank", 5);
    sequencerPatternBox.addItem("Vocal Chop", 6);
    sequencerPatternBox.addItem("Late Stab", 7);
    sequencerPatternBox.addItem("House Chord", 8);
    sequencerPatternBox.addItem("Tech Bass", 9);
    sequencerPatternBox.addItem("Minimal Pluck", 10);
    sequencerPatternBox.addItem("Techno Pulse", 11);
    sequencerPatternBox.addItem("Deep Chord", 12);
    sequencerPatternBox.addItem("Dub Chord", 13);
    sequencerPatternBox.addItem("Off Bass", 14);
    sequencerPatternBox.addItem("Rolling Bass", 15);
    sequencerPatternBox.setSelectedId(3, juce::dontSendNotification);
    addAndMakeVisible(sequencerPatternBox);

    sequencerGrooveTransformBox.addItem("Tighten", 1);
    sequencerGrooveTransformBox.addItem("Straight Anchors", 2);
    sequencerGrooveTransformBox.addItem("Swung Ghosts", 3);
    sequencerGrooveTransformBox.addItem("Late Stabs", 4);
    sequencerGrooveTransformBox.addItem("Vocal Push", 5);
    sequencerGrooveTransformBox.addItem("Humanize", 6);
    sequencerGrooveTransformBox.addSectionHeading("House Templates");
    sequencerGrooveTransformBox.addItem("House Shuffle", 7);
    sequencerGrooveTransformBox.addItem("UKG 2-Step Push", 8);
    sequencerGrooveTransformBox.addItem("Tech House Tight", 9);
    sequencerGrooveTransformBox.addItem("Minimal Skip", 10);
    sequencerGrooveTransformBox.addItem("Techno Drive", 11);
    sequencerGrooveTransformBox.addItem("Bass Contour", 12);
    sequencerGrooveTransformBox.addSectionHeading("Paint Tools");
    sequencerGrooveTransformBox.addItem("Chord Stab Paint", 14);
    sequencerGrooveTransformBox.addSectionHeading("Chain Tools");
    sequencerGrooveTransformBox.addItem("Build 4-Bar Chain", 13);
    sequencerGrooveTransformBox.setSelectedId(1, juce::dontSendNotification);
    sequencerGrooveTransformBox.setTooltip("Choose a timing transform or genre groove template for the current sequence");
    addAndMakeVisible(sequencerGrooveTransformBox);

    sequencerLaneViewBox.addItem("All Lanes", 1);
    sequencerLaneViewBox.addItem("Groove", 2);
    sequencerLaneViewBox.addItem("Dynamics", 3);
    sequencerLaneViewBox.addItem("Ratchets", 4);
    sequencerLaneViewBox.addItem("Lock/Slide", 5);
    sequencerLaneViewBox.setSelectedId(1, juce::dontSendNotification);
    sequencerLaneViewBox.setTextWhenNothingSelected("Lane View");
    sequencerLaneViewBox.setTooltip("Choose which step lanes are emphasized in the SEQ piano roll");
    sequencerLaneViewBox.onChange = [this]
    {
        const auto laneViewMode = juce::jlimit(0, 4, sequencerLaneViewBox.getSelectedId() - 1);
        sequencerGrid.setLaneViewMode(laneViewMode);
        expandedSequencerGrid.setLaneViewMode(laneViewMode);
        updateSequencerStepEditor();
    };
    addAndMakeVisible(sequencerLaneViewBox);

    sequencerLockDestinationBox.addItemList(Parameters::sequencerLockDestinationChoices(), 1);
    sequencerLockDestinationBox.setTextWhenNothingSelected("Lock");
    sequencerLockDestinationBox.setTooltip("Choose which safe synth or FX destination the per-step Lock lane moves");
    addAndMakeVisible(sequencerLockDestinationBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerLockDestination, sequencerLockDestinationBox));

    sampleModeBox.addItem("Gate", 1);
    sampleModeBox.addItem("One Shot", 2);
    sampleModeBox.addItem("Slice Keys", 3);
    sampleModeBox.setTextWhenNothingSelected("Mode");
    sampleModeBox.setTooltip("Gate follows note-off, One Shot plays the selected range, Slice Keys maps MIDI notes C3-G3 across the eight stored slice pads and repeats every octave");
    addAndMakeVisible(sampleModeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::samplePlaybackMode, sampleModeBox));

    sampleEngineBox.addItemList(Parameters::sampleEngineModeChoices(), 1);
    sampleEngineBox.setTextWhenNothingSelected("Engine");
    sampleEngineBox.setTooltip("Classic playback, Granular spray, Spectral freeze/smear, or Cloud grain playback for recorded/imported snippets");
    addAndMakeVisible(sampleEngineBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sampleEngineMode, sampleEngineBox));

    sampleSliceStyleBox.addItemList(Parameters::sampleSliceStyleChoices(), 1);
    sampleSliceStyleBox.setTextWhenNothingSelected("Slice Style");
    sampleSliceStyleBox.setTooltip("Choose how default slice pads set pitch, reverse, choke, and stutter behavior");
    addAndMakeVisible(sampleSliceStyleBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sampleSliceStyle, sampleSliceStyleBox));

    sampleStutterRateBox.addItem("1/8", 1);
    sampleStutterRateBox.addItem("1/16", 2);
    sampleStutterRateBox.addItem("1/32", 3);
    sampleStutterRateBox.setTextWhenNothingSelected("Rate");
    addAndMakeVisible(sampleStutterRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sampleStutterRate, sampleStutterRateBox));

    addAndMakeVisible(presetBox);

    presetCategoryBox.addItemList(presetCategoryChoices(), 1);
    presetCategoryBox.setSelectedItemIndex(0, juce::dontSendNotification);
    presetCategoryBox.setEditableText(true);
    presetCategoryBox.setTextWhenNothingSelected("Category / Folder");
    presetCategoryBox.setTooltip("Choose or type a save category. Use slashes for subfolders, like UKG/Bass.");
    presetCategoryBox.onChange = [this]
    {
        clearPresetOverwriteWarning();
        updatePresetSaveSummary();
    };
    addAndMakeVisible(presetCategoryBox);

    presetFilterBox.addItemList(presetFilterChoices(), 1);
    presetFilterBox.setSelectedItemIndex(0, juce::dontSendNotification);
    presetFilterBox.setTooltip("Choose basic filters or smart production crates built from tags, notes, pack, BPM, rating, source, and macro values");
    addAndMakeVisible(presetFilterBox);

    presetTagBox.addItemList(presetTagChoices(), 1);
    presetTagBox.setSelectedItemIndex(0, juce::dontSendNotification);
    addAndMakeVisible(presetTagBox);

    presetSortBox.addItemList(presetSortChoices(), 1);
    presetSortBox.setSelectedItemIndex(0, juce::dontSendNotification);
    presetSortBox.setTextWhenNothingSelected("Sort");
    presetSortBox.setTooltip("Sort the visible library by name, rating, newest save, category, or source");
    addAndMakeVisible(presetSortBox);

    presetBrowserPackFilterBox.addItem("All Packs", 1);
    presetBrowserPackFilterBox.addItemList(presetPackChoices(), 2);
    presetBrowserPackFilterBox.setSelectedId(1, juce::dontSendNotification);
    presetBrowserPackFilterBox.setTextWhenNothingSelected("All Packs");
    presetBrowserPackFilterBox.setTooltip("Filter the browser by pack without changing preset save metadata");
    addAndMakeVisible(presetBrowserPackFilterBox);

    presetRatingBox.addItem("No Stars", 1);
    presetRatingBox.addItem("1 Star", 2);
    presetRatingBox.addItem("2 Stars", 3);
    presetRatingBox.addItem("3 Stars", 4);
    presetRatingBox.addItem("4 Stars", 5);
    presetRatingBox.addItem("5 Stars", 6);
    presetRatingBox.setSelectedId(1, juce::dontSendNotification);
    presetRatingBox.setTextWhenNothingSelected("Preset Rating");
    presetRatingBox.setTooltip("Rate the selected browser preset from 1 to 5 stars");
    addAndMakeVisible(presetRatingBox);

    candidateRatingBox.addItem("No Stars", 1);
    candidateRatingBox.addItem("1 Star", 2);
    candidateRatingBox.addItem("2 Stars", 3);
    candidateRatingBox.addItem("3 Stars", 4);
    candidateRatingBox.addItem("4 Stars", 5);
    candidateRatingBox.addItem("5 Stars", 6);
    candidateRatingBox.setSelectedId(1, juce::dontSendNotification);
    candidateRatingBox.setTextWhenNothingSelected("Save Rating");
    candidateRatingBox.setTooltip("Rating to apply when saving the active random candidate");
    addAndMakeVisible(candidateRatingBox);

    presetPackBox.addItemList(presetPackChoices(), 1);
    presetPackBox.setSelectedId(1, juce::dontSendNotification);
    presetPackBox.setEditableText(true);
    presetPackBox.setTextWhenNothingSelected("Pack");
    presetPackBox.setTooltip("Choose or type a preset pack name");
    presetPackBox.onChange = [this] { updatePresetSaveSummary(); };
    addAndMakeVisible(presetPackBox);

    presetKeyBox.addItemList(presetKeyChoices(), 1);
    presetKeyBox.setSelectedId(1, juce::dontSendNotification);
    presetKeyBox.setEditableText(true);
    presetKeyBox.setTextWhenNothingSelected("Key");
    presetKeyBox.setTooltip("Store the musical key for browser search and sorting");
    presetKeyBox.onChange = [this] { updatePresetSaveSummary(); };
    addAndMakeVisible(presetKeyBox);

    presetBpmBox.addItemList(presetBpmChoices(), 1);
    presetBpmBox.setSelectedId(7, juce::dontSendNotification);
    presetBpmBox.setEditableText(true);
    presetBpmBox.setTextWhenNothingSelected("BPM");
    presetBpmBox.setTooltip("Store the target tempo for browser search, filtering, and sorting");
    presetBpmBox.onChange = [this] { updatePresetSaveSummary(); };
    addAndMakeVisible(presetBpmBox);

    fxAddBox.addSectionHeading("Tone & Drive");
    fxAddBox.addItem("Tone", 1);
    fxAddBox.addItem("EQ", 2);
    fxAddBox.addItem("Drive", 3);
    fxAddBox.addItem("Crush", 4);
    fxAddBox.addSectionHeading("Movement");
    fxAddBox.addItem("Pump", 5);
    fxAddBox.addItem("Tremolo", 6);
    fxAddBox.addItem("Ring Mod", 7);
    fxAddBox.addItem("Comb", 8);
    fxAddBox.addItem("Phaser", 9);
    fxAddBox.addItem("Flanger", 10);
    fxAddBox.addItem("Chorus", 11);
    fxAddBox.addSectionHeading("Space & Utility");
    fxAddBox.addItem("Delay", 12);
    fxAddBox.addItem("Reverb", 13);
    fxAddBox.addItem("Width", 14);
    fxAddBox.addItem("Guard", 15);
    fxAddBox.setTextWhenNothingSelected("Add FX");
    fxAddBox.setTooltip("Enable a fixed FX module and open its focused editor");
    addAndMakeVisible(fxAddBox);

    fxPresetBox.setTextWhenNothingSelected("Module Preset");
    fxPresetBox.setTooltip("Load a focused preset for the selected FX module");
    addAndMakeVisible(fxPresetBox);

    fxDelayRateBox.addItemList(Parameters::delayRateChoices(), 1);
    fxDelayRateBox.setTextWhenNothingSelected("Rate");
    fxDelayRateBox.setTooltip("Choose the tempo-synced delay division");
    addAndMakeVisible(fxDelayRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxDelayRate, fxDelayRateBox));

    fxPumpRateBox.addItem("1/4", 1);
    fxPumpRateBox.addItem("1/8", 2);
    fxPumpRateBox.addItem("1/8T", 3);
    fxPumpRateBox.addItem("1/16", 4);
    fxPumpRateBox.setTextWhenNothingSelected("Rate");
    addAndMakeVisible(fxPumpRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxPumpRate, fxPumpRateBox));

    fxPumpCurveBox.addItemList(Parameters::pumpCurveChoices(), 1);
    fxPumpCurveBox.setTextWhenNothingSelected("Curve");
    fxPumpCurveBox.setTooltip("Choose the pump/duck envelope shape");
    addAndMakeVisible(fxPumpCurveBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxPumpCurve, fxPumpCurveBox));

    fxTremoloRateBox.addItem("1/4", 1);
    fxTremoloRateBox.addItem("1/8", 2);
    fxTremoloRateBox.addItem("1/8T", 3);
    fxTremoloRateBox.addItem("1/16", 4);
    fxTremoloRateBox.setTextWhenNothingSelected("Rate");
    addAndMakeVisible(fxTremoloRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxTremoloRate, fxTremoloRateBox));

    const auto modDestinationChoices = Parameters::modulationDestinationChoices();
    for (auto index = 1; index < modDestinationChoices.size(); ++index)
        modInspectorDestinationBox.addItem(modDestinationChoices[index], index + 1);
    modInspectorDestinationBox.setSelectedId(2, juce::dontSendNotification);
    modInspectorDestinationBox.setTextWhenNothingSelected("Destination");
    modInspectorDestinationBox.setTooltip("Inspect active modulation routes for one destination");
    addAndMakeVisible(modInspectorDestinationBox);

    const auto modSourceChoices = Parameters::modulationSourceChoices();
    for (auto index = 1; index < modSourceChoices.size(); ++index)
        modInspectorSourceBox.addItem(modSourceChoices[index], index + 1);
    modInspectorSourceBox.setSelectedId(2, juce::dontSendNotification);
    modInspectorSourceBox.setTextWhenNothingSelected("Add Source");
    modInspectorSourceBox.setTooltip("Choose a modulation source to add to the inspected destination");
    addAndMakeVisible(modInspectorSourceBox);

    for (auto index = firstMacroModSourceIndex; index <= lastMacroModSourceIndex && index < modSourceChoices.size(); ++index)
        modMacroAssignSourceBox.addItem(modSourceChoices[index], index + 1);
    modMacroAssignSourceBox.setSelectedId(firstMacroModSourceIndex + 1, juce::dontSendNotification);
    modMacroAssignSourceBox.setTextWhenNothingSelected("Macro");
    modMacroAssignSourceBox.setTooltip("Choose the performance macro to edit");
    addAndMakeVisible(modMacroAssignSourceBox);

    for (auto index = 1; index < modDestinationChoices.size(); ++index)
        modMacroAssignDestinationBox.addItem(modDestinationChoices[index], index + 1);
    modMacroAssignDestinationBox.setSelectedId(2, juce::dontSendNotification);
    modMacroAssignDestinationBox.setTextWhenNothingSelected("Destination");
    modMacroAssignDestinationBox.setTooltip("Choose the destination controlled by the selected macro");
    addAndMakeVisible(modMacroAssignDestinationBox);

    lfo1ShapeBox.addItemList(Parameters::lfoShapeChoices(), 1);
    lfo1ShapeBox.setTextWhenNothingSelected("Shape");
    addAndMakeVisible(lfo1ShapeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::lfo1Shape, lfo1ShapeBox));

    lfo1SyncRateBox.addItemList(Parameters::lfoSyncRateChoices(), 1);
    lfo1SyncRateBox.setTextWhenNothingSelected("Rate");
    addAndMakeVisible(lfo1SyncRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::lfo1SyncRate, lfo1SyncRateBox));

    lfo2ShapeBox.addItemList(Parameters::lfo2ShapeChoices(), 1);
    lfo2ShapeBox.setTextWhenNothingSelected("LFO 2 Shape");
    lfo2ShapeBox.setTooltip("Choose the secondary LFO shape for extra groove, chop, and FX motion");
    addAndMakeVisible(lfo2ShapeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::lfo2Shape, lfo2ShapeBox));

    lfo2SyncRateBox.addItemList(Parameters::lfoSyncRateChoices(), 1);
    lfo2SyncRateBox.setTextWhenNothingSelected("LFO 2 Rate");
    addAndMakeVisible(lfo2SyncRateBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::lfo2SyncRate, lfo2SyncRateBox));

    lfoCurvePresetBox.addItemList(lfoCurvePresetChoices(), 1);
    lfoCurvePresetBox.setSelectedId(1, juce::dontSendNotification);
    lfoCurvePresetBox.setTextWhenNothingSelected("Curve Preset");
    lfoCurvePresetBox.setTooltip("Load an LFO curve shape for house, UKG, techno, and minimal movement");
    addAndMakeVisible(lfoCurvePresetBox);

    lfoCurveActionBox.addItem("Edit Curve", 1);
    lfoCurveActionBox.addItem("Invert", 2);
    lfoCurveActionBox.addItem("Reverse", 3);
    lfoCurveActionBox.addItem("Smooth", 4);
    lfoCurveActionBox.addItem("Quantize", 5);
    lfoCurveActionBox.addItem("Randomize", 6);
    lfoCurveActionBox.addItem("UKG Swing", 7);
    lfoCurveActionBox.setSelectedId(1, juce::dontSendNotification);
    lfoCurveActionBox.setTextWhenNothingSelected("Edit Curve");
    lfoCurveActionBox.setTooltip("Apply utility edits to the visible LFO/MSEG curve");
    addAndMakeVisible(lfoCurveActionBox);

    lfoCurveInvertButton.setTooltip("Invert the custom LFO curve around the centre line");
    lfoCurveInvertButton.onClick = [this] { applyLfoCurveTool(LfoCurveTool::invert); };
    addAndMakeVisible(lfoCurveInvertButton);

    lfoCurveReverseButton.setTooltip("Reverse the MSEG point order so the motion plays backward");
    lfoCurveReverseButton.onClick = [this] { applyLfoCurveTool(LfoCurveTool::reverse); };
    addAndMakeVisible(lfoCurveReverseButton);

    lfoCurveSmoothButton.setTooltip("Smooth neighbouring MSEG points for less stepped movement");
    lfoCurveSmoothButton.onClick = [this] { applyLfoCurveTool(LfoCurveTool::smooth); };
    addAndMakeVisible(lfoCurveSmoothButton);

    lfoCurveQuantizeButton.setTooltip("Quantize MSEG point values to eighth-depth steps");
    lfoCurveQuantizeButton.onClick = [this] { applyLfoCurveTool(LfoCurveTool::quantize); };
    addAndMakeVisible(lfoCurveQuantizeButton);

    lfoCurveRandomButton.setTooltip("Generate a controlled random MSEG curve for new movement ideas");
    lfoCurveRandomButton.onClick = [this] { applyLfoCurveTool(LfoCurveTool::randomize); };
    addAndMakeVisible(lfoCurveRandomButton);

    lfoCurveGarageButton.setTooltip("Apply a UK garage swing MSEG shape for shuffled bass, stabs, and chops");
    lfoCurveGarageButton.onClick = [this] { applyLfoCurveTool(LfoCurveTool::garage); };
    addAndMakeVisible(lfoCurveGarageButton);

    for (size_t index = 0; index < modSourceBoxes.size(); ++index)
    {
        auto& sourceBox = modSourceBoxes[index];
        sourceBox.addItemList(Parameters::modulationSourceChoices(), 1);
        sourceBox.setTextWhenNothingSelected("Source");
        addAndMakeVisible(sourceBox);
        comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::modMatrixSource[index], sourceBox));

        auto& destinationBox = modDestinationBoxes[index];
        destinationBox.addItemList(Parameters::modulationDestinationChoices(), 1);
        destinationBox.setTextWhenNothingSelected("Destination");
        addAndMakeVisible(destinationBox);
        comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::modMatrixDestination[index], destinationBox));

        auto& enabledButton = modSlotEnabledButtons[index];
        enabledButton.setButtonText("On");
        enabledButton.setTooltip("Bypass or enable modulation slot " + juce::String(static_cast<int>(index + 1)));
        addAndMakeVisible(enabledButton);
        buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::modMatrixEnabled[index], enabledButton));

        auto& duplicateButton = modSlotDuplicateButtons[index];
        duplicateButton.setButtonText("+");
        duplicateButton.setTooltip("Duplicate modulation slot " + juce::String(static_cast<int>(index + 1)) + " to the next free slot");
        duplicateButton.onClick = [this, index] { duplicateModRoute(index); };
        addAndMakeVisible(duplicateButton);

        auto& deleteButton = modSlotDeleteButtons[index];
        deleteButton.setButtonText("X");
        deleteButton.setTooltip("Delete modulation slot " + juce::String(static_cast<int>(index + 1)));
        deleteButton.onClick = [this, index] { deleteModRoute(index); };
        addAndMakeVisible(deleteButton);
    }

    monoButton.setButtonText("Mono");
    addAndMakeVisible(monoButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::monoMode, monoButton));

    sampleEnabledButton.setButtonText("On");
    addAndMakeVisible(sampleEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sampleEnabled, sampleEnabledButton));

    sampleReverseButton.setButtonText("Rev");
    addAndMakeVisible(sampleReverseButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sampleReverse, sampleReverseButton));

    sampleStutterEnabledButton.setButtonText("Stutter");
    addAndMakeVisible(sampleStutterEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sampleStutterEnabled, sampleStutterEnabledButton));

    sequencerEnabledButton.setButtonText("On");
    addAndMakeVisible(sequencerEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerEnabled, sequencerEnabledButton));

    sequencerChordMemoryButton.setButtonText("Memory");
    sequencerChordMemoryButton.setTooltip("Expand live notes through the selected chord mode and voicing");
    addAndMakeVisible(sequencerChordMemoryButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::sequencerChordMemory, sequencerChordMemoryButton));

    fxDistortionEnabledButton.setButtonText("Dist");
    addAndMakeVisible(fxDistortionEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxDistortionEnabled, fxDistortionEnabledButton));

    fxBitcrushEnabledButton.setButtonText("Crush");
    addAndMakeVisible(fxBitcrushEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxBitcrushEnabled, fxBitcrushEnabledButton));

    fxPumpEnabledButton.setButtonText("Pump");
    addAndMakeVisible(fxPumpEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxPumpEnabled, fxPumpEnabledButton));

    fxTremoloEnabledButton.setButtonText("Trem");
    addAndMakeVisible(fxTremoloEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxTremoloEnabled, fxTremoloEnabledButton));

    fxRingEnabledButton.setButtonText("Ring");
    addAndMakeVisible(fxRingEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxRingEnabled, fxRingEnabledButton));

    fxCombEnabledButton.setButtonText("Comb");
    addAndMakeVisible(fxCombEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxCombEnabled, fxCombEnabledButton));

    fxChorusEnabledButton.setButtonText("Chorus");
    addAndMakeVisible(fxChorusEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxChorusEnabled, fxChorusEnabledButton));

    fxDelayEnabledButton.setButtonText("Delay");
    addAndMakeVisible(fxDelayEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxDelayEnabled, fxDelayEnabledButton));

    fxDelaySyncButton.setButtonText("Sync");
    fxDelaySyncButton.setTooltip("Lock delay time to the host tempo division");
    addAndMakeVisible(fxDelaySyncButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxDelaySync, fxDelaySyncButton));

    fxReverbEnabledButton.setButtonText("Reverb");
    addAndMakeVisible(fxReverbEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxReverbEnabled, fxReverbEnabledButton));

    fxWidthEnabledButton.setButtonText("Width");
    addAndMakeVisible(fxWidthEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxWidthEnabled, fxWidthEnabledButton));

    fxToneEnabledButton.setButtonText("Tone");
    addAndMakeVisible(fxToneEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxToneEnabled, fxToneEnabledButton));

    fxEqEnabledButton.setButtonText("EQ");
    addAndMakeVisible(fxEqEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxEqEnabled, fxEqEnabledButton));

    fxPhaserEnabledButton.setButtonText("Phaser");
    addAndMakeVisible(fxPhaserEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxPhaserEnabled, fxPhaserEnabledButton));

    fxFlangerEnabledButton.setButtonText("Flanger");
    addAndMakeVisible(fxFlangerEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxFlangerEnabled, fxFlangerEnabledButton));

    fxGuardEnabledButton.setButtonText("Guard");
    addAndMakeVisible(fxGuardEnabledButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::fxGuardEnabled, fxGuardEnabledButton));

    randomLockPitchButton.setButtonText("Pitch");
    addAndMakeVisible(randomLockPitchButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomLockPitch, randomLockPitchButton));

    randomLockEnvelopeButton.setButtonText("Env");
    addAndMakeVisible(randomLockEnvelopeButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomLockEnvelope, randomLockEnvelopeButton));

    randomLockFilterButton.setButtonText("Filter");
    addAndMakeVisible(randomLockFilterButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomLockFilter, randomLockFilterButton));

    randomLockSourceButton.setButtonText("Source");
    addAndMakeVisible(randomLockSourceButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomLockSource, randomLockSourceButton));

    randomLockSampleButton.setButtonText("Sample");
    addAndMakeVisible(randomLockSampleButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomLockSample, randomLockSampleButton));

    randomLockFxButton.setButtonText("FX");
    addAndMakeVisible(randomLockFxButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomLockFx, randomLockFxButton));

    randomLockOutputButton.setButtonText("Output");
    addAndMakeVisible(randomLockOutputButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomLockOutput, randomLockOutputButton));

    randomLockSequencerButton.setButtonText("Seq");
    addAndMakeVisible(randomLockSequencerButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomLockSequencer, randomLockSequencerButton));

    lfo1SyncButton.setButtonText("Sync");
    addAndMakeVisible(lfo1SyncButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::lfo1Sync, lfo1SyncButton));

    lfo1RetriggerButton.setButtonText("Retrig");
    addAndMakeVisible(lfo1RetriggerButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::lfo1Retrigger, lfo1RetriggerButton));

    lfo2SyncButton.setButtonText("Sync");
    addAndMakeVisible(lfo2SyncButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::lfo2Sync, lfo2SyncButton));

    lfo2RetriggerButton.setButtonText("Retrig");
    addAndMakeVisible(lfo2RetriggerButton);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::lfo2Retrigger, lfo2RetriggerButton));

    configureSlider(octaveSlider, octaveLabel, "Oct", Parameters::ID::oscOctave);
    configureSlider(tuneSlider, tuneLabel, "Tune", Parameters::ID::oscTune);
    configureSlider(osc1LevelSlider, osc1LevelLabel, "Osc 1", Parameters::ID::osc1Level);
    configureSlider(osc2OctaveSlider, osc2OctaveLabel, "O2 Oct", Parameters::ID::osc2Octave);
    configureSlider(osc2TuneSlider, osc2TuneLabel, "O2 Tune", Parameters::ID::osc2Tune);
    configureSlider(osc2LevelSlider, osc2LevelLabel, "Osc 2", Parameters::ID::osc2Level);
    configureSlider(subLevelSlider, subLevelLabel, "Sub", Parameters::ID::subLevel);
    configureSlider(noiseLevelSlider, noiseLevelLabel, "Noise", Parameters::ID::noiseLevel);
    configureSlider(noiseDecaySlider, noiseDecayLabel, "N Decay", Parameters::ID::noiseDecay);
    configureSlider(oscWarpSlider, oscWarpLabel, "Osc Warp", Parameters::ID::oscWarp);
    configureSlider(oscWavetablePositionSlider, oscWavetablePositionLabel, "WT 1", Parameters::ID::oscWavetablePosition);
    configureSlider(osc2WavetablePositionSlider, osc2WavetablePositionLabel, "WT 2", Parameters::ID::osc2WavetablePosition);
    configureSlider(unisonVoicesSlider, unisonVoicesLabel, "Voices", Parameters::ID::unisonVoices);
    configureSlider(unisonDetuneSlider, unisonDetuneLabel, "Detune", Parameters::ID::unisonDetune);
    configureSlider(unisonBlendSlider, unisonBlendLabel, "Blend", Parameters::ID::unisonBlend);
    configureSlider(unisonSpreadSlider, unisonSpreadLabel, "Spread", Parameters::ID::unisonSpread);
    configureSlider(glideSlider, glideLabel, "Glide", Parameters::ID::glideTime);
    configureSlider(macroToneSlider, macroToneLabel, "Tone", Parameters::ID::macroTone);
    configureSlider(macroDirtSlider, macroDirtLabel, "Dirt", Parameters::ID::macroDirt);
    configureSlider(macroMotionSlider, macroMotionLabel, "Motion", Parameters::ID::macroMotion);
    configureSlider(macroSpaceSlider, macroSpaceLabel, "Space", Parameters::ID::macroSpace);
    configureSlider(macroWeightSlider, macroWeightLabel, "Weight", Parameters::ID::macroWeight);
    configureSlider(macroBounceSlider, macroBounceLabel, "Bounce", Parameters::ID::macroBounce);
    configureSlider(macroWarpSlider, macroWarpLabel, "Warp", Parameters::ID::macroWarp);
    configureSlider(macroThrowSlider, macroThrowLabel, "Throw", Parameters::ID::macroThrow);
    configureSlider(lfo1RateSlider, lfo1RateLabel, "Rate", Parameters::ID::lfo1Rate);
    configureSlider(lfo1DepthSlider, lfo1DepthLabel, "Depth", Parameters::ID::lfo1Depth);
    configureSlider(lfo1PhaseSlider, lfo1PhaseLabel, "Phase", Parameters::ID::lfo1Phase);
    configureSlider(lfo2RateSlider, lfo2RateLabel, "L2 Rate", Parameters::ID::lfo2Rate);
    configureSlider(lfo2DepthSlider, lfo2DepthLabel, "L2 Depth", Parameters::ID::lfo2Depth);
    configureSlider(lfo2PhaseSlider, lfo2PhaseLabel, "L2 Phase", Parameters::ID::lfo2Phase);
    lfoCurveDisplay.onPointChange = [this] (size_t index, float value)
    {
        if (index < Parameters::ID::lfo1Curve.size())
        {
            lfoCurvePresetBox.setSelectedId(1, juce::dontSendNotification);
            setPlainParameterValue(Parameters::ID::lfo1Curve[index], value);
            updateSelectedControlInspector("MSEG P" + juce::String(static_cast<int>(index + 1)),
                                           Parameters::ID::lfo1Curve[index],
                                           value);
        }
    };
    addAndMakeVisible(lfoCurveDisplay);
    pumpCurveDisplay.onPointChange = [this] (size_t index, float value)
    {
        if (index < Parameters::ID::fxPumpCustomCurve.size())
        {
            setPlainParameterValue(Parameters::ID::fxPumpCurve, 5.0f);
            setPlainParameterValue(Parameters::ID::fxPumpCustomCurve[index], value);
        }
    };
    addAndMakeVisible(pumpCurveDisplay);

    for (size_t index = 0; index < lfoCurveSliders.size(); ++index)
        configureCompactHorizontalSlider(lfoCurveSliders[index], Parameters::ID::lfo1Curve[index]);

    configureSlider(modEnv1AttackSlider, modEnv1AttackLabel, "Attack", Parameters::ID::modEnv1Attack);
    configureSlider(modEnv1DecaySlider, modEnv1DecayLabel, "Decay", Parameters::ID::modEnv1Decay);
    configureSlider(modEnv1SustainSlider, modEnv1SustainLabel, "Sustain", Parameters::ID::modEnv1Sustain);
    configureSlider(modEnv1ReleaseSlider, modEnv1ReleaseLabel, "Release", Parameters::ID::modEnv1Release);
    configureSlider(modEnv1DepthSlider, modEnv1DepthLabel, "Depth", Parameters::ID::modEnv1Depth);

    for (size_t index = 0; index < modAmountSliders.size(); ++index)
    {
        configureModAmountSlider(modAmountSliders[index],
                                  modAmountLabels[index],
                                  "Amt " + juce::String(static_cast<int>(index + 1)),
                                  Parameters::ID::modMatrixAmount[index]);
        modAmountSliders[index].addMouseListener(this, true);
    }

    modMacroAssignAmountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    modMacroAssignAmountSlider.setRange(-100.0, 100.0, 1.0);
    modMacroAssignAmountSlider.setValue(30.0, juce::dontSendNotification);
    modMacroAssignAmountSlider.setDoubleClickReturnValue(true, 30.0);
    modMacroAssignAmountSlider.setMouseDragSensitivity(135);
    applyFineDragMode(modMacroAssignAmountSlider, 0.36);
    modMacroAssignAmountSlider.setSliderSnapsToMousePosition(false);
    modMacroAssignAmountSlider.setScrollWheelEnabled(false);
    modMacroAssignAmountSlider.setPopupDisplayEnabled(true, true, this);
    modMacroAssignAmountSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 46, 18);
    modMacroAssignAmountSlider.setTextValueSuffix("%");
    modMacroAssignAmountSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff8ee6c9));
    modMacroAssignAmountSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff263035));
    modMacroAssignAmountSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdce7e4));
    modMacroAssignAmountSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff101619));
    modMacroAssignAmountSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    modMacroAssignAmountSlider.setTooltip(controlFeelTooltip("Macro amount"));
    addAndMakeVisible(modMacroAssignAmountSlider);

    configureSlider(attackSlider, attackLabel, "Attack", Parameters::ID::ampAttack);
    configureSlider(decaySlider, decayLabel, "Decay", Parameters::ID::ampDecay);
    configureSlider(sustainSlider, sustainLabel, "Sustain", Parameters::ID::ampSustain);
    configureSlider(releaseSlider, releaseLabel, "Release", Parameters::ID::ampRelease);
    configureSlider(cutoffSlider, cutoffLabel, "Cutoff", Parameters::ID::filterCutoff);
    configureSlider(resonanceSlider, resonanceLabel, "Res", Parameters::ID::filterResonance);
    configureSlider(filterEnvSlider, filterEnvLabel, "F Env", Parameters::ID::filterEnvAmount);
    configureSlider(driveSlider, driveLabel, "Drive", Parameters::ID::driveAmount);
    configureSlider(outputSlider, outputLabel, "Output", Parameters::ID::outputGain);
    cutoffSlider.onDragStart = [this] { setModInspectorDestination(1); };
    resonanceSlider.onDragStart = [this] { setModInspectorDestination(2); };
    filterEnvSlider.onDragStart = [this] { setModInspectorDestination(3); };
    driveSlider.onDragStart = [this] { setModInspectorDestination(4); };
    osc2TuneSlider.onDragStart = [this] { setModInspectorDestination(5); };
    osc2LevelSlider.onDragStart = [this] { setModInspectorDestination(6); };
    oscWarpSlider.onDragStart = [this] { setModInspectorDestination(17); };
    sampleStartSlider.onDragStart = [this] { setModInspectorDestination(12); };
    sampleMixSlider.onDragStart = [this] { setModInspectorDestination(13); };
    sampleTransposeSlider.onDragStart = [this] { setModInspectorDestination(14); };
    samplePitchRampSlider.onDragStart = [this] { setModInspectorDestination(15); };
    sampleStutterRepeatsSlider.onDragStart = [this] { setModInspectorDestination(16); };
    configureSlider(randomAmountSlider, randomAmountLabel, "Amount", Parameters::ID::randomAmount);
    configureSlider(randomChaosSlider, randomChaosLabel, "Chaos", Parameters::ID::randomChaos);
    configureSlider(brightnessSlider, brightnessLabel, "Bright", Parameters::ID::randomBrightnessBias);
    configureSlider(driveBiasSlider, driveBiasLabel, "Drive Bias", Parameters::ID::randomDriveBias);
    configureSlider(motionBiasSlider, motionBiasLabel, "Motion", Parameters::ID::randomMotionBias);
    const std::array<const char*, 7> randomSectionIntensityIDs {
        Parameters::ID::randomSourceIntensity,
        Parameters::ID::randomEnvelopeIntensity,
        Parameters::ID::randomFilterIntensity,
        Parameters::ID::randomSampleIntensity,
        Parameters::ID::randomFxIntensity,
        Parameters::ID::randomSequencerIntensity,
        Parameters::ID::randomMacroIntensity
    };
    const std::array<juce::String, 7> randomSectionIntensityNames {
        "Source",
        "Env",
        "Filter",
        "Sample",
        "FX",
        "Seq",
        "Macros"
    };
    for (size_t index = 0; index < randomSectionIntensitySliders.size(); ++index)
        configureRandomSectionSlider(randomSectionIntensitySliders[index],
                                     randomSectionIntensityLabels[index],
                                     randomSectionIntensityNames[index],
                                     randomSectionIntensityIDs[index]);
    configureHorizontalSlider(sampleStartSlider, sampleStartLabel, "Start", Parameters::ID::sampleStart);
    configureHorizontalSlider(sampleEndSlider, sampleEndLabel, "End", Parameters::ID::sampleEnd);
    configureSlider(sampleTransposeSlider, sampleTransposeLabel, "Pitch", Parameters::ID::sampleTranspose);
    configureSlider(samplePitchRampSlider, samplePitchRampLabel, "Ramp", Parameters::ID::samplePitchRamp);
    configureSlider(sampleGainSlider, sampleGainLabel, "Gain", Parameters::ID::sampleGain);
    configureSlider(sampleMixSlider, sampleMixLabel, "Mix", Parameters::ID::sampleMix);
    configureSlider(sampleStutterRepeatsSlider, sampleStutterRepeatsLabel, "Repeat", Parameters::ID::sampleStutterRepeats);
    configureSlider(sampleGrainSizeSlider, sampleGrainSizeLabel, "Grain", Parameters::ID::sampleGrainSize);
    configureSlider(sampleGrainSpraySlider, sampleGrainSprayLabel, "Spray", Parameters::ID::sampleGrainSpray);
    configureSlider(sampleSpectralFreezeSlider, sampleSpectralFreezeLabel, "Freeze", Parameters::ID::sampleSpectralFreeze);
    configureSlider(sequencerRootSlider, sequencerRootLabel, "Root", Parameters::ID::sequencerRoot);
    configureSlider(sequencerGateSlider, sequencerGateLabel, "Gate", Parameters::ID::sequencerGate);
    configureSlider(sequencerSwingSlider, sequencerSwingLabel, "Swing", Parameters::ID::sequencerSwing);
    configureSlider(sequencerChordStrumSlider, sequencerChordStrumLabel, "Strum", Parameters::ID::sequencerChordStrum);
    configureSlider(sequencerAccentSlider, sequencerAccentLabel, "Accent", Parameters::ID::sequencerAccent);
    configureSlider(sequencerOctaveSlider, sequencerOctaveLabel, "Oct", Parameters::ID::sequencerOctave);
    configureSlider(sequencerProbabilitySlider, sequencerProbabilityLabel, "Prob", Parameters::ID::sequencerProbability);
    configureSlider(sequencerRandomSlider, sequencerRandomLabel, "Rand", Parameters::ID::sequencerRandomAmount);
    configureSlider(sequencerLockDepthSlider, sequencerLockDepthLabel, "Lock Amt", Parameters::ID::sequencerLockDepth);
    configureSlider(fxDistortionAmountSlider, fxDistortionAmountLabel, "Drive", Parameters::ID::fxDistortionAmount);
    configureSlider(fxDistortionBassSafeSlider, fxDistortionBassSafeLabel, "Bass Safe", Parameters::ID::fxDistortionBassSafe);
    configureSlider(fxBitcrushBitsSlider, fxBitcrushBitsLabel, "Bits", Parameters::ID::fxBitcrushBits);
    configureSlider(fxBitcrushDownsampleSlider, fxBitcrushDownsampleLabel, "Down", Parameters::ID::fxBitcrushDownsample);
    configureSlider(fxBitcrushMixSlider, fxBitcrushMixLabel, "Mix", Parameters::ID::fxBitcrushMix);
    configureSlider(fxPumpDepthSlider, fxPumpDepthLabel, "Depth", Parameters::ID::fxPumpDepth);
    configureSlider(fxPumpShapeSlider, fxPumpShapeLabel, "Shape", Parameters::ID::fxPumpShape);
    configureSlider(fxPumpPhaseSlider, fxPumpPhaseLabel, "Phase", Parameters::ID::fxPumpPhase);
    configureSlider(fxTremoloDepthSlider, fxTremoloDepthLabel, "Depth", Parameters::ID::fxTremoloDepth);
    configureSlider(fxTremoloPanSlider, fxTremoloPanLabel, "Pan", Parameters::ID::fxTremoloPan);
    configureSlider(fxTremoloShapeSlider, fxTremoloShapeLabel, "Shape", Parameters::ID::fxTremoloShape);
    configureSlider(fxTremoloPhaseSlider, fxTremoloPhaseLabel, "Phase", Parameters::ID::fxTremoloPhase);
    configureSlider(fxRingFrequencySlider, fxRingFrequencyLabel, "Freq", Parameters::ID::fxRingFrequency);
    configureSlider(fxRingDepthSlider, fxRingDepthLabel, "Depth", Parameters::ID::fxRingDepth);
    configureSlider(fxRingMixSlider, fxRingMixLabel, "Mix", Parameters::ID::fxRingMix);
    configureSlider(fxRingBiasSlider, fxRingBiasLabel, "Bias", Parameters::ID::fxRingBias);
    configureSlider(fxCombFrequencySlider, fxCombFrequencyLabel, "Freq", Parameters::ID::fxCombFrequency);
    configureSlider(fxCombFeedbackSlider, fxCombFeedbackLabel, "Fdbk", Parameters::ID::fxCombFeedback);
    configureSlider(fxCombDampingSlider, fxCombDampingLabel, "Damp", Parameters::ID::fxCombDamping);
    configureSlider(fxCombMixSlider, fxCombMixLabel, "Mix", Parameters::ID::fxCombMix);
    configureSlider(fxChorusRateSlider, fxChorusRateLabel, "Rate", Parameters::ID::fxChorusRate);
    configureSlider(fxChorusDepthSlider, fxChorusDepthLabel, "Depth", Parameters::ID::fxChorusDepth);
    configureSlider(fxChorusMixSlider, fxChorusMixLabel, "Mix", Parameters::ID::fxChorusMix);
    configureSlider(fxDelayTimeSlider, fxDelayTimeLabel, "Time", Parameters::ID::fxDelayTime);
    configureSlider(fxDelayFeedbackSlider, fxDelayFeedbackLabel, "Fdbk", Parameters::ID::fxDelayFeedback);
    configureSlider(fxDelayMixSlider, fxDelayMixLabel, "Mix", Parameters::ID::fxDelayMix);
    configureSlider(fxSendDelaySlider, fxSendDelayLabel, "Send", Parameters::ID::fxSendDelay);
    configureSlider(fxReverbSizeSlider, fxReverbSizeLabel, "Size", Parameters::ID::fxReverbSize);
    configureSlider(fxReverbDampingSlider, fxReverbDampingLabel, "Damp", Parameters::ID::fxReverbDamping);
    configureSlider(fxReverbMixSlider, fxReverbMixLabel, "Mix", Parameters::ID::fxReverbMix);
    configureSlider(fxSendReverbSlider, fxSendReverbLabel, "Send", Parameters::ID::fxSendReverb);
    configureSlider(fxWidthAmountSlider, fxWidthAmountLabel, "Width", Parameters::ID::fxWidthAmount);
    configureSlider(fxWidthMonoCutoffSlider, fxWidthMonoCutoffLabel, "Mono Hz", Parameters::ID::fxWidthMonoCutoff);
    configureSlider(fxToneTiltSlider, fxToneTiltLabel, "Tilt", Parameters::ID::fxToneTilt);
    configureSlider(fxToneLowCutSlider, fxToneLowCutLabel, "Low Cut", Parameters::ID::fxToneLowCut);
    configureSlider(fxEqLowGainSlider, fxEqLowGainLabel, "Low", Parameters::ID::fxEqLowGain);
    configureSlider(fxEqMidGainSlider, fxEqMidGainLabel, "Mid", Parameters::ID::fxEqMidGain);
    configureSlider(fxEqHighGainSlider, fxEqHighGainLabel, "High", Parameters::ID::fxEqHighGain);
    configureSlider(fxEqTrimSlider, fxEqTrimLabel, "Trim", Parameters::ID::fxEqTrim);
    configureSlider(fxPhaserRateSlider, fxPhaserRateLabel, "Rate", Parameters::ID::fxPhaserRate);
    configureSlider(fxPhaserDepthSlider, fxPhaserDepthLabel, "Depth", Parameters::ID::fxPhaserDepth);
    configureSlider(fxPhaserMixSlider, fxPhaserMixLabel, "Mix", Parameters::ID::fxPhaserMix);
    configureSlider(fxFlangerRateSlider, fxFlangerRateLabel, "Rate", Parameters::ID::fxFlangerRate);
    configureSlider(fxFlangerDepthSlider, fxFlangerDepthLabel, "Depth", Parameters::ID::fxFlangerDepth);
    configureSlider(fxFlangerFeedbackSlider, fxFlangerFeedbackLabel, "Fdbk", Parameters::ID::fxFlangerFeedback);
    configureSlider(fxFlangerMixSlider, fxFlangerMixLabel, "Mix", Parameters::ID::fxFlangerMix);
    configureSlider(fxGuardPushSlider, fxGuardPushLabel, "Push", Parameters::ID::fxGuardPush);
    configureSlider(fxGuardGlueSlider, fxGuardGlueLabel, "Glue", Parameters::ID::fxGuardGlue);
    configureSlider(fxGuardPunchSlider, fxGuardPunchLabel, "Punch", Parameters::ID::fxGuardPunch);
    configureSlider(fxGuardClipMixSlider, fxGuardClipMixLabel, "Clip", Parameters::ID::fxGuardClipMix);
    configureSlider(fxGuardCeilingSlider, fxGuardCeilingLabel, "Ceil", Parameters::ID::fxGuardCeiling);

    generateButton.onClick = [this] { triggerRandomGenerate(); };
    mutateButton.onClick = [this] { triggerRandomMutate(); };
    variationButton.onClick = [this] { triggerRandomVariation(); };
    wildMutateButton.setTooltip("Make a stronger recipe-aware mutation while respecting active locks");
    wildMutateButton.onClick = [this] { triggerRandomWild(); };
    undoRandomButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        setRandomStatus(audioProcessor.undoRandomization() ? "Undo restored" : "Nothing to undo");
        updateSampleNameLabel();
        updateSampleWaveformDisplay();
        updateWavetableDisplay();
        updateRandomCandidateButtons();
        repaintSequencerGrids();
    };
    redoRandomButton.setTooltip("Redo the last undone randomization action");
    redoRandomButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        setRandomStatus(audioProcessor.redoRandomization() ? "Redo restored" : "Nothing to redo");
        updateSampleNameLabel();
        updateSampleWaveformDisplay();
        updateWavetableDisplay();
        updateRandomCandidateButtons();
        repaintSequencerGrids();
    };
    recallSnapshotAButton.setTooltip("Recall performance snapshot A");
    recallSnapshotAButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        if (audioProcessor.hasPerformanceSnapshot(0))
            captureGlobalEdit("Recall snapshot A");
        if (audioProcessor.recallPerformanceSnapshot(0))
        {
            updateSampleNameLabel();
            updateSampleWaveformDisplay();
            repaintSequencerGrids();
            updatePerformanceSnapshotButtons();
        }
    };
    captureSnapshotAButton.setTooltip("Store the current patch in performance snapshot A");
    captureSnapshotAButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        captureGlobalEdit("Capture snapshot A");
        audioProcessor.capturePerformanceSnapshot(0);
        updatePerformanceSnapshotButtons();
    };
    recallSnapshotBButton.setTooltip("Recall performance snapshot B");
    recallSnapshotBButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        if (audioProcessor.hasPerformanceSnapshot(1))
            captureGlobalEdit("Recall snapshot B");
        if (audioProcessor.recallPerformanceSnapshot(1))
        {
            updateSampleNameLabel();
            updateSampleWaveformDisplay();
            repaintSequencerGrids();
            updatePerformanceSnapshotButtons();
        }
    };
    captureSnapshotBButton.setTooltip("Store the current patch in performance snapshot B");
    captureSnapshotBButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        captureGlobalEdit("Capture snapshot B");
        audioProcessor.capturePerformanceSnapshot(1);
        updatePerformanceSnapshotButtons();
    };
    recallSnapshotCButton.setTooltip("Recall performance snapshot C");
    recallSnapshotCButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        if (audioProcessor.hasPerformanceSnapshot(2))
            captureGlobalEdit("Recall snapshot C");
        if (audioProcessor.recallPerformanceSnapshot(2))
        {
            updateSampleNameLabel();
            updateSampleWaveformDisplay();
            repaintSequencerGrids();
            updatePerformanceSnapshotButtons();
        }
    };
    captureSnapshotCButton.setTooltip("Store the current patch in performance snapshot C");
    captureSnapshotCButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        captureGlobalEdit("Capture snapshot C");
        audioProcessor.capturePerformanceSnapshot(2);
        updatePerformanceSnapshotButtons();
    };
    recallSnapshotDButton.setTooltip("Recall performance snapshot D");
    recallSnapshotDButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        if (audioProcessor.hasPerformanceSnapshot(3))
            captureGlobalEdit("Recall snapshot D");
        if (audioProcessor.recallPerformanceSnapshot(3))
        {
            updateSampleNameLabel();
            updateSampleWaveformDisplay();
            repaintSequencerGrids();
            updatePerformanceSnapshotButtons();
        }
    };
    captureSnapshotDButton.setTooltip("Store the current patch in performance snapshot D");
    captureSnapshotDButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        captureGlobalEdit("Capture snapshot D");
        audioProcessor.capturePerformanceSnapshot(3);
        updatePerformanceSnapshotButtons();
    };
    loadSampleButton.onClick = [this] { chooseSampleFile(); };
    clearSampleButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        captureGlobalEdit("Clear sample");
        audioProcessor.clearSample();
        sampleWaveformKey = "cleared";
        updateSampleNameLabel();
        updateSampleWaveformDisplay();
        updateSampleRecorderStatus();
    };
    sampleRecorderPanel.onRecordClicked = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        if (audioProcessor.isSampleCaptureEnabled())
        {
            audioProcessor.stopSampleCapture();
            setRandomStatus("Recorder stopped");
        }
        else
        {
            audioProcessor.beginSampleCapture();
            const auto lengthStatus = audioProcessor.getSampleCaptureLengthModeIndex() > 0
                ? juce::String(" | ") + audioProcessor.getSampleCaptureLengthModeName()
                : juce::String();
            const auto preRollStatus = audioProcessor.getSampleCapturePreRollModeIndex() > 0
                ? juce::String(" | ") + audioProcessor.getSampleCapturePreRollModeName()
                : juce::String();
            setRandomStatus(audioProcessor.isSampleCaptureWaitingForThreshold()
                                ? "Recorder armed " + audioProcessor.getSampleCaptureStartModeName() + lengthStatus + preRollStatus
                                : "Recording " + audioProcessor.getSampleCaptureSourceName()
                                      + lengthStatus + preRollStatus);
        }
        updateSampleRecorderStatus();
        returnKeyboardFocusToPiano();
    };
    sampleRecorderPanel.onCommitClicked = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        captureGlobalEdit("Commit recorded snippet");
        if (audioProcessor.commitSampleCaptureToSampler())
        {
            sampleWaveformKey.clear();
            updateSampleNameLabel();
            updateSampleWaveformDisplay();
            updateSampleSliceButtons();
            setRandomStatus("Recorded snippet committed");
        }
        else
        {
            setRandomStatus("Record first, then commit");
        }
        updateSampleRecorderStatus();
        returnKeyboardFocusToPiano();
    };
    sampleRecorderPanel.onAuditionClicked = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        setRandomStatus(audioProcessor.triggerSampleAudition() ? "Sample audition" : "No sample to play");
        returnKeyboardFocusToPiano();
    };
    sampleRecorderPanel.onAutoTrimClicked = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        captureGlobalEdit("Auto trim sample");
        setRandomStatus(audioProcessor.autoTrimSampleToContent() ? "Sample auto-trimmed" : "Trim skipped");
        updateSampleWaveformDisplay();
        returnKeyboardFocusToPiano();
    };
    sampleRecorderPanel.onSpliceClicked = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        captureGlobalEdit("Splice recorded sample");
        setRandomStatus(audioProcessor.spliceSampleToSlices() ? "Sample spliced to slices" : "Splice skipped");
        updateSampleSliceButtons();
        updateSampleSliceEditorStatus();
        updateSampleWaveformDisplay();
        returnKeyboardFocusToPiano();
    };
    sampleRecorderPanel.onMangleClicked = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        captureGlobalEdit("Mangle recorded sample");
        if (audioProcessor.randomizeRecordedSample())
        {
            updateSampleNameLabel();
            updateSampleSliceButtons();
            updateSampleSliceEditorStatus();
            updateSampleWaveformDisplay();
            updateFxRackControls();
            setRandomStatus("Recorded sample mangled");
        }
        else
        {
            setRandomStatus("Mangle skipped");
        }
        returnKeyboardFocusToPiano();
    };
    randomCutButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        captureGlobalEdit("Random sample cut");
        setRandomStatus(audioProcessor.randomizeSampleCut() ? "Sample randomized" : "Sample skipped");
        updateSampleWaveformDisplay();
        returnKeyboardFocusToPiano();
    };
    ukgChopButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        captureGlobalEdit("UKG chop");
        if (audioProcessor.randomizeUkgVocalChop())
        {
            repaintSequencerGrids();
            updateSampleWaveformDisplay();
            setRandomStatus("UKG chop ready");
        }
        else
        {
            setRandomStatus("UKG chop skipped");
        }
        returnKeyboardFocusToPiano();
    };
    sampleChopPanel.onSliceSelected = [this] (size_t index) { selectSampleSlice(index); };
    sampleChopPanel.onStoreClicked = [this] { storeSelectedSampleSliceSettings(); };
    sampleChopPanel.onRecallClicked = [this] { recallSelectedSampleSliceSettings(); };
    sampleChopPanel.onDetectClicked = [this] { detectSampleSliceMarkers(); };
    sampleChopPanel.onDiceClicked = [this] { randomizeSelectedSampleSliceSettings(); };
    sampleChopPanel.onReverseClicked = [this] { toggleSelectedSampleSliceReverse(); };
    sampleChopPanel.onChokeClicked = [this] { toggleSelectedSampleSliceChoke(); };
    sampleChopPanel.onPanClicked = [this] { cycleSelectedSampleSlicePan(); };
    sampleChopPanel.onGhostClicked = [this] { toggleSelectedSampleSliceGhost(); };
    sampleChopPanel.onNudgeClicked = [this] { cycleSelectedSampleSliceNudge(); };
    sampleChopPanel.onFadeClicked = [this] { cycleSelectedSampleSliceFade(); };
    randomSequencerButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        if (audioProcessor.randomizeSequencerPattern())
        {
            repaintSequencerGrids();
            setRandomStatus("Sequence generated");
        }
        else
        {
            setRandomStatus("Sequence skipped");
        }
    };
    mutateSequencerButton.setTooltip("Create a small variation of the current sequencer pattern");
    mutateSequencerButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        if (audioProcessor.mutateSequencerPattern())
        {
            repaintSequencerGrids();
            setRandomStatus("Sequence varied");
        }
        else
        {
            setRandomStatus("Sequence skipped");
        }
    };
    undoSequencerButton.setTooltip("Undo the last sequencer utility edit");
    undoSequencerButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        if (audioProcessor.undoSequencerEdit())
        {
            repaintSequencerGrids();
            updateSegmentedSelectors();
            setRandomStatus("Sequence undo");
        }
        else
        {
            setRandomStatus("No sequence undo");
        }
    };
    clearSequencerButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        audioProcessor.clearSequencerPattern();
        repaintSequencerGrids();
    };
    bassPatternButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        audioProcessor.applySequencerPatternPreset(0);
        repaintSequencerGrids();
    };
    stabPatternButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        audioProcessor.applySequencerPatternPreset(1);
        repaintSequencerGrids();
    };
    ukgPatternButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        audioProcessor.applySequencerPatternPreset(2);
        repaintSequencerGrids();
    };
    applyPatternButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        const auto selectedId = sequencerPatternBox.getSelectedId();
        audioProcessor.applySequencerPatternPreset(juce::jmax(1, selectedId) - 1);
        repaintSequencerGrids();
    };
    copySequencerButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        audioProcessor.copySequencerFirstHalfToSecondHalf();
        repaintSequencerGrids();
    };
    rotateSequencerLeftButton.setTooltip("Shift the whole sequencer pattern one step earlier");
    rotateSequencerLeftButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        audioProcessor.rotateSequencerPattern(-1);
        repaintSequencerGrids();
        setRandomStatus("Sequence rotated left");
    };
    rotateSequencerRightButton.setTooltip("Shift the whole sequencer pattern one step later");
    rotateSequencerRightButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        audioProcessor.rotateSequencerPattern(1);
        repaintSequencerGrids();
        setRandomStatus("Sequence rotated right");
    };
    exportSequencerMidiButton.setTooltip("Click to save the current sequencer pattern as MIDI, or drag into Ableton");
    exportSequencerMidiButton.onClick = [this] { exportSequencerMidiClip(); };
    exportSequencerMidiButton.onExternalDrag = [this] (juce::Component& sourceComponent)
    {
        return beginSequencerMidiDrag(sourceComponent, false);
    };
    exportSequencerChainButton.setTooltip("Click to save captured A/B/Fill/Drop scenes as one chained MIDI clip, or drag into Ableton");
    exportSequencerChainButton.onClick = [this] { exportSequencerSceneChainMidiClip(); };
    exportSequencerChainButton.onExternalDrag = [this] (juce::Component& sourceComponent)
    {
        return beginSequencerMidiDrag(sourceComponent, true);
    };
    sequencerSceneChainLiveButton.setTooltip("Play captured A/B/Fill/Drop scene steps as a live 2/4-bar chain");
    sequencerSceneChainLiveButton.setClickingTogglesState(true);
    sequencerSceneChainLiveButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        const auto shouldEnable = sequencerSceneChainLiveButton.getToggleState();
        audioProcessor.setSequencerSceneChainPlaybackEnabled(shouldEnable);
        updateSequencerSceneButtons();
        const auto chainLength = audioProcessor.getSequencerSceneChainPlaybackLength();
        setRandomStatus(shouldEnable
                            ? ("Live chain " + juce::String(juce::jmax(1, chainLength)) + " bar")
                            : "Live chain off");
    };
    addAndMakeVisible(sequencerSceneChainLiveButton);
    sequencerSceneChainLengthButton.setTooltip("Set scene-chain MIDI/live length: Auto, forced 2 bars, or forced 4 bars");
    sequencerSceneChainLengthButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        const auto currentBars = audioProcessor.getSequencerSceneChainClipBars();
        const auto nextBars = currentBars == 0 ? 2 : (currentBars == 2 ? 4 : 0);
        audioProcessor.setSequencerSceneChainClipBars(nextBars);
        updateSequencerSceneButtons();
        const auto modeText = nextBars == 0 ? juce::String("Auto") : juce::String(nextBars) + " Bar";
        setRandomStatus("Chain length " + modeText);
    };
    addAndMakeVisible(sequencerSceneChainLengthButton);
    applyGrooveTransformButton.setTooltip("Apply the selected groove transform to the current sequence");
    applyGrooveTransformButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        const auto selectedId = sequencerGrooveTransformBox.getSelectedId();
        const auto transformIndex = juce::jmax(1, selectedId) - 1;
        if (audioProcessor.applySequencerGrooveTransform(transformIndex))
        {
            repaintSequencerGrids();
            updateSegmentedSelectors();
            updateSequencerSceneButtons();
            setRandomStatus(transformIndex == sequencerFourBarChainTransformIndex ? "4-bar chain ready" : sequencerGrooveTransformBox.getText() + " shaped");
        }
        else
        {
            setRandomStatus("Groove skipped");
        }
    };
    const std::array<juce::String, 4> sceneLabels { "A", "B", "Fill", "Drop" };
    for (size_t index = 0; index < sequencerSceneRecallButtons.size(); ++index)
    {
        auto& recallButton = sequencerSceneRecallButtons[index];
        auto& captureButton = sequencerSceneCaptureButtons[index];
        const auto label = sceneLabels[index];
        recallButton.setButtonText(label);
        recallButton.setTooltip("Recall pattern scene " + label);
        recallButton.onClick = [this, index, label]
        {
            releaseRandomCandidateAudition(false);
            releasePresetAuditionNote();
            if (audioProcessor.hasSequencerPatternScene(static_cast<int>(index)))
                captureGlobalEdit("Recall pattern scene " + label);
            if (audioProcessor.recallSequencerPatternScene(static_cast<int>(index)))
            {
                repaintSequencerGrids();
                updateSegmentedSelectors();
                updateSequencerSceneButtons();
                setRandomStatus("Scene " + label + " recalled");
            }
            else
            {
                setRandomStatus("Scene " + label + " empty");
            }
        };
        addAndMakeVisible(recallButton);

        captureButton.setButtonText("Set " + label);
        captureButton.setTooltip("Capture the current sequencer pattern into scene " + label);
        captureButton.onClick = [this, index, label]
        {
            releaseRandomCandidateAudition(false);
            releasePresetAuditionNote();
            captureGlobalEdit("Capture pattern scene " + label);
            audioProcessor.captureSequencerPatternScene(static_cast<int>(index));
            updateSequencerSceneButtons();
            setRandomStatus("Scene " + label + " captured");
        };
        addAndMakeVisible(captureButton);
    }
    homeTabButton.onClick = [this] { setActivePanel(Panel::home); };
    synthTabButton.onClick = [this] { setActivePanel(Panel::synth); };
    labTabButton.onClick = [this] { setActivePanel(Panel::lab); };
    modTabButton.onClick = [this] { setActivePanel(Panel::mod); };
    sampleTabButton.onClick = [this] { setActivePanel(Panel::sample); };
    sequencerTabButton.onClick = [this] { setActivePanel(Panel::sequencer); };
    effectsTabButton.onClick = [this] { setActivePanel(Panel::effects); };
    libraryTabButton.onClick = [this] { setActivePanel(Panel::library); };
    infoTabButton.onClick = [this] { setActivePanel(Panel::info); };
    sineWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::oscWave, 0); };
    sawWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::oscWave, 1); };
    squareWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::oscWave, 2); };
    triangleWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::oscWave, 3); };
    wavetableWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::oscWave, 4); };
    organWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::oscWave, 5); };
    housePianoWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::oscWave, 6); };
    customWaveButton.onClick = [this]
    {
        setChoiceParameter(Parameters::ID::oscWave, 7);
        setRandomStatus("Wave editor: Osc 1 custom");
        updateWavetableDisplay();
    };
    osc2SineWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::osc2Wave, 0); };
    osc2SawWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::osc2Wave, 1); };
    osc2SquareWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::osc2Wave, 2); };
    osc2TriangleWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::osc2Wave, 3); };
    osc2WavetableWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::osc2Wave, 4); };
    osc2OrganWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::osc2Wave, 5); };
    osc2HousePianoWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::osc2Wave, 6); };
    osc2CustomWaveButton.onClick = [this]
    {
        setChoiceParameter(Parameters::ID::osc2Wave, 7);
        setRandomStatus("Wave editor: Osc 2 custom");
        updateWavetableDisplay();
    };
    lowpassFilterButton.onClick = [this] { setChoiceParameter(Parameters::ID::filterMode, 0); };
    bandpassFilterButton.onClick = [this] { setChoiceParameter(Parameters::ID::filterMode, 1); };
    highpassFilterButton.onClick = [this] { setChoiceParameter(Parameters::ID::filterMode, 2); };
    rateEighthButton.onClick = [this] { setChoiceParameter(Parameters::ID::sequencerRate, 0); };
    rateSixteenthButton.onClick = [this] { setChoiceParameter(Parameters::ID::sequencerRate, 1); };
    rateThirtySecondButton.onClick = [this] { setChoiceParameter(Parameters::ID::sequencerRate, 2); };
    previousPresetButton.onClick = [this] { loadPresetByOffset(-1); };
    nextPresetButton.onClick = [this] { loadPresetByOffset(1); };
    savePresetButton.onClick = [this] { saveCurrentPreset(); };
    candidateFavoriteButton.setClickingTogglesState(true);
    candidateFavoriteButton.setTooltip("Favorite the preset created by Save Slot");
    saveCandidateButton.setTooltip("Save the active random candidate into the Library without recalling it");
    saveCandidateButton.onClick = [this] { saveActiveRandomCandidatePreset(); };
    loadPresetButton.onClick = [this] { loadSelectedPreset(); };
    auditionPresetButton.onClick = [this] { auditionSelectedPreset(); };
    warmPresetPreviewsButton.setTooltip("Render previews for the selected and visible preset rows without loading patches");
    warmPresetPreviewsButton.onClick = [this] { warmVisiblePresetPreviews(); };
    refreshPresetsButton.onClick = [this]
    {
        audioProcessor.invalidatePresetLibraryCache();
        refreshPresetList();
    };
    favoritePresetButton.onClick = [this] { toggleFavoritePreset(); };
    comparePresetButton.setTooltip("Load a preset to compare it against the sound that was active before loading");
    comparePresetButton.onClick = [this] { togglePresetCompare(); };
    revertPresetButton.setTooltip("Restore the sound from before the most recent Library preset load");
    revertPresetButton.onClick = [this] { revertPresetCompare(); };
    updatePresetCompareButtons();
    const std::array<juce::String, 10> quickFilterLabels { "All", "Fav", "Recent", "Similar", "Bass", "Lead", "Chord", "Pad", "FX", "Seq" };
    for (size_t index = 0; index < presetQuickFilterButtons.size(); ++index)
    {
        auto& button = presetQuickFilterButtons[index];
        button.setButtonText(quickFilterLabels[index]);
        button.setTooltip("Quick Library filter: " + quickFilterLabels[index]);
        button.setClickingTogglesState(false);
        button.setWantsKeyboardFocus(false);
        button.setMouseClickGrabsKeyboardFocus(false);
        button.onClick = [this, index] { applyPresetQuickFilter(index); };
        addAndMakeVisible(button);
    }
    presetFilterBox.onChange = [this] { refreshPresetList(); };
    presetTagBox.onChange = [this] { refreshPresetList(); };
    presetSortBox.onChange = [this] { refreshPresetList(); };
    presetRatingBox.onChange = [this] { setSelectedPresetRating(); };
    presetSearchEditor.onTextChange = [this] { refreshPresetList(); };
    presetBox.onChange = [this]
    {
        updateFavoritePresetButton();

        const auto selectedName = presetBox.getText();
        for (auto index = 0; index < getNumRows(); ++index)
        {
            if (visiblePresetBrowserPresets[static_cast<size_t>(index)].name != selectedName)
                continue;

            ignorePresetBrowserSelection = true;
            presetBrowserList.selectRow(index);
            ignorePresetBrowserSelection = false;
            break;
        }
    };
    fxAddBox.onChange = [this]
    {
        const auto selectedId = fxAddBox.getSelectedId();
        if (selectedId > 0)
            addFxModule(static_cast<FxModule>(selectedId - 1));
    };
    fxPresetBox.onChange = [this] { applySelectedFxPreset(); };
    modInspectorDestinationBox.onChange = [this] { updateModInspectorStatus(); };
    modInspectorSourceBox.onChange = [this] { updateSelectedControlActionState(); };
    modMacroAssignSourceBox.onChange = [this] { updateMacroAssignmentEditorStatus(); };
    modMacroAssignDestinationBox.onChange = [this] { updateMacroAssignmentEditorStatus(); };
    modMacroAssignAmountSlider.onValueChange = [this] { updateMacroAssignmentEditorStatus(); };
    wavetableDrawModeBox.onChange = [this]
    {
        switch (wavetableDrawModeBox.getSelectedId())
        {
            case 2: wavetableDisplay.setCustomDrawMode(UI::WavetableDisplay::CustomDrawMode::line); break;
            case 3: wavetableDisplay.setCustomDrawMode(UI::WavetableDisplay::CustomDrawMode::smooth); break;
            case 4: wavetableDisplay.setCustomDrawMode(UI::WavetableDisplay::CustomDrawMode::step); break;
            case 5: wavetableDisplay.setCustomDrawMode(UI::WavetableDisplay::CustomDrawMode::erase); break;
            case 1:
            default: wavetableDisplay.setCustomDrawMode(UI::WavetableDisplay::CustomDrawMode::point); break;
        }
        returnKeyboardFocusToPiano();
    };
    wavetableToolBox.onChange = [this] { applySelectedWavetableTool(); };
    randomSectionActionBox.onChange = [this] { applySelectedRandomSectionAction(); };
    randomLockActionBox.onChange = [this] { applySelectedRandomLockAction(); };
    presetBrowserPackFilterBox.onChange = [this] { refreshPresetList(); };
    lfoCurvePresetBox.onChange = [this]
    {
        const auto selectedId = lfoCurvePresetBox.getSelectedId();
        if (selectedId > 1)
            applyLfoCurvePreset(selectedId);
    };
    lfoCurveActionBox.onChange = [this] { applySelectedLfoCurveAction(); };
    fxRemoveButton.onClick = [this] { removeSelectedFxModule(); };
    fxMoveUpButton.onClick = [this] { moveSelectedFxModule(-1); };
    fxMoveDownButton.onClick = [this] { moveSelectedFxModule(1); };
    fxResetOrderButton.onClick = [this] { resetFxModuleOrder(); };
    fxThrowDelayButton.onClick = [this] { applyDelayThrow(); };
    fxThrowSpaceButton.onClick = [this] { applySpaceThrow(); };
    fxThrowPumpButton.onClick = [this] { applyPumpDrop(); };
    fxThrowDryButton.onClick = [this] { clearFxThrows(); };
    fxHoldDelayButton.setTooltip("Hold for a temporary delay throw, release to restore");
    fxHoldDelayButton.onStateChange = [this]
    {
        if (fxHoldDelayButton.isDown())
            beginMomentaryFxAction(MomentaryFxAction::delay);
        else
            endMomentaryFxAction(MomentaryFxAction::delay);
    };
    fxHoldSpaceButton.setTooltip("Hold for a temporary delay and reverb wash, release to restore");
    fxHoldSpaceButton.onStateChange = [this]
    {
        if (fxHoldSpaceButton.isDown())
            beginMomentaryFxAction(MomentaryFxAction::space);
        else
            endMomentaryFxAction(MomentaryFxAction::space);
    };
    fxHoldPumpButton.setTooltip("Hold for a temporary pump/duck move, release to restore");
    fxHoldPumpButton.onStateChange = [this]
    {
        if (fxHoldPumpButton.isDown())
            beginMomentaryFxAction(MomentaryFxAction::pump);
        else
            endMomentaryFxAction(MomentaryFxAction::pump);
    };
    fxMuteDropButton.setTooltip("Hold for a temporary mute drop, release to restore");
    fxMuteDropButton.onStateChange = [this]
    {
        if (fxMuteDropButton.isDown())
            beginMomentaryFxAction(MomentaryFxAction::mute);
        else
            endMomentaryFxAction(MomentaryFxAction::mute);
    };
    fxApplyPresetButton.setTooltip("Reload the selected FX module preset");
    fxApplyPresetButton.onClick = [this] { applySelectedFxPreset(); };
    modInspectorAddButton.setTooltip("Add the selected source to the inspected destination");
    modInspectorAddButton.onClick = [this] { addInspectedModRoute(); };
    modInspectorClearButton.setTooltip("Delete all active routes targeting the inspected destination");
    modInspectorClearButton.onClick = [this] { clearInspectedModRoutes(); };
    infoOpenLabButton.setTooltip("Open the Random Lab generation and mutation panel");
    infoOpenLabButton.onClick = [this] { setActivePanel(Panel::lab); };
    infoOpenModButton.setTooltip("Open modulation sources, MSEG, and routing");
    infoOpenModButton.onClick = [this] { setActivePanel(Panel::mod); };
    infoOpenFxButton.setTooltip("Open the slot-based FX rack");
    infoOpenFxButton.onClick = [this] { setActivePanel(Panel::effects); };
    infoOpenLibraryButton.setTooltip("Open preset save, search, categories, ratings, and browser tools");
    infoOpenLibraryButton.onClick = [this] { setActivePanel(Panel::library); };
    undoEditButton.setTooltip("Undo the last captured sound-design edit");
    undoEditButton.onClick = [this] { triggerGlobalUndo(); };
    undoEditButton.setEnabled(false);
    redoEditButton.setTooltip("Redo the last undone sound-design edit");
    redoEditButton.onClick = [this] { triggerGlobalRedo(); };
    redoEditButton.setEnabled(false);
    selectedControlAddModButton.setTooltip("Touch a modulatable control, then add the selected MOD source to it");
    selectedControlAddModButton.onClick = [this] { addModRouteForSelectedControl(); };
    selectedControlAddModButton.setEnabled(false);
    selectedControlOpenModButton.setTooltip("Open the MOD panel focused on the selected control");
    selectedControlOpenModButton.onClick = [this] { focusSelectedControlModDestination(); };
    selectedControlOpenModButton.setEnabled(false);
    modMacroAssignAddButton.setTooltip("Add or update this macro assignment");
    modMacroAssignAddButton.onClick = [this] { addMacroAssignment(false); };
    modMacroAssignReplaceButton.setTooltip("Replace all assignments for the selected macro with this one destination");
    modMacroAssignReplaceButton.onClick = [this] { addMacroAssignment(true); };
    modMacroAssignClearButton.setTooltip("Delete all routes owned by the selected macro");
    modMacroAssignClearButton.onClick = [this] { clearSelectedMacroAssignments(); };
    fxToneSlotButton.onClick = [this] { selectFxModule(FxModule::tone); };
    fxEqSlotButton.onClick = [this] { selectFxModule(FxModule::eq); };
    fxDistortionSlotButton.onClick = [this] { selectFxModule(FxModule::distortion); };
    fxBitcrushSlotButton.onClick = [this] { selectFxModule(FxModule::bitcrush); };
    fxPumpSlotButton.onClick = [this] { selectFxModule(FxModule::pump); };
    fxTremoloSlotButton.onClick = [this] { selectFxModule(FxModule::tremolo); };
    fxRingSlotButton.onClick = [this] { selectFxModule(FxModule::ring); };
    fxCombSlotButton.onClick = [this] { selectFxModule(FxModule::comb); };
    fxPhaserSlotButton.onClick = [this] { selectFxModule(FxModule::phaser); };
    fxFlangerSlotButton.onClick = [this] { selectFxModule(FxModule::flanger); };
    fxChorusSlotButton.onClick = [this] { selectFxModule(FxModule::chorus); };
    fxDelaySlotButton.onClick = [this] { selectFxModule(FxModule::delay); };
    fxReverbSlotButton.onClick = [this] { selectFxModule(FxModule::reverb); };
    fxWidthSlotButton.onClick = [this] { selectFxModule(FxModule::width); };
    fxGuardSlotButton.onClick = [this] { selectFxModule(FxModule::guard); };

    auto updateLockStatus = [this] { setRandomStatus("Locks updated"); };
    randomLockPitchButton.onClick = updateLockStatus;
    randomLockEnvelopeButton.onClick = updateLockStatus;
    randomLockFilterButton.onClick = updateLockStatus;
    randomLockSourceButton.onClick = updateLockStatus;
    randomLockSampleButton.onClick = updateLockStatus;
    randomLockFxButton.onClick = updateLockStatus;
    randomLockOutputButton.onClick = updateLockStatus;
    randomLockSequencerButton.onClick = updateLockStatus;

    const std::array<juce::String, 5> randomLabPageLabels {
        "Generate",
        "Mutate",
        "Recipe",
        "History",
        "Save"
    };

    const std::array<juce::String, 5> randomLabPageTooltips {
        "Generate fresh house and UKG-ready ideas",
        "Vary or push the current generated sound",
        "Tune recipe, scope, locks, and generation bias",
        "Cue, recall, and promote generated candidate slots",
        "Name, categorize, star, rate, and save generated patches"
    };

    for (size_t index = 0; index < randomLabPageButtons.size(); ++index)
    {
        auto& button = randomLabPageButtons[index];
        button.setButtonText(randomLabPageLabels[index]);
        button.setTooltip(randomLabPageTooltips[index]);
        button.setWantsKeyboardFocus(false);
        button.setMouseClickGrabsKeyboardFocus(false);
        button.onClick = [this, index]
        {
            setActiveRandomLabPage(static_cast<RandomLabPage>(index));
        };
        addAndMakeVisible(button);
    }

    const std::array<ModWorkflowPage, 4> modWorkflowPages {
        ModWorkflowPage::matrix,
        ModWorkflowPage::sources,
        ModWorkflowPage::macros,
        ModWorkflowPage::curves
    };
    const std::array<juce::String, 4> modWorkflowLabels {
        "Matrix",
        "Sources",
        "Macros",
        "Curves"
    };

    const std::array<juce::String, 4> modWorkflowTooltips {
        "Edit active source-to-destination routes",
        "Inspect live modulation sources and envelope/LFO activity",
        "Assign performance macros to destinations",
        "Edit LFO and MSEG curve shapes"
    };

    for (size_t index = 0; index < modWorkflowButtons.size(); ++index)
    {
        auto& button = modWorkflowButtons[index];
        button.setButtonText(modWorkflowLabels[index]);
        button.setTooltip(modWorkflowTooltips[index]);
        button.setWantsKeyboardFocus(false);
        button.setMouseClickGrabsKeyboardFocus(false);
        button.onClick = [this, page = modWorkflowPages[index]]
        {
            setActiveModWorkflowPage(page);
        };
        addAndMakeVisible(button);
    }

    const std::array<juce::String, 7> sectionRollLabels {
        "Source",
        "Env",
        "Filter",
        "Sample",
        "FX",
        "Seq",
        "Macros"
    };

    for (size_t index = 0; index < randomSectionRollButtons.size(); ++index)
    {
        auto& button = randomSectionRollButtons[index];
        button.setButtonText(sectionRollLabels[index]);
        button.setTooltip("Mutate only the " + sectionRollLabels[index] + " section while preserving the rest of the patch");
        button.setWantsKeyboardFocus(false);
        button.setMouseClickGrabsKeyboardFocus(false);
        button.onClick = [this, index] { triggerRandomSectionRoll(index); };
        addAndMakeVisible(button);
    }

    for (size_t index = 0; index < randomCandidateButtons.size(); ++index)
    {
        auto& button = randomCandidateButtons[index];
        button.setButtonText("Slot " + juce::String(static_cast<int>(index + 1)));
        button.setTooltip("Recall this generated random candidate");
        button.onClick = [this, index] { recallRandomCandidate(index); };
        addAndMakeVisible(button);

        auto& auditionButton = randomCandidateAuditionButtons[index];
        auditionButton.setButtonText("Cue");
        auditionButton.setTooltip("Preview this generated random candidate, then restore the current patch");
        auditionButton.onClick = [this, index] { auditionRandomCandidate(index); };
        addAndMakeVisible(auditionButton);
    }

    promoteCandidateAButton.setTooltip("Promote the active random candidate to performance snapshot A");
    promoteCandidateAButton.onClick = [this] { promoteActiveRandomCandidate(0); };
    addAndMakeVisible(promoteCandidateAButton);

    promoteCandidateBButton.setTooltip("Promote the active random candidate to performance snapshot B");
    promoteCandidateBButton.onClick = [this] { promoteActiveRandomCandidate(1); };
    addAndMakeVisible(promoteCandidateBButton);

    addAndMakeVisible(generateButton);
    addAndMakeVisible(mutateButton);
    addAndMakeVisible(variationButton);
    addAndMakeVisible(wildMutateButton);
    addAndMakeVisible(undoRandomButton);
    addAndMakeVisible(redoRandomButton);
    addAndMakeVisible(recallSnapshotAButton);
    addAndMakeVisible(captureSnapshotAButton);
    addAndMakeVisible(recallSnapshotBButton);
    addAndMakeVisible(captureSnapshotBButton);
    addAndMakeVisible(recallSnapshotCButton);
    addAndMakeVisible(captureSnapshotCButton);
    addAndMakeVisible(recallSnapshotDButton);
    addAndMakeVisible(captureSnapshotDButton);
    addAndMakeVisible(loadSampleButton);
    addAndMakeVisible(clearSampleButton);
    addAndMakeVisible(sampleChopPanel);
    addAndMakeVisible(sampleRecorderPanel);
    addAndMakeVisible(randomCutButton);
    addAndMakeVisible(ukgChopButton);
    addAndMakeVisible(randomSequencerButton);
    addAndMakeVisible(mutateSequencerButton);
    addAndMakeVisible(undoSequencerButton);
    addAndMakeVisible(clearSequencerButton);
    addAndMakeVisible(bassPatternButton);
    addAndMakeVisible(stabPatternButton);
    addAndMakeVisible(ukgPatternButton);
    addAndMakeVisible(applyPatternButton);
    addAndMakeVisible(copySequencerButton);
    addAndMakeVisible(rotateSequencerLeftButton);
    addAndMakeVisible(rotateSequencerRightButton);
    addAndMakeVisible(exportSequencerMidiButton);
    addAndMakeVisible(exportSequencerChainButton);
    addAndMakeVisible(applyGrooveTransformButton);
    addAndMakeVisible(homeTabButton);
    addAndMakeVisible(synthTabButton);
    addAndMakeVisible(labTabButton);
    addAndMakeVisible(modTabButton);
    addAndMakeVisible(sampleTabButton);
    addAndMakeVisible(sequencerTabButton);
    addAndMakeVisible(effectsTabButton);
    addAndMakeVisible(libraryTabButton);
    addAndMakeVisible(infoTabButton);
    for (auto* tabButton : {
             &homeTabButton,
             &synthTabButton,
             &labTabButton,
             &modTabButton,
             &sampleTabButton,
             &sequencerTabButton,
             &effectsTabButton,
             &libraryTabButton,
             &infoTabButton })
    {
        tabButton->setWantsKeyboardFocus(false);
        tabButton->setMouseClickGrabsKeyboardFocus(false);
    }
    addAndMakeVisible(sineWaveButton);
    addAndMakeVisible(sawWaveButton);
    addAndMakeVisible(squareWaveButton);
    addAndMakeVisible(triangleWaveButton);
    addAndMakeVisible(wavetableWaveButton);
    addAndMakeVisible(organWaveButton);
    addAndMakeVisible(housePianoWaveButton);
    addAndMakeVisible(customWaveButton);
    addAndMakeVisible(osc2SineWaveButton);
    addAndMakeVisible(osc2SawWaveButton);
    addAndMakeVisible(osc2SquareWaveButton);
    addAndMakeVisible(osc2TriangleWaveButton);
    addAndMakeVisible(osc2WavetableWaveButton);
    addAndMakeVisible(osc2OrganWaveButton);
    addAndMakeVisible(osc2HousePianoWaveButton);
    addAndMakeVisible(osc2CustomWaveButton);
    addAndMakeVisible(lowpassFilterButton);
    addAndMakeVisible(bandpassFilterButton);
    addAndMakeVisible(highpassFilterButton);
    addAndMakeVisible(rateEighthButton);
    addAndMakeVisible(rateSixteenthButton);
    addAndMakeVisible(rateThirtySecondButton);
    addAndMakeVisible(previousPresetButton);
    addAndMakeVisible(nextPresetButton);
    addAndMakeVisible(savePresetButton);
    addAndMakeVisible(loadPresetButton);
    addAndMakeVisible(auditionPresetButton);
    addAndMakeVisible(warmPresetPreviewsButton);
    addAndMakeVisible(refreshPresetsButton);
    addAndMakeVisible(favoritePresetButton);
    addAndMakeVisible(comparePresetButton);
    addAndMakeVisible(revertPresetButton);
    addAndMakeVisible(candidateFavoriteButton);
    addAndMakeVisible(saveCandidateButton);
    addAndMakeVisible(fxMoveUpButton);
    addAndMakeVisible(fxMoveDownButton);
    addAndMakeVisible(fxResetOrderButton);
    addAndMakeVisible(fxRemoveButton);
    addAndMakeVisible(fxThrowDelayButton);
    addAndMakeVisible(fxThrowSpaceButton);
    addAndMakeVisible(fxThrowPumpButton);
    addAndMakeVisible(fxThrowDryButton);
    addAndMakeVisible(fxHoldDelayButton);
    addAndMakeVisible(fxHoldSpaceButton);
    addAndMakeVisible(fxHoldPumpButton);
    addAndMakeVisible(fxMuteDropButton);
    addAndMakeVisible(fxApplyPresetButton);
    addAndMakeVisible(modInspectorAddButton);
    addAndMakeVisible(modInspectorClearButton);
    addAndMakeVisible(infoOpenLabButton);
    addAndMakeVisible(infoOpenModButton);
    addAndMakeVisible(infoOpenFxButton);
    addAndMakeVisible(infoOpenLibraryButton);
    addAndMakeVisible(undoEditButton);
    addAndMakeVisible(redoEditButton);
    addAndMakeVisible(selectedControlAddModButton);
    addAndMakeVisible(selectedControlOpenModButton);
    addAndMakeVisible(modMacroAssignAddButton);
    addAndMakeVisible(modMacroAssignReplaceButton);
    addAndMakeVisible(modMacroAssignClearButton);
    addAndMakeVisible(fxToneSlotButton);
    addAndMakeVisible(fxEqSlotButton);
    addAndMakeVisible(fxDistortionSlotButton);
    addAndMakeVisible(fxBitcrushSlotButton);
    addAndMakeVisible(fxPumpSlotButton);
    addAndMakeVisible(fxTremoloSlotButton);
    addAndMakeVisible(fxRingSlotButton);
    addAndMakeVisible(fxCombSlotButton);
    addAndMakeVisible(fxPhaserSlotButton);
    addAndMakeVisible(fxFlangerSlotButton);
    addAndMakeVisible(fxChorusSlotButton);
    addAndMakeVisible(fxDelaySlotButton);
    addAndMakeVisible(fxReverbSlotButton);
    addAndMakeVisible(fxWidthSlotButton);
    addAndMakeVisible(fxGuardSlotButton);

    const auto sequencerStepGetter = [this] (int index) { return audioProcessor.getSequencerStep(index); };
    const auto sequencerStepSetter = [this] (int index, Sequencer::Step step)
    {
        audioProcessor.setSequencerStep(index, step);
        repaintSequencerGrids();
        updateSequencerStepEditor();
    };
    sequencerGrid.setCallbacks(sequencerStepGetter, sequencerStepSetter);
    sequencerGrid.onSelectionChanged = [this] (int index, Sequencer::Step)
    {
        if (expandedSequencerGrid.getSelectedStepIndex() != index)
            expandedSequencerGrid.selectStep(index);
        updateSequencerStepEditor();
    };
    expandedSequencerGrid.setComponentID("ExpandedSequencerGrid");
    expandedSequencerGrid.setCallbacks(sequencerStepGetter, sequencerStepSetter);
    expandedSequencerGrid.onSelectionChanged = [this] (int index, Sequencer::Step)
    {
        if (sequencerGrid.getSelectedStepIndex() != index)
            sequencerGrid.selectStep(index);
        updateSequencerStepEditor();
    };
    addAndMakeVisible(sequencerGrid);
    addAndMakeVisible(expandedSequencerGrid);
    applyThemeColours();
    updateSampleNameLabel();
    updateSampleRecorderStatus();
    updateSampleSliceButtons();
    updateHouseLayerRackDisplay();
    refreshPresetList();
    installGlobalKeyboardListeners();
    setActivePanel(Panel::home);
    returnKeyboardFocusToPiano();
    startTimerHz(12);
}

NateVSTAudioProcessorEditor::~NateVSTAudioProcessorEditor()
{
    restoreFxMomentarySnapshot(fxMomentarySnapshot);
    releaseComputerKeyboardNotes();
    removeGlobalKeyboardListeners();
    removeMouseListener(this);
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();
    pruneSequencerDragMidiFiles();
    stopTimer();
    presetBrowserList.setModel(nullptr);
    setLookAndFeel(nullptr);
}

const UI::Theme& NateVSTAudioProcessorEditor::uiTheme() const noexcept
{
    return lookAndFeel.theme();
}

void NateVSTAudioProcessorEditor::applyThemeColours()
{
    const auto& theme = uiTheme();

    titleLabel.setColour(juce::Label::textColourId, theme.text);
    keyboardRangeLabel.setColour(juce::Label::textColourId, theme.text);
    sequencerRootValueLabel.setColour(juce::Label::textColourId, theme.text);
    sequencerRootValueLabel.setColour(juce::Label::backgroundColourId, theme.field);
    sequencerRootValueLabel.setColour(juce::Label::outlineColourId, theme.outline);
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, theme.keyboardWhite);
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, theme.keyboardBlack);
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, theme.outline);
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, theme.accent.withAlpha(0.20f));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, theme.accent.withAlpha(0.67f));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, theme.keyboardText);

    for (auto* label : {
             &homeSectionLabel, &homeEngineLabel, &homeShapeLabel, &homeLabLabel, &homeLibraryLabel,
             &synthSectionLabel, &synthSourceLabel, &synthVoiceLabel, &synthFilterLabel, &synthAmpLabel,
             &randomSectionLabel, &modSectionLabel, &modSourceLabel, &modMacroLabel, &modLfoLabel,
             &modLfo2Label, &modEnvelopeLabel, &modMatrixLabel, &sampleSectionLabel, &sampleSourceLabel,
             &sampleChopLabel, &sampleShapeLabel, &sequencerSectionLabel, &futureSectionLabel,
             &librarySectionLabel, &libraryFindLabel, &libraryBrowserLabel, &librarySaveLabel,
             &libraryInspectorLabel, &infoSectionLabel, &infoAboutLabel, &infoWorkflowLabel,
             &infoDetailsLabel, &infoFocusLabel })
    {
        label->setColour(juce::Label::textColourId, theme.accent);
    }

    for (auto* label : {
             &sampleNameLabel, &presetStatusLabel, &randomStatusLabel,
             &modMatrixStatusLabel, &modInspectorStatusLabel, &modMacroAssignStatusLabel,
             &performanceStatusLabel, &selectedControlStatusLabel, &fxRackStatusLabel })
    {
        label->setColour(juce::Label::textColourId, theme.textMuted);
    }

    for (auto* label : {
             &modMatrixSourceHeader, &modMatrixDestinationHeader, &modMatrixAmountHeader,
             &modMatrixSourceHeaderB, &modMatrixDestinationHeaderB, &modMatrixAmountHeaderB,
             &modInspectorLabel, &modMacroAssignLabel })
    {
        label->setColour(juce::Label::textColourId, theme.textDim);
    }

    selectedControlHeaderLabel.setColour(juce::Label::textColourId, theme.accent);
    selectedControlHeaderLabel.setColour(juce::Label::backgroundColourId, theme.panel.withAlpha(0.28f));
    selectedControlHeaderLabel.setColour(juce::Label::outlineColourId, theme.outline);
    selectedControlStatusLabel.setColour(juce::Label::backgroundColourId, theme.panelAlt.withAlpha(0.93f));
    selectedControlStatusLabel.setColour(juce::Label::outlineColourId, theme.outline);
    hostSyncStatusLabel.setColour(juce::Label::textColourId, theme.textDim);
    hostSyncStatusLabel.setColour(juce::Label::backgroundColourId, theme.panel.withAlpha(0.13f));
    hostSyncStatusLabel.setColour(juce::Label::outlineColourId, theme.outline);
    presetBrowserHeaderLabel.setColour(juce::Label::textColourId, theme.accent);
    presetBrowserHeaderLabel.setColour(juce::Label::backgroundColourId, theme.panelAlt);
    focusOverlayTitleLabel.setColour(juce::Label::textColourId, theme.text);
    randomRecipeInfoLabel.setColour(juce::Label::textColourId, theme.accentBright);
    randomRecipeInfoLabel.setColour(juce::Label::backgroundColourId, theme.panelAlt.withAlpha(0.93f));
    randomRecipeInfoLabel.setColour(juce::Label::outlineColourId, theme.outline);

    presetBrowserList.setColour(juce::ListBox::backgroundColourId, theme.panelAlt);
    presetBrowserList.setColour(juce::ListBox::outlineColourId, theme.outlineStrong);
    presetBrowserList.setColour(juce::ListBox::textColourId, theme.text);

    houseLayerRackDisplay.setTheme(theme);
    expandedHouseLayerRackDisplay.setTheme(theme);
    sampleChopPanel.applyTheme(theme);
    sampleRecorderPanel.applyTheme(theme);

    modMacroAssignAmountSlider.setColour(juce::Slider::trackColourId, theme.accent);
    modMacroAssignAmountSlider.setColour(juce::Slider::backgroundColourId, theme.outline);
    modMacroAssignAmountSlider.setColour(juce::Slider::textBoxTextColourId, theme.text);
    modMacroAssignAmountSlider.setColour(juce::Slider::textBoxBackgroundColourId, theme.field);

    auto applyTextEditorTheme = [&theme] (juce::TextEditor& editor)
    {
        editor.setColour(juce::TextEditor::backgroundColourId, theme.field);
        editor.setColour(juce::TextEditor::outlineColourId, theme.outlineStrong);
        editor.setColour(juce::TextEditor::focusedOutlineColourId, theme.fieldFocus);
        editor.setColour(juce::TextEditor::textColourId, theme.text);
        editor.setColour(juce::TextEditor::highlightColourId, theme.accent.withAlpha(0.34f));
    };

    for (auto* editor : {
             &presetNameEditor, &presetSearchEditor, &presetAuthorEditor, &presetNotesEditor,
             &randomCandidateDetailEditor, &infoAboutEditor, &infoWorkflowEditor, &infoDetailEditor })
    {
        applyTextEditorTheme(*editor);
    }

    for (auto& label : modSlotRows)
        label.setColour(juce::Label::textColourId, theme.accent);

    repaint();
}

void NateVSTAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto& theme = uiTheme();
    g.fillAll(backgroundColour(theme));

    auto bounds = getLocalBounds().reduced(16);
    const auto topArea = bounds.removeFromTop(48);
    const auto keyboardArea = bounds.removeFromBottom(pianoKeyboardHeight);
    bounds.removeFromBottom(10);
    const auto contentArea = bounds.reduced(0, 8);

    g.setColour(panelColour(theme));
    g.fillRoundedRectangle(topArea.toFloat(), 7.0f);
    g.fillRoundedRectangle(contentArea.toFloat(), 7.0f);
    g.fillRoundedRectangle(keyboardArea.toFloat(), 7.0f);

    g.setColour(theme.outline);
    g.drawRoundedRectangle(topArea.toFloat(), 7.0f, 1.0f);
    g.drawRoundedRectangle(contentArea.toFloat(), 7.0f, 1.0f);
    g.drawRoundedRectangle(keyboardArea.toFloat(), 7.0f, 1.0f);
    g.drawVerticalLine(keyboardArea.getX() + keyboardControlsWidth,
                       static_cast<float>(keyboardArea.getY() + 9),
                       static_cast<float>(keyboardArea.getBottom() - 9));

    if (activePanel == Panel::home)
    {
        auto homeContent = contentArea.reduced(18).withTrimmedTop(36);
        auto topRow = homeContent.removeFromTop(juce::jlimit(246, 300, homeContent.getHeight() / 2 + 18));
        const auto engineColumnWidth = juce::jlimit(270, 318, topRow.getWidth() / 4);
        const auto macroColumnWidth = juce::jlimit(276, 326, topRow.getWidth() / 3);
        auto engineArea = topRow.removeFromLeft(engineColumnWidth).reduced(5);
        auto shapeArea = topRow.removeFromRight(macroColumnWidth).reduced(5);
        auto overviewArea = topRow.reduced(5);
        homeContent.removeFromTop(8);
        auto bottomRow = homeContent;
        auto labArea = bottomRow.removeFromLeft(juce::jlimit(310, 366, bottomRow.getWidth() / 4 + 42)).reduced(5);
        auto libraryArea = bottomRow.reduced(5);

        for (auto area : { engineArea, overviewArea, shapeArea, labArea, libraryArea })
        {
            g.setColour(theme.panelAlt);
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(theme.outline);
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }
    }

    if (activePanel == Panel::synth)
    {
        auto synthContent = contentArea.reduced(18).withTrimmedTop(36);
        auto workspace = synthContent.withTrimmedTop(8);
        auto sourceArea = workspace.removeFromLeft(juce::jlimit(300, 360, workspace.getWidth() / 4)).reduced(5);
        workspace.removeFromLeft(10);
        auto inspectorArea = workspace.removeFromRight(juce::jlimit(300, 370, workspace.getWidth() / 3)).reduced(5);
        auto canvasArea = workspace.reduced(5);

        for (auto area : { sourceArea, canvasArea, inspectorArea })
        {
            g.setColour(theme.panelAlt);
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(theme.outline);
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }
    }

    if (activePanel == Panel::mod)
    {
        auto modContent = contentArea.reduced(18).withTrimmedTop(36);
        modContent.removeFromTop(38);
        auto workspace = modContent.withTrimmedTop(8).reduced(5);

        auto drawPanel = [&g, &theme] (juce::Rectangle<int> area)
        {
            g.setColour(theme.panelAlt);
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(theme.outline);
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        };

        if (activeModWorkflowPage == ModWorkflowPage::sources)
        {
            auto sourceArea = workspace.removeFromLeft(juce::jlimit(420, 590, workspace.getWidth() / 2)).reduced(0);
            workspace.removeFromLeft(12);
            auto generatorArea = workspace;
            auto lfo2Area = generatorArea.removeFromTop(juce::jlimit(146, 188, generatorArea.getHeight() / 2));
            generatorArea.removeFromTop(10);
            drawPanel(sourceArea);
            drawPanel(lfo2Area);
            drawPanel(generatorArea);
        }
        else if (activeModWorkflowPage == ModWorkflowPage::macros)
        {
            const auto mapWidth = juce::jlimit(250, 360, workspace.getWidth() / 3);
            auto mapArea = workspace.removeFromLeft(mapWidth);
            workspace.removeFromLeft(10);
            drawPanel(mapArea);
            drawPanel(workspace);
        }
        else
        {
            drawPanel(workspace);
        }
    }

    if (activePanel == Panel::sample)
    {
        auto sampleContent = contentArea.reduced(18);
        sampleContent.removeFromTop(28);
        auto workspace = sampleContent.withTrimmedTop(8);
        auto sourceArea = workspace.removeFromLeft(juce::jlimit(260, 310, workspace.getWidth() / 4)).reduced(5);
        workspace.removeFromLeft(10);
        auto chopArea = workspace.reduced(5);

        for (auto area : { sourceArea, chopArea })
        {
            g.setColour(theme.panelAlt);
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(theme.outline);
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }
    }

    if (activePanel == Panel::sequencer)
    {
        auto sequencerContent = contentArea.reduced(18).withTrimmedTop(36);
        auto controlArea = sequencerContent.removeFromLeft(juce::jlimit(228, 286, sequencerContent.getWidth() / 5)).reduced(5);
        sequencerContent.removeFromLeft(10);
        auto gridArea = sequencerContent.reduced(5, 7);

        for (auto area : { controlArea, gridArea })
        {
            g.setColour(theme.panelAlt);
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(theme.outline);
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }
    }

    if (activePanel == Panel::effects)
    {
        auto fxContent = contentArea.reduced(18);
        fxContent.removeFromTop(28);
        auto commandArea = fxContent.removeFromTop(44).reduced(5);
        auto performArea = fxContent.removeFromTop(42).reduced(5);
        fxContent.removeFromTop(8);
        auto rackArea = fxContent.removeFromLeft(260).reduced(5);
        auto detailArea = fxContent.reduced(5);

        for (auto area : { commandArea, performArea, rackArea, detailArea })
        {
            g.setColour(theme.panelAlt);
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(theme.outline);
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }

        g.setColour(theme.textDim);
        g.setFont(12.0f);
        g.drawText("RACK", rackArea.removeFromTop(24).reduced(12, 0), juce::Justification::centredLeft);
        g.drawText(fxModuleName(selectedFxModule).toUpperCase(), detailArea.removeFromTop(24).reduced(14, 0), juce::Justification::centredLeft);
    }

    if (activePanel == Panel::library)
    {
        auto libraryContent = contentArea.reduced(18).withTrimmedTop(36);
        const auto leftWidth = juce::jlimit(174, 212, libraryContent.getWidth() / 6);
        const auto inspectorWidth = juce::jlimit(232, 286, libraryContent.getWidth() / 4);
        auto leftArea = libraryContent.removeFromLeft(leftWidth).reduced(5);
        auto inspectorArea = libraryContent.removeFromRight(inspectorWidth).reduced(5);
        auto browserArea = libraryContent.reduced(5);

        for (auto area : { leftArea, browserArea, inspectorArea })
        {
            g.setColour(theme.panelAlt.darker(0.08f));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(theme.outline);
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }

        const auto savePanelHeight = juce::jlimit(252, 304, inspectorArea.getHeight() - 96);
        auto saveArea = inspectorArea.reduced(10).removeFromTop(savePanelHeight);
        auto inspectArea = inspectorArea.reduced(10).withTrimmedTop(savePanelHeight + 12);
        auto drawInset = [&g, &theme] (juce::Rectangle<int> area, juce::Colour accent)
        {
            auto inset = area.toFloat();
            g.setColour(theme.background.brighter(0.03f));
            g.fillRoundedRectangle(inset, 5.0f);
            g.setColour(theme.outline);
            g.drawRoundedRectangle(inset, 5.0f, 1.0f);
            g.setColour(accent.withAlpha(0.56f));
            g.fillRoundedRectangle(inset.removeFromTop(3.0f), 2.0f);
        };

        drawInset(saveArea, theme.accent);
        drawInset(inspectArea, theme.warning);

        const auto browserHeaderHeight = 154;
        auto browserHeader = browserArea.removeFromTop(browserHeaderHeight).reduced(10, 8);
        g.setColour(theme.panelRaised);
        g.fillRoundedRectangle(browserHeader.toFloat(), 5.0f);
        g.setColour(theme.outlineStrong);
        g.drawRoundedRectangle(browserHeader.toFloat(), 5.0f, 1.0f);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(theme.textDim);
        g.drawFittedText("SELECT, AUDITION, LOAD, COMPARE, AND RATE PRODUCTION PATCHES",
                         browserHeader.reduced(10, 6),
                         juce::Justification::centredLeft,
                         1,
                         0.72f);
    }

    if (activePanel == Panel::info)
    {
        auto infoContent = contentArea.reduced(18).withTrimmedTop(36);
        auto topRow = infoContent.removeFromTop(172);
        auto aboutArea = topRow.removeFromLeft(352).reduced(5);
        auto workflowArea = topRow.reduced(5);
        infoContent.removeFromTop(10);
        auto detailRow = infoContent;
        auto focusArea = detailRow.removeFromRight(214).reduced(5);
        auto detailArea = detailRow.reduced(5);

        for (auto area : { aboutArea, workflowArea, detailArea, focusArea })
        {
            g.setColour(theme.panelAlt);
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(theme.outline);
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }
    }
}

void NateVSTAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(16);
    auto top = bounds.removeFromTop(42);

    outputMeter.setBounds(top.removeFromRight(156).reduced(6, 5));
    titleLabel.setBounds(top.removeFromLeft(132).reduced(8, 0));

    auto placeTab = [&top] (juce::TextButton& button, int width)
    {
        button.setBounds(top.removeFromLeft(width).reduced(3, 4));
    };

    placeTab(homeTabButton, 64);
    placeTab(synthTabButton, 64);
    placeTab(labTabButton, 52);
    placeTab(modTabButton, 52);
    placeTab(sampleTabButton, 72);
    placeTab(sequencerTabButton, 54);
    placeTab(effectsTabButton, 48);
    placeTab(libraryTabButton, 82);
    placeTab(infoTabButton, 54);

    bounds.removeFromTop(14);
    auto keyboardArea = bounds.removeFromBottom(pianoKeyboardHeight);
    auto keyboardControlArea = keyboardArea.removeFromLeft(keyboardControlsWidth).reduced(8, 6);
    keyboardOctaveDownButton.setBounds(keyboardControlArea.removeFromLeft(54).reduced(2, 4));
    keyboardRangeLabel.setBounds(keyboardControlArea.removeFromLeft(92).reduced(3, 4));
    keyboardHomeButton.setBounds(keyboardControlArea.removeFromLeft(58).reduced(2, 4));
    keyboardOctaveUpButton.setBounds(keyboardControlArea.removeFromLeft(54).reduced(2, 4));
    keyboardPanicButton.setBounds(keyboardControlArea.removeFromLeft(74).reduced(2, 4));
    auto pianoKeyboardBounds = keyboardArea.reduced(8, 6);
    pianoKeyboard.setBounds(pianoKeyboardBounds);
    pinPianoKeyboardVisualRange();
    updateKeyboardRangeLabel();
    bounds.removeFromBottom(10);
    auto content = bounds.reduced(18);
    hidePanelComponents();
    updateTabButtons();

    auto selectedControlRow = content.withHeight(28);
    const auto selectedControlWidth = juce::jlimit(520, content.getWidth(), content.getWidth() - 260);
    auto selectedControlArea = selectedControlRow.removeFromRight(selectedControlWidth).reduced(0, 2);
    selectedControlHeaderLabel.setVisible(true);
    selectedControlStatusLabel.setVisible(true);
    undoEditButton.setVisible(true);
    redoEditButton.setVisible(true);
    selectedControlAddModButton.setVisible(true);
    selectedControlOpenModButton.setVisible(true);
    selectedControlHeaderLabel.setBounds(selectedControlArea.removeFromLeft(72).reduced(2, 0));
    undoEditButton.setBounds(selectedControlArea.removeFromRight(78).reduced(2, 0));
    redoEditButton.setBounds(selectedControlArea.removeFromRight(78).reduced(2, 0));
    selectedControlAddModButton.setBounds(selectedControlArea.removeFromRight(58).reduced(2, 0));
    selectedControlOpenModButton.setBounds(selectedControlArea.removeFromRight(50).reduced(2, 0));
    selectedControlStatusLabel.setBounds(selectedControlArea.reduced(2, 0));
    refreshGlobalEditControls();

    switch (activePanel)
    {
        case Panel::home:
        {
            homeSectionLabel.setVisible(true);
            homeEngineLabel.setVisible(true);
            homeShapeLabel.setVisible(true);
            homeLabLabel.setVisible(true);
            homeLibraryLabel.setVisible(true);
            recipeBox.setVisible(true);
            randomScopeBox.setVisible(true);
            generateButton.setVisible(true);
            mutateButton.setVisible(true);
            variationButton.setVisible(true);
            presetBox.setVisible(true);
            previousPresetButton.setVisible(true);
            nextPresetButton.setVisible(true);
            loadPresetButton.setVisible(true);
            auditionPresetButton.setVisible(true);
            favoritePresetButton.setVisible(true);
            refreshPresetsButton.setVisible(true);
            presetStatusLabel.setVisible(true);
            randomStatusLabel.setVisible(true);
            homeOverviewDisplay.setVisible(true);
            homeSignalFlowDisplay.setVisible(true);
            homeSessionDisplay.setVisible(true);
            outputOscilloscopeDisplay.setVisible(true);
            outputSpectrumDisplay.setVisible(true);
            stereoFieldDisplay.setVisible(true);
            clubMonitorDisplay.setVisible(true);
            lowEndAssistant.setVisible(true);
            macroPerformanceMap.setVisible(true);
            homeMacroExpandButton.setVisible(true);
            performanceXYPad.setVisible(true);
            performanceStatusLabel.setVisible(true);
            recallSnapshotAButton.setVisible(true);
            captureSnapshotAButton.setVisible(true);
            recallSnapshotBButton.setVisible(true);
            captureSnapshotBButton.setVisible(true);
            recallSnapshotCButton.setVisible(true);
            captureSnapshotCButton.setVisible(true);
            recallSnapshotDButton.setVisible(true);
            captureSnapshotDButton.setVisible(true);

            homeSectionLabel.setBounds(content.removeFromTop(28));
            auto dashboard = content.withTrimmedTop(8);
            auto topRow = dashboard.removeFromTop(juce::jlimit(246, 300, dashboard.getHeight() / 2 + 18));
            const auto engineColumnWidth = juce::jlimit(270, 318, topRow.getWidth() / 4);
            const auto macroColumnWidth = juce::jlimit(276, 326, topRow.getWidth() / 3);
            auto performArea = topRow.removeFromLeft(engineColumnWidth).reduced(14, 10);
            auto macroArea = topRow.removeFromRight(macroColumnWidth).reduced(14, 10);
            auto overviewArea = topRow.reduced(14, 10);
            dashboard.removeFromTop(8);
            auto bottomRow = dashboard;
            auto labArea = bottomRow.removeFromLeft(juce::jlimit(310, 366, bottomRow.getWidth() / 4 + 42)).reduced(14, 10);
            auto libraryArea = bottomRow.reduced(14, 10);

            homeEngineLabel.setBounds(performArea.removeFromTop(24));
            setSliderVisible(subLevelSlider, subLevelLabel, true);
            setSliderVisible(cutoffSlider, cutoffLabel, true);
            setSliderVisible(driveSlider, driveLabel, true);
            setSliderVisible(outputSlider, outputLabel, true);
            layoutKnobRow(performArea.removeFromTop(88).withTrimmedTop(4), { &subLevelSlider, &cutoffSlider, &driveSlider, &outputSlider });
            lowEndAssistant.setBounds(performArea.removeFromTop(68).reduced(2, 5));
            clubMonitorDisplay.setBounds(performArea.reduced(2, 4));

            homeOverviewDisplay.setBounds(overviewArea.removeFromTop(118).reduced(2, 0));
            homeSignalFlowDisplay.setBounds(overviewArea.removeFromTop(44).withTrimmedTop(6).reduced(4, 0));
            auto scopeRow = overviewArea.removeFromTop(juce::jlimit(58, 76, overviewArea.getHeight() / 3)).withTrimmedTop(5);
            outputOscilloscopeDisplay.setBounds(scopeRow.reduced(4, 1));
            auto analysisRow = overviewArea.withTrimmedTop(6);
            const auto stereoWidth = juce::jlimit(112, 158, analysisRow.getWidth() / 3);
            stereoFieldDisplay.setBounds(analysisRow.removeFromRight(stereoWidth).reduced(4, 0));
            outputSpectrumDisplay.setBounds(analysisRow.reduced(4, 0));

            auto homeMacroHeader = macroArea.removeFromTop(24);
            homeMacroExpandButton.setBounds(homeMacroHeader.removeFromRight(28).reduced(3, 2));
            homeShapeLabel.setBounds(homeMacroHeader);
            auto macroControlArea = macroArea.removeFromTop(132).withTrimmedTop(4);
            const auto xyWidth = juce::jlimit(106, 128, macroControlArea.getWidth() / 3);
            performanceXYPad.setBounds(macroControlArea.removeFromRight(xyWidth).reduced(4, 0));
            macroPerformanceMap.setBounds(macroControlArea.reduced(4, 0));
            auto snapshotArea = macroArea.removeFromTop(68).withTrimmedTop(5);
            performanceStatusLabel.setBounds(snapshotArea.removeFromLeft(juce::jlimit(68, 96, snapshotArea.getWidth() / 4)).reduced(4, 4));
            auto recallRow = snapshotArea.removeFromTop(30);
            auto storeRow = snapshotArea.withTrimmedTop(2);
            const auto recallButtonWidth = juce::jmax(1, recallRow.getWidth() / 4);
            recallSnapshotAButton.setBounds(recallRow.removeFromLeft(recallButtonWidth).reduced(3, 4));
            recallSnapshotBButton.setBounds(recallRow.removeFromLeft(recallButtonWidth).reduced(3, 4));
            recallSnapshotCButton.setBounds(recallRow.removeFromLeft(recallButtonWidth).reduced(3, 4));
            recallSnapshotDButton.setBounds(recallRow.reduced(3, 4));
            const auto storeButtonWidth = juce::jmax(1, storeRow.getWidth() / 4);
            captureSnapshotAButton.setBounds(storeRow.removeFromLeft(storeButtonWidth).reduced(3, 4));
            captureSnapshotBButton.setBounds(storeRow.removeFromLeft(storeButtonWidth).reduced(3, 4));
            captureSnapshotCButton.setBounds(storeRow.removeFromLeft(storeButtonWidth).reduced(3, 4));
            captureSnapshotDButton.setBounds(storeRow.reduced(3, 4));

            homeLabLabel.setBounds(labArea.removeFromTop(24));
            auto randomSelectRow = labArea.removeFromTop(38);
            recipeBox.setBounds(randomSelectRow.removeFromLeft(juce::jlimit(168, 230, randomSelectRow.getWidth() / 2)).reduced(3, 4));
            randomScopeBox.setBounds(randomSelectRow.reduced(3, 4));
            auto labButtonRow = labArea.removeFromTop(38).withTrimmedTop(4);
            const auto labButtonWidth = juce::jmax(1, labButtonRow.getWidth() / 3);
            generateButton.setBounds(labButtonRow.removeFromLeft(labButtonWidth).reduced(3, 4));
            variationButton.setBounds(labButtonRow.removeFromLeft(labButtonWidth).reduced(3, 4));
            mutateButton.setBounds(labButtonRow.reduced(3, 4));
            auto randomStatusRow = labArea.removeFromTop(44).withTrimmedTop(8);
            randomStatusLabel.setBounds(randomStatusRow.reduced(5, 4));

            homeLibraryLabel.setBounds(libraryArea.removeFromTop(24));
            const auto playViewHeight = juce::jlimit(74, 104, libraryArea.getHeight() - 112);
            homeSessionDisplay.setBounds(libraryArea.removeFromTop(playViewHeight).reduced(3, 5));
            auto loadRow = libraryArea.removeFromTop(38).withTrimmedTop(2);
            previousPresetButton.setBounds(loadRow.removeFromLeft(42).reduced(3, 4));
            presetBox.setBounds(loadRow.removeFromLeft(juce::jmax(120, loadRow.getWidth() - 42)).reduced(3, 4));
            nextPresetButton.setBounds(loadRow.removeFromLeft(42).reduced(3, 4));
            auto actionRow = libraryArea.removeFromTop(38).withTrimmedTop(3);
            loadPresetButton.setBounds(actionRow.removeFromLeft(88).reduced(3, 4));
            auditionPresetButton.setBounds(actionRow.removeFromLeft(96).reduced(3, 4));
            favoritePresetButton.setBounds(actionRow.removeFromLeft(68).reduced(3, 4));
            refreshPresetsButton.setBounds(actionRow.removeFromLeft(92).reduced(3, 4));
            presetStatusLabel.setBounds(libraryArea.removeFromTop(34).reduced(5, 4));
            break;
        }

        case Panel::synth:
        {
            synthSectionLabel.setVisible(true);
            synthSourceLabel.setVisible(true);
            synthVoiceLabel.setVisible(true);
            synthFilterLabel.setVisible(true);
            filterResponseDisplay.setVisible(true);
            wavetableDisplay.setVisible(true);
            outputOscilloscopeDisplay.setVisible(true);
            outputSpectrumDisplay.setVisible(true);
            synthAmpLabel.setVisible(true);
            waveformBox.setVisible(true);
            osc2WaveBox.setVisible(true);
            customWaveButton.setVisible(true);
            osc2CustomWaveButton.setVisible(true);
            waveEditorFocusButton.setVisible(true);
            wavetableToolBox.setVisible(true);
            wavetableDrawModeBox.setVisible(true);
            filterModeBox.setVisible(true);
            lowpassFilterButton.setVisible(false);
            bandpassFilterButton.setVisible(false);
            highpassFilterButton.setVisible(false);
            noiseTypeBox.setVisible(true);
            filterCharacterBox.setVisible(true);
            filterSlopeBox.setVisible(true);
            monoButton.setVisible(true);
            houseLayerRackDisplay.setVisible(true);
            sourceLayerExpandButton.setVisible(true);
            synthSectionLabel.setBounds(content.removeFromTop(28));
            setSliderVisible(osc1LevelSlider, osc1LevelLabel, true);
            setSliderVisible(osc2LevelSlider, osc2LevelLabel, true);
            setSliderVisible(subLevelSlider, subLevelLabel, true);
            setSliderVisible(noiseLevelSlider, noiseLevelLabel, true);
            setSliderVisible(noiseDecaySlider, noiseDecayLabel, true);
            setSliderVisible(oscWarpSlider, oscWarpLabel, true);
            setSliderVisible(oscWavetablePositionSlider, oscWavetablePositionLabel, true);
            setSliderVisible(osc2WavetablePositionSlider, osc2WavetablePositionLabel, true);
            setSliderVisible(osc2OctaveSlider, osc2OctaveLabel, true);
            setSliderVisible(osc2TuneSlider, osc2TuneLabel, true);
            setSliderVisible(octaveSlider, octaveLabel, true);
            setSliderVisible(tuneSlider, tuneLabel, true);
            setSliderVisible(unisonVoicesSlider, unisonVoicesLabel, true);
            setSliderVisible(unisonDetuneSlider, unisonDetuneLabel, true);
            setSliderVisible(unisonBlendSlider, unisonBlendLabel, true);
            setSliderVisible(unisonSpreadSlider, unisonSpreadLabel, true);
            setSliderVisible(glideSlider, glideLabel, true);
            setSliderVisible(cutoffSlider, cutoffLabel, true);
            setSliderVisible(resonanceSlider, resonanceLabel, true);
            setSliderVisible(filterEnvSlider, filterEnvLabel, true);
            setSliderVisible(driveSlider, driveLabel, true);
            setSliderVisible(outputSlider, outputLabel, true);
            setSliderVisible(attackSlider, attackLabel, true);
            setSliderVisible(decaySlider, decayLabel, true);
            setSliderVisible(sustainSlider, sustainLabel, true);
            setSliderVisible(releaseSlider, releaseLabel, true);
            auto workspace = content.withTrimmedTop(8);
            auto sourceArea = workspace.removeFromLeft(juce::jlimit(300, 360, workspace.getWidth() / 4)).reduced(18, 12);
            workspace.removeFromLeft(10);
            auto inspectorArea = workspace.removeFromRight(juce::jlimit(300, 370, workspace.getWidth() / 3)).reduced(18, 12);
            auto canvasArea = workspace.reduced(18, 12);

            auto sourceHeader = sourceArea.removeFromTop(22);
            sourceLayerExpandButton.setBounds(sourceHeader.removeFromRight(28).reduced(3, 1));
            synthSourceLabel.setBounds(sourceHeader);

            auto osc1Row = sourceArea.removeFromTop(34).withTrimmedTop(2);
            customWaveButton.setButtonText("Edit O1");
            customWaveButton.setTooltip("Select Osc 1 Custom and edit it in the center wave editor");
            customWaveButton.setBounds(osc1Row.removeFromRight(78).reduced(4, 4));
            waveformBox.setBounds(osc1Row.reduced(4, 4));

            auto osc2Row = sourceArea.removeFromTop(34).withTrimmedTop(2);
            osc2CustomWaveButton.setButtonText("Edit O2");
            osc2CustomWaveButton.setTooltip("Select Osc 2 Custom and edit it in the center wave editor");
            osc2CustomWaveButton.setBounds(osc2Row.removeFromRight(78).reduced(4, 4));
            osc2WaveBox.setBounds(osc2Row.reduced(4, 4));

            const auto compactSourceCard = sourceArea.getHeight() < 260;
            const auto rackHeight = compactSourceCard ? 58 : 76;
            const auto levelRowHeight = compactSourceCard ? 62 : 72;
            houseLayerRackDisplay.setBounds(sourceArea.removeFromTop(rackHeight).reduced(2, 4));
            layoutKnobRow(sourceArea.removeFromTop(levelRowHeight).withTrimmedTop(3), {
                &osc1LevelSlider,
                &osc2LevelSlider,
                &subLevelSlider,
                &noiseLevelSlider
            });
            auto sourceTextureRow = sourceArea.withTrimmedTop(2);
            noiseTypeBox.setBounds(sourceTextureRow.removeFromTop(30).reduced(4, 4));
            layoutKnobRow(sourceTextureRow, {
                &noiseDecaySlider,
                &oscWavetablePositionSlider,
                &osc2WavetablePositionSlider
            });

            auto canvasHeader = canvasArea.removeFromTop(22);
            synthFilterLabel.setBounds(canvasHeader);
            auto waveToolRow = canvasArea.removeFromTop(36).withTrimmedTop(2);
            const auto waveEditWidth = juce::jlimit(86, 112, waveToolRow.getWidth() / 3);
            waveEditorFocusButton.setBounds(waveToolRow.removeFromRight(waveEditWidth).reduced(4));
            wavetableDrawModeBox.setBounds(waveToolRow.removeFromLeft(juce::jlimit(102, 144, waveToolRow.getWidth() / 3)).reduced(4));
            wavetableToolBox.setBounds(waveToolRow.reduced(4));
            const auto wavetableHeight = juce::jlimit(158, 238, canvasArea.getHeight() / 2);
            wavetableDisplay.setBounds(canvasArea.removeFromTop(wavetableHeight).reduced(2, 6));

            auto liveVisualRow = canvasArea.removeFromTop(juce::jlimit(48, 62, canvasArea.getHeight() / 5)).withTrimmedTop(4);
            const auto liveScopeWidth = juce::jlimit(150, 230, liveVisualRow.getWidth() / 3);
            outputOscilloscopeDisplay.setBounds(liveVisualRow.removeFromLeft(liveScopeWidth).reduced(3, 2));
            outputSpectrumDisplay.setBounds(liveVisualRow.reduced(3, 2));

            auto filterSelectRow = canvasArea.removeFromTop(38).withTrimmedTop(2);
            monoButton.setBounds(filterSelectRow.removeFromRight(78).reduced(4));
            filterSlopeBox.setBounds(filterSelectRow.removeFromRight(78).reduced(4));
            filterCharacterBox.setBounds(filterSelectRow.removeFromRight(juce::jlimit(110, 138, filterSelectRow.getWidth() / 2)).reduced(4));
            filterModeBox.setBounds(filterSelectRow.reduced(4));

            const auto responseHeight = juce::jlimit(86, 140, canvasArea.getHeight() / 2);
            filterResponseDisplay.setBounds(canvasArea.removeFromTop(responseHeight).reduced(2, 6));
            layoutKnobRow(canvasArea.removeFromTop(86).withTrimmedTop(4), {
                &cutoffSlider,
                &resonanceSlider,
                &filterEnvSlider,
                &driveSlider
            });

            synthVoiceLabel.setBounds(inspectorArea.removeFromTop(22));
            layoutKnobRow(inspectorArea.removeFromTop(74).withTrimmedTop(4), {
                &octaveSlider,
                &tuneSlider,
                &glideSlider
            });
            layoutKnobRow(inspectorArea.removeFromTop(74).withTrimmedTop(4), {
                &osc2OctaveSlider,
                &osc2TuneSlider,
                &oscWarpSlider
            });
            layoutKnobRow(inspectorArea.removeFromTop(78).withTrimmedTop(5), {
                &unisonVoicesSlider,
                &unisonDetuneSlider,
                &unisonBlendSlider,
                &unisonSpreadSlider
            });

            inspectorArea.removeFromTop(8);
            synthAmpLabel.setBounds(inspectorArea.removeFromTop(22));
            layoutKnobRow(inspectorArea.removeFromTop(78).withTrimmedTop(5), {
                &attackSlider,
                &decaySlider,
                &sustainSlider
            });
            layoutKnobRow(inspectorArea.removeFromTop(78).withTrimmedTop(5), {
                &releaseSlider,
                &outputSlider
            });
            break;
        }

        case Panel::lab:
        {
            randomSectionLabel.setVisible(true);
            randomSectionLabel.setBounds(content.removeFromTop(28));

            auto labWorkspace = content.withTrimmedTop(8);
            auto pageRail = labWorkspace.removeFromLeft(juce::jlimit(132, 168, labWorkspace.getWidth() / 6)).reduced(12, 8);
            content = labWorkspace.reduced(18, 10);
            const auto pageButtonHeight = juce::jlimit(32, 42, pageRail.getHeight() / static_cast<int>(randomLabPageButtons.size()));
            for (auto& button : randomLabPageButtons)
            {
                button.setVisible(true);
                button.setBounds(pageRail.removeFromTop(pageButtonHeight).reduced(4));
            }
            updateRandomLabPageButtons();

            auto showRecipeControls = [&content, this] (bool includeGenerate)
            {
                recipeBox.setVisible(true);
                randomScopeBox.setVisible(true);
                if (includeGenerate)
                    generateButton.setVisible(true);

                auto row = content.removeFromTop(48).withTrimmedTop(4);
                recipeBox.setBounds(row.removeFromLeft(juce::jlimit(220, 320, row.getWidth() / 2)).reduced(4));
                randomScopeBox.setBounds(row.removeFromLeft(juce::jlimit(120, 160, row.getWidth() / 3)).reduced(4));
                if (includeGenerate)
                    generateButton.setBounds(row.removeFromLeft(juce::jlimit(100, 132, row.getWidth())).reduced(4));
            };

            auto showLockAction = [&content, this]
            {
                randomLockActionBox.setVisible(true);
                auto row = content.removeFromTop(42).withTrimmedTop(4);
                randomLockActionBox.setBounds(row.removeFromLeft(juce::jlimit(180, 260, row.getWidth() / 3)).reduced(4));
            };

            auto showRandomBiasKnobs = [&content, this]
            {
                setSliderVisible(randomAmountSlider, randomAmountLabel, true);
                setSliderVisible(randomChaosSlider, randomChaosLabel, true);
                setSliderVisible(brightnessSlider, brightnessLabel, true);
                setSliderVisible(driveBiasSlider, driveBiasLabel, true);
                setSliderVisible(motionBiasSlider, motionBiasLabel, true);
                layoutKnobRow(content.removeFromTop(112).withTrimmedTop(8), {
                    &randomAmountSlider,
                    &randomChaosSlider,
                    &brightnessSlider,
                    &driveBiasSlider,
                    &motionBiasSlider
                });
            };

            auto showSectionIntensityControls = [&content, this]
            {
                auto row = content.removeFromTop(84).withTrimmedTop(6);
                const auto count = static_cast<int>(randomSectionIntensitySliders.size());
                const auto cellWidth = row.getWidth() / juce::jmax(1, count);

                for (size_t index = 0; index < randomSectionIntensitySliders.size(); ++index)
                {
                    auto cell = row.removeFromLeft(cellWidth).reduced(4, 2);
                    randomSectionIntensityLabels[index].setVisible(true);
                    randomSectionIntensitySliders[index].setVisible(true);
                    randomSectionIntensityLabels[index].setBounds(cell.removeFromTop(22));
                    randomSectionIntensitySliders[index].setBounds(cell.removeFromTop(30));
                }
            };

            auto showCandidateRow = [&content, this] (bool includePromote)
            {
                for (auto& button : randomCandidateButtons)
                    button.setVisible(true);
                for (auto& button : randomCandidateAuditionButtons)
                    button.setVisible(true);

                if (includePromote)
                {
                    promoteCandidateAButton.setVisible(true);
                    promoteCandidateBButton.setVisible(true);
                }

                updateRandomCandidateButtons();

                auto row = content.removeFromTop(54).withTrimmedTop(8);
                if (includePromote)
                {
                    auto promoteArea = row.removeFromRight(112);
                    promoteCandidateAButton.setBounds(promoteArea.removeFromLeft(56).reduced(4));
                    promoteCandidateBButton.setBounds(promoteArea.removeFromLeft(56).reduced(4));
                }

                const auto buttonWidth = row.getWidth() / static_cast<int>(randomCandidateButtons.size());
                for (size_t index = 0; index < randomCandidateButtons.size(); ++index)
                {
                    auto candidateCell = row.removeFromLeft(buttonWidth).reduced(4);
                    randomCandidateAuditionButtons[index].setBounds(candidateCell.removeFromRight(44).withTrimmedLeft(3));
                    randomCandidateButtons[index].setBounds(candidateCell);
                }
            };

            auto showStatus = [&content, this]
            {
                randomStatusLabel.setVisible(true);
                randomStatusLabel.setBounds(content.removeFromTop(34).withTrimmedTop(6).reduced(4));
            };

            switch (activeRandomLabPage)
            {
                case RandomLabPage::generate:
                {
                    recipeBox.setVisible(true);
                    randomScopeBox.setVisible(true);
                    randomSectionActionBox.setVisible(true);
                    generateButton.setVisible(true);
                    variationButton.setVisible(true);
                    randomMorphPad.setVisible(true);

                    auto topRow = content.removeFromTop(48).withTrimmedTop(4);
                    const auto actionButtonWidth = juce::jlimit(82, 118, topRow.getWidth() / 7);
                    generateButton.setBounds(topRow.removeFromRight(actionButtonWidth).reduced(4));
                    variationButton.setBounds(topRow.removeFromRight(actionButtonWidth).reduced(4));
                    randomSectionActionBox.setBounds(topRow.removeFromRight(juce::jlimit(150, 220, topRow.getWidth() / 3)).reduced(4));
                    randomScopeBox.setBounds(topRow.removeFromRight(juce::jlimit(118, 160, topRow.getWidth() / 4)).reduced(4));
                    recipeBox.setBounds(topRow.reduced(4));

                    const auto morphHeight = juce::jlimit(190, 278, juce::jmax(190, content.getHeight() - 178));
                    updateRandomMorphPad();
                    randomMorphPad.setBounds(content.removeFromTop(morphHeight).withTrimmedTop(6).reduced(4));
                    showSectionIntensityControls();
                    showCandidateRow(true);
                    showStatus();
                    break;
                }

                case RandomLabPage::mutate:
                {
                    recipeBox.setVisible(true);
                    randomScopeBox.setVisible(true);
                    variationButton.setVisible(true);
                    mutateButton.setVisible(true);
                    wildMutateButton.setVisible(true);
                    undoRandomButton.setVisible(true);
                    redoRandomButton.setVisible(true);
                    randomSectionActionBox.setVisible(true);

                    auto rows = content.removeFromTop(84).withTrimmedTop(4);
                    auto topRow = rows.removeFromTop(40);
                    auto bottomRow = rows.withTrimmedTop(2);
                    recipeBox.setBounds(topRow.removeFromLeft(juce::jlimit(190, 260, topRow.getWidth() / 3)).reduced(4));
                    randomScopeBox.setBounds(topRow.removeFromLeft(juce::jlimit(118, 160, topRow.getWidth() / 3)).reduced(4));
                    randomSectionActionBox.setBounds(topRow.removeFromLeft(juce::jlimit(160, 230, topRow.getWidth())).reduced(4));
                    variationButton.setBounds(bottomRow.removeFromLeft(bottomRow.getWidth() / 5).reduced(4));
                    mutateButton.setBounds(bottomRow.removeFromLeft(bottomRow.getWidth() / 4).reduced(4));
                    wildMutateButton.setBounds(bottomRow.removeFromLeft(bottomRow.getWidth() / 3).reduced(4));
                    undoRandomButton.setBounds(bottomRow.removeFromLeft(bottomRow.getWidth() / 2).reduced(4));
                    redoRandomButton.setBounds(bottomRow.reduced(4));
                    showRandomBiasKnobs();
                    showSectionIntensityControls();
                    showStatus();
                    break;
                }

                case RandomLabPage::recipe:
                {
                    showRecipeControls(true);
                    randomRecipeInfoLabel.setVisible(true);
                    updateRandomRecipeInfo();
                    randomRecipeInfoLabel.setBounds(content.removeFromTop(46).withTrimmedTop(6).reduced(4));
                    showLockAction();
                    showRandomBiasKnobs();
                    showStatus();
                    break;
                }

                case RandomLabPage::history:
                {
                    showCandidateRow(true);
                    randomCandidateDetailEditor.setVisible(true);
                    updateRandomCandidateDetail();
                    randomCandidateDetailEditor.setBounds(content.removeFromTop(118).withTrimmedTop(6).reduced(4));

                    undoRandomButton.setVisible(true);
                    redoRandomButton.setVisible(true);
                    variationButton.setVisible(true);
                    mutateButton.setVisible(true);

                    auto row = content.removeFromTop(46).withTrimmedTop(6);
                    undoRandomButton.setBounds(row.removeFromLeft(90).reduced(4));
                    redoRandomButton.setBounds(row.removeFromLeft(90).reduced(4));
                    variationButton.setBounds(row.removeFromLeft(90).reduced(4));
                    mutateButton.setBounds(row.removeFromLeft(100).reduced(4));
                    showStatus();
                    break;
                }

                case RandomLabPage::save:
                {
                    showCandidateRow(false);

                    presetCategoryBox.setVisible(true);
                    presetNameEditor.setVisible(true);
                    presetPackBox.setVisible(true);
                    presetBpmBox.setVisible(true);
                    presetNotesEditor.setVisible(true);
                    presetNotesTemplateBox.setVisible(true);
                    candidateRatingBox.setVisible(true);
                    candidateFavoriteButton.setVisible(true);
                    savePresetButton.setVisible(true);
                    saveCandidateButton.setVisible(true);
                    presetStatusLabel.setVisible(true);

                    auto saveRow = content.removeFromTop(48).withTrimmedTop(6);
                    presetCategoryBox.setBounds(saveRow.removeFromLeft(136).reduced(4));
                    presetNameEditor.setBounds(saveRow.removeFromLeft(194).reduced(4));
                    presetPackBox.setBounds(saveRow.removeFromLeft(126).reduced(4));
                    presetBpmBox.setBounds(saveRow.removeFromLeft(78).reduced(4));
                    candidateRatingBox.setBounds(saveRow.removeFromLeft(96).reduced(4));
                    candidateFavoriteButton.setBounds(saveRow.removeFromLeft(54).reduced(4));
                    savePresetButton.setBounds(saveRow.removeFromLeft(72).reduced(4));
                    saveCandidateButton.setBounds(saveRow.removeFromLeft(100).reduced(4));

                    auto notesToolRow = content.removeFromTop(38).withTrimmedTop(4);
                    presetNotesTemplateBox.setBounds(notesToolRow.removeFromLeft(180).reduced(4));

                    presetNotesEditor.setBounds(content.removeFromTop(78).withTrimmedTop(4).reduced(4));
                    presetStatusLabel.setBounds(content.removeFromTop(34).withTrimmedTop(6).reduced(4));
                    break;
                }
            }

            break;
        }

        case Panel::mod:
        {
            modSectionLabel.setVisible(true);
            modSectionLabel.setBounds(content.removeFromTop(28));

            auto workflowRow = content.removeFromTop(38).withTrimmedTop(4);
            for (size_t index = 0; index < modWorkflowButtons.size(); ++index)
            {
                auto& button = modWorkflowButtons[index];
                button.setVisible(true);
                const auto remaining = static_cast<int>(modWorkflowButtons.size() - index);
                button.setBounds(workflowRow.removeFromLeft(workflowRow.getWidth() / juce::jmax(1, remaining)).reduced(4));
            }
            updateModWorkflowButtons();

            auto modContent = content.withTrimmedTop(8).reduced(18, 10);

            auto layoutCompactSourceRail = [this] (juce::Rectangle<int>& area)
            {
                modSourceLabel.setVisible(true);

                auto railArea = area.removeFromTop(94);
                modSourceLabel.setBounds(railArea.removeFromTop(18).withTrimmedLeft(4));
                railArea = railArea.withTrimmedTop(4);

                constexpr auto sourceColumnCount = 5;
                constexpr auto sourceRowCount = 4;
                const auto sourceCellHeight = juce::jmax(10, railArea.getHeight() / sourceRowCount);
                for (size_t index = 0; index < modSourceRows.size(); ++index)
                {
                    modSourceRows[index].setVisible(true);
                    const auto row = static_cast<int>(index) / sourceColumnCount;
                    const auto column = static_cast<int>(index) % sourceColumnCount;
                    auto rowArea = railArea.withTrimmedTop(row * sourceCellHeight).withHeight(sourceCellHeight);
                    const auto cellWidth = rowArea.getWidth() / sourceColumnCount;
                    modSourceRows[index].setBounds(rowArea.withTrimmedLeft(column * cellWidth)
                                                          .withWidth(cellWidth)
                                                          .reduced(3, 2));
                }

                area.removeFromTop(8);
            };

            auto layoutSources = [this] (juce::Rectangle<int> area)
            {
                modSourceLabel.setVisible(true);
                modLfo2Label.setVisible(true);
                modEnvelopeLabel.setVisible(true);
                lfo2ShapeBox.setVisible(true);
                lfo2SyncRateBox.setVisible(true);
                lfo2SyncButton.setVisible(true);
                lfo2RetriggerButton.setVisible(true);

                auto sourceArea = area.removeFromLeft(juce::jlimit(420, 590, area.getWidth() / 2));
                area.removeFromLeft(12);
                auto generatorArea = area;

                modSourceLabel.setBounds(sourceArea.removeFromTop(22));
                auto sourceListArea = sourceArea.withTrimmedTop(6);
                constexpr auto sourceColumnCount = 5;
                constexpr auto sourceRowCount = 4;
                const auto sourceCellHeight = juce::jmax(1, sourceListArea.getHeight() / sourceRowCount);
                for (size_t index = 0; index < modSourceRows.size(); ++index)
                {
                    modSourceRows[index].setVisible(true);
                    const auto row = static_cast<int>(index) / sourceColumnCount;
                    const auto column = static_cast<int>(index) % sourceColumnCount;
                    auto rowArea = sourceListArea.withTrimmedTop(row * sourceCellHeight).withHeight(sourceCellHeight);
                    const auto cellWidth = rowArea.getWidth() / sourceColumnCount;
                    modSourceRows[index].setBounds(rowArea.withTrimmedLeft(column * cellWidth)
                                                           .withWidth(cellWidth)
                                                           .reduced(4, 3));
                }

                auto lfo2Area = generatorArea.removeFromTop(juce::jlimit(146, 188, generatorArea.getHeight() / 2));
                generatorArea.removeFromTop(10);
                auto envelopeArea = generatorArea;

                modLfo2Label.setBounds(lfo2Area.removeFromTop(22));
                auto lfo2ModeRow = lfo2Area.removeFromTop(36).withTrimmedTop(3);
                lfo2ShapeBox.setBounds(lfo2ModeRow.removeFromLeft(lfo2ModeRow.getWidth() / 2).reduced(4));
                lfo2SyncRateBox.setBounds(lfo2ModeRow.reduced(4));
                auto lfo2ToggleRow = lfo2Area.removeFromTop(34).withTrimmedTop(3);
                lfo2SyncButton.setBounds(lfo2ToggleRow.removeFromLeft(lfo2ToggleRow.getWidth() / 2).reduced(4));
                lfo2RetriggerButton.setBounds(lfo2ToggleRow.reduced(4));
                setSliderVisible(lfo2RateSlider, lfo2RateLabel, true);
                setSliderVisible(lfo2DepthSlider, lfo2DepthLabel, true);
                setSliderVisible(lfo2PhaseSlider, lfo2PhaseLabel, true);
                layoutKnobRow(lfo2Area.withTrimmedTop(4), { &lfo2RateSlider, &lfo2DepthSlider, &lfo2PhaseSlider });

                modEnvelopeLabel.setBounds(envelopeArea.removeFromTop(22));
                setSliderVisible(modEnv1AttackSlider, modEnv1AttackLabel, true);
                setSliderVisible(modEnv1DecaySlider, modEnv1DecayLabel, true);
                setSliderVisible(modEnv1SustainSlider, modEnv1SustainLabel, true);
                setSliderVisible(modEnv1ReleaseSlider, modEnv1ReleaseLabel, true);
                setSliderVisible(modEnv1DepthSlider, modEnv1DepthLabel, true);
                layoutKnobRow(envelopeArea.removeFromTop(76).withTrimmedTop(4), {
                    &modEnv1AttackSlider,
                    &modEnv1DecaySlider,
                    &modEnv1SustainSlider
                });
                layoutKnobRow(envelopeArea.removeFromTop(76).withTrimmedTop(4), {
                    &modEnv1ReleaseSlider,
                    &modEnv1DepthSlider
                });
            };

            auto layoutMacros = [this, &layoutCompactSourceRail] (juce::Rectangle<int> area)
            {
                layoutCompactSourceRail(area);
                modMacroLabel.setVisible(true);
                modMacroAssignLabel.setVisible(true);
                modMacroAssignStatusLabel.setVisible(true);
                macroAssignmentPad.setVisible(true);
                macroPerformanceMap.setVisible(true);
                modMacroExpandButton.setVisible(true);

                auto macroHeader = area.removeFromTop(26);
                modMacroLabel.setBounds(macroHeader.removeFromLeft(78));
                modMacroAssignLabel.setBounds(macroHeader.removeFromLeft(66));
                modMacroExpandButton.setBounds(macroHeader.removeFromRight(30).reduced(3, 2));
                modMacroAssignStatusLabel.setBounds(macroHeader.reduced(4, 1));

                area.removeFromTop(8);
                const auto mapWidth = juce::jlimit(250, 360, area.getWidth() / 3);
                macroPerformanceMap.setBounds(area.removeFromLeft(mapWidth).reduced(4));
                area.removeFromLeft(10);
                macroAssignmentPad.setBounds(area.reduced(4));
            };

            auto layoutCurves = [this, &layoutCompactSourceRail] (juce::Rectangle<int> area)
            {
                layoutCompactSourceRail(area);
                modLfoLabel.setVisible(true);
                lfo1ShapeBox.setVisible(true);
                lfo1SyncRateBox.setVisible(true);
                lfoCurvePresetBox.setVisible(true);
                lfoCurveActionBox.setVisible(true);
                lfo1SyncButton.setVisible(true);
                lfo1RetriggerButton.setVisible(true);
                lfoCurveDisplay.setVisible(true);

                modLfoLabel.setBounds(area.removeFromTop(22));
                auto controlRow = area.removeFromTop(38).withTrimmedTop(3);
                lfo1ShapeBox.setBounds(controlRow.removeFromLeft(juce::jlimit(150, 220, controlRow.getWidth() / 4)).reduced(4));
                lfo1SyncRateBox.setBounds(controlRow.removeFromLeft(juce::jlimit(110, 160, controlRow.getWidth() / 4)).reduced(4));
                lfoCurvePresetBox.setBounds(controlRow.removeFromLeft(juce::jlimit(150, 230, controlRow.getWidth() / 3)).reduced(4));
                lfoCurveActionBox.setBounds(controlRow.removeFromLeft(juce::jlimit(130, 180, controlRow.getWidth() / 2)).reduced(4));
                lfo1SyncButton.setBounds(controlRow.removeFromLeft(82).reduced(4));
                lfo1RetriggerButton.setBounds(controlRow.removeFromLeft(88).reduced(4));

                setSliderVisible(lfo1RateSlider, lfo1RateLabel, true);
                setSliderVisible(lfo1DepthSlider, lfo1DepthLabel, true);
                setSliderVisible(lfo1PhaseSlider, lfo1PhaseLabel, true);
                const auto knobHeight = juce::jlimit(62, 84, area.getHeight() / 4);
                lfoCurveDisplay.setBounds(area.removeFromTop(area.getHeight() - knobHeight).withTrimmedTop(4));
                layoutKnobRow(area.removeFromTop(knobHeight).withTrimmedTop(6), { &lfo1RateSlider, &lfo1DepthSlider, &lfo1PhaseSlider });
            };

            auto layoutMatrix = [this] (juce::Rectangle<int> matrixArea)
            {
                modMatrixLabel.setVisible(true);
                modMatrixStatusLabel.setVisible(true);
                modInspectorLabel.setVisible(true);
                modInspectorDestinationBox.setVisible(true);
                modInspectorSourceBox.setVisible(true);
                modInspectorStatusLabel.setVisible(true);
                modInspectorAddButton.setVisible(true);
                modInspectorClearButton.setVisible(true);
                modMatrixSourceHeader.setVisible(true);
                modMatrixDestinationHeader.setVisible(true);
                modMatrixAmountHeader.setVisible(true);
                modMatrixSourceHeaderB.setVisible(false);
                modMatrixDestinationHeaderB.setVisible(false);
                modMatrixAmountHeaderB.setVisible(false);

            const auto canShowRouteMap = matrixArea.getHeight() >= modMatrixTitleHeight
                                                                + modMatrixInspectorHeight
                                                                + modMatrixRouteMapHeight
                                                                + modMatrixHeaderHeight
                                                                + (modMatrixRouteMapMinimumRowHeight * static_cast<int>(modMatrixRows.size()));
            auto matrixTitleRow = matrixArea.removeFromTop(modMatrixTitleHeight);
            modMatrixLabel.setBounds(matrixTitleRow.removeFromLeft(70).withTrimmedTop(5));
            modMatrixStatusLabel.setBounds(matrixTitleRow.removeFromLeft(150).reduced(3, 5));
            modInspectorStatusLabel.setBounds(matrixTitleRow.reduced(5, 5));

            auto inspectorRow = matrixArea.removeFromTop(modMatrixInspectorHeight);
            modInspectorLabel.setBounds(inspectorRow.removeFromLeft(62).withTrimmedTop(6));
            modInspectorDestinationBox.setBounds(inspectorRow.removeFromLeft(150).reduced(3, 4));
            modInspectorSourceBox.setBounds(inspectorRow.removeFromLeft(138).reduced(3, 4));
            modInspectorAddButton.setBounds(inspectorRow.removeFromLeft(58).reduced(3, 4));
            modInspectorClearButton.setBounds(inspectorRow.removeFromLeft(66).reduced(3, 4));

            if (canShowRouteMap)
            {
                auto routeMapRow = matrixArea.removeFromTop(modMatrixRouteMapHeight).withTrimmedTop(4);
                modRouteMapDisplay.setVisible(true);
                modRouteMapDisplay.setBounds(routeMapRow.reduced(3, 2));
            }
            else
            {
                modRouteMapDisplay.setVisible(false);
            }

            auto matrixHeaderRow = matrixArea.removeFromTop(modMatrixHeaderHeight).reduced(3, 1);
            auto placeHeader = [] (juce::Rectangle<int> header,
                                   juce::Label& source,
                                   juce::Label& destination,
                                   juce::Label& amount)
            {
                constexpr auto slotWidth = 26;
                const auto actionWidth = juce::jlimit(70, 78, header.getWidth() / 6);
                const auto fieldAreaWidth = juce::jmax(1, header.getWidth() - slotWidth - actionWidth);
                const auto sourceWidth = juce::jlimit(140, 190, fieldAreaWidth / 4);
                const auto destinationWidth = juce::jlimit(170, 240, fieldAreaWidth / 3);

                header.removeFromLeft(slotWidth);
                header.removeFromRight(actionWidth);
                source.setBounds(header.removeFromLeft(sourceWidth).reduced(5, 0));
                destination.setBounds(header.removeFromLeft(destinationWidth).reduced(5, 0));
                amount.setBounds(header.reduced(5, 0));
            };
            placeHeader(matrixHeaderRow, modMatrixSourceHeader, modMatrixDestinationHeader, modMatrixAmountHeader);

            const auto rowsPerBankToShow = juce::jlimit(1,
                                                        static_cast<int>(modMatrixRows.size()),
                                                        matrixArea.getHeight() / modMatrixMinimumRowHeight);
            auto setRouteVisible = [this] (size_t index, bool shouldBeVisible)
            {
                modMatrixRows[index].setVisible(shouldBeVisible);
                modSlotRows[index].setVisible(shouldBeVisible);
                modSourceBoxes[index].setVisible(shouldBeVisible);
                modDestinationBoxes[index].setVisible(shouldBeVisible);
                modAmountSliders[index].setVisible(shouldBeVisible);
                modSlotEnabledButtons[index].setVisible(shouldBeVisible);
                modSlotDuplicateButtons[index].setVisible(shouldBeVisible);
                modSlotDeleteButtons[index].setVisible(shouldBeVisible);
            };
            for (size_t index = 0; index < modSlotRows.size(); ++index)
                setRouteVisible(index, static_cast<int>(index) < rowsPerBankToShow);

            auto placeRouteRow = [this] (size_t index, juce::Rectangle<int>& bank, int rowsRemaining)
            {
                const auto rowHeight = juce::jmax(modMatrixMinimumRowHeight, bank.getHeight() / rowsRemaining);
                auto rowBounds = bank.removeFromTop(rowHeight).reduced(3, 1);
                modMatrixRows[index].setBounds(rowBounds);

                auto row = rowBounds.reduced(2, 2);
                constexpr auto slotWidth = 26;
                const auto actionWidth = juce::jlimit(70, 78, row.getWidth() / 6);
                const auto fieldAreaWidth = juce::jmax(1, row.getWidth() - slotWidth - actionWidth);
                const auto sourceWidth = juce::jlimit(140, 190, fieldAreaWidth / 4);
                const auto destinationWidth = juce::jlimit(170, 240, fieldAreaWidth / 3);

                modSlotRows[index].setBounds(row.removeFromLeft(slotWidth).reduced(2, 0));
                modSourceBoxes[index].setBounds(row.removeFromLeft(sourceWidth).reduced(3, 0));
                modDestinationBoxes[index].setBounds(row.removeFromLeft(destinationWidth).reduced(3, 0));
                auto actionArea = row.removeFromRight(actionWidth);
                modSlotEnabledButtons[index].setBounds(actionArea.removeFromLeft(30).reduced(1, 0));
                modSlotDuplicateButtons[index].setBounds(actionArea.removeFromLeft(22).reduced(1, 0));
                modSlotDeleteButtons[index].setBounds(actionArea.reduced(1, 0));
                modAmountSliders[index].setBounds(row.reduced(3, 0));
            };

            for (auto row = 0; row < rowsPerBankToShow; ++row)
                placeRouteRow(static_cast<size_t>(row), matrixArea, rowsPerBankToShow - row);
            };

            switch (activeModWorkflowPage)
            {
                case ModWorkflowPage::sources: layoutSources(modContent); break;
                case ModWorkflowPage::macros: layoutMacros(modContent); break;
                case ModWorkflowPage::curves: layoutCurves(modContent); break;
                case ModWorkflowPage::matrix:
                default: layoutMatrix(modContent); break;
            }

            break;
        }

        case Panel::sample:
        {
            sampleSectionLabel.setVisible(true);
            sampleSourceLabel.setVisible(true);
            sampleChopLabel.setVisible(true);
            sampleShapeLabel.setVisible(true);
            loadSampleButton.setVisible(true);
            clearSampleButton.setVisible(true);
            sampleRecorderPanel.setVisible(true);
            randomCutButton.setVisible(true);
            ukgChopButton.setVisible(true);
            sampleEnabledButton.setVisible(true);
            sampleReverseButton.setVisible(true);
            sampleModeBox.setVisible(true);
            sampleEngineBox.setVisible(true);
            sampleSliceStyleBox.setVisible(true);
            sampleStutterEnabledButton.setVisible(true);
            sampleStutterRateBox.setVisible(true);
            sampleChopExpandButton.setVisible(true);
            sampleWaveformDisplay.setVisible(true);
            sampleChopPanel.setVisible(true);
            sampleNameLabel.setVisible(true);
            sampleSectionLabel.setBounds(content.removeFromTop(28));
            setSliderVisible(sampleStartSlider, sampleStartLabel, true);
            setSliderVisible(sampleEndSlider, sampleEndLabel, true);
            setSliderVisible(sampleTransposeSlider, sampleTransposeLabel, true);
            setSliderVisible(samplePitchRampSlider, samplePitchRampLabel, true);
            setSliderVisible(sampleGainSlider, sampleGainLabel, true);
            setSliderVisible(sampleMixSlider, sampleMixLabel, true);
            setSliderVisible(sampleStutterRepeatsSlider, sampleStutterRepeatsLabel, true);
            setSliderVisible(sampleGrainSizeSlider, sampleGrainSizeLabel, true);
            setSliderVisible(sampleGrainSpraySlider, sampleGrainSprayLabel, true);
            setSliderVisible(sampleSpectralFreezeSlider, sampleSpectralFreezeLabel, true);

            auto workspace = content.withTrimmedTop(8);
            auto sourceArea = workspace.removeFromLeft(juce::jlimit(260, 310, workspace.getWidth() / 4)).reduced(18, 12);
            workspace.removeFromLeft(10);
            auto chopArea = workspace.reduced(18, 12);

            sampleSourceLabel.setBounds(sourceArea.removeFromTop(22));
            auto actionTop = sourceArea.removeFromTop(30);
            loadSampleButton.setBounds(actionTop.removeFromLeft(actionTop.getWidth() / 2).reduced(3, 4));
            clearSampleButton.setBounds(actionTop.reduced(3, 4));
            auto actionBottom = sourceArea.removeFromTop(30);
            sampleEnabledButton.setBounds(actionBottom.removeFromLeft(actionBottom.getWidth() / 2).reduced(3, 4));
            sampleReverseButton.setBounds(actionBottom.reduced(3, 4));

            sampleNameLabel.setBounds(sourceArea.removeFromTop(24).reduced(5, 3));
            sampleRecorderPanel.setBounds(sourceArea.removeFromTop(sampleRecorderPanel.preferredHeight()));

            auto sampleModeRow = sourceArea.removeFromTop(30);
            sampleModeBox.setBounds(sampleModeRow.removeFromLeft(sampleModeRow.getWidth() / 2).reduced(4));
            sampleEngineBox.setBounds(sampleModeRow.reduced(4));
            sampleSliceStyleBox.setBounds(sourceArea.removeFromTop(30).reduced(4));
            auto stutterRow = sourceArea.removeFromTop(30);
            sampleStutterEnabledButton.setBounds(stutterRow.removeFromLeft(stutterRow.getWidth() / 2).reduced(3, 4));
            sampleStutterRateBox.setBounds(stutterRow.reduced(3, 4));
            auto cutRecipeRow = sourceArea.removeFromTop(32).withTrimmedTop(2);
            randomCutButton.setBounds(cutRecipeRow.removeFromLeft(cutRecipeRow.getWidth() / 2).reduced(3, 4));
            ukgChopButton.setBounds(cutRecipeRow.reduced(3, 4));

            sampleShapeLabel.setBounds(sourceArea.removeFromTop(18).withTrimmedLeft(4));
            if (sourceArea.getHeight() < 138)
            {
                const auto compactShapeHeight = sourceArea.getHeight();
                const auto showSecondaryShapeRow = compactShapeHeight >= 72;
                const auto sampleShapeRowHeight = showSecondaryShapeRow ? juce::jmax(36, compactShapeHeight / 2)
                                                                        : juce::jmax(36, compactShapeHeight);
                layoutKnobRow(sourceArea.removeFromTop(juce::jmin(sampleShapeRowHeight, sourceArea.getHeight())).withTrimmedTop(2),
                              { &sampleTransposeSlider, &sampleGainSlider, &sampleMixSlider, &samplePitchRampSlider });
                if (showSecondaryShapeRow)
                {
                    layoutKnobRow(sourceArea.withTrimmedTop(2),
                                  { &sampleStutterRepeatsSlider, &sampleGrainSizeSlider, &sampleGrainSpraySlider, &sampleSpectralFreezeSlider });
                }
                else
                {
                    setSliderVisible(sampleStutterRepeatsSlider, sampleStutterRepeatsLabel, false);
                    setSliderVisible(sampleGrainSizeSlider, sampleGrainSizeLabel, false);
                    setSliderVisible(sampleGrainSpraySlider, sampleGrainSprayLabel, false);
                    setSliderVisible(sampleSpectralFreezeSlider, sampleSpectralFreezeLabel, false);
                }
            }
            else
            {
                const auto sampleShapeRowHeight = juce::jlimit(46, 64, sourceArea.getHeight() / 3);
                layoutKnobRow(sourceArea.removeFromTop(sampleShapeRowHeight).withTrimmedTop(3),
                              { &sampleTransposeSlider, &sampleGainSlider, &sampleMixSlider });
                layoutKnobRow(sourceArea.removeFromTop(sampleShapeRowHeight).withTrimmedTop(3),
                              { &samplePitchRampSlider, &sampleStutterRepeatsSlider, &sampleGrainSizeSlider });
                layoutKnobRow(sourceArea.removeFromTop(juce::jmin(sampleShapeRowHeight, sourceArea.getHeight())).withTrimmedTop(3),
                              { &sampleGrainSpraySlider, &sampleSpectralFreezeSlider });
            }

            auto chopHeader = chopArea.removeFromTop(24);
            sampleChopExpandButton.setBounds(chopHeader.removeFromRight(30).reduced(3, 1));
            sampleChopLabel.setBounds(chopHeader.withTrimmedLeft(4));
            const auto waveformHeight = juce::jlimit(170, 260, chopArea.getHeight() / 2);
            sampleWaveformDisplay.setBounds(chopArea.removeFromTop(waveformHeight).reduced(4, 6));
            sampleChopPanel.setBounds(chopArea.removeFromTop(sampleChopPanel.compactHeight()));
            auto cutRow = chopArea.removeFromTop(54).withTrimmedTop(6);
            sampleStartSlider.setBounds(cutRow.removeFromLeft(cutRow.getWidth() / 2).reduced(48, 6));
            sampleEndSlider.setBounds(cutRow.reduced(48, 6));
            break;
        }

        case Panel::sequencer:
        {
            sequencerSectionLabel.setVisible(true);
            sequencerEnabledButton.setVisible(true);
            hostSyncStatusLabel.setVisible(true);
            rateEighthButton.setVisible(true);
            rateSixteenthButton.setVisible(true);
            rateThirtySecondButton.setVisible(true);
            sequencerGrooveBox.setVisible(true);
            sequencerScaleBox.setVisible(true);
            sequencerChordBox.setVisible(true);
            sequencerVoicingBox.setVisible(true);
            sequencerChordMemoryButton.setVisible(true);
            sequencerPatternBox.setVisible(true);
            sequencerGrooveTransformBox.setVisible(true);
            sequencerLaneViewBox.setVisible(true);
            sequencerLockDestinationBox.setVisible(true);
            sequencerRootDownButton.setVisible(true);
            sequencerRootUpButton.setVisible(true);
            sequencerRootValueLabel.setVisible(true);
            sequencerStepEditorLabel.setVisible(true);
            for (auto& button : sequencerStepEditorButtons)
                button.setVisible(true);
            applyPatternButton.setVisible(true);
            copySequencerButton.setVisible(true);
            rotateSequencerLeftButton.setVisible(true);
            rotateSequencerRightButton.setVisible(true);
            exportSequencerMidiButton.setVisible(true);
            exportSequencerChainButton.setVisible(true);
            sequencerSceneChainLiveButton.setVisible(true);
            applyGrooveTransformButton.setVisible(true);
            randomSequencerButton.setVisible(true);
            mutateSequencerButton.setVisible(true);
            undoSequencerButton.setVisible(true);
            clearSequencerButton.setVisible(true);
            for (auto& button : sequencerSceneRecallButtons)
                button.setVisible(true);
            for (auto& button : sequencerSceneCaptureButtons)
                button.setVisible(true);
            sequencerGrid.setVisible(true);
            sequencerExpandButton.setVisible(true);
            auto sequencerHeader = content.removeFromTop(28);
            sequencerExpandButton.setBounds(sequencerHeader.removeFromRight(30).reduced(3, 1));
            sequencerSectionLabel.setBounds(sequencerHeader);
            auto workspace = content.withTrimmedTop(8);
            auto controlArea = workspace.removeFromLeft(juce::jlimit(228, 286, workspace.getWidth() / 5)).reduced(18, 12);
            workspace.removeFromLeft(10);
            auto gridArea = workspace.reduced(18, 12);
            const auto showAdvancedSequencerControls = controlArea.getHeight() >= 570;

            auto timingRow = controlArea.removeFromTop(34);
            sequencerEnabledButton.setBounds(timingRow.removeFromLeft(62).reduced(4));
            hostSyncStatusLabel.setBounds(timingRow.reduced(4));

            auto rateRow = controlArea.removeFromTop(34);
            const auto rateButtonWidth = rateRow.getWidth() / 3;
            rateEighthButton.setBounds(rateRow.removeFromLeft(rateButtonWidth).reduced(3, 4));
            rateSixteenthButton.setBounds(rateRow.removeFromLeft(rateButtonWidth).reduced(3, 4));
            rateThirtySecondButton.setBounds(rateRow.reduced(3, 4));

            auto grooveRow = controlArea.removeFromTop(34);
            sequencerGrooveBox.setBounds(grooveRow.removeFromLeft(grooveRow.getWidth() / 2).reduced(4));
            sequencerScaleBox.setBounds(grooveRow.reduced(4));
            auto harmonyRow = controlArea.removeFromTop(34);
            sequencerChordBox.setBounds(harmonyRow.removeFromLeft(harmonyRow.getWidth() / 2).reduced(4));
            sequencerVoicingBox.setBounds(harmonyRow.reduced(4));

            auto patternRow = controlArea.removeFromTop(34);
            sequencerChordMemoryButton.setBounds(patternRow.removeFromLeft(86).reduced(4));
            sequencerPatternBox.setBounds(patternRow.reduced(4));
            auto generateRow = controlArea.removeFromTop(34);
            applyPatternButton.setBounds(generateRow.removeFromLeft(generateRow.getWidth() / 2).reduced(4));
            randomSequencerButton.setBounds(generateRow.reduced(4));
            auto editRow = controlArea.removeFromTop(34);
            mutateSequencerButton.setBounds(editRow.removeFromLeft(editRow.getWidth() / 3).reduced(4));
            undoSequencerButton.setBounds(editRow.removeFromLeft(editRow.getWidth() / 2).reduced(4));
            clearSequencerButton.setBounds(editRow.reduced(4));

            auto rootStepperRow = controlArea.removeFromTop(34).withTrimmedTop(2);
            sequencerRootDownButton.setBounds(rootStepperRow.removeFromLeft(38).reduced(4));
            sequencerRootUpButton.setBounds(rootStepperRow.removeFromRight(38).reduced(4));
            sequencerRootValueLabel.setBounds(rootStepperRow.reduced(4));
            updateSequencerRootStepper();

            auto laneViewRow = controlArea.removeFromTop(34).withTrimmedTop(2);
            sequencerLaneViewBox.setBounds(laneViewRow.reduced(4));

            auto stepEditorArea = controlArea.removeFromTop(104).withTrimmedTop(4);
            sequencerStepEditorLabel.setBounds(stepEditorArea.removeFromTop(24).reduced(4, 2));
            auto stepPitchRow = stepEditorArea.removeFromTop(26);
            for (auto index = 0; index < 4; ++index)
                sequencerStepEditorButtons[static_cast<size_t>(index)].setBounds(stepPitchRow.removeFromLeft(stepPitchRow.getWidth() / (4 - index)).reduced(3, 3));
            auto stepValueRow = stepEditorArea.removeFromTop(26);
            for (auto index = 4; index < 10; ++index)
                sequencerStepEditorButtons[static_cast<size_t>(index)].setBounds(stepValueRow.removeFromLeft(stepValueRow.getWidth() / (10 - index)).reduced(3, 3));
            auto stepFlagRow = stepEditorArea.removeFromTop(26);
            for (auto index = 10; index < 13; ++index)
                sequencerStepEditorButtons[static_cast<size_t>(index)].setBounds(stepFlagRow.removeFromLeft(stepFlagRow.getWidth() / (13 - index)).reduced(3, 3));
            updateSequencerStepEditor();

            auto transformRow = controlArea.removeFromTop(34).withTrimmedTop(2);
            sequencerLockDestinationBox.setBounds(transformRow.removeFromLeft(transformRow.getWidth() / 2).reduced(4));
            sequencerGrooveTransformBox.setBounds(transformRow.reduced(4));
            applyGrooveTransformButton.setBounds(controlArea.removeFromTop(32).reduced(4));

            if (showAdvancedSequencerControls)
            {
                auto sceneArea = controlArea.removeFromTop(68).withTrimmedTop(2);
                auto sceneTopRow = sceneArea.removeFromTop(32);
                auto sceneBottomRow = sceneArea.withTrimmedTop(2);
                auto placeSceneSlot = [this] (size_t index, juce::Rectangle<int>& row, int slotsRemaining)
                {
                    auto slotArea = row.removeFromLeft(row.getWidth() / slotsRemaining);
                    sequencerSceneRecallButtons[index].setBounds(slotArea.removeFromLeft(slotArea.getWidth() / 2).reduced(3, 4));
                    sequencerSceneCaptureButtons[index].setBounds(slotArea.reduced(3, 4));
                };
                placeSceneSlot(0, sceneTopRow, 2);
                placeSceneSlot(1, sceneTopRow, 1);
                placeSceneSlot(2, sceneBottomRow, 2);
                placeSceneSlot(3, sceneBottomRow, 1);

                auto utilityRow = controlArea.removeFromTop(34).withTrimmedTop(2);
                copySequencerButton.setBounds(utilityRow.removeFromLeft(utilityRow.getWidth() / 3).reduced(4));
                rotateSequencerLeftButton.setBounds(utilityRow.removeFromLeft(utilityRow.getWidth() / 2).reduced(4));
                rotateSequencerRightButton.setBounds(utilityRow.reduced(4));
                auto exportRow = controlArea.removeFromTop(34).withTrimmedTop(2);
                exportSequencerMidiButton.setBounds(exportRow.removeFromLeft(exportRow.getWidth() / 3).reduced(4));
                exportSequencerChainButton.setBounds(exportRow.removeFromLeft(exportRow.getWidth() / 2).reduced(4));
                sequencerSceneChainLiveButton.setBounds(exportRow.reduced(4));
            }
            else
            {
                copySequencerButton.setVisible(false);
                rotateSequencerLeftButton.setVisible(false);
                rotateSequencerRightButton.setVisible(false);
                exportSequencerMidiButton.setVisible(false);
                exportSequencerChainButton.setVisible(false);
                sequencerSceneChainLiveButton.setVisible(false);
                for (auto& button : sequencerSceneRecallButtons)
                    button.setVisible(false);
                for (auto& button : sequencerSceneCaptureButtons)
                    button.setVisible(false);
            }

            setSliderVisible(sequencerRootSlider, sequencerRootLabel, false);
            setSliderVisible(sequencerGateSlider, sequencerGateLabel, true);
            setSliderVisible(sequencerSwingSlider, sequencerSwingLabel, true);
            setSliderVisible(sequencerChordStrumSlider, sequencerChordStrumLabel, true);
            setSliderVisible(sequencerAccentSlider, sequencerAccentLabel, true);
            setSliderVisible(sequencerOctaveSlider, sequencerOctaveLabel, true);
            setSliderVisible(sequencerProbabilitySlider, sequencerProbabilityLabel, true);
            setSliderVisible(sequencerRandomSlider, sequencerRandomLabel, true);
            setSliderVisible(sequencerLockDepthSlider, sequencerLockDepthLabel, true);
            const auto showPrimarySequencerKnobs = controlArea.getHeight() >= 58;
            setSliderVisible(sequencerGateSlider, sequencerGateLabel, showPrimarySequencerKnobs);
            setSliderVisible(sequencerSwingSlider, sequencerSwingLabel, showPrimarySequencerKnobs);
            setSliderVisible(sequencerChordStrumSlider, sequencerChordStrumLabel, showPrimarySequencerKnobs);
            if (showPrimarySequencerKnobs)
            {
                const auto primarySequencerKnobHeight = juce::jlimit(58, 72, controlArea.getHeight());
                layoutKnobRow(controlArea.removeFromTop(primarySequencerKnobHeight).withTrimmedTop(4), {
                    &sequencerGateSlider,
                    &sequencerSwingSlider,
                    &sequencerChordStrumSlider
                });
            }

            const auto showSecondarySequencerKnobs = controlArea.getHeight() >= 58;
            setSliderVisible(sequencerAccentSlider, sequencerAccentLabel, showSecondarySequencerKnobs);
            setSliderVisible(sequencerOctaveSlider, sequencerOctaveLabel, showSecondarySequencerKnobs);
            setSliderVisible(sequencerProbabilitySlider, sequencerProbabilityLabel, showSecondarySequencerKnobs);
            setSliderVisible(sequencerRandomSlider, sequencerRandomLabel, showSecondarySequencerKnobs);
            setSliderVisible(sequencerLockDepthSlider, sequencerLockDepthLabel, showSecondarySequencerKnobs);
            if (showSecondarySequencerKnobs)
            {
                layoutKnobRow(controlArea.withTrimmedTop(4), {
                    &sequencerAccentSlider,
                    &sequencerOctaveSlider,
                    &sequencerProbabilitySlider,
                    &sequencerRandomSlider,
                    &sequencerLockDepthSlider
                });
            }
            sequencerGrid.setBounds(gridArea.reduced(4, 8));
            break;
        }

        case Panel::effects:
        {
            futureSectionLabel.setVisible(true);
            futureSectionLabel.setBounds(content.removeFromTop(28));
            updateFxRackControls();

            auto actionRow = content.removeFromTop(44);
            fxAddBox.setVisible(true);
            fxMoveUpButton.setVisible(true);
            fxMoveDownButton.setVisible(true);
            fxResetOrderButton.setVisible(true);
            fxRemoveButton.setVisible(true);
            fxThrowDelayButton.setVisible(true);
            fxThrowSpaceButton.setVisible(true);
            fxThrowPumpButton.setVisible(true);
            fxThrowDryButton.setVisible(true);
            fxHoldDelayButton.setVisible(true);
            fxHoldSpaceButton.setVisible(true);
            fxHoldPumpButton.setVisible(true);
            fxMuteDropButton.setVisible(true);
            fxRackStatusLabel.setVisible(true);
            fxPresetBox.setVisible(true);
            fxApplyPresetButton.setVisible(true);
            hostSyncStatusLabel.setVisible(true);
            fxAddBox.setBounds(actionRow.removeFromLeft(160).reduced(4));
            fxMoveUpButton.setBounds(actionRow.removeFromLeft(52).reduced(4));
            fxMoveDownButton.setBounds(actionRow.removeFromLeft(58).reduced(4));
            fxResetOrderButton.setBounds(actionRow.removeFromLeft(72).reduced(4));
            fxRemoveButton.setBounds(actionRow.removeFromLeft(86).reduced(4));
            hostSyncStatusLabel.setBounds(actionRow.removeFromRight(126).reduced(4));
            fxRackStatusLabel.setBounds(actionRow.reduced(8, 4));

            auto performRow = content.removeFromTop(42).withTrimmedTop(2);
            fxThrowDelayButton.setBounds(performRow.removeFromLeft(102).reduced(4));
            fxThrowSpaceButton.setBounds(performRow.removeFromLeft(106).reduced(4));
            fxThrowPumpButton.setBounds(performRow.removeFromLeft(96).reduced(4));
            fxThrowDryButton.setBounds(performRow.removeFromLeft(88).reduced(4));
            fxHoldDelayButton.setBounds(performRow.removeFromLeft(84).reduced(4));
            fxHoldSpaceButton.setBounds(performRow.removeFromLeft(84).reduced(4));
            fxHoldPumpButton.setBounds(performRow.removeFromLeft(92).reduced(4));
            fxMuteDropButton.setBounds(performRow.removeFromLeft(90).reduced(4));

            content.removeFromTop(8);
            auto rackArea = content.removeFromLeft(260).reduced(18, 14);
            rackArea.removeFromTop(26);
            auto detailArea = content.reduced(24, 16);
            detailArea.removeFromTop(30);

            std::array<UI::FxRackRow*, 15> visibleFxSlots {};
            auto visibleFxSlotCount = 0;

            const auto moduleOrder = fxModuleOrder();
            for (const auto module : moduleOrder)
            {
                auto& slotButton = fxSlotButton(module);
                const auto isVisible = shouldShowFxModule(module);
                slotButton.setVisible(isVisible);

                if (isVisible)
                    visibleFxSlots[static_cast<size_t>(visibleFxSlotCount++)] = &slotButton;
            }

            const auto useTwoColumnRack = visibleFxSlotCount > 10;
            const auto slotGap = useTwoColumnRack ? 4 : 6;
            const auto columnGap = useTwoColumnRack ? 6 : 0;
            const auto rackColumnCount = useTwoColumnRack ? 2 : 1;
            const auto rackRowCount = visibleFxSlotCount > 0
                ? (visibleFxSlotCount + rackColumnCount - 1) / rackColumnCount
                : 0;
            const auto slotHeight = rackRowCount > 0
                ? juce::jlimit(24,
                               34,
                               (rackArea.getHeight() - ((rackRowCount - 1) * slotGap)) / rackRowCount)
                : 34;
            const auto slotWidth = (rackArea.getWidth() - columnGap) / rackColumnCount;

            for (auto index = 0; index < visibleFxSlotCount; ++index)
            {
                const auto column = useTwoColumnRack ? index / rackRowCount : 0;
                const auto row = useTwoColumnRack ? index % rackRowCount : index;
                const auto x = rackArea.getX() + (column * (slotWidth + columnGap));
                const auto y = rackArea.getY() + (row * (slotHeight + slotGap));
                visibleFxSlots[static_cast<size_t>(index)]->setBounds(x, y, slotWidth, slotHeight);
            }

            auto detailHeader = detailArea.removeFromTop(38);
            fxApplyPresetButton.setBounds(detailHeader.removeFromRight(62).reduced(3, 4));
            fxPresetBox.setBounds(detailHeader.removeFromRight(156).reduced(3, 4));
            auto controlsArea = detailArea.withTrimmedTop(16);

            switch (selectedFxModule)
            {
                case FxModule::tone:
                    fxToneEnabledButton.setVisible(true);
                    fxToneEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxToneTiltSlider, fxToneTiltLabel, true);
                    setSliderVisible(fxToneLowCutSlider, fxToneLowCutLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxToneTiltSlider, &fxToneLowCutSlider });
                    break;

                case FxModule::eq:
                    fxEqEnabledButton.setVisible(true);
                    fxEqEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxEqLowGainSlider, fxEqLowGainLabel, true);
                    setSliderVisible(fxEqMidGainSlider, fxEqMidGainLabel, true);
                    setSliderVisible(fxEqHighGainSlider, fxEqHighGainLabel, true);
                    setSliderVisible(fxEqTrimSlider, fxEqTrimLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), {
                        &fxEqLowGainSlider,
                        &fxEqMidGainSlider,
                        &fxEqHighGainSlider,
                        &fxEqTrimSlider
                    });
                    break;

                case FxModule::distortion:
                    fxDistortionEnabledButton.setVisible(true);
                    fxDistortionEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxDistortionAmountSlider, fxDistortionAmountLabel, true);
                    setSliderVisible(fxDistortionBassSafeSlider, fxDistortionBassSafeLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), {
                        &fxDistortionAmountSlider,
                        &fxDistortionBassSafeSlider
                    });
                    break;

                case FxModule::bitcrush:
                    fxBitcrushEnabledButton.setVisible(true);
                    fxBitcrushEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxBitcrushBitsSlider, fxBitcrushBitsLabel, true);
                    setSliderVisible(fxBitcrushDownsampleSlider, fxBitcrushDownsampleLabel, true);
                    setSliderVisible(fxBitcrushMixSlider, fxBitcrushMixLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxBitcrushBitsSlider, &fxBitcrushDownsampleSlider, &fxBitcrushMixSlider });
                    break;

                case FxModule::pump:
                    fxPumpEnabledButton.setVisible(true);
                    fxPumpRateBox.setVisible(true);
                    fxPumpCurveBox.setVisible(true);
                    pumpCurveDisplay.setVisible(true);
                    fxPumpEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    fxPumpRateBox.setBounds(detailHeader.removeFromLeft(96).reduced(3, 4));
                    fxPumpCurveBox.setBounds(detailHeader.removeFromLeft(118).reduced(3, 4));
                    pumpCurveDisplay.setBounds(controlsArea.removeFromTop(86).reduced(4, 2));
                    setSliderVisible(fxPumpDepthSlider, fxPumpDepthLabel, true);
                    setSliderVisible(fxPumpShapeSlider, fxPumpShapeLabel, true);
                    setSliderVisible(fxPumpPhaseSlider, fxPumpPhaseLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(128).withTrimmedTop(8), { &fxPumpDepthSlider, &fxPumpShapeSlider, &fxPumpPhaseSlider });
                    break;

                case FxModule::tremolo:
                    fxTremoloEnabledButton.setVisible(true);
                    fxTremoloRateBox.setVisible(true);
                    fxTremoloEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    fxTremoloRateBox.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxTremoloDepthSlider, fxTremoloDepthLabel, true);
                    setSliderVisible(fxTremoloPanSlider, fxTremoloPanLabel, true);
                    setSliderVisible(fxTremoloShapeSlider, fxTremoloShapeLabel, true);
                    setSliderVisible(fxTremoloPhaseSlider, fxTremoloPhaseLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), {
                        &fxTremoloDepthSlider,
                        &fxTremoloPanSlider,
                        &fxTremoloShapeSlider,
                        &fxTremoloPhaseSlider
                    });
                    break;

                case FxModule::ring:
                    fxRingEnabledButton.setVisible(true);
                    fxRingEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxRingFrequencySlider, fxRingFrequencyLabel, true);
                    setSliderVisible(fxRingDepthSlider, fxRingDepthLabel, true);
                    setSliderVisible(fxRingMixSlider, fxRingMixLabel, true);
                    setSliderVisible(fxRingBiasSlider, fxRingBiasLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), {
                        &fxRingFrequencySlider,
                        &fxRingDepthSlider,
                        &fxRingMixSlider,
                        &fxRingBiasSlider
                    });
                    break;

                case FxModule::comb:
                    fxCombEnabledButton.setVisible(true);
                    fxCombEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxCombFrequencySlider, fxCombFrequencyLabel, true);
                    setSliderVisible(fxCombFeedbackSlider, fxCombFeedbackLabel, true);
                    setSliderVisible(fxCombDampingSlider, fxCombDampingLabel, true);
                    setSliderVisible(fxCombMixSlider, fxCombMixLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), {
                        &fxCombFrequencySlider,
                        &fxCombFeedbackSlider,
                        &fxCombDampingSlider,
                        &fxCombMixSlider
                    });
                    break;

                case FxModule::phaser:
                    fxPhaserEnabledButton.setVisible(true);
                    fxPhaserEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxPhaserRateSlider, fxPhaserRateLabel, true);
                    setSliderVisible(fxPhaserDepthSlider, fxPhaserDepthLabel, true);
                    setSliderVisible(fxPhaserMixSlider, fxPhaserMixLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxPhaserRateSlider, &fxPhaserDepthSlider, &fxPhaserMixSlider });
                    break;

                case FxModule::flanger:
                    fxFlangerEnabledButton.setVisible(true);
                    fxFlangerEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxFlangerRateSlider, fxFlangerRateLabel, true);
                    setSliderVisible(fxFlangerDepthSlider, fxFlangerDepthLabel, true);
                    setSliderVisible(fxFlangerFeedbackSlider, fxFlangerFeedbackLabel, true);
                    setSliderVisible(fxFlangerMixSlider, fxFlangerMixLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), {
                        &fxFlangerRateSlider,
                        &fxFlangerDepthSlider,
                        &fxFlangerFeedbackSlider,
                        &fxFlangerMixSlider
                    });
                    break;

                case FxModule::chorus:
                    fxChorusEnabledButton.setVisible(true);
                    fxChorusEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxChorusRateSlider, fxChorusRateLabel, true);
                    setSliderVisible(fxChorusDepthSlider, fxChorusDepthLabel, true);
                    setSliderVisible(fxChorusMixSlider, fxChorusMixLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxChorusRateSlider, &fxChorusDepthSlider, &fxChorusMixSlider });
                    break;

                case FxModule::delay:
                    fxDelayEnabledButton.setVisible(true);
                    fxDelaySyncButton.setVisible(true);
                    fxDelayRateBox.setVisible(true);
                    fxDelayEnabledButton.setBounds(detailHeader.removeFromLeft(96).reduced(3, 4));
                    fxDelaySyncButton.setBounds(detailHeader.removeFromLeft(72).reduced(3, 4));
                    fxDelayRateBox.setBounds(detailHeader.removeFromLeft(104).reduced(3, 4));
                    setSliderVisible(fxDelayTimeSlider, fxDelayTimeLabel, true);
                    setSliderVisible(fxDelayFeedbackSlider, fxDelayFeedbackLabel, true);
                    setSliderVisible(fxDelayMixSlider, fxDelayMixLabel, true);
                    setSliderVisible(fxSendDelaySlider, fxSendDelayLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxDelayTimeSlider, &fxDelayFeedbackSlider, &fxDelayMixSlider, &fxSendDelaySlider });
                    break;

                case FxModule::reverb:
                    fxReverbEnabledButton.setVisible(true);
                    fxReverbEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxReverbSizeSlider, fxReverbSizeLabel, true);
                    setSliderVisible(fxReverbDampingSlider, fxReverbDampingLabel, true);
                    setSliderVisible(fxReverbMixSlider, fxReverbMixLabel, true);
                    setSliderVisible(fxSendReverbSlider, fxSendReverbLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxReverbSizeSlider, &fxReverbDampingSlider, &fxReverbMixSlider, &fxSendReverbSlider });
                    break;

                case FxModule::width:
                    fxWidthEnabledButton.setVisible(true);
                    fxWidthEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxWidthAmountSlider, fxWidthAmountLabel, true);
                    setSliderVisible(fxWidthMonoCutoffSlider, fxWidthMonoCutoffLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxWidthAmountSlider, &fxWidthMonoCutoffSlider });
                    break;

                case FxModule::guard:
                    fxGuardEnabledButton.setVisible(true);
                    fxGuardEnabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
                    setSliderVisible(fxGuardPushSlider, fxGuardPushLabel, true);
                    setSliderVisible(fxGuardGlueSlider, fxGuardGlueLabel, true);
                    setSliderVisible(fxGuardPunchSlider, fxGuardPunchLabel, true);
                    setSliderVisible(fxGuardClipMixSlider, fxGuardClipMixLabel, true);
                    setSliderVisible(fxGuardCeilingSlider, fxGuardCeilingLabel, true);
                    layoutKnobRow(controlsArea.removeFromTop(150), {
                        &fxGuardPushSlider,
                        &fxGuardGlueSlider,
                        &fxGuardPunchSlider,
                        &fxGuardClipMixSlider,
                        &fxGuardCeilingSlider
                    });
                    break;
            }
            break;
        }

        case Panel::library:
        {
            librarySectionLabel.setVisible(true);
            libraryFindLabel.setVisible(true);
            libraryBrowserLabel.setVisible(true);
            librarySaveLabel.setVisible(true);
            libraryInspectorLabel.setVisible(true);
            presetNameEditor.setVisible(true);
            presetCategoryBox.setVisible(true);
            presetFilterBox.setVisible(true);
            presetTagBox.setVisible(true);
            presetSortBox.setVisible(true);
            presetBrowserPackFilterBox.setVisible(true);
            presetRatingBox.setVisible(true);
            presetPackBox.setVisible(true);
            presetKeyBox.setVisible(true);
            presetBpmBox.setVisible(true);
            presetSearchEditor.setVisible(true);
            presetAuthorEditor.setVisible(true);
            presetNotesEditor.setVisible(true);
            presetNotesTemplateBox.setVisible(true);
            savePresetButton.setVisible(true);
            presetBox.setVisible(true);
            previousPresetButton.setVisible(true);
            nextPresetButton.setVisible(true);
            loadPresetButton.setVisible(true);
            auditionPresetButton.setVisible(true);
            warmPresetPreviewsButton.setVisible(true);
            favoritePresetButton.setVisible(true);
            refreshPresetsButton.setVisible(true);
            comparePresetButton.setVisible(true);
            revertPresetButton.setVisible(true);
            presetStatusLabel.setVisible(true);
            presetBrowserHeaderLabel.setVisible(true);
            presetBrowserList.setVisible(true);
            presetCrateMapDisplay.setVisible(true);
            presetLibrarySummary.setVisible(true);
            presetSaveSummary.setVisible(true);
            for (auto& button : presetQuickFilterButtons)
                button.setVisible(true);
            librarySectionLabel.setBounds(content.removeFromTop(28));
            auto libraryArea = content.withTrimmedTop(8);
            const auto leftWidth = juce::jlimit(174, 212, libraryArea.getWidth() / 6);
            const auto inspectorWidth = juce::jlimit(232, 286, libraryArea.getWidth() / 4);
            auto findArea = libraryArea.removeFromLeft(leftWidth).reduced(12, 10);
            auto inspectorArea = libraryArea.removeFromRight(inspectorWidth).reduced(12, 10);
            auto browserArea = libraryArea.reduced(12, 10);

            libraryFindLabel.setBounds(findArea.removeFromTop(24));
            presetSearchEditor.setBounds(findArea.removeFromTop(38).reduced(2, 4));
            auto quickRowA = findArea.removeFromTop(30).withTrimmedTop(2);
            auto quickRowB = findArea.removeFromTop(30).withTrimmedTop(2);
            for (size_t index = 0; index < 5; ++index)
                presetQuickFilterButtons[index].setBounds(quickRowA.removeFromLeft(quickRowA.getWidth() / static_cast<int>(5 - index)).reduced(2, 3));
            for (size_t index = 5; index < presetQuickFilterButtons.size(); ++index)
                presetQuickFilterButtons[index].setBounds(quickRowB.removeFromLeft(quickRowB.getWidth() / static_cast<int>(presetQuickFilterButtons.size() - index)).reduced(2, 3));

            findArea.removeFromTop(5);
            presetFilterBox.setBounds(findArea.removeFromTop(34).reduced(2, 4));
            presetTagBox.setBounds(findArea.removeFromTop(34).reduced(2, 4));
            presetBrowserPackFilterBox.setBounds(findArea.removeFromTop(34).reduced(2, 4));
            auto sortRefreshRow = findArea.removeFromTop(36).withTrimmedTop(2);
            refreshPresetsButton.setBounds(sortRefreshRow.removeFromRight(76).reduced(2, 4));
            presetSortBox.setBounds(sortRefreshRow.reduced(2, 4));

            const auto crateMapHeight = juce::jlimit(96, 132, findArea.getHeight() - 36);
            presetCrateMapDisplay.setBounds(findArea.removeFromTop(crateMapHeight).reduced(2, 5));
            presetStatusLabel.setBounds(findArea.reduced(2, 4));

            libraryBrowserLabel.setBounds(browserArea.removeFromTop(24));
            auto selectedRow = browserArea.removeFromTop(38).withTrimmedTop(2);
            previousPresetButton.setBounds(selectedRow.removeFromLeft(38).reduced(2, 4));
            nextPresetButton.setBounds(selectedRow.removeFromRight(38).reduced(2, 4));
            presetBox.setBounds(selectedRow.reduced(2, 4));
            auto primaryActions = browserArea.removeFromTop(38).withTrimmedTop(3);
            loadPresetButton.setBounds(primaryActions.removeFromLeft(76).reduced(2, 4));
            auditionPresetButton.setBounds(primaryActions.removeFromLeft(92).reduced(2, 4));
            warmPresetPreviewsButton.setBounds(primaryActions.removeFromLeft(72).reduced(2, 4));
            favoritePresetButton.setBounds(primaryActions.removeFromLeft(70).reduced(2, 4));
            auto compareActions = browserArea.removeFromTop(34).withTrimmedTop(1);
            comparePresetButton.setBounds(compareActions.removeFromLeft(82).reduced(2, 4));
            revertPresetButton.setBounds(compareActions.removeFromLeft(82).reduced(2, 4));
            presetRatingBox.setBounds(compareActions.reduced(2, 4));
            const auto rowLayout = presetBrowserRowLayoutForWidth(browserArea.getWidth(), presetBrowserList.getRowHeight());
            const auto browserHeaderText = rowLayout.compact
                ? juce::String("SOUND / PACK     PREVIEW     INFO")
                : juce::String("SOUND / PACK     PREVIEW     SOURCE / RATING");
            if (presetBrowserHeaderLabel.getText() != browserHeaderText)
                presetBrowserHeaderLabel.setText(browserHeaderText, juce::dontSendNotification);
            presetBrowserHeaderLabel.setBounds(browserArea.removeFromTop(24).reduced(6, 3));
            presetBrowserList.setBounds(browserArea.reduced(2, 5));

            const auto savePanelHeight = juce::jlimit(252, 304, inspectorArea.getHeight() - 96);
            auto saveArea = inspectorArea.removeFromTop(savePanelHeight).reduced(10, 8);
            librarySaveLabel.setBounds(saveArea.removeFromTop(24));
            presetSaveSummary.setBounds(saveArea.removeFromTop(72).reduced(2, 5));
            auto nameRow = saveArea.removeFromTop(34).withTrimmedTop(1);
            presetNameEditor.setBounds(nameRow.reduced(2, 4));
            auto folderRow = saveArea.removeFromTop(32);
            presetCategoryBox.setBounds(folderRow.reduced(2, 4));
            auto metadataRow = saveArea.removeFromTop(32);
            presetAuthorEditor.setBounds(metadataRow.removeFromLeft(metadataRow.getWidth() / 2).reduced(2, 4));
            presetPackBox.setBounds(metadataRow.reduced(2, 4));
            auto keyRow = saveArea.removeFromTop(32);
            const auto keyCellWidth = keyRow.getWidth() / 2;
            presetKeyBox.setBounds(keyRow.removeFromLeft(keyCellWidth).reduced(2, 4));
            presetBpmBox.setBounds(keyRow.reduced(2, 4));
            auto saveActionRow = saveArea.removeFromTop(32).withTrimmedTop(2);
            presetNotesTemplateBox.setBounds(saveActionRow.removeFromLeft(118).reduced(2, 4));
            savePresetButton.setBounds(saveActionRow.reduced(2, 4));
            presetNotesEditor.setBounds(saveArea.reduced(2, 4));

            auto summaryArea = inspectorArea.reduced(10, 6);
            libraryInspectorLabel.setBounds(summaryArea.removeFromTop(24));
            presetLibrarySummary.setBounds(summaryArea.reduced(2, 4));
            break;
        }

        case Panel::info:
        {
            infoSectionLabel.setVisible(true);
            infoAboutLabel.setVisible(true);
            infoWorkflowLabel.setVisible(true);
            infoDetailsLabel.setVisible(true);
            infoFocusLabel.setVisible(true);
            infoAboutEditor.setVisible(true);
            infoWorkflowEditor.setVisible(true);
            infoDetailEditor.setVisible(true);
            infoTopicBox.setVisible(true);
            infoOpenLabButton.setVisible(true);
            infoOpenModButton.setVisible(true);
            infoOpenFxButton.setVisible(true);
            infoOpenLibraryButton.setVisible(true);

            infoSectionLabel.setBounds(content.removeFromTop(28));
            auto infoContent = content.withTrimmedTop(8);
            auto topRow = infoContent.removeFromTop(172);
            auto aboutArea = topRow.removeFromLeft(352).reduced(18, 12);
            auto workflowArea = topRow.reduced(18, 12);
            infoContent.removeFromTop(10);
            auto detailRow = infoContent;
            auto focusArea = detailRow.removeFromRight(214).reduced(18, 12);
            auto detailArea = detailRow.reduced(18, 12);

            infoAboutLabel.setBounds(aboutArea.removeFromTop(24));
            infoAboutEditor.setBounds(aboutArea.reduced(2, 4));

            infoWorkflowLabel.setBounds(workflowArea.removeFromTop(24));
            infoWorkflowEditor.setBounds(workflowArea.reduced(2, 4));

            auto detailHeader = detailArea.removeFromTop(38);
            infoDetailsLabel.setBounds(detailHeader.removeFromLeft(94));
            infoTopicBox.setBounds(detailHeader.removeFromLeft(240).reduced(4));
            infoDetailEditor.setBounds(detailArea.withTrimmedTop(6).reduced(2, 4));

            infoFocusLabel.setBounds(focusArea.removeFromTop(24));
            auto focusButtonHeight = 38;
            infoOpenLabButton.setBounds(focusArea.removeFromTop(focusButtonHeight).reduced(4));
            infoOpenModButton.setBounds(focusArea.removeFromTop(focusButtonHeight).reduced(4));
            infoOpenFxButton.setBounds(focusArea.removeFromTop(focusButtonHeight).reduced(4));
            infoOpenLibraryButton.setBounds(focusArea.removeFromTop(focusButtonHeight).reduced(4));
            break;
        }
    }

    selectedControlHeaderLabel.toFront(false);
    selectedControlStatusLabel.toFront(false);
    undoEditButton.toFront(false);
    redoEditButton.toFront(false);
    selectedControlAddModButton.toFront(false);
    selectedControlOpenModButton.toFront(false);
    layoutFocusOverlay();
}

bool NateVSTAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& filePath : files)
    {
        const auto file = juce::File(filePath);
        const auto extension = file.getFileExtension().toLowerCase();
        if (extension == ".wav" || extension == ".aif" || extension == ".aiff")
            return true;
    }

    return false;
}

void NateVSTAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    for (const auto& filePath : files)
    {
        const auto file = juce::File(filePath);
        const auto extension = file.getFileExtension().toLowerCase();
        if (extension == ".wav" || extension == ".aif" || extension == ".aiff")
        {
            loadSampleFile(file);
            return;
        }
    }
}

juce::StringArray NateVSTAudioProcessorEditor::runLayoutAudit()
{
    struct PanelAuditSpec
    {
        Panel panel;
        const char* name = "";
    };

    struct LayoutSizeAuditSpec
    {
        const char* name = "";
        int width = editorDefaultWidth;
        int height = editorDefaultHeight;
    };

    const std::array<PanelAuditSpec, 9> panels {
        PanelAuditSpec { Panel::home, "HOME" },
        PanelAuditSpec { Panel::synth, "SYNTH" },
        PanelAuditSpec { Panel::lab, "LAB" },
        PanelAuditSpec { Panel::mod, "MOD" },
        PanelAuditSpec { Panel::sample, "SAMPLE" },
        PanelAuditSpec { Panel::sequencer, "SEQ" },
        PanelAuditSpec { Panel::effects, "FX" },
        PanelAuditSpec { Panel::library, "LIBRARY" },
        PanelAuditSpec { Panel::info, "INFO" }
    };

    const std::array<FxModule, 15> fxModules {
        FxModule::tone,
        FxModule::eq,
        FxModule::distortion,
        FxModule::bitcrush,
        FxModule::pump,
        FxModule::tremolo,
        FxModule::ring,
        FxModule::comb,
        FxModule::phaser,
        FxModule::flanger,
        FxModule::chorus,
        FxModule::delay,
        FxModule::reverb,
        FxModule::width,
        FxModule::guard
    };

    const std::array<std::pair<RandomLabPage, const char*>, 5> randomLabPages {
        std::pair<RandomLabPage, const char*> { RandomLabPage::generate, "Generate" },
        std::pair<RandomLabPage, const char*> { RandomLabPage::mutate, "Mutate" },
        std::pair<RandomLabPage, const char*> { RandomLabPage::recipe, "Recipe" },
        std::pair<RandomLabPage, const char*> { RandomLabPage::history, "History" },
        std::pair<RandomLabPage, const char*> { RandomLabPage::save, "Save" }
    };

    const std::array<std::pair<ModWorkflowPage, const char*>, 4> modWorkflowPages {
        std::pair<ModWorkflowPage, const char*> { ModWorkflowPage::matrix, "Matrix" },
        std::pair<ModWorkflowPage, const char*> { ModWorkflowPage::sources, "Sources" },
        std::pair<ModWorkflowPage, const char*> { ModWorkflowPage::macros, "Macros" },
        std::pair<ModWorkflowPage, const char*> { ModWorkflowPage::curves, "Curves" }
    };

    const std::array<LayoutSizeAuditSpec, 7> layoutSizes {
        LayoutSizeAuditSpec { "Min", editorMinWidth, editorMinHeight },
        LayoutSizeAuditSpec { "MinTall", editorMinWidth, 900 },
        LayoutSizeAuditSpec { "Default", editorDefaultWidth, editorDefaultHeight },
        LayoutSizeAuditSpec { "DefaultTall", editorDefaultWidth, 900 },
        LayoutSizeAuditSpec { "WideMinHeight", 1180, editorMinHeight },
        LayoutSizeAuditSpec { "Wide", 1180, 820 },
        LayoutSizeAuditSpec { "Max", editorMaxWidth, editorMaxHeight }
    };

    const auto originalPanel = activePanel;
    const auto originalFocusOverlay = activeFocusOverlay;
    const auto originalFxModule = selectedFxModule;
    const auto originalRandomLabPage = activeRandomLabPage;
    const auto originalModWorkflowPage = activeModWorkflowPage;
    const auto originalWidth = getWidth();
    const auto originalHeight = getHeight();
    const auto originalKeyboardBaseNote = keyboardTypingBaseNote;
    juce::StringArray issues;

    auto auditComputerKeyboardRange = [this, &issues]
    {
        auto expectBase = [&issues, this] (const juce::String& state, int expectedNote)
        {
            const auto baseNote = computerKeyboardBaseNote();
            if (baseNote != expectedNote)
            {
                issues.add("Keyboard " + state + ": laptop A maps to MIDI "
                           + juce::String(baseNote)
                           + " (" + abletonNoteName(baseNote) + ") instead of MIDI "
                           + juce::String(expectedNote)
                           + " (" + abletonNoteName(expectedNote) + ")");
            }

            if (syncedPianoKeyboardMappingBaseNote != expectedNote)
            {
                issues.add("Keyboard " + state + ": synced QWERTY base MIDI "
                           + juce::String(syncedPianoKeyboardMappingBaseNote)
                           + " does not match expected MIDI "
                           + juce::String(expectedNote));
            }

            if (pianoKeyboard.getLowestVisibleKey() != keyboardVisualLowestNote)
            {
                issues.add("Keyboard " + state + ": visible piano strip starts at MIDI "
                           + juce::String(pianoKeyboard.getLowestVisibleKey())
                           + " (" + abletonNoteName(pianoKeyboard.getLowestVisibleKey())
                           + ") instead of fixed MIDI "
                           + juce::String(keyboardVisualLowestNote)
                           + " (" + abletonNoteName(keyboardVisualLowestNote) + ")");
            }

            const auto expectedKeyWidth = responsiveKeyboardKeyWidthForBounds(pianoKeyboard.getBounds(),
                                                                             keyboardVisualLowestNote);
            if (std::abs(pianoKeyboard.getKeyWidth() - expectedKeyWidth) > 0.25f)
            {
                issues.add("Keyboard " + state + ": fixed piano key width "
                           + juce::String(pianoKeyboard.getKeyWidth(), 2)
                           + " does not match expected "
                           + juce::String(expectedKeyWidth, 2));
            }
        };

        releaseComputerKeyboardNotes();
        audioProcessor.getMidiKeyboardState().allNotesOff(1);
        keyboardTypingBaseNote = keyboardTypingBaseNoteForLowestNote(keyboardInitialLowestNote);
        pinPianoKeyboardVisualRange();
        updateKeyboardRangeLabel();
        expectBase("home", keyboardInitialLowestNote);
        if (pianoKeyboard.getOctaveForMiddleC() != abletonMiddleCOctave)
            issues.add("Keyboard home: piano strip C labels use middle-C octave "
                       + juce::String(pianoKeyboard.getOctaveForMiddleC())
                       + " instead of Ableton octave "
                       + juce::String(abletonMiddleCOctave));
        if (keyboardRangeLabel.getText() != "A:C4\n;:E5")
            issues.add("Keyboard home: range label should read A:C4 to ;:E5, got "
                       + keyboardRangeLabel.getText().quoted());

        shiftKeyboardOctave(12);
        expectBase("Oct+", keyboardMaxLowestVisibleNote);
        if (keyboardRangeLabel.getText() != "A:C4\n;:E5")
            issues.add("Keyboard Oct+: range label should read A:C4 to ;:E5, got "
                       + keyboardRangeLabel.getText().quoted());

        shiftKeyboardOctave(12);
        expectBase("Oct+ clamp", keyboardMaxLowestVisibleNote);
        if (keyboardOctaveUpButton.isEnabled())
            issues.add("Keyboard Oct+ clamp: up button remains enabled at the top laptop range");

        shiftKeyboardOctave(-12);
        expectBase("Oct-", 60);
        if (keyboardRangeLabel.getText() != "A:C3\n;:E4")
            issues.add("Keyboard Oct-: range label should read A:C3 to ;:E4, got "
                       + keyboardRangeLabel.getText().quoted());

        shiftKeyboardOctave(-12);
        expectBase("Oct-", 48);
        if (keyboardRangeLabel.getText() != "A:C2\n;:E3")
            issues.add("Keyboard Oct-: range label should read A:C2 to ;:E3, got "
                       + keyboardRangeLabel.getText().quoted());

        shiftKeyboardOctave(-12);
        expectBase("Oct- floor", keyboardMinLowestVisibleNote);
        if (keyboardRangeLabel.getText() != "A:C1\n;:E2")
            issues.add("Keyboard Oct- floor: range label should read A:C1 to ;:E2, got "
                       + keyboardRangeLabel.getText().quoted());

        shiftKeyboardOctave(-12);
        expectBase("Oct- clamp", keyboardMinLowestVisibleNote);
        if (keyboardOctaveDownButton.isEnabled())
            issues.add("Keyboard Oct- clamp: down button remains enabled at the bottom laptop range");

        keyboardTypingBaseNote = keyboardTypingBaseNoteForLowestNote(keyboardInitialLowestNote);
        pinPianoKeyboardVisualRange();
        updateKeyboardRangeLabel();
    };

    setSize(editorDefaultWidth, editorDefaultHeight);
    resized();
    auditComputerKeyboardRange();

    auto auditCurrentLayout = [this, &issues] (const juce::String& panelName)
    {
        resized();
        appendVisibleLayoutIssues(*this, *this, panelName, {}, issues);

        if (panelName.contains("Focus"))
        {
            if (! focusOverlayPanel.isVisible())
            {
                issues.add(panelName + ": focus overlay panel is not visible");
            }
            else
            {
                const auto overlayOrigin = focusOverlayPanel.getBounds().getPosition();
                const auto panelBounds = focusOverlayPanel.getPanelBounds().translated(overlayOrigin.x,
                                                                                       overlayOrigin.y);

                if (getHeight() >= 900 && panelBounds.getHeight() < 620)
                {
                    issues.add(panelName + ": focus overlay panel does not expand for tall editor sizes "
                               + panelBounds.toString());
                }

                if (getWidth() >= 1180 && panelBounds.getWidth() < 980)
                {
                    issues.add(panelName + ": focus overlay panel does not use wide editor space "
                               + panelBounds.toString());
                }
            }
        }

        if (panelName.contains("SAMPLE/Chop Focus"))
        {
            if (! expandedSampleWaveformDisplay.isVisible())
            {
                issues.add(panelName + ": expanded sample waveform is not visible");
            }
            else
            {
                const auto waveformBounds = getLocalArea(expandedSampleWaveformDisplay.getParentComponent(),
                                                         expandedSampleWaveformDisplay.getBounds());
                const auto minimumWaveformHeight = getHeight() >= 900 ? 456
                                             : getHeight() >= editorDefaultHeight ? 330
                                                                                  : 270;

                if (waveformBounds.getHeight() < minimumWaveformHeight)
                {
                    issues.add(panelName + ": expanded sample waveform is too short for focused chop editing "
                               + waveformBounds.toString());
                }
            }
        }

        if (panelName.contains("SAMPLE") && sampleRecorderPanel.isVisible())
        {
            const auto hasCapture = audioProcessor.getSampleCaptureDurationSeconds() >= 0.05f;
            const auto hasLoadedSample = audioProcessor.hasLoadedSample();
            issues.addArray(sampleRecorderPanel.runLayoutAudit(panelName, hasCapture, hasLoadedSample));
        }

        if (panelName.contains("SYNTH"))
        {
            if (! houseLayerRackDisplay.isVisible())
            {
                issues.add(panelName + ": HouseLayerRackDisplay is not visible in the SYNTH source card");
            }
            else
            {
                const auto rackBounds = getLocalArea(houseLayerRackDisplay.getParentComponent(),
                                                     houseLayerRackDisplay.getBounds());
                if (rackBounds.getHeight() < 38 || rackBounds.getWidth() < 240)
                {
                    issues.add(panelName + ": HouseLayerRackDisplay is too small "
                               + rackBounds.toString());
                }

                const auto rackMetrics = houseLayerRackDisplay.getLayoutMetricsForAudit();
                if (! rackMetrics.readable)
                {
                    issues.add(panelName + ": HouseLayerRackDisplay cards are not readable min "
                               + juce::String(rackMetrics.minCardWidth, 1) + "x"
                               + juce::String(rackMetrics.minCardHeight, 1));
                }

                for (const auto* sourceControl : {
                         static_cast<const juce::Component*>(&osc1LevelSlider),
                         static_cast<const juce::Component*>(&osc2LevelSlider),
                         static_cast<const juce::Component*>(&subLevelSlider),
                         static_cast<const juce::Component*>(&noiseLevelSlider),
                         static_cast<const juce::Component*>(&noiseDecaySlider),
                         static_cast<const juce::Component*>(&oscWavetablePositionSlider),
                         static_cast<const juce::Component*>(&osc2WavetablePositionSlider),
                         static_cast<const juce::Component*>(&noiseTypeBox) })
                {
                    if (sourceControl == nullptr || ! sourceControl->isVisible())
                        continue;

                    const auto controlBounds = getLocalArea(sourceControl->getParentComponent(),
                                                            sourceControl->getBounds());
                    if (rackBounds.intersects(controlBounds))
                    {
                        issues.add(panelName + ": HouseLayerRackDisplay overlaps "
                                   + layoutAuditComponentName(*sourceControl, 0)
                                   + " at " + rackBounds.toString()
                                   + " / " + controlBounds.toString());
                    }
                }

                for (const auto* waveControl : {
                         static_cast<const juce::Component*>(&wavetableDrawModeBox),
                         static_cast<const juce::Component*>(&wavetableToolBox),
                         static_cast<const juce::Component*>(&waveEditorFocusButton) })
                {
                    if (waveControl == nullptr || ! waveControl->isVisible())
                        continue;

                    const auto controlBounds = getLocalArea(waveControl->getParentComponent(),
                                                            waveControl->getBounds());
                    if (controlBounds.getWidth() < 76 || controlBounds.getHeight() < 22)
                    {
                        issues.add(panelName + ": SYNTH wave editor control "
                                   + layoutAuditComponentName(*waveControl, 0)
                                   + " is too compressed "
                                   + controlBounds.toString());
                    }
                }

                const auto drawBounds = getLocalArea(wavetableDrawModeBox.getParentComponent(),
                                                     wavetableDrawModeBox.getBounds());
                const auto toolBounds = getLocalArea(wavetableToolBox.getParentComponent(),
                                                     wavetableToolBox.getBounds());
                const auto editBounds = getLocalArea(waveEditorFocusButton.getParentComponent(),
                                                     waveEditorFocusButton.getBounds());
                if (wavetableDrawModeBox.isVisible() && wavetableToolBox.isVisible() && drawBounds.intersects(toolBounds))
                {
                    issues.add(panelName + ": SYNTH wave draw-mode dropdown overlaps wave tools "
                               + drawBounds.toString() + " / " + toolBounds.toString());
                }
                if (wavetableToolBox.isVisible() && waveEditorFocusButton.isVisible() && toolBounds.intersects(editBounds))
                {
                    issues.add(panelName + ": SYNTH wave tools dropdown overlaps Edit Wave action "
                               + toolBounds.toString() + " / " + editBounds.toString());
                }
            }

            if (panelName.contains("Source Focus"))
            {
                if (! expandedHouseLayerRackDisplay.isVisible())
                {
                    issues.add(panelName + ": expanded source layer rack is not visible");
                }
                else
                {
                    const auto expandedMetrics = expandedHouseLayerRackDisplay.getLayoutMetricsForAudit();
                    if (! expandedMetrics.readable || expandedMetrics.minCardWidth < 84.0f || expandedMetrics.minCardHeight < 130.0f)
                    {
                        issues.add(panelName + ": expanded source layer rack cards are too small "
                                   + juce::String(expandedMetrics.minCardWidth, 1) + "x"
                                   + juce::String(expandedMetrics.minCardHeight, 1));
                    }
                }
            }
        }

        auto visibleModRows = 0;
        auto minModRowHeight = std::numeric_limits<int>::max();
        for (const auto& row : modMatrixRows)
        {
            if (! row.isVisible())
                continue;

            ++visibleModRows;
            minModRowHeight = juce::jmin(minModRowHeight, row.getBounds().getHeight());
        }

        if (visibleModRows > 0)
        {
            if (minModRowHeight < 20)
            {
                issues.add(panelName + ": MOD matrix rows are too short at "
                           + juce::String(minModRowHeight) + "px");
            }

            if (modRouteMapDisplay.isVisible() && minModRowHeight < 24)
            {
                issues.add(panelName + ": MOD route map is visible while matrix rows are only "
                           + juce::String(minModRowHeight) + "px tall");
            }

            if (panelName.contains("MOD/Matrix"))
            {
                if (visibleModRows != static_cast<int>(modMatrixRows.size()))
                {
                    issues.add(panelName + ": focused Matrix mode hides route rows "
                               + juce::String(visibleModRows) + "/"
                               + juce::String(static_cast<int>(modMatrixRows.size())));
                }

                if (minModRowHeight < 28)
                {
                    issues.add(panelName + ": focused Matrix rows are still cramped at "
                               + juce::String(minModRowHeight) + "px");
                }

                for (const auto& sourceMeter : modSourceRows)
                {
                    if (sourceMeter.isVisible())
                    {
                        issues.add(panelName + ": source meter remains visible on the focused Matrix surface");
                        break;
                    }
                }
            }
        }

        if (panelName.contains("MOD"))
        {
            for (const auto& sourceMeter : modSourceRows)
            {
                if (! sourceMeter.isVisible())
                    continue;

                const auto sourceBounds = getLocalArea(sourceMeter.getParentComponent(),
                                                       sourceMeter.getBounds());
                if (sourceBounds.getHeight() < 10)
                {
                    issues.add(panelName + ": MOD source meter is too short for compact rendering "
                               + sourceBounds.toString());
                }
            }

            for (const auto& amountSlider : modAmountSliders)
            {
                if (! amountSlider.isVisible())
                    continue;

                const auto sliderBounds = getLocalArea(amountSlider.getParentComponent(),
                                                       amountSlider.getBounds());
                const auto textBoxWidth = static_cast<int>(amountSlider.getProperties()
                                                               .getWithDefault("compactTextBoxWidth", 62));
                const auto trackWidth = sliderBounds.getWidth() - textBoxWidth;
                if (trackWidth < 34)
                {
                    issues.add(panelName + ": MOD route amount slider track is too narrow "
                               + juce::String(trackWidth)
                               + "px in " + sliderBounds.toString());
                }
            }
        }

        const auto keyboardLowestForAudit = keyboardVisualLowestNote;
        const auto expectedKeyboardKeyWidth = responsiveKeyboardKeyWidthForBounds(pianoKeyboard.getBounds(),
                                                                                 keyboardLowestForAudit);
        if (std::abs(pianoKeyboard.getKeyWidth() - expectedKeyboardKeyWidth) > 0.25f)
        {
            issues.add(panelName + ": piano keyboard key width "
                       + juce::String(pianoKeyboard.getKeyWidth(), 2)
                       + " does not match responsive width "
                       + juce::String(expectedKeyboardKeyWidth, 2)
                       + " for bounds "
                       + pianoKeyboard.getBounds().toString());
        }

        if (pianoKeyboard.getLowestVisibleKey() != keyboardVisualLowestNote)
        {
            issues.add(panelName + ": piano keyboard visible range starts at "
                       + abletonNoteName(pianoKeyboard.getLowestVisibleKey())
                       + " instead of fixed "
                       + abletonNoteName(keyboardVisualLowestNote));
        }

        const auto keyboardPaintCoverage = static_cast<float>(whiteKeyCountInRange(keyboardVisualLowestNote,
                                                                                  keyboardVisualHighestNote))
                                         * pianoKeyboard.getKeyWidth();
        if (keyboardPaintCoverage < static_cast<float>(pianoKeyboard.getWidth()) - 1.0f)
        {
            issues.add(panelName + ": piano keyboard painted width "
                       + juce::String(keyboardPaintCoverage, 2)
                       + " leaves unused space inside bounds "
                       + pianoKeyboard.getBounds().toString());
        }

        const auto defaultTypingBaseNote = keyboardTypingBaseNoteForLowestNote(keyboardInitialLowestNote);
        if (defaultTypingBaseNote != keyboardInitialLowestNote)
        {
            issues.add(panelName + ": default laptop keyboard maps A to MIDI "
                       + juce::String(defaultTypingBaseNote)
                       + " instead of MIDI "
                       + juce::String(keyboardInitialLowestNote)
                       + "/" + abletonNoteName(keyboardInitialLowestNote)
                       + " in Ableton note names");
        }

        for (const auto* keyboardControl : {
                 static_cast<const juce::Component*>(&keyboardOctaveDownButton),
                 static_cast<const juce::Component*>(&keyboardRangeLabel),
                 static_cast<const juce::Component*>(&keyboardHomeButton),
                 static_cast<const juce::Component*>(&keyboardOctaveUpButton),
                 static_cast<const juce::Component*>(&keyboardPanicButton) })
        {
            if (keyboardControl == nullptr || ! keyboardControl->isVisible())
                continue;

            const auto controlBounds = getLocalArea(keyboardControl->getParentComponent(),
                                                    keyboardControl->getBounds());
            if (controlBounds.getWidth() < 34 || controlBounds.getHeight() < 20)
            {
                issues.add(panelName + ": keyboard control "
                           + layoutAuditComponentName(*keyboardControl, 0)
                           + " is too compressed "
                           + controlBounds.toString());
            }
        }

        if (panelName.contains("SEQ"))
        {
            const auto isSequencerFocus = panelName.contains("Pattern Focus");
            const auto& gridForAudit = isSequencerFocus ? expandedSequencerGrid : sequencerGrid;
            if (isSequencerFocus && ! expandedSequencerGrid.isVisible())
                issues.add(panelName + ": expanded sequencer grid is not visible");

            const auto metrics = gridForAudit.getLayoutMetricsForAudit();
            const auto sequencerGridBounds = getLocalArea(gridForAudit.getParentComponent(),
                                                          gridForAudit.getBounds());
            const auto minimumGridWidth = isSequencerFocus ? 760 : 680;
            const auto minimumGridHeight = isSequencerFocus ? 320 : 236;
            const auto minimumStepCellWidth = isSequencerFocus ? 44.0f : 36.0f;
            const auto minimumNoteRowHeight = isSequencerFocus ? 8.0f : 5.75f;
            const auto minimumLaneRowHeight = isSequencerFocus ? 11.0f : 8.75f;
            const auto minimumLabelWidth = isSequencerFocus ? 42 : 38;
            const auto focusedGridHeightFloor = getHeight() >= 900 ? 520
                                                : getHeight() >= editorDefaultHeight ? 400
                                                                                    : minimumGridHeight;

            if (sequencerGridBounds.getWidth() < minimumGridWidth || sequencerGridBounds.getHeight() < minimumGridHeight)
            {
                issues.add(panelName + ": sequencer grid bounds are too compressed for 16-step editing "
                           + sequencerGridBounds.toString());
            }

            if (isSequencerFocus && sequencerGridBounds.getHeight() < focusedGridHeightFloor)
            {
                issues.add(panelName + ": expanded sequencer grid does not grow enough for focused pattern editing "
                           + sequencerGridBounds.toString());
            }

            if (metrics.stepCellWidth < minimumStepCellWidth)
            {
                issues.add(panelName + ": sequencer step cells are too narrow "
                           + juce::String(metrics.stepCellWidth, 2)
                           + "px in " + sequencerGridBounds.toString());
            }

            if (metrics.noteRowHeight < minimumNoteRowHeight)
            {
                issues.add(panelName + ": sequencer note rows are too short "
                           + juce::String(metrics.noteRowHeight, 2)
                           + "px in " + sequencerGridBounds.toString());
            }

            if (metrics.laneRowHeight < minimumLaneRowHeight)
            {
                issues.add(panelName + ": sequencer modulation lane rows are too short "
                           + juce::String(metrics.laneRowHeight, 2)
                           + "px in " + sequencerGridBounds.toString());
            }

            if (metrics.noteLabels.getWidth() < minimumLabelWidth || metrics.laneLabels.getWidth() < minimumLabelWidth)
            {
                issues.add(panelName + ": sequencer label columns are too narrow note "
                           + metrics.noteLabels.toString()
                           + " lane " + metrics.laneLabels.toString());
            }
        }

        if (panelName.contains("LIBRARY") && presetBrowserList.isVisible())
        {
            const auto browserBounds = getLocalArea(presetBrowserList.getParentComponent(),
                                                    presetBrowserList.getBounds());
            const auto rowLayout = presetBrowserRowLayoutForWidth(browserBounds.getWidth(),
                                                                  presetBrowserList.getRowHeight());
            if (rowLayout.compact)
            {
                if (rowLayout.showsMacroStrip)
                {
                    issues.add(panelName + ": compact preset browser row still reserves macro-strip space");
                }

                if (rowLayout.nameWidth < 136)
                {
                    issues.add(panelName + ": compact preset browser name/meta column is too narrow "
                               + juce::String(rowLayout.nameWidth)
                               + "px in " + browserBounds.toString());
                }

                if (presetBrowserHeaderLabel.getText().contains("MACROS"))
                {
                    issues.add(panelName + ": compact preset browser header still advertises the dense macro column");
                }
            }
        }
    };

    for (const auto& layoutSize : layoutSizes)
    {
        setSize(layoutSize.width, layoutSize.height);
        const auto layoutPrefix = juce::String(layoutSize.name)
                                + " "
                                + juce::String(layoutSize.width)
                                + "x"
                                + juce::String(layoutSize.height)
                                + " ";

        for (const auto& panel : panels)
        {
            activePanel = panel.panel;
            activeFocusOverlay = FocusOverlay::none;
            updatePanelVisibility();

            if (panel.panel == Panel::effects)
            {
                for (const auto module : fxModules)
                {
                    selectedFxModule = module;
                    auditCurrentLayout(layoutPrefix + panel.name + "/" + fxModuleName(module));
                }
            }
            else if (panel.panel == Panel::lab)
            {
                for (const auto& page : randomLabPages)
                {
                    activeRandomLabPage = page.first;
                    auditCurrentLayout(layoutPrefix + panel.name + "/" + page.second);
                }
            }
            else if (panel.panel == Panel::mod)
            {
                for (const auto& page : modWorkflowPages)
                {
                    activeModWorkflowPage = page.first;
                    auditCurrentLayout(layoutPrefix + panel.name + "/" + page.second);
                }
            }
            else
            {
                auditCurrentLayout(layoutPrefix + panel.name);
            }
        }

        for (const auto panel : { Panel::home, Panel::mod })
        {
            activePanel = panel;
            activeFocusOverlay = FocusOverlay::macroEditor;
            updatePanelVisibility();
            auditCurrentLayout(layoutPrefix + (panel == Panel::home ? "HOME/Macro Focus" : "MOD/Macro Focus"));
        }

        activePanel = Panel::sample;
        activeFocusOverlay = FocusOverlay::sampleChopEditor;
        updatePanelVisibility();
        auditCurrentLayout(layoutPrefix + "SAMPLE/Chop Focus");

        activePanel = Panel::synth;
        activeFocusOverlay = FocusOverlay::sourceLayerEditor;
        updatePanelVisibility();
        auditCurrentLayout(layoutPrefix + "SYNTH/Source Focus");

        activePanel = Panel::sequencer;
        activeFocusOverlay = FocusOverlay::sequencerEditor;
        updatePanelVisibility();
        auditCurrentLayout(layoutPrefix + "SEQ/Pattern Focus");
    }

    setSize(originalWidth, originalHeight);
    activePanel = originalPanel;
    activeFocusOverlay = originalFocusOverlay;
    selectedFxModule = originalFxModule;
    activeRandomLabPage = originalRandomLabPage;
    activeModWorkflowPage = originalModWorkflowPage;
    keyboardTypingBaseNote = originalKeyboardBaseNote;
    updateKeyboardRangeLabel();
    pinPianoKeyboardVisualRange();
    updatePanelVisibility();
    resized();

    return issues;
}

void NateVSTAudioProcessorEditor::configureSlider(juce::Slider& slider,
                                                    juce::Label& label,
                                                    const juce::String& labelText,
                                                    const juce::String& parameterID)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setMouseDragSensitivity(rotaryDragSensitivityForParameter(parameterID));
    applyFineDragMode(slider, 0.32);
    slider.setSliderSnapsToMousePosition(false);
    slider.setScrollWheelEnabled(false);
    slider.setWantsKeyboardFocus(false);
    slider.setMouseClickGrabsKeyboardFocus(false);
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    const auto tooltipText = controlFeelTooltip(labelText);
    slider.setTooltip(tooltipText);
    slider.getProperties().set("baseTooltip", tooltipText);
    const auto& theme = uiTheme();
    slider.setColour(juce::Slider::textBoxTextColourId, theme.text);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, theme.field);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, theme.textMuted);
    label.attachToComponent(&slider, false);
    addAndMakeVisible(label);

    if (auto* parameter = audioProcessor.getValueTreeState().getParameter(parameterID))
        slider.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));

    auto* sliderPointer = &slider;
    slider.onDragStart = [this, labelText]
    {
        captureGlobalEdit("Edit " + labelText);
    };
    slider.onValueChange = [this, sliderPointer, labelText, parameterID]
    {
        updateSelectedControlInspector(labelText, parameterID, sliderPointer->getValue());
    };
    slider.onDragEnd = [this] { returnKeyboardFocusToPiano(); };

    registerModulationMenuTarget(slider, labelText, parameterID);
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.getValueTreeState(), parameterID, slider));
}

void NateVSTAudioProcessorEditor::configureHorizontalSlider(juce::Slider& slider,
                                                              juce::Label& label,
                                                              const juce::String& labelText,
                                                              const juce::String& parameterID)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setMouseDragSensitivity(150);
    applyFineDragMode(slider, 0.40);
    slider.setSliderSnapsToMousePosition(false);
    slider.setScrollWheelEnabled(false);
    slider.setWantsKeyboardFocus(false);
    slider.setMouseClickGrabsKeyboardFocus(false);
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 62, 18);
    const auto tooltipText = controlFeelTooltip(labelText);
    slider.setTooltip(tooltipText);
    slider.getProperties().set("baseTooltip", tooltipText);
    const auto& theme = uiTheme();
    slider.setColour(juce::Slider::trackColourId, theme.accent);
    slider.setColour(juce::Slider::backgroundColourId, theme.outline);
    slider.setColour(juce::Slider::textBoxTextColourId, theme.text);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, theme.field);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredLeft);
    label.setColour(juce::Label::textColourId, theme.textMuted);
    label.attachToComponent(&slider, true);
    addAndMakeVisible(label);

    if (auto* parameter = audioProcessor.getValueTreeState().getParameter(parameterID))
        slider.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));

    auto* sliderPointer = &slider;
    slider.onDragStart = [this, labelText]
    {
        captureGlobalEdit("Edit " + labelText);
    };
    slider.onValueChange = [this, sliderPointer, labelText, parameterID]
    {
        updateSelectedControlInspector(labelText, parameterID, sliderPointer->getValue());
    };
    slider.onDragEnd = [this] { returnKeyboardFocusToPiano(); };

    registerModulationMenuTarget(slider, labelText, parameterID);
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.getValueTreeState(), parameterID, slider));
}

void NateVSTAudioProcessorEditor::configureModAmountSlider(juce::Slider& slider,
                                                            juce::Label& label,
                                                            const juce::String& labelText,
                                                            const juce::String& parameterID)
{
    configureHorizontalSlider(slider, label, labelText, parameterID);
    slider.setMouseDragSensitivity(120);
    applyFineDragMode(slider, 0.34);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setNumDecimalPlacesToDisplay(2);
    slider.getProperties().set("compactTextBoxWidth", 0);
    slider.setTooltip("Mod route amount: drag to set depth, Shift or Cmd for fine movement, double-click to reset, or right-click for route actions. The selected-control strip shows the exact value.");
    slider.getProperties().set("baseTooltip", slider.getTooltip());
}

void NateVSTAudioProcessorEditor::configureCompactHorizontalSlider(juce::Slider& slider,
                                                                    const juce::String& parameterID)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setMouseDragSensitivity(110);
    applyFineDragMode(slider, 0.38);
    slider.setSliderSnapsToMousePosition(false);
    slider.setScrollWheelEnabled(false);
    slider.setWantsKeyboardFocus(false);
    slider.setMouseClickGrabsKeyboardFocus(false);
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 42, 16);
    const auto tooltipText = juce::String("Curve point: drag to adjust, hold Shift or Cmd for fine movement, double-click to reset, or type a value.");
    slider.setTooltip(tooltipText);
    slider.getProperties().set("baseTooltip", tooltipText);
    slider.setNumDecimalPlacesToDisplay(2);
    const auto& theme = uiTheme();
    slider.setColour(juce::Slider::trackColourId, theme.accent);
    slider.setColour(juce::Slider::backgroundColourId, theme.outline);
    slider.setColour(juce::Slider::textBoxTextColourId, theme.text);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, theme.field);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);

    if (auto* parameter = audioProcessor.getValueTreeState().getParameter(parameterID))
        slider.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));

    auto* sliderPointer = &slider;
    slider.onDragStart = [this]
    {
        captureGlobalEdit("Edit curve point");
    };
    slider.onValueChange = [this, sliderPointer, parameterID]
    {
        updateSelectedControlInspector("Curve", parameterID, sliderPointer->getValue());
    };
    slider.onDragEnd = [this] { returnKeyboardFocusToPiano(); };

    registerModulationMenuTarget(slider, "Curve", parameterID);
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.getValueTreeState(), parameterID, slider));
}

void NateVSTAudioProcessorEditor::configureRandomSectionSlider(juce::Slider& slider,
                                                                juce::Label& label,
                                                                const juce::String& labelText,
                                                                const juce::String& parameterID)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setMouseDragSensitivity(140);
    applyFineDragMode(slider, 0.36);
    slider.setSliderSnapsToMousePosition(false);
    slider.setScrollWheelEnabled(false);
    slider.setWantsKeyboardFocus(false);
    slider.setMouseClickGrabsKeyboardFocus(false);
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 42, 16);
    slider.setNumDecimalPlacesToDisplay(2);
    const auto tooltipText = "Random Lab " + labelText
        + " strength: drag to scale how far generated values move this section, Shift or Cmd for fine movement, double-click to reset.";
    slider.setTooltip(tooltipText);
    slider.getProperties().set("baseTooltip", tooltipText);
    const auto& theme = uiTheme();
    slider.setColour(juce::Slider::trackColourId, theme.accent);
    slider.setColour(juce::Slider::backgroundColourId, theme.outline);
    slider.setColour(juce::Slider::textBoxTextColourId, theme.text);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, theme.field);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centredLeft);
    label.setColour(juce::Label::textColourId, theme.textMuted);
    addAndMakeVisible(label);

    if (auto* parameter = audioProcessor.getValueTreeState().getParameter(parameterID))
        slider.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));

    auto* sliderPointer = &slider;
    slider.onDragStart = [this, labelText]
    {
        captureGlobalEdit("Edit Random " + labelText);
    };
    slider.onValueChange = [this, sliderPointer, labelText, parameterID]
    {
        updateSelectedControlInspector("Rand " + labelText, parameterID, sliderPointer->getValue());
    };
    slider.onDragEnd = [this] { returnKeyboardFocusToPiano(); };

    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.getValueTreeState(), parameterID, slider));
}

void NateVSTAudioProcessorEditor::registerModulationMenuTarget(juce::Component& component,
                                                               const juce::String& labelText,
                                                               const juce::String& parameterID)
{
    if (modulationDestinationIndexForParameter(parameterID) <= 0)
        return;

    component.addMouseListener(this, true);
    modulationMenuTargets.push_back({ &component, labelText, parameterID });
}

const NateVSTAudioProcessorEditor::ModulationMenuTarget*
NateVSTAudioProcessorEditor::findModulationMenuTarget(const juce::Component* component) const
{
    while (component != nullptr && component != this)
    {
        for (const auto& target : modulationMenuTargets)
            if (target.component == component)
                return &target;

        component = component->getParentComponent();
    }

    return nullptr;
}

int NateVSTAudioProcessorEditor::findModRouteAmountIndex(const juce::Component* component) const
{
    while (component != nullptr && component != this)
    {
        for (size_t index = 0; index < modAmountSliders.size(); ++index)
            if (component == &modAmountSliders[index])
                return static_cast<int>(index);

        component = component->getParentComponent();
    }

    return -1;
}

void NateVSTAudioProcessorEditor::beginModSourceDrag(int sourceIndex, juce::Component& sourceComponent)
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    if (! juce::isPositiveAndBelow(sourceIndex, sourceChoices.size()))
        return;

    startDragging("mod-source:" + juce::String(sourceIndex), &sourceComponent);
    modMatrixStatusLabel.setText("Drag " + sourceChoices[sourceIndex] + " onto a control",
                                 juce::dontSendNotification);
}

bool NateVSTAudioProcessorEditor::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    return dragSourceDetails.description.toString().startsWith("mod-source:");
}

void NateVSTAudioProcessorEditor::itemDropped(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    const auto description = dragSourceDetails.description.toString();
    if (! description.startsWith("mod-source:"))
        return;

    const auto sourceIndex = description.fromFirstOccurrenceOf(":", false, false).getIntValue();
    if (! handleDroppedModSource(sourceIndex, dragSourceDetails.localPosition.roundToInt()))
        modMatrixStatusLabel.setText("Drop onto a modulatable control",
                                     juce::dontSendNotification);
}

bool NateVSTAudioProcessorEditor::handleDroppedModSource(int sourceIndex, juce::Point<int> localPosition)
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    if (! juce::isPositiveAndBelow(sourceIndex, sourceChoices.size()))
        return false;

    auto* component = getComponentAt(localPosition);
    const auto* target = findModulationMenuTarget(component);
    if (target == nullptr || target->component == nullptr)
        return false;

    const auto destinationIndex = modulationDestinationIndexForParameter(target->parameterID);
    if (destinationIndex <= 0)
        return false;

    selectedControlName = target->labelText;
    selectedControlParameterID = target->parameterID;
    selectedControlPlainValue = readPlainParameterValue(target->parameterID, 0.0f);
    addModRouteForParameter(target->parameterID, target->labelText, sourceIndex);
    return true;
}

void NateVSTAudioProcessorEditor::mouseEnter(const juce::MouseEvent& event)
{
    if (const auto* target = findModulationMenuTarget(event.originalComponent))
        updateSelectedControlInspector(target->labelText,
                                       target->parameterID,
                                       readPlainParameterValue(target->parameterID, 0.0f));
}

void NateVSTAudioProcessorEditor::mouseUp(const juce::MouseEvent& event)
{
    if (! event.mods.isPopupMenu())
    {
        for (auto* current = event.originalComponent; current != nullptr; current = current->getParentComponent())
        {
            if (dynamic_cast<juce::TextEditor*>(current) != nullptr)
                return;
        }

        returnKeyboardFocusToPiano();
        return;
    }

    if (const auto routeAmountIndex = findModRouteAmountIndex(event.originalComponent); routeAmountIndex >= 0)
    {
        showModRouteAmountMenu(static_cast<size_t>(routeAmountIndex), modAmountSliders[static_cast<size_t>(routeAmountIndex)]);
        return;
    }

    if (const auto* target = findModulationMenuTarget(event.originalComponent))
        if (target->component != nullptr)
            showModulationMenuForControl(*target, *target->component);
}

void NateVSTAudioProcessorEditor::showModulationMenuForControl(const ModulationMenuTarget& target,
                                                               juce::Component& component)
{
    const auto destinationIndex = modulationDestinationIndexForParameter(target.parameterID);
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    const auto sourceChoices = Parameters::modulationSourceChoices();
    if (! juce::isPositiveAndBelow(destinationIndex, destinationChoices.size()))
        return;

    juce::PopupMenu menu;
    menu.addSectionHeader("Modulate " + destinationChoices[destinationIndex]);

    const auto selectedSourceIndex = juce::jlimit(1,
                                                  sourceChoices.size() - 1,
                                                  modInspectorSourceBox.getSelectedId() - 1);
    for (auto sourceIndex = 1; sourceIndex < sourceChoices.size(); ++sourceIndex)
    {
        const auto canUseSource = ! (destinationUsesGlobalModulationSources(destinationIndex)
                                     && (sourceIndex == 2 || sourceIndex == 3));
        menu.addItem(1000 + sourceIndex,
                     sourceChoices[sourceIndex],
                     canUseSource,
                     sourceIndex == selectedSourceIndex);
    }

    menu.addSeparator();
    menu.addItem(1, "Open MOD focused here");

    const auto parameterID = target.parameterID;
    const auto labelText = target.labelText;
    juce::Component::SafePointer<NateVSTAudioProcessorEditor> safeEditor(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&component),
                       [safeEditor, parameterID, labelText] (int result)
                       {
                           auto* editor = safeEditor.getComponent();
                           if (editor == nullptr)
                               return;

                           if (result == 1)
                           {
                               editor->selectedControlParameterID = parameterID;
                               editor->selectedControlName = labelText;
                               editor->selectedControlPlainValue = editor->readPlainParameterValue(parameterID, 0.0f);
                               editor->focusSelectedControlModDestination();
                               return;
                           }

                           if (result >= 1000)
                               editor->addModRouteForParameter(parameterID, labelText, result - 1000);
                       });
}

void NateVSTAudioProcessorEditor::showModRouteAmountMenu(size_t slotIndex, juce::Component& component)
{
    if (slotIndex >= Parameters::ID::modMatrixSource.size())
        return;

    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    const auto sourceIndex = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixSource[slotIndex], 0.0f));
    const auto destinationIndex = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixDestination[slotIndex], 0.0f));
    const auto amount = readPlainParameterValue(Parameters::ID::modMatrixAmount[slotIndex], 0.0f);
    const auto isConfiguredRoute = sourceIndex > 0 && destinationIndex > 0 && std::abs(amount) > 0.001f;
    const auto sourceName = juce::isPositiveAndBelow(sourceIndex, sourceChoices.size()) ? sourceChoices[sourceIndex] : juce::String("Source");
    const auto destinationName = juce::isPositiveAndBelow(destinationIndex, destinationChoices.size()) ? destinationChoices[destinationIndex] : juce::String("Destination");

    juce::PopupMenu menu;
    menu.addSectionHeader("Route S" + juce::String(static_cast<int>(slotIndex + 1)));
    menu.addItem(1,
                 sourceName + " -> " + destinationName + " "
                     + (amount >= 0.0f ? "+" : "") + juce::String(juce::roundToInt(amount * 100.0f)) + "%",
                 false,
                 false);
    menu.addSeparator();
    menu.addItem(10, "Invert Amount", isConfiguredRoute);
    menu.addItem(11, "Set +25%", isConfiguredRoute);
    menu.addItem(12, "Set +50%", isConfiguredRoute);
    menu.addItem(13, "Set -25%", isConfiguredRoute);
    menu.addItem(14, "Set -50%", isConfiguredRoute);
    menu.addSeparator();
    const auto shapeSummary = modRouteShapeSummary(slotIndex);
    menu.addSectionHeader(shapeSummary.isNotEmpty() ? "Shape: " + shapeSummary : "Shape");
    menu.addItem(30, "Reset Shape", isConfiguredRoute, shapeSummary.isEmpty());
    menu.addItem(31, "Unipolar +", isConfiguredRoute);
    menu.addItem(32, "Invert Source", isConfiguredRoute);
    menu.addItem(33, "Soft + Slew", isConfiguredRoute);
    menu.addItem(34, "Gate Steps", isConfiguredRoute);
    menu.addItem(35, "Narrow Range", isConfiguredRoute);
    menu.addItem(36, "Positive Range", isConfiguredRoute);
    menu.addItem(37, "Smooth Only", isConfiguredRoute);
    menu.addSeparator();
    const auto isStepLfoRoute = sourceIndex == Modulation::stepLfoSourceIndex;
    menu.addSectionHeader("Step LFO Pattern");
    menu.addItem(40, "Classic Gate", isConfiguredRoute && isStepLfoRoute);
    menu.addItem(41, "Trance Chopper", isConfiguredRoute && isStepLfoRoute);
    menu.addItem(42, "House Lift", isConfiguredRoute && isStepLfoRoute);
    menu.addItem(43, "Techno Ratchet", isConfiguredRoute && isStepLfoRoute);
    menu.addSeparator();
    menu.addItem(20, "Duplicate Route", isConfiguredRoute);
    menu.addItem(21, "Clear Route", isConfiguredRoute);

    juce::Component::SafePointer<NateVSTAudioProcessorEditor> safeEditor(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&component),
                       [safeEditor, slotIndex, amount] (int result)
                       {
                           auto* editor = safeEditor.getComponent();
                           if (editor == nullptr)
                               return;

                           switch (result)
                           {
                               case 10: editor->setModRouteAmount(slotIndex, -amount); break;
                               case 11: editor->setModRouteAmount(slotIndex, 0.25f); break;
                               case 12: editor->setModRouteAmount(slotIndex, 0.50f); break;
                               case 13: editor->setModRouteAmount(slotIndex, -0.25f); break;
                               case 14: editor->setModRouteAmount(slotIndex, -0.50f); break;
                               case 30:
                               case 31:
                               case 32:
                               case 33:
                               case 34:
                               case 35:
                               case 36:
                               case 37:
                                   editor->applyModRouteShapePreset(slotIndex, result - 30);
                                   break;
                               case 40:
                               case 41:
                               case 42:
                               case 43:
                                   editor->applyStepLfoPreset(result - 40);
                                   break;
                               case 20: editor->duplicateModRoute(slotIndex); break;
                               case 21: editor->deleteModRoute(slotIndex); break;
                               default: break;
                           }
                       });
}

void NateVSTAudioProcessorEditor::setModRouteAmount(size_t slotIndex, float amount)
{
    if (slotIndex >= Parameters::ID::modMatrixAmount.size())
        return;

    amount = juce::jlimit(-1.0f, 1.0f, amount);
    captureGlobalEdit("Edit mod route amount");
    setPlainParameterValue(Parameters::ID::modMatrixAmount[slotIndex], amount);

    updateModMatrixRows();
    updateModDestinationIndicators();
    updateModInspectorStatus();
    updateMacroAssignmentEditorStatus();

    modMatrixStatusLabel.setText("Set S" + juce::String(static_cast<int>(slotIndex + 1))
                                     + " amount " + (amount >= 0.0f ? "+" : "")
                                     + juce::String(juce::roundToInt(amount * 100.0f)) + "%",
                                 juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::applyModRouteShapePreset(size_t slotIndex, int presetId)
{
    if (slotIndex >= Parameters::ID::modMatrixSource.size())
        return;

    captureGlobalEdit("Edit mod route shape");
    resetModRouteShape(slotIndex);

    switch (presetId)
    {
        case 1:
            setPlainParameterValue(Parameters::ID::modMatrixPolarity[slotIndex], 1.0f);
            break;

        case 2:
            setPlainParameterValue(Parameters::ID::modMatrixPolarity[slotIndex], 3.0f);
            break;

        case 3:
            setPlainParameterValue(Parameters::ID::modMatrixCurve[slotIndex], 1.0f);
            setPlainParameterValue(Parameters::ID::modMatrixSlew[slotIndex], 0.28f);
            break;

        case 4:
            setPlainParameterValue(Parameters::ID::modMatrixPolarity[slotIndex], 1.0f);
            setPlainParameterValue(Parameters::ID::modMatrixCurve[slotIndex], 4.0f);
            break;

        case 5:
            setPlainParameterValue(Parameters::ID::modMatrixRangeMin[slotIndex], -0.55f);
            setPlainParameterValue(Parameters::ID::modMatrixRangeMax[slotIndex], 0.55f);
            break;

        case 6:
            setPlainParameterValue(Parameters::ID::modMatrixRangeMin[slotIndex], 0.0f);
            setPlainParameterValue(Parameters::ID::modMatrixRangeMax[slotIndex], 1.0f);
            break;

        case 7:
            setPlainParameterValue(Parameters::ID::modMatrixSlew[slotIndex], 0.38f);
            break;

        case 0:
        default:
            break;
    }

    updateModMatrixRows();
    updateModDestinationIndicators();
    updateModInspectorStatus();
    updateMacroAssignmentEditorStatus();

    const auto summary = modRouteShapeSummary(slotIndex);
    modMatrixStatusLabel.setText("S" + juce::String(static_cast<int>(slotIndex + 1))
                                     + (summary.isNotEmpty() ? " shape " + summary : " shape reset"),
                                 juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::applyStepLfoPreset(int presetId)
{
    static constexpr std::array<std::array<float, 8>, 4> stepPresets {{
        { 1.0f, -0.15f, 0.72f, -0.45f, 0.92f, 0.08f, -0.68f, 0.32f },
        { 1.0f, -1.0f, 0.78f, -1.0f, 1.0f, -0.72f, 0.58f, -1.0f },
        { -0.20f, 0.18f, 0.46f, 0.82f, -0.10f, 0.28f, 0.58f, 1.0f },
        { 1.0f, 0.35f, -0.82f, 0.35f, 0.78f, -0.45f, 1.0f, -0.70f }
    }};

    static constexpr std::array<const char*, 4> names {
        "Classic Gate",
        "Trance Chopper",
        "House Lift",
        "Techno Ratchet"
    };

    const auto safePreset = juce::jlimit(0, static_cast<int>(stepPresets.size() - 1), presetId);
    captureGlobalEdit("Load Step LFO pattern");
    setPlainParameterValue(Parameters::ID::stepLfoSync, 1.0f);
    setPlainParameterValue(Parameters::ID::stepLfoSyncRate, safePreset == 1 || safePreset == 3 ? 3.0f : 1.0f);
    setPlainParameterValue(Parameters::ID::stepLfoRate, 2.0f);
    setPlainParameterValue(Parameters::ID::stepLfoDepth, safePreset == 1 ? 0.72f : 0.58f);
    setPlainParameterValue(Parameters::ID::stepLfoSlew, safePreset == 2 ? 0.18f : 0.0f);

    for (size_t index = 0; index < Parameters::ID::stepLfoValue.size(); ++index)
        setPlainParameterValue(Parameters::ID::stepLfoValue[index], stepPresets[static_cast<size_t>(safePreset)][index]);

    updateModMatrixRows();
    updateModDestinationIndicators();
    modMatrixStatusLabel.setText("Loaded Step LFO " + juce::String(names[static_cast<size_t>(safePreset)]),
                                 juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::resetModRouteShape(size_t slotIndex)
{
    if (slotIndex >= Parameters::ID::modMatrixSource.size())
        return;

    setPlainParameterValue(Parameters::ID::modMatrixPolarity[slotIndex], 0.0f);
    setPlainParameterValue(Parameters::ID::modMatrixCurve[slotIndex], 0.0f);
    setPlainParameterValue(Parameters::ID::modMatrixRangeMin[slotIndex], -1.0f);
    setPlainParameterValue(Parameters::ID::modMatrixRangeMax[slotIndex], 1.0f);
    setPlainParameterValue(Parameters::ID::modMatrixSlew[slotIndex], 0.0f);
}

juce::String NateVSTAudioProcessorEditor::modRouteShapeSummary(size_t slotIndex) const
{
    if (slotIndex >= Parameters::ID::modMatrixSource.size())
        return {};

    juce::StringArray parts;
    const auto polarityChoices = Parameters::modulationRoutePolarityChoices();
    const auto curveChoices = Parameters::modulationRouteCurveChoices();
    const auto polarityIndex = juce::jlimit(0,
                                            polarityChoices.size() - 1,
                                            juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixPolarity[slotIndex], 0.0f)));
    const auto curveIndex = juce::jlimit(0,
                                         curveChoices.size() - 1,
                                         juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixCurve[slotIndex], 0.0f)));
    const auto rangeMin = readPlainParameterValue(Parameters::ID::modMatrixRangeMin[slotIndex], -1.0f);
    const auto rangeMax = readPlainParameterValue(Parameters::ID::modMatrixRangeMax[slotIndex], 1.0f);
    const auto slew = readPlainParameterValue(Parameters::ID::modMatrixSlew[slotIndex], 0.0f);

    if (polarityIndex > 0)
        parts.add(polarityChoices[polarityIndex]);
    if (curveIndex > 0)
        parts.add(curveChoices[curveIndex]);
    if (std::abs(rangeMin + 1.0f) > 0.002f || std::abs(rangeMax - 1.0f) > 0.002f)
    {
        const auto minPercent = juce::roundToInt(rangeMin * 100.0f);
        const auto maxPercent = juce::roundToInt(rangeMax * 100.0f);
        parts.add("Range " + juce::String(minPercent) + ".." + juce::String(maxPercent) + "%");
    }
    if (slew > 0.002f)
        parts.add("Slew " + juce::String(juce::roundToInt(slew * 100.0f)) + "%");

    return parts.joinIntoString(", ");
}

void NateVSTAudioProcessorEditor::addModRouteForParameter(const juce::String& parameterID,
                                                          const juce::String& labelText,
                                                          int sourceIndex)
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto destinationIndex = modulationDestinationIndexForParameter(parameterID);
    if (destinationIndex <= 0 || ! juce::isPositiveAndBelow(sourceIndex, sourceChoices.size()))
        return;

    setModInspectorDestination(destinationIndex);
    modInspectorSourceBox.setSelectedId(sourceIndex + 1, juce::dontSendNotification);
    addInspectedModRoute();

    updateSelectedControlInspector(labelText, parameterID, readPlainParameterValue(parameterID, 0.0f));
}

void NateVSTAudioProcessorEditor::configureSectionLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centredLeft);
    label.setColour(juce::Label::textColourId, uiTheme().accent);
    addAndMakeVisible(label);
}

juce::Rectangle<int> NateVSTAudioProcessorEditor::layoutKnobRow(juce::Rectangle<int> area,
                                                                  std::initializer_list<juce::Component*> components)
{
    const auto count = static_cast<int>(components.size());
    if (count == 0)
        return area;

    const auto cellWidth = juce::jmax(1, area.getWidth() / count);
    const auto horizontalPadding = cellWidth < 64 ? 2 : 4;

    for (auto* component : components)
    {
        component->setVisible(true);
        component->setBounds(area.removeFromLeft(cellWidth).reduced(horizontalPadding, 0));
    }

    return area;
}

void NateVSTAudioProcessorEditor::chooseSampleFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load sample",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.aif;*.aiff");

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [this] (const juce::FileChooser& chooser)
                             {
                                 const auto file = chooser.getResult();
                                 if (file.existsAsFile())
                                     loadSampleFile(file);
                             });
}

void NateVSTAudioProcessorEditor::exportSequencerMidiClip()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Export sequencer MIDI",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile("Nate VST Sequence.mid"),
        "*.mid;*.midi");

    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode
                                 | juce::FileBrowserComponent::canSelectFiles
                                 | juce::FileBrowserComponent::warnAboutOverwriting,
                             [this] (const juce::FileChooser& chooser)
                             {
                                 auto file = chooser.getResult();
                                 if (file == juce::File{})
                                     return;

                                 if (! file.hasFileExtension(".mid;.midi"))
                                     file = file.withFileExtension(".mid");

                                 setRandomStatus(audioProcessor.exportSequencerMidiFile(file)
                                                     ? "MIDI exported"
                                                     : "MIDI export skipped");
	                             });
}

void NateVSTAudioProcessorEditor::exportSequencerSceneChainMidiClip()
{
    const auto chainBars = audioProcessor.getSequencerSceneChainClipBars();
    const auto chainSuffix = chainBars == 0 ? juce::String() : " " + juce::String(chainBars) + " Bar";
    fileChooser = std::make_unique<juce::FileChooser>(
        "Export chained sequencer MIDI",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile("Nate VST Scene Chain" + chainSuffix + ".mid"),
        "*.mid;*.midi");

    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode
                                 | juce::FileBrowserComponent::canSelectFiles
                                 | juce::FileBrowserComponent::warnAboutOverwriting,
                             [this] (const juce::FileChooser& chooser)
                             {
                                 auto file = chooser.getResult();
                                 if (file == juce::File{})
                                     return;

                                 if (! file.hasFileExtension(".mid;.midi"))
                                     file = file.withFileExtension(".mid");

                                 setRandomStatus(audioProcessor.exportSequencerSceneChainMidiFile(file)
                                                     ? "Chain MIDI exported"
                                                     : "Chain export skipped");
                             });
}

bool NateVSTAudioProcessorEditor::beginSequencerMidiDrag(juce::Component& sourceComponent, bool sceneChain)
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();
    pruneSequencerDragMidiFiles();

    const auto dragDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("Nate VST MIDI Drag");

    if (! dragDirectory.createDirectory())
    {
        setRandomStatus("MIDI drag folder unavailable");
        return false;
    }

    const auto chainBars = sceneChain ? audioProcessor.getSequencerSceneChainClipBars() : 0;
    const auto chainSuffix = chainBars == 0 ? juce::String() : " " + juce::String(chainBars) + " Bar";
    const auto file = dragDirectory.getNonexistentChildFile(sceneChain ? "Nate VST Scene Chain" + chainSuffix
                                                                      : "Nate VST Sequence",
                                                            ".mid",
                                                            false);
    const auto exported = sceneChain ? audioProcessor.exportSequencerSceneChainMidiFile(file)
                                    : audioProcessor.exportSequencerMidiFile(file);
    if (! exported || ! file.existsAsFile())
    {
        setRandomStatus(sceneChain ? "Chain MIDI drag skipped" : "MIDI drag skipped");
        return false;
    }

    juce::StringArray files;
    files.add(file.getFullPathName());
    if (! juce::DragAndDropContainer::performExternalDragDropOfFiles(files, false, &sourceComponent))
    {
        file.deleteFile();
        setRandomStatus("MIDI drag unavailable");
        return false;
    }

    sequencerDragMidiFiles.push_back(file);
    setRandomStatus(sceneChain ? ("Drag" + chainSuffix + " chain MIDI into Ableton") : "Drag MIDI into Ableton");
    return true;
}

void NateVSTAudioProcessorEditor::pruneSequencerDragMidiFiles()
{
    for (const auto& file : sequencerDragMidiFiles)
        if (file.existsAsFile())
            file.deleteFile();

    sequencerDragMidiFiles.clear();
}

void NateVSTAudioProcessorEditor::loadSampleFile(const juce::File& file)
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();
    captureGlobalEdit("Load sample");

    if (audioProcessor.loadSampleFile(file))
    {
        sampleWaveformKey.clear();
        updateSampleNameLabel();
        updateSampleWaveformDisplay();
        updateSampleRecorderStatus();
    }
}

void NateVSTAudioProcessorEditor::updateSampleNameLabel()
{
    const auto sampleName = audioProcessor.getLoadedSampleName();
    const auto sampleMissing = audioProcessor.hasMissingSampleReference();
    const auto missingFile = juce::File(audioProcessor.getLoadedSamplePath()).getFileName();
    sampleNameLabel.setText(sampleName.isNotEmpty() ? sampleName
                            : sampleMissing && missingFile.isNotEmpty() ? "Missing: " + missingFile
                                                                         : "No sample",
                            juce::dontSendNotification);
    sampleNameLabel.setColour(juce::Label::textColourId,
                              sampleMissing ? juce::Colour(0xffffc36b) : juce::Colour(0xffa8b6b8));
}

void NateVSTAudioProcessorEditor::updateSampleRecorderStatus()
{
    sampleRecorderPanel.setState({
        audioProcessor.isSampleCaptureEnabled(),
        audioProcessor.getSampleCaptureDurationSeconds(),
        audioProcessor.getSampleCaptureCapacitySeconds(),
        audioProcessor.hasLoadedSample(),
        audioProcessor.getSampleCaptureSourceIndex(),
        audioProcessor.getSampleCaptureStartModeIndex(),
        audioProcessor.getSampleCaptureLengthModeIndex(),
        audioProcessor.getSampleCapturePreRollModeIndex(),
        audioProcessor.getSampleCaptureSourcePeak(),
        audioProcessor.getSampleCaptureTargetDurationSeconds(),
        audioProcessor.getSampleCapturePreRollDurationSeconds(),
        audioProcessor.isSampleCaptureWaitingForThreshold()
    });
}

void NateVSTAudioProcessorEditor::selectSampleSlice(size_t sliceIndex)
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto sliceCount = static_cast<float>(sampleChopPanel.getSliceCount());
    if (sliceCount <= 0.0f)
        return;

    const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, sliceIndex);
    const auto hasCustomSettings = sampleSliceHasCustomSettings(safeIndex);
    const auto start = hasCustomSettings
        ? readPlainParameterValue(Parameters::ID::sampleSliceStart[safeIndex], static_cast<float>(safeIndex) / sliceCount)
        : static_cast<float>(safeIndex) / sliceCount;
    const auto end = hasCustomSettings
        ? readPlainParameterValue(Parameters::ID::sampleSliceEnd[safeIndex], static_cast<float>(safeIndex + 1) / sliceCount)
        : static_cast<float>(safeIndex + 1) / sliceCount;
    selectedSampleSliceIndex = safeIndex;

    setPlainParameterValue(Parameters::ID::sampleEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::sampleStart, start);
    setPlainParameterValue(Parameters::ID::sampleEnd, end);
    if (readPlainParameterValue(Parameters::ID::samplePlaybackMode, 1.0f) < 1.5f)
        setPlainParameterValue(Parameters::ID::samplePlaybackMode, 1.0f);

    if (hasCustomSettings)
        recallSampleSliceSettings(safeIndex);
    else
    {
        applySampleSliceStyleDefaults(safeIndex);
        captureCurrentSampleSliceSettings(safeIndex, false);
    }

    const auto didAudition = audioProcessor.triggerSampleSliceAudition(static_cast<int>(safeIndex));
    setRandomStatus("Slice " + juce::String(static_cast<int>(safeIndex + 1))
                    + (sampleSliceHasCustomSettings(safeIndex) ? " custom" : " " + sampleSliceStyleBox.getText())
                    + (didAudition ? " auditioned" : " selected"));
    updateSampleSliceButtons();
    updateSampleSliceEditorStatus();
    updateSampleWaveformDisplay();
}

void NateVSTAudioProcessorEditor::applySampleSliceStyleDefaults(size_t sliceIndex)
{
    const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, sliceIndex);
    const auto styleIndex = juce::jlimit(0, 4, sampleSliceStyleBox.getSelectedItemIndex());
    const auto slicePosition = static_cast<int>(safeIndex);
    const std::array<float, 8> pitchLadder { -12.0f, -7.0f, -5.0f, 0.0f, 3.0f, 7.0f, 10.0f, 12.0f };
    const std::array<float, 8> garagePitch { -12.0f, 0.0f, 7.0f, -5.0f, 0.0f, 12.0f, 3.0f, -7.0f };
    const std::array<float, 8> garageRamp { 7.0f, 0.0f, -5.0f, 12.0f, 0.0f, -7.0f, 5.0f, -12.0f };
    setPlainParameterValue(Parameters::ID::sampleSliceNudge[safeIndex], 0.0f);
    setPlainParameterValue(Parameters::ID::sampleSliceFade[safeIndex], 0.0f);

    switch (styleIndex)
    {
        case 1: // Pitch
            setPlainParameterValue(Parameters::ID::sampleSliceChoke[safeIndex], 0.0f);
            setPlainParameterValue(Parameters::ID::sampleReverse, 0.0f);
            setPlainParameterValue(Parameters::ID::sampleTranspose, pitchLadder[safeIndex]);
            setPlainParameterValue(Parameters::ID::samplePitchRamp, 0.0f);
            setPlainParameterValue(Parameters::ID::sampleGain, -7.0f + static_cast<float>(slicePosition % 3));
            setPlainParameterValue(Parameters::ID::sampleStutterEnabled, 0.0f);
            break;

        case 2: // Reverse
            setPlainParameterValue(Parameters::ID::sampleSliceChoke[safeIndex], 0.0f);
            setPlainParameterValue(Parameters::ID::sampleReverse, (slicePosition % 2) == 0 ? 0.0f : 1.0f);
            setPlainParameterValue(Parameters::ID::sampleTranspose, pitchLadder[static_cast<size_t>((slicePosition + 2) % 8)]);
            setPlainParameterValue(Parameters::ID::samplePitchRamp, (slicePosition % 3) == 0 ? -7.0f : 0.0f);
            setPlainParameterValue(Parameters::ID::sampleGain, -7.5f);
            setPlainParameterValue(Parameters::ID::sampleStutterEnabled, 0.0f);
            break;

        case 3: // Stutter
            setPlainParameterValue(Parameters::ID::sampleSliceChoke[safeIndex], 1.0f);
            setPlainParameterValue(Parameters::ID::sampleReverse, 0.0f);
            setPlainParameterValue(Parameters::ID::sampleTranspose, garagePitch[static_cast<size_t>((slicePosition + 1) % 8)] * 0.5f);
            setPlainParameterValue(Parameters::ID::samplePitchRamp, 0.0f);
            setPlainParameterValue(Parameters::ID::sampleGain, -8.0f);
            setPlainParameterValue(Parameters::ID::sampleStutterEnabled, 1.0f);
            setPlainParameterValue(Parameters::ID::sampleStutterRate, static_cast<float>((slicePosition % 2) + 1));
            setPlainParameterValue(Parameters::ID::sampleStutterRepeats, static_cast<float>(2 + (slicePosition % 4)));
            break;

        case 4: // Garage
            setPlainParameterValue(Parameters::ID::sampleSliceChoke[safeIndex], 1.0f);
            setPlainParameterValue(Parameters::ID::sampleReverse, (slicePosition == 2 || slicePosition == 6) ? 1.0f : 0.0f);
            setPlainParameterValue(Parameters::ID::sampleTranspose, garagePitch[safeIndex]);
            setPlainParameterValue(Parameters::ID::samplePitchRamp, garageRamp[safeIndex]);
            setPlainParameterValue(Parameters::ID::sampleGain, -8.5f + static_cast<float>(slicePosition % 4) * 0.8f);
            setPlainParameterValue(Parameters::ID::sampleStutterEnabled, (slicePosition == 3 || slicePosition == 7) ? 1.0f : 0.0f);
            setPlainParameterValue(Parameters::ID::sampleStutterRate, (slicePosition == 7) ? 2.0f : 1.0f);
            setPlainParameterValue(Parameters::ID::sampleStutterRepeats, (slicePosition == 7) ? 5.0f : 3.0f);
            break;

        case 0: // Clean
        default:
            setPlainParameterValue(Parameters::ID::sampleSliceChoke[safeIndex], 0.0f);
            setPlainParameterValue(Parameters::ID::sampleReverse, 0.0f);
            setPlainParameterValue(Parameters::ID::sampleTranspose, 0.0f);
            setPlainParameterValue(Parameters::ID::samplePitchRamp, 0.0f);
            setPlainParameterValue(Parameters::ID::sampleGain, -6.0f);
            setPlainParameterValue(Parameters::ID::sampleStutterEnabled, 0.0f);
            break;
    }
}

void NateVSTAudioProcessorEditor::captureCurrentSampleSliceSettings(size_t sliceIndex, bool markCustom)
{
    const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, sliceIndex);
    const auto sliceCount = static_cast<float>(sampleChopPanel.getSliceCount());
    setPlainParameterValue(Parameters::ID::sampleSliceCustom[safeIndex], markCustom ? 1.0f : 0.0f);
    setPlainParameterValue(Parameters::ID::sampleSliceStart[safeIndex],
                           readPlainParameterValue(Parameters::ID::sampleStart, static_cast<float>(safeIndex) / sliceCount));
    setPlainParameterValue(Parameters::ID::sampleSliceEnd[safeIndex],
                           readPlainParameterValue(Parameters::ID::sampleEnd, static_cast<float>(safeIndex + 1) / sliceCount));
    setPlainParameterValue(Parameters::ID::sampleSliceReverse[safeIndex], readPlainParameterValue(Parameters::ID::sampleReverse, 0.0f));
    setPlainParameterValue(Parameters::ID::sampleSliceTranspose[safeIndex], readPlainParameterValue(Parameters::ID::sampleTranspose, 0.0f));
    setPlainParameterValue(Parameters::ID::sampleSliceGain[safeIndex], readPlainParameterValue(Parameters::ID::sampleGain, -6.0f));
    setPlainParameterValue(Parameters::ID::sampleSlicePan[safeIndex], readPlainParameterValue(Parameters::ID::sampleSlicePan[safeIndex], 0.0f));
    setPlainParameterValue(Parameters::ID::sampleSliceProbability[safeIndex], readPlainParameterValue(Parameters::ID::sampleSliceProbability[safeIndex], 1.0f));
    setPlainParameterValue(Parameters::ID::sampleSliceStutter[safeIndex], readPlainParameterValue(Parameters::ID::sampleStutterEnabled, 0.0f));
    setPlainParameterValue(Parameters::ID::sampleSliceStutterRepeats[safeIndex], readPlainParameterValue(Parameters::ID::sampleStutterRepeats, 3.0f));
    setPlainParameterValue(Parameters::ID::sampleSliceNudge[safeIndex], readPlainParameterValue(Parameters::ID::sampleSliceNudge[safeIndex], 0.0f));
    setPlainParameterValue(Parameters::ID::sampleSliceFade[safeIndex], readPlainParameterValue(Parameters::ID::sampleSliceFade[safeIndex], 0.0f));
}

void NateVSTAudioProcessorEditor::recallSampleSliceSettings(size_t sliceIndex)
{
    const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, sliceIndex);
    const auto sliceCount = static_cast<float>(sampleChopPanel.getSliceCount());
    setPlainParameterValue(Parameters::ID::sampleStart,
                           readPlainParameterValue(Parameters::ID::sampleSliceStart[safeIndex], static_cast<float>(safeIndex) / sliceCount));
    setPlainParameterValue(Parameters::ID::sampleEnd,
                           readPlainParameterValue(Parameters::ID::sampleSliceEnd[safeIndex], static_cast<float>(safeIndex + 1) / sliceCount));
    setPlainParameterValue(Parameters::ID::sampleReverse, readPlainParameterValue(Parameters::ID::sampleSliceReverse[safeIndex], 0.0f));
    setPlainParameterValue(Parameters::ID::sampleTranspose, readPlainParameterValue(Parameters::ID::sampleSliceTranspose[safeIndex], 0.0f));
    setPlainParameterValue(Parameters::ID::sampleGain, readPlainParameterValue(Parameters::ID::sampleSliceGain[safeIndex], -6.0f));
    setPlainParameterValue(Parameters::ID::sampleStutterEnabled, readPlainParameterValue(Parameters::ID::sampleSliceStutter[safeIndex], 0.0f));
    setPlainParameterValue(Parameters::ID::sampleStutterRepeats, readPlainParameterValue(Parameters::ID::sampleSliceStutterRepeats[safeIndex], 3.0f));
}

void NateVSTAudioProcessorEditor::editSampleSliceBoundary(size_t boundaryIndex, float position)
{
    if (! audioProcessor.hasLoadedSample() || sampleChopPanel.getSliceCount() == 0)
        return;

    constexpr auto minimumSliceWidth = 0.004f;
    const auto sliceCount = static_cast<float>(sampleChopPanel.getSliceCount());
    const auto maxBoundaryIndex = sampleChopPanel.getSliceCount();
    const auto safeBoundaryIndex = juce::jlimit<size_t>(0, maxBoundaryIndex, boundaryIndex);

    auto materializeSlice = [this] (size_t sliceIndex)
    {
        const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, sliceIndex);
        if (sampleSliceHasCustomSettings(safeIndex))
            return;

        const auto preview = defaultSlicePreviewSettings(safeIndex, sampleSliceStyleBox.getSelectedItemIndex());
        setPlainParameterValue(Parameters::ID::sampleSliceReverse[safeIndex], preview.reverse ? 1.0f : 0.0f);
        setPlainParameterValue(Parameters::ID::sampleSliceTranspose[safeIndex], preview.pitch);
        setPlainParameterValue(Parameters::ID::sampleSliceGain[safeIndex], preview.gain);
        setPlainParameterValue(Parameters::ID::sampleSlicePan[safeIndex], preview.pan);
        setPlainParameterValue(Parameters::ID::sampleSliceProbability[safeIndex], preview.probability);
        setPlainParameterValue(Parameters::ID::sampleSliceStutter[safeIndex], preview.stutter ? 1.0f : 0.0f);
        setPlainParameterValue(Parameters::ID::sampleSliceChoke[safeIndex], preview.choke ? 1.0f : 0.0f);
        setPlainParameterValue(Parameters::ID::sampleSliceStutterRepeats[safeIndex], static_cast<float>(preview.repeats));
        setPlainParameterValue(Parameters::ID::sampleSliceNudge[safeIndex], preview.nudgePercent);
        setPlainParameterValue(Parameters::ID::sampleSliceFade[safeIndex], preview.fade);
        setPlainParameterValue(Parameters::ID::sampleSliceCustom[safeIndex], 1.0f);
    };

    auto sliceStart = [this, sliceCount] (size_t sliceIndex)
    {
        const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, sliceIndex);
        const auto fallback = static_cast<float>(safeIndex) / sliceCount;
        return juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::sampleSliceStart[safeIndex], fallback));
    };

    auto sliceEnd = [this, sliceCount] (size_t sliceIndex)
    {
        const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, sliceIndex);
        const auto fallback = static_cast<float>(safeIndex + 1) / sliceCount;
        return juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::sampleSliceEnd[safeIndex], fallback));
    };

    auto clampBoundary = [] (float lower, float upper, float value)
    {
        if (upper <= lower)
            return juce::jlimit(0.0f, 1.0f, (lower + upper) * 0.5f);

        return juce::jlimit(lower, upper, juce::jlimit(0.0f, 1.0f, value));
    };

    position = juce::jlimit(0.0f, 1.0f, position);
    if (safeBoundaryIndex == 0)
    {
        materializeSlice(0);
        position = clampBoundary(0.0f, sliceEnd(0) - minimumSliceWidth, position);
        setPlainParameterValue(Parameters::ID::sampleSliceStart[0], position);
        selectedSampleSliceIndex = 0;
    }
    else if (safeBoundaryIndex == maxBoundaryIndex)
    {
        const auto lastSliceIndex = sampleChopPanel.getSliceCount() - 1;
        materializeSlice(lastSliceIndex);
        position = clampBoundary(sliceStart(lastSliceIndex) + minimumSliceWidth, 1.0f, position);
        setPlainParameterValue(Parameters::ID::sampleSliceEnd[lastSliceIndex], position);
        selectedSampleSliceIndex = lastSliceIndex;
    }
    else
    {
        const auto leftIndex = safeBoundaryIndex - 1;
        const auto rightIndex = safeBoundaryIndex;
        materializeSlice(leftIndex);
        materializeSlice(rightIndex);
        position = clampBoundary(sliceStart(leftIndex) + minimumSliceWidth,
                                 sliceEnd(rightIndex) - minimumSliceWidth,
                                 position);
        setPlainParameterValue(Parameters::ID::sampleSliceEnd[leftIndex], position);
        setPlainParameterValue(Parameters::ID::sampleSliceStart[rightIndex], position);
        selectedSampleSliceIndex = rightIndex;
    }

    const auto selectedIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, selectedSampleSliceIndex);
    setPlainParameterValue(Parameters::ID::sampleEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::samplePlaybackMode, 2.0f);
    setPlainParameterValue(Parameters::ID::sampleStart, sliceStart(selectedIndex));
    setPlainParameterValue(Parameters::ID::sampleEnd, sliceEnd(selectedIndex));
    updateSampleSliceButtons();
    updateSampleSliceEditorStatus();
    updateSampleWaveformDisplay();
}

void NateVSTAudioProcessorEditor::storeSelectedSampleSliceSettings()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    captureGlobalEdit("Store sample slice");
    captureCurrentSampleSliceSettings(selectedSampleSliceIndex, true);
    setRandomStatus("Stored slice " + juce::String(static_cast<int>(selectedSampleSliceIndex + 1)));
    updateSampleSliceButtons();
    updateSampleSliceEditorStatus();
    updateSampleWaveformDisplay();
}

void NateVSTAudioProcessorEditor::recallSelectedSampleSliceSettings()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, selectedSampleSliceIndex);
    captureGlobalEdit("Recall sample slice");
    recallSampleSliceSettings(safeIndex);

    const auto didAudition = audioProcessor.triggerSampleSliceAudition(static_cast<int>(safeIndex));
    setRandomStatus("Recalled slice " + juce::String(static_cast<int>(safeIndex + 1)) + (didAudition ? " auditioned" : ""));
    updateSampleSliceButtons();
    updateSampleSliceEditorStatus();
    updateSampleWaveformDisplay();
}

void NateVSTAudioProcessorEditor::detectSampleSliceMarkers()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    if (! audioProcessor.hasLoadedSample())
    {
        setRandomStatus("Detect skipped: load a sample first");
        return;
    }

    captureGlobalEdit("Detect sample slices");
    const auto transientCount = audioProcessor.detectSampleTransientSlices();
    if (transientCount < 0)
    {
        setRandomStatus("Detect skipped: load a sample first");
        return;
    }

    selectedSampleSliceIndex = 0;
    recallSampleSliceSettings(selectedSampleSliceIndex);
    const auto didAudition = audioProcessor.triggerSampleSliceAudition(static_cast<int>(selectedSampleSliceIndex));
    setRandomStatus("Detected "
                    + juce::String(transientCount)
                    + " transient"
                    + (transientCount == 1 ? "" : "s")
                    + " into 8 slices"
                    + (didAudition ? " | S1 auditioned" : ""));
    updateSampleSliceButtons();
    updateSampleSliceEditorStatus();
    updateSampleWaveformDisplay();
}

void NateVSTAudioProcessorEditor::randomizeSelectedSampleSliceSettings()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, selectedSampleSliceIndex);
    static const std::array<float, 9> garagePitchChoices { -12.0f, -7.0f, -5.0f, 0.0f, 3.0f, 5.0f, 7.0f, 10.0f, 12.0f };
    auto& random = juce::Random::getSystemRandom();
    const auto pitch = garagePitchChoices[static_cast<size_t>(random.nextInt(static_cast<int>(garagePitchChoices.size())))];
    const auto gain = -10.5f + (random.nextFloat() * 6.5f);
    const auto reverse = random.nextFloat() < 0.28f;
    const auto stutter = random.nextFloat() < 0.38f;
    const auto choke = stutter || random.nextFloat() < 0.62f;
    const auto repeats = static_cast<float>(2 + random.nextInt(5));
    const auto pan = random.nextFloat() < 0.68f ? (random.nextFloat() * 0.9f) - 0.45f : 0.0f;
    const auto probability = random.nextFloat() < 0.34f ? 0.62f + (random.nextFloat() * 0.28f) : 1.0f;
    const auto nudge = random.nextFloat() < 0.52f ? ((random.nextFloat() * 5.0f) - 2.5f) : 0.0f;
    const auto fade = random.nextFloat() < 0.44f ? 0.22f + (random.nextFloat() * 0.48f) : 0.0f;

    captureGlobalEdit("Dice sample slice");
    setPlainParameterValue(Parameters::ID::sampleTranspose, pitch);
    setPlainParameterValue(Parameters::ID::sampleGain, gain);
    setPlainParameterValue(Parameters::ID::sampleReverse, reverse ? 1.0f : 0.0f);
    setPlainParameterValue(Parameters::ID::sampleStutterEnabled, stutter ? 1.0f : 0.0f);
    setPlainParameterValue(Parameters::ID::sampleSliceChoke[safeIndex], choke ? 1.0f : 0.0f);
    setPlainParameterValue(Parameters::ID::sampleStutterRepeats, repeats);
    setPlainParameterValue(Parameters::ID::sampleSlicePan[safeIndex], pan);
    setPlainParameterValue(Parameters::ID::sampleSliceProbability[safeIndex], probability);
    setPlainParameterValue(Parameters::ID::sampleSliceNudge[safeIndex], nudge);
    setPlainParameterValue(Parameters::ID::sampleSliceFade[safeIndex], fade);
    captureCurrentSampleSliceSettings(safeIndex, true);

    const auto didAudition = audioProcessor.triggerSampleSliceAudition(static_cast<int>(safeIndex));
    setRandomStatus("Diced slice " + juce::String(static_cast<int>(safeIndex + 1)) + (didAudition ? " auditioned" : ""));
    updateSampleSliceButtons();
    updateSampleSliceEditorStatus();
    updateSampleWaveformDisplay();
}

void NateVSTAudioProcessorEditor::toggleSelectedSampleSliceReverse()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, selectedSampleSliceIndex);
    const auto currentReverse = sampleSliceHasCustomSettings(safeIndex)
        ? readPlainParameterValue(Parameters::ID::sampleSliceReverse[safeIndex], 0.0f) >= 0.5f
        : defaultSlicePreviewSettings(safeIndex, sampleSliceStyleBox.getSelectedItemIndex()).reverse;
    const auto nextReverse = ! currentReverse;
    captureGlobalEdit("Toggle sample slice reverse");
    setPlainParameterValue(Parameters::ID::sampleReverse, nextReverse ? 1.0f : 0.0f);
    captureCurrentSampleSliceSettings(safeIndex, true);

    const auto didAudition = audioProcessor.triggerSampleSliceAudition(static_cast<int>(safeIndex));
    setRandomStatus((nextReverse ? "Reversed" : "Forward") + juce::String(" slice ") + juce::String(static_cast<int>(safeIndex + 1))
                    + (didAudition ? " auditioned" : ""));
    updateSampleSliceButtons();
    updateSampleSliceEditorStatus();
    updateSampleWaveformDisplay();
}

void NateVSTAudioProcessorEditor::toggleSelectedSampleSliceChoke()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, selectedSampleSliceIndex);
    const auto currentChoke = sampleSliceHasCustomSettings(safeIndex)
        ? readPlainParameterValue(Parameters::ID::sampleSliceChoke[safeIndex], 0.0f) >= 0.5f
        : defaultSlicePreviewSettings(safeIndex, sampleSliceStyleBox.getSelectedItemIndex()).choke;
    const auto nextChoke = ! currentChoke;
    captureGlobalEdit("Toggle sample slice choke");
    setPlainParameterValue(Parameters::ID::sampleSliceChoke[safeIndex], nextChoke ? 1.0f : 0.0f);
    captureCurrentSampleSliceSettings(safeIndex, true);

    const auto didAudition = audioProcessor.triggerSampleSliceAudition(static_cast<int>(safeIndex));
    setRandomStatus((nextChoke ? "Choked" : "Open") + juce::String(" slice ") + juce::String(static_cast<int>(safeIndex + 1))
                    + (didAudition ? " auditioned" : ""));
    updateSampleSliceButtons();
    updateSampleSliceEditorStatus();
    updateSampleWaveformDisplay();
}

void NateVSTAudioProcessorEditor::cycleSelectedSampleSlicePan()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, selectedSampleSliceIndex);
    const auto currentPan = sampleSliceHasCustomSettings(safeIndex)
        ? readPlainParameterValue(Parameters::ID::sampleSlicePan[safeIndex], 0.0f)
        : 0.0f;
    const auto nextPan = currentPan < -0.2f ? 0.35f : currentPan > 0.2f ? 0.0f : -0.35f;

    captureGlobalEdit("Pan sample slice");
    setPlainParameterValue(Parameters::ID::sampleSlicePan[safeIndex], nextPan);
    captureCurrentSampleSliceSettings(safeIndex, true);

    const auto didAudition = audioProcessor.triggerSampleSliceAudition(static_cast<int>(safeIndex));
    setRandomStatus("Panned slice " + juce::String(static_cast<int>(safeIndex + 1))
                    + " " + slicePanText(nextPan)
                    + (didAudition ? " auditioned" : ""));
    updateSampleSliceButtons();
    updateSampleSliceEditorStatus();
    updateSampleWaveformDisplay();
}

void NateVSTAudioProcessorEditor::toggleSelectedSampleSliceGhost()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, selectedSampleSliceIndex);
    const auto currentProbability = sampleSliceHasCustomSettings(safeIndex)
        ? readPlainParameterValue(Parameters::ID::sampleSliceProbability[safeIndex], 1.0f)
        : 1.0f;
    const auto nextProbability = currentProbability < 0.995f ? 1.0f : 0.72f;

    captureGlobalEdit("Ghost sample slice");
    setPlainParameterValue(Parameters::ID::sampleSliceProbability[safeIndex], nextProbability);
    captureCurrentSampleSliceSettings(safeIndex, true);

    const auto didAudition = audioProcessor.triggerSampleSliceAudition(static_cast<int>(safeIndex));
    setRandomStatus("Slice " + juce::String(static_cast<int>(safeIndex + 1))
                    + " chance " + sliceChanceText(nextProbability)
                    + (didAudition ? " auditioned" : ""));
    updateSampleSliceButtons();
    updateSampleSliceEditorStatus();
    updateSampleWaveformDisplay();
}

void NateVSTAudioProcessorEditor::cycleSelectedSampleSliceNudge()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, selectedSampleSliceIndex);
    const auto currentNudge = sampleSliceHasCustomSettings(safeIndex)
        ? readPlainParameterValue(Parameters::ID::sampleSliceNudge[safeIndex], 0.0f)
        : 0.0f;
    const auto nextNudge = currentNudge < -0.1f ? 1.5f : currentNudge > 0.1f ? 0.0f : -1.5f;

    captureGlobalEdit("Nudge sample slice");
    setPlainParameterValue(Parameters::ID::sampleSliceNudge[safeIndex], nextNudge);
    captureCurrentSampleSliceSettings(safeIndex, true);

    const auto didAudition = audioProcessor.triggerSampleSliceAudition(static_cast<int>(safeIndex));
    setRandomStatus("Slice " + juce::String(static_cast<int>(safeIndex + 1))
                    + " " + sliceNudgeText(nextNudge)
                    + (didAudition ? " auditioned" : ""));
    updateSampleSliceButtons();
    updateSampleSliceEditorStatus();
    updateSampleWaveformDisplay();
}

void NateVSTAudioProcessorEditor::cycleSelectedSampleSliceFade()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, selectedSampleSliceIndex);
    const auto currentFade = sampleSliceHasCustomSettings(safeIndex)
        ? readPlainParameterValue(Parameters::ID::sampleSliceFade[safeIndex], 0.0f)
        : 0.0f;
    const auto nextFade = currentFade < 0.12f ? 0.35f : currentFade < 0.55f ? 0.75f : 0.0f;

    captureGlobalEdit("Fade sample slice");
    setPlainParameterValue(Parameters::ID::sampleSliceFade[safeIndex], nextFade);
    captureCurrentSampleSliceSettings(safeIndex, true);

    const auto didAudition = audioProcessor.triggerSampleSliceAudition(static_cast<int>(safeIndex));
    setRandomStatus("Slice " + juce::String(static_cast<int>(safeIndex + 1))
                    + " fade " + sliceFadeText(nextFade)
                    + (didAudition ? " auditioned" : ""));
    updateSampleSliceButtons();
    updateSampleSliceEditorStatus();
    updateSampleWaveformDisplay();
}

bool NateVSTAudioProcessorEditor::sampleSliceHasCustomSettings(size_t sliceIndex) const
{
    const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, sliceIndex);
    return readPlainParameterValue(Parameters::ID::sampleSliceCustom[safeIndex], 0.0f) >= 0.5f;
}

void NateVSTAudioProcessorEditor::updateSampleSliceEditorStatus()
{
    const auto safeIndex = juce::jlimit<size_t>(0, sampleChopPanel.getSliceCount() - 1, selectedSampleSliceIndex);
    const auto custom = sampleSliceHasCustomSettings(safeIndex);
    const auto sliceCount = static_cast<float>(sampleChopPanel.getSliceCount());
    const auto defaultStart = static_cast<float>(safeIndex) / sliceCount;
    const auto defaultEnd = static_cast<float>(safeIndex + 1) / sliceCount;
    const auto regionStart = juce::jlimit(0.0f,
                                          1.0f,
                                          custom ? readPlainParameterValue(Parameters::ID::sampleSliceStart[safeIndex], defaultStart)
                                                 : defaultStart);
    const auto regionEnd = juce::jlimit(0.0f,
                                        1.0f,
                                        custom ? readPlainParameterValue(Parameters::ID::sampleSliceEnd[safeIndex], defaultEnd)
                                               : defaultEnd);
    const auto regionLower = juce::jmin(regionStart, regionEnd);
    const auto regionUpper = juce::jmax(regionStart, regionEnd);
    auto preview = defaultSlicePreviewSettings(safeIndex, sampleSliceStyleBox.getSelectedItemIndex());
    if (custom)
    {
        preview.pitch = readPlainParameterValue(Parameters::ID::sampleSliceTranspose[safeIndex], 0.0f);
        preview.gain = readPlainParameterValue(Parameters::ID::sampleSliceGain[safeIndex], -6.0f);
        preview.reverse = readPlainParameterValue(Parameters::ID::sampleSliceReverse[safeIndex], 0.0f) >= 0.5f;
        preview.stutter = readPlainParameterValue(Parameters::ID::sampleSliceStutter[safeIndex], 0.0f) >= 0.5f;
        preview.choke = readPlainParameterValue(Parameters::ID::sampleSliceChoke[safeIndex], 0.0f) >= 0.5f;
        preview.repeats = juce::roundToInt(readPlainParameterValue(Parameters::ID::sampleSliceStutterRepeats[safeIndex], 3.0f));
        preview.pan = readPlainParameterValue(Parameters::ID::sampleSlicePan[safeIndex], 0.0f);
        preview.probability = readPlainParameterValue(Parameters::ID::sampleSliceProbability[safeIndex], 1.0f);
        preview.nudgePercent = readPlainParameterValue(Parameters::ID::sampleSliceNudge[safeIndex], 0.0f);
        preview.fade = readPlainParameterValue(Parameters::ID::sampleSliceFade[safeIndex], 0.0f);
    }
    const auto sliceKeysMode = readPlainParameterValue(Parameters::ID::samplePlaybackMode, 1.0f) >= 1.5f;
    const auto pitchText = (preview.pitch >= 0.0f ? "+" : "") + juce::String(preview.pitch, 1) + "st";
    const auto status = "S" + juce::String(static_cast<int>(safeIndex + 1))
        + (custom ? " custom" : " default")
        + " | " + juce::String(juce::roundToInt(regionLower * 100.0f)) + "-"
        + juce::String(juce::roundToInt(regionUpper * 100.0f)) + "%"
        + " | " + pitchText
        + " | " + juce::String(preview.gain, 1) + "dB"
        + " | " + sliceNudgeText(preview.nudgePercent)
        + " F " + sliceFadeText(preview.fade)
        + " | " + slicePanText(preview.pan)
        + " " + sliceChanceText(preview.probability)
        + (preview.reverse ? " | rev" : " | fwd")
        + (preview.choke ? " | choke" : " | open")
        + (preview.stutter ? " | stut x" + juce::String(preview.repeats) : " | no stut")
        + (sliceKeysMode ? " | C3-G3" : "");

    sampleChopPanel.setStatus(status, "Selected slice memory: " + status);
    sampleChopPanel.setActionState({
        preview.reverse,
        preview.choke,
        preview.probability < 0.995f,
        std::abs(preview.nudgePercent) > 0.1f,
        preview.fade > 0.12f,
        custom
    });
}

void NateVSTAudioProcessorEditor::updateSampleSliceButtons()
{
    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    const auto start = juce::jlimit(0.0f, 1.0f, readParameter(Parameters::ID::sampleStart, 0.0f));
    const auto end = juce::jlimit(0.0f, 1.0f, readParameter(Parameters::ID::sampleEnd, 1.0f));
    const auto orderedStart = juce::jmin(start, end);
    const auto orderedEnd = juce::jmax(start, end);
    const auto sliceCount = static_cast<float>(sampleChopPanel.getSliceCount());

    for (size_t index = 0; index < sampleChopPanel.getSliceCount(); ++index)
    {
        const auto custom = sampleSliceHasCustomSettings(index);
        const auto defaultStart = static_cast<float>(index) / sliceCount;
        const auto defaultEnd = static_cast<float>(index + 1) / sliceCount;
        const auto sliceStart = juce::jlimit(0.0f,
                                             1.0f,
                                             custom ? readPlainParameterValue(Parameters::ID::sampleSliceStart[index], defaultStart)
                                                    : defaultStart);
        const auto sliceEnd = juce::jlimit(0.0f,
                                           1.0f,
                                           custom ? readPlainParameterValue(Parameters::ID::sampleSliceEnd[index], defaultEnd)
                                                  : defaultEnd);
        const auto orderedSliceStart = juce::jmin(sliceStart, sliceEnd);
        const auto orderedSliceEnd = juce::jmax(sliceStart, sliceEnd);
        const auto isSelected = std::abs(orderedStart - orderedSliceStart) < 0.005f
            && std::abs(orderedEnd - orderedSliceEnd) < 0.005f;
        auto preview = defaultSlicePreviewSettings(index, sampleSliceStyleBox.getSelectedItemIndex());
        if (custom)
        {
            preview.pitch = readPlainParameterValue(Parameters::ID::sampleSliceTranspose[index], 0.0f);
            preview.gain = readPlainParameterValue(Parameters::ID::sampleSliceGain[index], -6.0f);
            preview.reverse = readPlainParameterValue(Parameters::ID::sampleSliceReverse[index], 0.0f) >= 0.5f;
            preview.stutter = readPlainParameterValue(Parameters::ID::sampleSliceStutter[index], 0.0f) >= 0.5f;
            preview.choke = readPlainParameterValue(Parameters::ID::sampleSliceChoke[index], 0.0f) >= 0.5f;
            preview.repeats = juce::roundToInt(readPlainParameterValue(Parameters::ID::sampleSliceStutterRepeats[index], 3.0f));
            preview.pan = readPlainParameterValue(Parameters::ID::sampleSlicePan[index], 0.0f);
            preview.probability = readPlainParameterValue(Parameters::ID::sampleSliceProbability[index], 1.0f);
            preview.nudgePercent = readPlainParameterValue(Parameters::ID::sampleSliceNudge[index], 0.0f);
            preview.fade = readPlainParameterValue(Parameters::ID::sampleSliceFade[index], 0.0f);
        }
        sampleChopPanel.setSliceButtonState(
            index,
            juce::String(static_cast<int>(index + 1)) + (custom ? "*" : ""),
            "Slice " + juce::String(static_cast<int>(index + 1))
                + (custom ? " custom" : " default")
                + " | Region " + juce::String(juce::roundToInt(orderedSliceStart * 100.0f))
                + "-" + juce::String(juce::roundToInt(orderedSliceEnd * 100.0f)) + "%"
                + " | Pitch " + (preview.pitch >= 0.0f ? "+" : "") + juce::String(preview.pitch, 1)
                + " st | Gain " + juce::String(preview.gain, 1)
                + " dB | Pan " + slicePanText(preview.pan)
                + " | Chance " + sliceChanceText(preview.probability)
                + " | " + sliceNudgeText(preview.nudgePercent)
                + " | Fade " + sliceFadeText(preview.fade)
                + " | " + (preview.reverse ? "Reverse" : "Forward")
                + " | " + (preview.choke ? "Choke" : "Open")
                + " | " + (preview.stutter ? "Stutter" : "No stutter"),
            isSelected);
    }

    updateSampleSliceEditorStatus();
}

void NateVSTAudioProcessorEditor::updateSampleWaveformDisplay()
{
    auto startModAmount = 0.0f;
    auto mixModAmount = 0.0f;
    auto pitchModAmount = 0.0f;
    auto rampModAmount = 0.0f;
    auto stutterModAmount = 0.0f;
    auto sampleRouteCount = 0;
    juce::StringArray sampleSources;
    const auto sourceChoices = Parameters::modulationSourceChoices();
    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    for (size_t index = 0; index < Parameters::ID::modMatrixDestination.size(); ++index)
    {
        const auto sourceIndex = juce::roundToInt(readParameter(Parameters::ID::modMatrixSource[index], 0.0f));
        const auto destinationIndex = juce::roundToInt(readParameter(Parameters::ID::modMatrixDestination[index], 0.0f));
        const auto amount = readParameter(Parameters::ID::modMatrixAmount[index], 0.0f);
        const auto enabled = readParameter(Parameters::ID::modMatrixEnabled[index], 1.0f) >= 0.5f;

        if (! enabled || sourceIndex <= 0)
            continue;

        if (destinationIndex == 12)
            startModAmount += amount;
        else if (destinationIndex == 13)
            mixModAmount += amount;
        else if (destinationIndex == 14)
            pitchModAmount += amount;
        else if (destinationIndex == 15)
            rampModAmount += amount;
        else if (destinationIndex == 16)
            stutterModAmount += amount;
        else
            continue;

        ++sampleRouteCount;
        if (juce::isPositiveAndBelow(sourceIndex, sourceChoices.size()))
            sampleSources.addIfNotAlreadyThere(sourceChoices[sourceIndex]);
    }

    const auto clippedStartMod = juce::jlimit(-1.0f, 1.0f, startModAmount);
    const auto clippedMixMod = juce::jlimit(-1.0f, 1.0f, mixModAmount);
    const auto clippedPitchMod = juce::jlimit(-1.0f, 1.0f, pitchModAmount);
    const auto clippedRampMod = juce::jlimit(-1.0f, 1.0f, rampModAmount);
    const auto clippedStutterMod = juce::jlimit(-1.0f, 1.0f, stutterModAmount);
    const auto sampleSourceSummary = sampleSources.joinIntoString(", ");
    auto applyModulationState = [&] (UI::SampleWaveformDisplay& display)
    {
        display.setModulationState(clippedStartMod,
                                   clippedMixMod,
                                   clippedPitchMod,
                                   clippedRampMod,
                                   clippedStutterMod,
                                   sampleRouteCount,
                                   sampleSourceSummary);
    };
    applyModulationState(sampleWaveformDisplay);
    applyModulationState(expandedSampleWaveformDisplay);

    std::array<UI::SampleWaveformDisplay::SliceMarker, 8> sliceMarkers;
    const auto currentStart = juce::jlimit(0.0f, 1.0f, readParameter(Parameters::ID::sampleStart, 0.0f));
    const auto currentEnd = juce::jlimit(0.0f, 1.0f, readParameter(Parameters::ID::sampleEnd, 1.0f));
    const auto orderedCurrentStart = juce::jmin(currentStart, currentEnd);
    const auto orderedCurrentEnd = juce::jmax(currentStart, currentEnd);
    const auto sliceCount = static_cast<float>(sliceMarkers.size());
    const auto styleIndex = sampleSliceStyleBox.getSelectedItemIndex();
    for (size_t index = 0; index < sliceMarkers.size(); ++index)
    {
        const auto custom = sampleSliceHasCustomSettings(index);
        const auto defaultStart = static_cast<float>(index) / sliceCount;
        const auto defaultEnd = static_cast<float>(index + 1) / sliceCount;
        const auto markerStart = juce::jlimit(0.0f,
                                              1.0f,
                                              custom ? readPlainParameterValue(Parameters::ID::sampleSliceStart[index], defaultStart)
                                                     : defaultStart);
        const auto markerEnd = juce::jlimit(0.0f,
                                            1.0f,
                                            custom ? readPlainParameterValue(Parameters::ID::sampleSliceEnd[index], defaultEnd)
                                                   : defaultEnd);
        const auto orderedMarkerStart = juce::jmin(markerStart, markerEnd);
        const auto orderedMarkerEnd = juce::jmax(markerStart, markerEnd);
        auto preview = defaultSlicePreviewSettings(index, styleIndex);
        if (custom)
        {
            preview.pitch = readPlainParameterValue(Parameters::ID::sampleSliceTranspose[index], 0.0f);
            preview.gain = readPlainParameterValue(Parameters::ID::sampleSliceGain[index], -6.0f);
            preview.reverse = readPlainParameterValue(Parameters::ID::sampleSliceReverse[index], 0.0f) >= 0.5f;
            preview.stutter = readPlainParameterValue(Parameters::ID::sampleSliceStutter[index], 0.0f) >= 0.5f;
            preview.choke = readPlainParameterValue(Parameters::ID::sampleSliceChoke[index], 0.0f) >= 0.5f;
            preview.repeats = juce::roundToInt(readPlainParameterValue(Parameters::ID::sampleSliceStutterRepeats[index], 3.0f));
            preview.pan = readPlainParameterValue(Parameters::ID::sampleSlicePan[index], 0.0f);
            preview.probability = readPlainParameterValue(Parameters::ID::sampleSliceProbability[index], 1.0f);
            preview.nudgePercent = readPlainParameterValue(Parameters::ID::sampleSliceNudge[index], 0.0f);
            preview.fade = readPlainParameterValue(Parameters::ID::sampleSliceFade[index], 0.0f);
        }

        auto& marker = sliceMarkers[index];
        const auto nudgeAmount = preview.nudgePercent / 100.0f;
        const auto nudgedStart = juce::jlimit(0.0f, 1.0f, orderedMarkerStart + nudgeAmount);
        const auto nudgedEnd = juce::jlimit(0.0f, 1.0f, orderedMarkerEnd + nudgeAmount);
        marker.start = juce::jmin(nudgedStart, nudgedEnd);
        marker.end = juce::jmax(nudgedStart, nudgedEnd);
        marker.pitch = preview.pitch;
        marker.gain = preview.gain;
        marker.pan = preview.pan;
        marker.probability = preview.probability;
        marker.nudgePercent = preview.nudgePercent;
        marker.fade = preview.fade;
        marker.repeats = preview.repeats;
        marker.custom = custom;
        marker.selected = index == selectedSampleSliceIndex
            || (std::abs(orderedCurrentStart - orderedMarkerStart) < 0.005f
                && std::abs(orderedCurrentEnd - orderedMarkerEnd) < 0.005f);
        marker.reverse = preview.reverse;
        marker.stutter = preview.stutter;
        marker.choke = preview.choke;
    }
    sampleWaveformDisplay.setSliceMarkers(sliceMarkers);
    expandedSampleWaveformDisplay.setSliceMarkers(sliceMarkers);

    const auto sampleName = audioProcessor.getLoadedSampleName();
    if (sampleName.isEmpty())
    {
        if (sampleWaveformKey.isNotEmpty())
        {
            sampleWaveformDisplay.setOverview({});
            expandedSampleWaveformDisplay.setOverview({});
            sampleWaveformKey.clear();
        }

        sampleWaveformDisplay.setRange(0.0f, 1.0f);
        expandedSampleWaveformDisplay.setRange(0.0f, 1.0f);
        return;
    }

    if (sampleWaveformKey != sampleName)
    {
        const auto overview = audioProcessor.createSamplePeakOverview(256);
        sampleWaveformDisplay.setOverview(overview);
        expandedSampleWaveformDisplay.setOverview(overview);
        sampleWaveformKey = sampleName;
    }

    sampleWaveformDisplay.setRange(currentStart, currentEnd);
    expandedSampleWaveformDisplay.setRange(currentStart, currentEnd);
}

int NateVSTAudioProcessorEditor::selectedRandomMutationScope() const
{
    return juce::jmax(0, randomScopeBox.getSelectedId() - 1);
}

void NateVSTAudioProcessorEditor::updateRandomMorphPad()
{
    UI::RandomMorphPad::State state;
    state.brightness = readPlainParameterValue(Parameters::ID::randomBrightnessBias, 0.0f);
    state.motion = readPlainParameterValue(Parameters::ID::randomMotionBias, 0.0f);
    state.drive = readPlainParameterValue(Parameters::ID::randomDriveBias, 0.0f);
    state.amount = readPlainParameterValue(Parameters::ID::randomAmount, 0.45f);
    state.chaos = readPlainParameterValue(Parameters::ID::randomChaos, 0.25f);
    state.x = (state.brightness + 1.0f) * 0.5f;
    state.y = (state.motion + 1.0f) * 0.5f;
    state.recipe = recipeBox.getText().trim().isNotEmpty() ? recipeBox.getText().trim() : juce::String("House");
    state.scope = randomScopeBox.getText().trim().isNotEmpty() ? randomScopeBox.getText().trim() : juce::String("All");

    const std::array<const char*, 7> sectionIDs {
        Parameters::ID::randomSourceIntensity,
        Parameters::ID::randomEnvelopeIntensity,
        Parameters::ID::randomFilterIntensity,
        Parameters::ID::randomSampleIntensity,
        Parameters::ID::randomFxIntensity,
        Parameters::ID::randomSequencerIntensity,
        Parameters::ID::randomMacroIntensity
    };

    for (size_t index = 0; index < sectionIDs.size(); ++index)
        state.sectionIntensities[index] = readPlainParameterValue(sectionIDs[index], 1.0f);

    randomMorphPad.setState(state);
}

void NateVSTAudioProcessorEditor::applyRandomMorphPad(float x, float y, bool createVariation)
{
    x = juce::jlimit(0.0f, 1.0f, x);
    y = juce::jlimit(0.0f, 1.0f, y);
    const auto dx = x - 0.5f;
    const auto dy = y - 0.5f;
    const auto distance = juce::jlimit(0.0f, 1.0f, std::sqrt((dx * dx) + (dy * dy)) * 1.42f);

    const auto amount = juce::jlimit(0.08f, 1.0f, 0.24f + (distance * 0.68f) + (y * 0.08f));
    const auto chaos = juce::jlimit(0.0f, 1.0f, (distance * 0.72f) + (y * 0.20f));
    const auto brightness = juce::jlimit(-1.0f, 1.0f, (x * 2.0f) - 1.0f);
    const auto drive = juce::jlimit(-1.0f, 1.0f, ((1.0f - x) * 0.82f) + (distance * 0.74f) - 0.58f);
    const auto motion = juce::jlimit(-1.0f, 1.0f, (y * 2.0f) - 1.0f);

    setPlainParameterValue(Parameters::ID::randomAmount, amount);
    setPlainParameterValue(Parameters::ID::randomChaos, chaos);
    setPlainParameterValue(Parameters::ID::randomBrightnessBias, brightness);
    setPlainParameterValue(Parameters::ID::randomDriveBias, drive);
    setPlainParameterValue(Parameters::ID::randomMotionBias, motion);
    setPlainParameterValue(Parameters::ID::macroTone, juce::jlimit(0.0f, 1.0f, x));
    setPlainParameterValue(Parameters::ID::macroDirt, juce::jlimit(0.0f, 1.0f, 0.18f + (std::abs(drive) * 0.74f)));
    setPlainParameterValue(Parameters::ID::macroMotion, juce::jlimit(0.0f, 1.0f, y));
    setPlainParameterValue(Parameters::ID::macroSpace, juce::jlimit(0.0f, 1.0f, 0.18f + (y * 0.72f)));
    setPlainParameterValue(Parameters::ID::macroWarp, juce::jlimit(0.0f, 1.0f, 0.20f + (distance * 0.70f)));
    setPlainParameterValue(Parameters::ID::macroBounce, juce::jlimit(0.0f, 1.0f, chaos));

    const std::array<std::pair<const char*, float>, 7> sectionValues {
        std::pair<const char*, float> { Parameters::ID::randomSourceIntensity, juce::jlimit(0.25f, 1.0f, 0.46f + (distance * 0.42f) + ((1.0f - std::abs(dx * 2.0f)) * 0.12f)) },
        std::pair<const char*, float> { Parameters::ID::randomEnvelopeIntensity, juce::jlimit(0.20f, 1.0f, 0.42f + ((1.0f - y) * 0.38f) + (distance * 0.12f)) },
        std::pair<const char*, float> { Parameters::ID::randomFilterIntensity, juce::jlimit(0.25f, 1.0f, 0.44f + (x * 0.38f) + (distance * 0.16f)) },
        std::pair<const char*, float> { Parameters::ID::randomSampleIntensity, juce::jlimit(0.18f, 1.0f, 0.34f + ((1.0f - x) * y * 0.44f) + (distance * 0.16f)) },
        std::pair<const char*, float> { Parameters::ID::randomFxIntensity, juce::jlimit(0.22f, 1.0f, 0.38f + (y * 0.42f) + (distance * 0.16f)) },
        std::pair<const char*, float> { Parameters::ID::randomSequencerIntensity, juce::jlimit(0.20f, 1.0f, 0.32f + (y * 0.36f) + ((1.0f - x) * 0.18f) + (distance * 0.10f)) },
        std::pair<const char*, float> { Parameters::ID::randomMacroIntensity, juce::jlimit(0.25f, 1.0f, 0.38f + (distance * 0.52f) + (y * 0.10f)) }
    };

    for (const auto& section : sectionValues)
        setPlainParameterValue(section.first, section.second);

    updateRandomMorphPad();
    const auto status = "Map "
        + juce::String(juce::roundToInt(x * 100.0f)) + "/"
        + juce::String(juce::roundToInt(y * 100.0f))
        + " | Amt " + juce::String(juce::roundToInt(amount * 100.0f))
        + " | Chaos " + juce::String(juce::roundToInt(chaos * 100.0f));
    setRandomStatus(status);

    if (createVariation)
    {
        setRandomStatus(status + " | Live map");
    }
}

void NateVSTAudioProcessorEditor::triggerRandomGenerate()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    audioProcessor.generateRandomPatch(selectedRandomMutationScope());
    prepareRandomPresetDraft("Generate");
    updateSegmentedSelectors();
    updateSampleNameLabel();
    updateSampleWaveformDisplay();
    updateWavetableDisplay();
    repaintSequencerGrids();
    updateRandomCandidateButtons();
    setRandomStatus("Generated");
}

void NateVSTAudioProcessorEditor::triggerRandomMutate()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    audioProcessor.mutateRandomPatch(selectedRandomMutationScope());
    prepareRandomPresetDraft("Mutate");
    updateSegmentedSelectors();
    updateSampleNameLabel();
    updateSampleWaveformDisplay();
    updateWavetableDisplay();
    repaintSequencerGrids();
    updateRandomCandidateButtons();
    setRandomStatus("Mutated");
}

void NateVSTAudioProcessorEditor::triggerRandomVariation()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    audioProcessor.createRandomVariation(selectedRandomMutationScope());
    prepareRandomPresetDraft("Vary");
    updateSegmentedSelectors();
    updateSampleNameLabel();
    updateSampleWaveformDisplay();
    updateWavetableDisplay();
    repaintSequencerGrids();
    updateRandomCandidateButtons();
    setRandomStatus("Variation");
}

void NateVSTAudioProcessorEditor::triggerRandomWild()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    audioProcessor.wildMutateRandomPatch(selectedRandomMutationScope());
    prepareRandomPresetDraft("Wild");
    updateSegmentedSelectors();
    updateSampleNameLabel();
    updateSampleWaveformDisplay();
    updateWavetableDisplay();
    repaintSequencerGrids();
    updateRandomCandidateButtons();
    setRandomStatus("Wild");
}

void NateVSTAudioProcessorEditor::triggerRandomSectionRoll(size_t sectionIndex)
{
    if (sectionIndex >= randomSectionRollButtons.size())
        return;

    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto scopeIndex = static_cast<int>(sectionIndex + 1);
    randomScopeBox.setSelectedId(scopeIndex + 1, juce::dontSendNotification);

    const auto sectionName = randomSectionRollButtons[sectionIndex].getButtonText();
    audioProcessor.mutateRandomPatch(scopeIndex);
    prepareRandomPresetDraft(sectionName + " Roll");
    updateSegmentedSelectors();
    updateSampleNameLabel();
    updateSampleWaveformDisplay();
    updateWavetableDisplay();
    repaintSequencerGrids();
    updateRandomCandidateButtons();
    setRandomStatus("Rolled " + sectionName);
}

void NateVSTAudioProcessorEditor::applySelectedRandomSectionAction()
{
    const auto selectedId = randomSectionActionBox.getSelectedId();
    if (selectedId <= 1)
        return;

    if (selectedId >= 2 && selectedId <= 8)
    {
        triggerRandomSectionRoll(static_cast<size_t>(selectedId - 2));
    }
    else
    {
        switch (selectedId)
        {
            case 9:
                triggerRandomGenerate();
                break;

            case 10:
                triggerRandomVariation();
                break;

            case 11:
                triggerRandomWild();
                break;

            case 12:
                releaseRandomCandidateAudition(false);
                releasePresetAuditionNote();
                captureGlobalEdit("Dice sequence");
                if (audioProcessor.randomizeSequencerPattern())
                {
                    repaintSequencerGrids();
                    updateSequencerSceneButtons();
                    setRandomStatus("Sequence diced");
                }
                else
                {
                    setRandomStatus("Sequence dice skipped");
                }
                break;

            case 13:
                triggerRandomSectionRoll(6);
                setRandomStatus("Macro motion rolled");
                break;

            case 14:
                triggerRandomSectionRoll(4);
                setRandomStatus("FX space rolled");
                break;

            case 15:
                triggerRandomGenerate();
                triggerRandomVariation();
                triggerRandomWild();
                updateRandomCandidateButtons();
                setRandomStatus("Explored 3 candidates");
                break;

            default:
                break;
        }
    }

    randomSectionActionBox.setSelectedId(1, juce::dontSendNotification);
    returnKeyboardFocusToPiano();
}

void NateVSTAudioProcessorEditor::applySelectedRandomLockAction()
{
    const auto selectedId = randomLockActionBox.getSelectedId();
    if (selectedId <= 1)
        return;

    struct LockTarget
    {
        const char* parameterID = nullptr;
        juce::ToggleButton* button = nullptr;
        const char* label = nullptr;
    };

    const std::array<LockTarget, 8> targets {
        LockTarget { Parameters::ID::randomLockPitch, &randomLockPitchButton, "Pitch" },
        LockTarget { Parameters::ID::randomLockEnvelope, &randomLockEnvelopeButton, "Envelope" },
        LockTarget { Parameters::ID::randomLockFilter, &randomLockFilterButton, "Filter" },
        LockTarget { Parameters::ID::randomLockSource, &randomLockSourceButton, "Source" },
        LockTarget { Parameters::ID::randomLockSample, &randomLockSampleButton, "Sample" },
        LockTarget { Parameters::ID::randomLockFx, &randomLockFxButton, "FX" },
        LockTarget { Parameters::ID::randomLockOutput, &randomLockOutputButton, "Output" },
        LockTarget { Parameters::ID::randomLockSequencer, &randomLockSequencerButton, "Sequencer" }
    };

    const auto targetIndex = selectedId - 2;
    if (! juce::isPositiveAndBelow(targetIndex, static_cast<int>(targets.size())))
        return;

    const auto& target = targets[static_cast<size_t>(targetIndex)];
    const auto nextValue = readPlainParameterValue(target.parameterID, 0.0f) >= 0.5f ? 0.0f : 1.0f;
    setPlainParameterValue(target.parameterID, nextValue);
    target.button->setToggleState(nextValue >= 0.5f, juce::dontSendNotification);
    randomLockActionBox.setSelectedId(1, juce::dontSendNotification);
    setRandomStatus(juce::String(target.label) + (nextValue >= 0.5f ? " locked" : " unlocked"));
    returnKeyboardFocusToPiano();
}

void NateVSTAudioProcessorEditor::recallRandomCandidate(size_t slotIndex)
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    if (! audioProcessor.recallRandomCandidate(static_cast<int>(slotIndex)))
    {
        setRandomStatus("Candidate empty");
        updateRandomCandidateButtons();
        return;
    }

    updateSegmentedSelectors();
    updateSampleNameLabel();
    updateSampleWaveformDisplay();
    updateWavetableDisplay();
    repaintSequencerGrids();
    updateRandomCandidateButtons();
    prepareRandomPresetDraft("Candidate " + juce::String(static_cast<int>(slotIndex + 1)));
    setRandomStatus("Recalled candidate " + juce::String(static_cast<int>(slotIndex + 1)));
}

void NateVSTAudioProcessorEditor::auditionRandomCandidate(size_t slotIndex)
{
    releasePresetAuditionNote();
    releaseRandomCandidateAudition(false);

    if (! audioProcessor.beginRandomCandidateAudition(static_cast<int>(slotIndex)))
    {
        setRandomStatus("Candidate empty");
        updateRandomCandidateButtons();
        return;
    }

    updateSegmentedSelectors();
    updateSampleNameLabel();
    updateSampleWaveformDisplay();
    updateWavetableDisplay();
    repaintSequencerGrids();

    auto note = juce::roundToInt(readPlainParameterValue(Parameters::ID::sequencerRoot, 60.0f)
                                 + (readPlainParameterValue(Parameters::ID::sequencerOctave, 0.0f) * 12.0f));
    while (note < 48)
        note += 12;
    while (note > 72)
        note -= 12;

    activeRandomCandidateAuditionSlot = static_cast<int>(slotIndex);
    activeRandomCandidateAuditionNote = juce::jlimit(0, 127, note);
    randomCandidateAuditionNoteOffMs = juce::Time::getMillisecondCounterHiRes() + candidateAuditionDurationMs;
    audioProcessor.getMidiKeyboardState().noteOn(1, activeRandomCandidateAuditionNote, candidateAuditionVelocity);

    updateRandomCandidateButtons();
    setRandomStatus("Cue candidate " + juce::String(static_cast<int>(slotIndex + 1))
                    + " | " + juce::MidiMessage::getMidiNoteName(activeRandomCandidateAuditionNote, true, true, 3));
}

void NateVSTAudioProcessorEditor::releaseRandomCandidateAudition(bool updateStatus)
{
    const auto hadAudition = activeRandomCandidateAuditionSlot >= 0 || activeRandomCandidateAuditionNote >= 0;

    if (activeRandomCandidateAuditionNote >= 0)
        audioProcessor.getMidiKeyboardState().noteOff(1, activeRandomCandidateAuditionNote, 0.0f);

    const auto restored = audioProcessor.endRandomCandidateAudition();
    activeRandomCandidateAuditionNote = -1;
    activeRandomCandidateAuditionSlot = -1;
    randomCandidateAuditionNoteOffMs = 0.0;

    if (! restored)
    {
        if (hadAudition)
            updateRandomCandidateButtons();
        return;
    }

    updateSegmentedSelectors();
    updateSampleNameLabel();
    updateSampleWaveformDisplay();
    updateWavetableDisplay();
    repaintSequencerGrids();
    updateRandomCandidateButtons();

    if (updateStatus)
        setRandomStatus("Cue restored");
}

void NateVSTAudioProcessorEditor::promoteActiveRandomCandidate(int snapshotSlotIndex)
{
    releaseRandomCandidateAudition(false);

    const auto activeSlot = audioProcessor.getActiveRandomCandidateIndex();
    if (! audioProcessor.promoteRandomCandidateToPerformanceSnapshot(activeSlot, snapshotSlotIndex))
    {
        setRandomStatus("No active candidate");
        updateRandomCandidateButtons();
        updatePerformanceSnapshotButtons();
        return;
    }

    updateRandomCandidateButtons();
    updatePerformanceSnapshotButtons();
    setRandomStatus("Candidate -> " + juce::String(snapshotSlotIndex == 0 ? "A" : "B"));
}

void NateVSTAudioProcessorEditor::updateRandomCandidateButtons()
{
    const auto activeSlot = audioProcessor.getActiveRandomCandidateIndex();
    const auto activeCandidateReady = audioProcessor.hasRandomCandidate(activeSlot);

    for (size_t index = 0; index < randomCandidateButtons.size(); ++index)
    {
        const auto slotIndex = static_cast<int>(index);
        const auto hasCandidate = audioProcessor.hasRandomCandidate(slotIndex);
        auto& button = randomCandidateButtons[index];
        const auto prefix = juce::String(static_cast<int>(index + 1)) + ". ";
        const auto summary = hasCandidate ? audioProcessor.getRandomCandidateSummary(slotIndex) : juce::String("Empty");
        const auto compare = hasCandidate ? audioProcessor.getRandomCandidateCompareSummary(slotIndex) : juce::String();
        const auto sectionCount = hasCandidate ? audioProcessor.getRandomCandidateChangedSectionCount(slotIndex) : 0;
        const auto sectionSummary = hasCandidate ? audioProcessor.getRandomCandidateChangedSectionsSummary(slotIndex) : juce::String();
        const auto diffSummary = hasCandidate ? audioProcessor.getRandomCandidateDiffSummary(slotIndex) : juce::String();
        const auto sectionBadge = hasCandidate && sectionCount > 0
            ? " [" + juce::String(sectionCount) + "]"
            : juce::String();
        auto& auditionButton = randomCandidateAuditionButtons[index];

        button.setEnabled(hasCandidate);
        button.setToggleState(hasCandidate && activeSlot == slotIndex, juce::dontSendNotification);
        button.setButtonText(prefix + summary + sectionBadge);
        button.setTooltip(hasCandidate
                              ? "Recall " + summary
                                    + " | Compared with current: " + compare
                                    + " | Changed sections: " + sectionSummary
                                    + " | Diff: " + diffSummary
                              : "Generate or mutate a patch to fill this candidate slot");

        auditionButton.setEnabled(hasCandidate);
        auditionButton.setToggleState(hasCandidate && activeRandomCandidateAuditionSlot == slotIndex,
                                      juce::dontSendNotification);
        auditionButton.setTooltip(hasCandidate
                                      ? "Cue " + summary
                                            + " without committing the current patch | Sections: " + sectionSummary
                                            + " | Diff: " + diffSummary
                                      : "Generate or mutate a patch before cueing this slot");
    }

    promoteCandidateAButton.setEnabled(activeCandidateReady);
    promoteCandidateBButton.setEnabled(activeCandidateReady);
    candidateRatingBox.setEnabled(activeCandidateReady);
    candidateFavoriteButton.setEnabled(activeCandidateReady);
    saveCandidateButton.setEnabled(activeCandidateReady);
    promoteCandidateAButton.setTooltip(activeCandidateReady ? "Promote active random candidate to performance snapshot A"
                                                            : "Recall or generate a candidate before promoting");
    promoteCandidateBButton.setTooltip(activeCandidateReady ? "Promote active random candidate to performance snapshot B"
                                                            : "Recall or generate a candidate before promoting");
    candidateRatingBox.setTooltip(activeCandidateReady ? "Rating to apply when Save Slot writes this candidate into the Library"
                                                       : "Generate, cue, or recall a candidate before choosing save rating");
    candidateFavoriteButton.setTooltip(activeCandidateReady ? "Favorite the preset created by Save Slot"
                                                            : "Generate, cue, or recall a candidate before starring a save");
    saveCandidateButton.setTooltip(activeCandidateReady ? "Save the active random candidate with the current category, pack, BPM, and generated-source metadata"
                                                        : "Generate, cue, or recall a random candidate before saving a slot");
    updateRandomCandidateDetail();
    updateHomeSessionDisplay();
}

void NateVSTAudioProcessorEditor::updateRandomCandidateDetail()
{
    auto detailSlot = audioProcessor.getActiveRandomCandidateIndex();
    if (! audioProcessor.hasRandomCandidate(detailSlot))
    {
        detailSlot = -1;
        for (size_t index = 0; index < randomCandidateButtons.size(); ++index)
        {
            const auto slotIndex = static_cast<int>(index);
            if (audioProcessor.hasRandomCandidate(slotIndex))
            {
                detailSlot = slotIndex;
                break;
            }
        }
    }

    if (! audioProcessor.hasRandomCandidate(detailSlot))
    {
        const auto emptyText = juce::String("No active candidate\nSections: -\nTraits: -\nDiffs: -");
        randomCandidateDetailEditor.setText(emptyText, juce::dontSendNotification);
        randomCandidateDetailEditor.setTooltip(emptyText);
        return;
    }

    const auto slotLabel = "Slot " + juce::String(detailSlot + 1);
    const auto summary = audioProcessor.getRandomCandidateSummary(detailSlot);
    const auto sections = audioProcessor.getRandomCandidateChangedSectionsSummary(detailSlot);
    const auto compare = audioProcessor.getRandomCandidateCompareSummary(detailSlot);
    const auto diffs = audioProcessor.getRandomCandidateDiffSummary(detailSlot);
    const auto validation = audioProcessor.getRandomCandidateValidationSummary(detailSlot);
    const auto cueText = activeRandomCandidateAuditionSlot == detailSlot ? juce::String("\nCue: auditioning") : juce::String();
    const auto detailText = slotLabel + " | " + summary
        + "\nSections: " + sections
        + "\nTraits: " + compare
        + "\nDiffs: " + diffs
        + (validation.isNotEmpty() ? "\nValidation: " + validation : juce::String())
        + cueText;

    randomCandidateDetailEditor.setText(detailText, juce::dontSendNotification);
    randomCandidateDetailEditor.setTooltip(detailText);
}

void NateVSTAudioProcessorEditor::prepareRandomPresetDraft(const juce::String& actionLabel)
{
    const auto category = suggestedPresetCategoryForRecipe();
    const auto pack = suggestedPresetPackForCategory(category);
    const auto bpm = suggestedPresetBpmForCategory(category);
    const auto previousDraftWasGenerated = currentPresetDraftIsGenerated;
    currentPresetDraftIsGenerated = true;
    currentGeneratedPresetRecipe = recipeBox.getText().trim().isNotEmpty() ? recipeBox.getText().trim() : juce::String("Random Lab");

    const auto currentName = presetNameEditor.getText().trim();
    const auto selectedPresetName = presetBox.getText().trim();
    const auto shouldWriteDraftName = currentName.isEmpty()
        || presetNameIsRandomDraft
        || (selectedPresetName.isNotEmpty() && currentName == selectedPresetName);

    if (shouldWriteDraftName)
    {
        const auto recipeName = recipeBox.getText().trim().isNotEmpty() ? recipeBox.getText().trim() : juce::String("Random");
        const auto scopeName = randomScopeBox.getText().trim();
        juce::StringArray nameParts;
        nameParts.add(recipeName);
        if (scopeName.isNotEmpty() && scopeName != "All")
            nameParts.add(scopeName);
        nameParts.add(actionLabel);
        nameParts.add(juce::String(static_cast<int>(juce::Time::getMillisecondCounter() % 1000000)));
        presetNameEditor.setText(nameParts.joinIntoString(" "), juce::dontSendNotification);
        presetNameIsRandomDraft = true;
    }

    const auto currentCategory = presetCategoryBox.getText().trim();
    if (currentCategory.isEmpty() || currentCategory == "User" || presetNameIsRandomDraft)
        presetCategoryBox.setText(category, juce::dontSendNotification);

    const auto currentPack = presetPackBox.getText().trim();
    if (currentPack.isEmpty() || currentPack == "User Pack" || presetNameIsRandomDraft)
        presetPackBox.setText(pack, juce::dontSendNotification);

    const auto currentBpm = presetBpmBox.getText().trim();
    if (currentBpm.isEmpty() || currentBpm == "Any Tempo" || presetNameIsRandomDraft)
        presetBpmBox.setText(formatPresetBpm(bpm), juce::dontSendNotification);

    const auto activeSlot = audioProcessor.getActiveRandomCandidateIndex();
    const auto shouldWriteDraftNotes = presetNotesEditor.getText().trim().isEmpty()
        || presetNotesIsRandomDraft
        || ! previousDraftWasGenerated;
    if (shouldWriteDraftNotes)
    {
        presetNotesEditor.setText(generatedPresetNotes(category,
                                                       currentGeneratedPresetRecipe,
                                                       activeSlot),
                                  juce::dontSendNotification);
        presetNotesIsRandomDraft = true;
    }

    presetStatusLabel.setText("Draft -> " + category + " / " + pack + " / " + formatPresetBpm(bpm),
                              juce::dontSendNotification);
    updatePresetSaveSummary();
}

juce::String NateVSTAudioProcessorEditor::generatedPresetNotes(const juce::String& category,
                                                                const juce::String& recipe,
                                                                int candidateSlotIndex)
{
    juce::StringArray lines;
    const auto recipeName = recipe.trim().isNotEmpty() ? recipe.trim() : juce::String("Random Lab");
    const auto categoryName = category.trim().isNotEmpty() ? category.trim() : suggestedPresetCategoryForRecipe();
    const auto scopeName = randomScopeBox.getText().trim().isNotEmpty() ? randomScopeBox.getText().trim() : juce::String("All");
    const auto locks = audioProcessor.getActiveRandomizationLockSummary();

    lines.add("Recipe: " + recipeName);
    lines.add("Scope: " + scopeName);
    lines.add("Category: " + categoryName);
    if (locks.isNotEmpty() && ! locks.equalsIgnoreCase("No locks"))
        lines.add("Locks: " + locks);
    lines.add("Use: " + suggestedPresetUseForCategory(categoryName));

    if (candidateSlotIndex >= 0 && audioProcessor.hasRandomCandidate(candidateSlotIndex))
    {
        lines.add("Candidate: Slot " + juce::String(candidateSlotIndex + 1));
        lines.add("Sections: " + audioProcessor.getRandomCandidateChangedSectionsSummary(candidateSlotIndex));
        lines.add("Traits: " + audioProcessor.getRandomCandidateCompareSummary(candidateSlotIndex));
        lines.add("Diffs: " + audioProcessor.getRandomCandidateDiffSummary(candidateSlotIndex));
        const auto validation = audioProcessor.getRandomCandidateValidationSummary(candidateSlotIndex);
        if (validation.isNotEmpty())
            lines.add("Validation: " + validation);
    }

    lines.add("Saved: " + juce::Time::getCurrentTime().formatted("%Y-%m-%d %H:%M"));
    return lines.joinIntoString("\n");
}

juce::String NateVSTAudioProcessorEditor::presetNoteTemplateForId(int templateId) const
{
    const auto category = presetCategoryBox.getText().trim().isNotEmpty()
        ? presetCategoryBox.getText().trim()
        : suggestedPresetCategoryForRecipe();
    const auto recipe = currentGeneratedPresetRecipe.trim().isNotEmpty()
        ? currentGeneratedPresetRecipe.trim()
        : (recipeBox.getText().trim().isNotEmpty() ? recipeBox.getText().trim() : juce::String("Random Lab"));
    const auto bpm = presetBpmBox.getText().trim().isNotEmpty()
        ? presetBpmBox.getText().trim()
        : formatPresetBpm(suggestedPresetBpmForCategory(category));

    switch (templateId)
    {
        case 2:
            return "Macro Intent:\n"
                "Tone: brighten/darken core harmonics\n"
                "Dirt: drive edge without clipping the sub\n"
                "Motion: filter/warp movement for groove\n"
                "Space: delay/reverb throw amount\n"
                "Weight: mono low-end focus\n"
                "Bounce: pump or swing feel\n"
                "Warp: source movement amount\n"
                "Throw: momentary FX send";

        case 3:
            return "Ableton Use:\n"
                "Role: " + suggestedPresetUseForCategory(category) + "\n"
                "Tempo: " + bpm + "\n"
                "Clip idea: 4-bar loop, automate Motion and Throw\n"
                "Arrangement: duplicate for breakdown and mute low end before drop";

        case 4:
            return "UKG Variation:\n"
                "Groove: 2-step push with swung 16ths\n"
                "Bass: mono sub, moving upper layer, short glide\n"
                "Chop: pitched vocal or organ response on offbeats\n"
                "FX: short delay throw, guarded reverb tail\n"
                "Recipe source: " + recipe;

        case 5:
            return "Mix Safety:\n"
                "Sub: mono below 120 Hz\n"
                "Peak: leave headroom before Ableton group processing\n"
                "Drive: check harshness after Dirt/Warp automation\n"
                "Space: keep reverb low for bass presets\n"
                "Compare: A/B against saved candidate before overwriting";

        case 6:
            return "Pack Notes:\n"
                "Category: " + category + "\n"
                "Recipe: " + recipe + "\n"
                "Pack role: starter, variation, or performance macro patch\n"
                "Tags to add: Generated, Random Lab, house, UKG, tech-house, minimal, techno";

        default:
            break;
    }

    return {};
}

juce::String NateVSTAudioProcessorEditor::suggestedPresetCategoryForRecipe() const
{
    const auto recipe = recipeBox.getText().trim();

    if (recipe.containsIgnoreCase("Afro House"))
        return "Afro House/Plucks";
    if (recipe.containsIgnoreCase("Amapiano"))
        return "Amapiano/Bass";
    if (recipe.containsIgnoreCase("Bass House"))
        return "Bass House/Bass";
    if (recipe.containsIgnoreCase("Dub Techno"))
        return "Dub Techno/Chords";
    if (recipe.containsIgnoreCase("Detroit"))
        return "Techno/Stabs";
    if (recipe.containsIgnoreCase("Hardgroove"))
        return "Techno/Rumble";
    if (recipe.containsIgnoreCase("Microhouse"))
        return "Minimal/Perc";
    if (recipe.containsIgnoreCase("Garage Vocal"))
        return "UKG/Chops";
    if (recipe.containsIgnoreCase("Future Garage"))
        return "UKG/Pads";
    if (recipe.containsIgnoreCase("Peak Techno"))
        return "Techno/Acid";
    if (recipe.containsIgnoreCase("Lo-Fi House"))
        return "House/Keys";
    if (recipe.containsIgnoreCase("Tribal Tech"))
        return "Tech House/Perc";
    if (recipe.containsIgnoreCase("Tech House Chord"))
        return "Tech House/Stabs";
    if (recipe.containsIgnoreCase("Dred") || recipe.containsIgnoreCase("2-Step Bass"))
        return "UKG/Bass";
    if (recipe.containsIgnoreCase("Organ"))
        return "UKG/Organ";
    if (recipe.containsIgnoreCase("Chord"))
        return "UKG/Stabs";
    if (recipe.containsIgnoreCase("Bell"))
        return "UKG/Bells";
    if (recipe.containsIgnoreCase("Deep House"))
        return "House/Bass";
    if (recipe.containsIgnoreCase("Rolling Tech"))
        return "Tech House/Bass";
    if (recipe.containsIgnoreCase("Minimal"))
        return "Minimal/Plucks";
    if (recipe.containsIgnoreCase("Dark Stab"))
        return "Techno/Stabs";
    if (recipe.containsIgnoreCase("Acid"))
        return "Techno";
    if (recipe.containsIgnoreCase("Noise"))
        return "FX";

    return "User";
}

juce::String NateVSTAudioProcessorEditor::suggestedPresetUseForCategory(const juce::String& category) const
{
    if (category.startsWithIgnoreCase("UKG/Bass"))
        return "UKG bassline or Dred/Reese layer around 130-136 BPM";
    if (category.startsWithIgnoreCase("UKG/Chops"))
        return "garage chop or vocal-like call-response hook";
    if (category.startsWithIgnoreCase("UKG/Pads"))
        return "future-garage pad, washed chord, or atmospheric bed";
    if (category.startsWithIgnoreCase("UKG/Organ"))
        return "short organ stab for 2-step or speed-garage hooks";
    if (category.startsWithIgnoreCase("UKG/Stabs"))
        return "minor chord stab or shuffled garage response";
    if (category.startsWithIgnoreCase("UKG/Bells"))
        return "bright garage pluck, fill, or call-response hook";
    if (category.startsWithIgnoreCase("House"))
        return "warm house bass, key layer, or chord around 118-126 BPM";
    if (category.startsWithIgnoreCase("Afro House"))
        return "syncopated Afro house pluck or hook";
    if (category.startsWithIgnoreCase("Amapiano"))
        return "log bass or sparse amapiano motif";
    if (category.startsWithIgnoreCase("Bass House"))
        return "driven bass-house Reese or hook layer";
    if (category.startsWithIgnoreCase("Dub Techno"))
        return "dub chord stab or smoky delayed chord";
    if (category.startsWithIgnoreCase("Tech House"))
        return "rolling bass, chord tool, or percussive tech-house hook";
    if (category.startsWithIgnoreCase("Minimal"))
        return "short pluck or sparse rhythmic top line";
    if (category.startsWithIgnoreCase("Techno"))
        return "warehouse stab, acid line, or darker riff";
    if (category == "FX")
        return "transition, riser, fill, or noisy impact";

    return "generated patch starting point";
}

juce::String NateVSTAudioProcessorEditor::suggestedPresetPackForCategory(const juce::String& category) const
{
    const auto recipe = recipeBox.getText().trim();

    if (recipe.containsIgnoreCase("Dred") || recipe.containsIgnoreCase("2-Step Bass"))
        return "UKG Basslines";
    if (recipe.containsIgnoreCase("Afro House"))
        return "Afro House Rituals";
    if (recipe.containsIgnoreCase("Amapiano"))
        return "Amapiano Log Lab";
    if (recipe.containsIgnoreCase("Bass House"))
        return "Bass House Weapons";
    if (recipe.containsIgnoreCase("Dub Techno"))
        return "Dub Techno Chords";
    if (recipe.containsIgnoreCase("Hardgroove") || recipe.containsIgnoreCase("Peak Techno") || recipe.containsIgnoreCase("Detroit"))
        return "Techno Tools";
    if (recipe.containsIgnoreCase("Lo-Fi House"))
        return "House Tools";
    if (category.startsWithIgnoreCase("UKG"))
        return "UKG Essentials";
    if (category.startsWithIgnoreCase("House"))
        return "House Tools";
    if (category.startsWithIgnoreCase("Tech House"))
        return "Tech House Tools";
    if (category.startsWithIgnoreCase("Minimal"))
        return "Minimal Tools";
    if (category.startsWithIgnoreCase("Techno"))
        return "Techno Tools";
    if (category == "FX")
        return "Project Pack";

    return "User Pack";
}

int NateVSTAudioProcessorEditor::suggestedPresetBpmForCategory(const juce::String& category) const
{
    const auto recipe = recipeBox.getText().trim();

    if (recipe.containsIgnoreCase("Amapiano"))
        return 113;
    if (recipe.containsIgnoreCase("Afro House"))
        return 122;
    if (recipe.containsIgnoreCase("Bass House") || recipe.containsIgnoreCase("Tech House Chord") || recipe.containsIgnoreCase("Tribal Tech"))
        return 126;
    if (recipe.containsIgnoreCase("Dub Techno") || recipe.containsIgnoreCase("Lo-Fi House"))
        return 124;
    if (recipe.containsIgnoreCase("Hardgroove") || recipe.containsIgnoreCase("Peak Techno"))
        return 136;
    if (recipe.containsIgnoreCase("Detroit"))
        return 130;
    if (recipe.containsIgnoreCase("Future Garage") || recipe.containsIgnoreCase("Garage Vocal"))
        return 132;
    if (category.startsWithIgnoreCase("UKG"))
        return recipe.containsIgnoreCase("Dred") ? 134 : 132;
    if (category.startsWithIgnoreCase("Tech House"))
        return 126;
    if (category.startsWithIgnoreCase("House"))
        return 124;
    if (category.startsWithIgnoreCase("Techno"))
        return recipe.containsIgnoreCase("Acid") ? 130 : 132;
    if (category.startsWithIgnoreCase("Minimal"))
        return 125;
    if (category == "FX")
        return 128;

    return 0;
}

void NateVSTAudioProcessorEditor::setRandomStatus(const juce::String& action)
{
    const auto locks = audioProcessor.getActiveRandomizationLockSummary();
    const auto history = audioProcessor.getRandomHistorySummary();
    juce::StringArray details;
    if (history.isNotEmpty())
        details.add(history);
    if (randomScopeBox.getSelectedId() > 1)
        details.add("Scope: " + randomScopeBox.getText());
    details.add(locks.isNotEmpty() ? "Locked: " + locks : "No locks");
    const auto validation = audioProcessor.getLastRandomValidationSummary();
    if (validation.isNotEmpty())
        details.add("Validation: " + validation);
    randomStatusLabel.setText(action + " | " + details.joinIntoString(" | "), juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::shiftKeyboardOctave(int semitones)
{
    const auto currentBaseNote = computerKeyboardBaseNote();
    const auto nextBaseNote = clampedKeyboardLowestVisibleNote(currentBaseNote + semitones);

    releaseComputerKeyboardNotes();
    audioProcessor.getMidiKeyboardState().allNotesOff(1);
    keyboardTypingBaseNote = nextBaseNote;
    syncPianoKeyboardComputerMapping();
    updateKeyboardRangeLabel();
    returnKeyboardFocusToPiano();
}

void NateVSTAudioProcessorEditor::updateKeyboardRangeLabel()
{
    const auto mappedBaseNote = clampedKeyboardLowestVisibleNote(keyboardTypingBaseNote);
    if (keyboardTypingBaseNote != mappedBaseNote)
    {
        releaseComputerKeyboardNotes();
        audioProcessor.getMidiKeyboardState().allNotesOff(1);
        keyboardTypingBaseNote = mappedBaseNote;
    }

    syncPianoKeyboardComputerMapping();

    const auto noteName = abletonNoteName(mappedBaseNote);
    const auto mappedTopNote = juce::jlimit(0,
                                           keyboardHighestNote,
                                           mappedBaseNote + keyboardTypingKeySpanSemitones);
    const auto mappedTopName = abletonNoteName(mappedTopNote);

    pinPianoKeyboardVisualRange();

    const auto labelText = "A:" + noteName + "\n;:" + mappedTopName;
    if (keyboardRangeLabel.getText() != labelText)
        keyboardRangeLabel.setText(labelText, juce::dontSendNotification);

    keyboardRangeLabel.setTooltip("Laptop keys A through ; map from " + noteName + " to " + mappedTopName
                                  + " using Ableton-style octave names; the visible piano strip stays C1 to C5");

    keyboardOctaveDownButton.setEnabled(mappedBaseNote > keyboardMinLowestVisibleNote);
    keyboardOctaveUpButton.setEnabled(mappedBaseNote < keyboardMaxLowestVisibleNote);
}

int NateVSTAudioProcessorEditor::computerKeyboardBaseNote() const noexcept
{
    return juce::jlimit(keyboardMinLowestVisibleNote,
                        keyboardMaxLowestVisibleNote,
                        keyboardTypingBaseNote);
}

void NateVSTAudioProcessorEditor::syncPianoKeyboardComputerMapping()
{
    const auto baseNote = computerKeyboardBaseNote();
    if (syncedPianoKeyboardMappingBaseNote == baseNote)
        return;

    // Own laptop-key note triggering in the editor so DAW focus changes cannot
    // fall back to JUCE's default C0-ish keyboard mapping.
    pianoKeyboard.clearKeyMappings();
    syncedPianoKeyboardMappingBaseNote = baseNote;
}

void NateVSTAudioProcessorEditor::pinPianoKeyboardVisualRange()
{
    pianoKeyboard.setAvailableRange(keyboardVisualLowestNote, keyboardVisualHighestNote);
    pianoKeyboard.setScrollButtonsVisible(false);

    const auto expectedKeyWidth = responsiveKeyboardKeyWidthForBounds(pianoKeyboard.getBounds(),
                                                                     keyboardVisualLowestNote);
    if (std::abs(pianoKeyboard.getKeyWidth() - expectedKeyWidth) > 0.25f)
        pianoKeyboard.setKeyWidth(expectedKeyWidth);

    if (pianoKeyboard.getLowestVisibleKey() != keyboardVisualLowestNote)
        pianoKeyboard.setLowestVisibleKey(keyboardVisualLowestNote);
}

void NateVSTAudioProcessorEditor::releaseComputerKeyboardNotes()
{
    auto& keyboardState = audioProcessor.getMidiKeyboardState();
    const auto baseNote = computerKeyboardBaseNote();
    for (size_t index = 0; index < computerKeyboardNotesDown.size(); ++index)
    {
        if (! computerKeyboardNotesDown[index])
            continue;

        computerKeyboardNotesDown[index] = false;
        keyboardState.noteOff(1,
                              juce::jlimit(0, 127, baseNote + static_cast<int>(index)),
                              0.0f);
    }
}

bool NateVSTAudioProcessorEditor::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    for (const auto keyCode : computerKeyboardKeyCodes)
        if (key.getKeyCode() == keyCode)
            return keyStateChanged(true, originatingComponent);

    return false;
}

bool NateVSTAudioProcessorEditor::keyStateChanged(bool, juce::Component* originatingComponent)
{
    auto isTextEntryComponent = [] (juce::Component* component)
    {
        for (auto* current = component; current != nullptr; current = current->getParentComponent())
            if (dynamic_cast<juce::TextEditor*>(current) != nullptr
                || dynamic_cast<juce::ComboBox*>(current) != nullptr)
                return true;

        return false;
    };

    if (isTextEntryComponent(originatingComponent))
    {
        releaseComputerKeyboardNotes();
        return false;
    }

    auto used = false;
    auto& keyboardState = audioProcessor.getMidiKeyboardState();
    const auto baseNote = computerKeyboardBaseNote();
    for (size_t index = 0; index < computerKeyboardKeyCodes.size(); ++index)
    {
        const auto note = juce::jlimit(0, 127, baseNote + static_cast<int>(index));
        const auto isDown = juce::KeyPress::isKeyCurrentlyDown(computerKeyboardKeyCodes[index]);
        if (isDown && ! computerKeyboardNotesDown[index])
        {
            computerKeyboardNotesDown[index] = true;
            keyboardState.noteOn(1, note, 0.86f);
            used = true;
        }
        else if (! isDown && computerKeyboardNotesDown[index])
        {
            computerKeyboardNotesDown[index] = false;
            keyboardState.noteOff(1, note, 0.0f);
            used = true;
        }
    }

    if (used)
        pinPianoKeyboardVisualRange();

    return used;
}

void NateVSTAudioProcessorEditor::stepSequencerRoot(int semitones)
{
    const auto currentRoot = juce::roundToInt(readPlainParameterValue(Parameters::ID::sequencerRoot, 36.0f));
    const auto nextRoot = juce::jlimit(24, 84, currentRoot + semitones);
    if (nextRoot == currentRoot)
    {
        updateSequencerRootStepper();
        returnKeyboardFocusToPiano();
        return;
    }

    captureGlobalEdit("Edit sequencer root");
    setPlainParameterValue(Parameters::ID::sequencerRoot, static_cast<float>(nextRoot));
    updateSelectedControlInspector("Seq Root", Parameters::ID::sequencerRoot, static_cast<double>(nextRoot));
    updateSequencerRootStepper();
    updateSequencerGridContext();
    repaintSequencerGrids();
    returnKeyboardFocusToPiano();
}

void NateVSTAudioProcessorEditor::updateSequencerRootStepper()
{
    const auto root = juce::jlimit(24, 84, juce::roundToInt(readPlainParameterValue(Parameters::ID::sequencerRoot, 36.0f)));
    const auto octave = juce::roundToInt(readPlainParameterValue(Parameters::ID::sequencerOctave, 0.0f));
    const auto displayRoot = juce::jlimit(0, 127, root + (octave * 12));
    const auto rootName = juce::MidiMessage::getMidiNoteName(root, true, true, 3);
    const auto displayName = juce::MidiMessage::getMidiNoteName(displayRoot, true, true, 3);
    const auto labelText = octave == 0 ? "Root " + rootName
                                       : "Root " + rootName + " -> " + displayName;

    if (sequencerRootValueLabel.getText() != labelText)
        sequencerRootValueLabel.setText(labelText, juce::dontSendNotification);

    sequencerRootValueLabel.setTooltip("Sequencer root note. The arrow buttons move the root by semitone.");
    sequencerRootDownButton.setEnabled(root > 24);
    sequencerRootUpButton.setEnabled(root < 84);
}

void NateVSTAudioProcessorEditor::returnKeyboardFocusToPiano()
{
    for (const auto* textEditor : {
             static_cast<const juce::TextEditor*>(&presetSearchEditor),
             static_cast<const juce::TextEditor*>(&presetNameEditor),
             static_cast<const juce::TextEditor*>(&presetAuthorEditor),
             static_cast<const juce::TextEditor*>(&presetNotesEditor) })
    {
        if (textEditor != nullptr && textEditor->hasKeyboardFocus(true))
            return;
    }

    juce::Component::SafePointer<NateVSTAudioProcessorEditor> safeEditor(this);
    juce::MessageManager::callAsync([safeEditor]
    {
        if (safeEditor != nullptr)
        {
            safeEditor->pinPianoKeyboardVisualRange();
            safeEditor->pianoKeyboard.grabKeyboardFocus();
        }
    });
}

void NateVSTAudioProcessorEditor::installGlobalKeyboardListeners()
{
    if (globalKeyboardListenersInstalled)
        return;

    installGlobalKeyboardListenersFor(*this);
    globalKeyboardListenersInstalled = true;
}

void NateVSTAudioProcessorEditor::removeGlobalKeyboardListeners()
{
    if (! globalKeyboardListenersInstalled)
        return;

    removeGlobalKeyboardListenersFor(*this);
    globalKeyboardListenersInstalled = false;
}

void NateVSTAudioProcessorEditor::installGlobalKeyboardListenersFor(juce::Component& component)
{
    component.addKeyListener(this);

    for (auto childIndex = 0; childIndex < component.getNumChildComponents(); ++childIndex)
        if (auto* child = component.getChildComponent(childIndex))
            installGlobalKeyboardListenersFor(*child);
}

void NateVSTAudioProcessorEditor::removeGlobalKeyboardListenersFor(juce::Component& component)
{
    component.removeKeyListener(this);

    for (auto childIndex = 0; childIndex < component.getNumChildComponents(); ++childIndex)
        if (auto* child = component.getChildComponent(childIndex))
            removeGlobalKeyboardListenersFor(*child);
}

void NateVSTAudioProcessorEditor::addFxModule(FxModule module)
{
    captureGlobalEdit("Add FX " + fxModuleName(module));
    setPlainParameterValue(fxEnabledParameterID(module), 1.0f);
    selectedFxModule = module;
    fxAddBox.setSelectedId(0, juce::dontSendNotification);
    updateFxRackControls();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::removeSelectedFxModule()
{
    if (selectedFxModule == FxModule::guard)
    {
        fxRackStatusLabel.setText("Guard stays available as the output safety module", juce::dontSendNotification);
        return;
    }

    captureGlobalEdit("Remove FX " + fxModuleName(selectedFxModule));
    setPlainParameterValue(fxEnabledParameterID(selectedFxModule), 0.0f);

    selectedFxModule = FxModule::guard;

    updateFxRackControls();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::selectFxModule(FxModule module)
{
    selectedFxModule = module;
    updateFxRackControls();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::moveSelectedFxModule(int direction)
{
    if (selectedFxModule == FxModule::guard)
    {
        fxRackStatusLabel.setText("Guard stays last as the output safety stage", juce::dontSendNotification);
        return;
    }

    auto order = fxModuleOrder();
    const auto position = fxOrderPosition(selectedFxModule) - 1;
    const auto lastMovablePosition = static_cast<int>(order.size()) - 2;

    if (position < 0)
        return;

    const auto step = direction < 0 ? -1 : 1;
    auto targetPosition = position + step;

    while (targetPosition >= 0
           && targetPosition <= lastMovablePosition
           && ! shouldShowFxModule(order[static_cast<size_t>(targetPosition)]))
    {
        targetPosition += step;
    }

    if (targetPosition < 0 || targetPosition > lastMovablePosition)
        return;

    captureGlobalEdit("Move FX " + fxModuleName(selectedFxModule));
    std::swap(order[static_cast<size_t>(position)], order[static_cast<size_t>(targetPosition)]);
    setFxModuleOrder(order);
    updateFxRackControls();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::resetFxModuleOrder()
{
    captureGlobalEdit("Reset FX order");
    setFxModuleOrder(fxDefaultModuleOrder());
    updateFxRackControls();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::applyDelayThrow()
{
    captureGlobalEdit("Delay throw");
    setPlainParameterValue(Parameters::ID::fxSendTailKill, 0.0f);
    setPlainParameterValue(Parameters::ID::fxDelayEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxDelaySync, 1.0f);
    setPlainParameterValue(Parameters::ID::fxDelayRate, 2.0f);
    setPlainParameterValue(Parameters::ID::fxDelayTime, 0.31f);
    setPlainParameterValue(Parameters::ID::fxDelayFeedback, 0.58f);
    setPlainParameterValue(Parameters::ID::fxDelayMix, 0.42f);
    setPlainParameterValue(Parameters::ID::fxSendDelay, 0.72f);
    setPlainParameterValue(Parameters::ID::fxReverbEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxReverbSize, 0.28f);
    setPlainParameterValue(Parameters::ID::fxReverbDamping, 0.50f);
    setPlainParameterValue(Parameters::ID::fxReverbMix, 0.14f);
    setPlainParameterValue(Parameters::ID::fxSendReverb, 0.12f);
    setPlainParameterValue(Parameters::ID::fxWidthEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxWidthAmount, 1.18f);
    setPlainParameterValue(Parameters::ID::fxWidthMonoCutoff, 140.0f);
    setPlainParameterValue(Parameters::ID::fxGuardEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxGuardPush, 0.08f);
    setPlainParameterValue(Parameters::ID::fxGuardGlue, 0.22f);
    setPlainParameterValue(Parameters::ID::fxGuardPunch, 0.12f);
    setPlainParameterValue(Parameters::ID::fxGuardClipMix, 0.92f);
    setPlainParameterValue(Parameters::ID::fxGuardCeiling, 0.90f);

    selectedFxModule = FxModule::delay;
    resized();
    repaint();
    setFxRackStatusOverride("Delay throw armed | send bus + synced 1/8D feedback");
}

void NateVSTAudioProcessorEditor::applySpaceThrow()
{
    captureGlobalEdit("Space throw");
    setPlainParameterValue(Parameters::ID::fxSendTailKill, 0.0f);
    setPlainParameterValue(Parameters::ID::fxDelayEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxDelaySync, 1.0f);
    setPlainParameterValue(Parameters::ID::fxDelayRate, 1.0f);
    setPlainParameterValue(Parameters::ID::fxDelayTime, 0.42f);
    setPlainParameterValue(Parameters::ID::fxDelayFeedback, 0.46f);
    setPlainParameterValue(Parameters::ID::fxDelayMix, 0.26f);
    setPlainParameterValue(Parameters::ID::fxSendDelay, 0.38f);
    setPlainParameterValue(Parameters::ID::fxReverbEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxReverbSize, 0.72f);
    setPlainParameterValue(Parameters::ID::fxReverbDamping, 0.38f);
    setPlainParameterValue(Parameters::ID::fxReverbMix, 0.38f);
    setPlainParameterValue(Parameters::ID::fxSendReverb, 0.66f);
    setPlainParameterValue(Parameters::ID::fxWidthEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxWidthAmount, 1.36f);
    setPlainParameterValue(Parameters::ID::fxWidthMonoCutoff, 145.0f);
    setPlainParameterValue(Parameters::ID::fxGuardEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxGuardPush, 0.05f);
    setPlainParameterValue(Parameters::ID::fxGuardGlue, 0.18f);
    setPlainParameterValue(Parameters::ID::fxGuardPunch, 0.08f);
    setPlainParameterValue(Parameters::ID::fxGuardClipMix, 0.86f);
    setPlainParameterValue(Parameters::ID::fxGuardCeiling, 0.90f);

    selectedFxModule = FxModule::reverb;
    resized();
    repaint();
    setFxRackStatusOverride("Space throw armed | send delay into larger reverb");
}

void NateVSTAudioProcessorEditor::applyPumpDrop()
{
    captureGlobalEdit("Pump drop");
    setPlainParameterValue(Parameters::ID::fxPumpEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxPumpRate, 1.0f);
    setPlainParameterValue(Parameters::ID::fxPumpCurve, 2.0f);
    setPlainParameterValue(Parameters::ID::fxPumpDepth, 0.58f);
    setPlainParameterValue(Parameters::ID::fxPumpShape, 0.68f);
    setPlainParameterValue(Parameters::ID::fxPumpPhase, 0.08f);
    setPlainParameterValue(Parameters::ID::fxWidthEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxWidthAmount, 1.08f);
    setPlainParameterValue(Parameters::ID::fxWidthMonoCutoff, 135.0f);
    setPlainParameterValue(Parameters::ID::fxGuardEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxGuardPush, 0.10f);
    setPlainParameterValue(Parameters::ID::fxGuardGlue, 0.30f);
    setPlainParameterValue(Parameters::ID::fxGuardPunch, 0.16f);
    setPlainParameterValue(Parameters::ID::fxGuardClipMix, 0.94f);
    setPlainParameterValue(Parameters::ID::fxGuardCeiling, 0.90f);
    fxPumpRateBox.setSelectedItemIndex(1, juce::dontSendNotification);
    fxPumpCurveBox.setSelectedItemIndex(2, juce::dontSendNotification);

    selectedFxModule = FxModule::pump;
    resized();
    repaint();
    setFxRackStatusOverride("Pump drop armed | 1/8 ducking with Guard safety");
}

void NateVSTAudioProcessorEditor::clearFxThrows()
{
    captureGlobalEdit("Clear throws");
    audioProcessor.requestFxSendTailKill();
    setPlainParameterValue(Parameters::ID::fxDelayEnabled, 0.0f);
    setPlainParameterValue(Parameters::ID::fxDelaySync, 0.0f);
    setPlainParameterValue(Parameters::ID::fxDelayFeedback, 0.18f);
    setPlainParameterValue(Parameters::ID::fxDelayMix, 0.0f);
    setPlainParameterValue(Parameters::ID::fxSendDelay, 0.0f);
    setPlainParameterValue(Parameters::ID::fxReverbEnabled, 0.0f);
    setPlainParameterValue(Parameters::ID::fxReverbMix, 0.0f);
    setPlainParameterValue(Parameters::ID::fxSendReverb, 0.0f);
    setPlainParameterValue(Parameters::ID::fxSendTailKill, 0.0f);
    setPlainParameterValue(Parameters::ID::fxPumpEnabled, 0.0f);
    setPlainParameterValue(Parameters::ID::fxPumpDepth, 0.0f);
    setPlainParameterValue(Parameters::ID::fxGuardEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxGuardPush, 0.04f);
    setPlainParameterValue(Parameters::ID::fxGuardGlue, 0.10f);
    setPlainParameterValue(Parameters::ID::fxGuardPunch, 0.05f);
    setPlainParameterValue(Parameters::ID::fxGuardClipMix, 1.0f);
    setPlainParameterValue(Parameters::ID::fxGuardCeiling, 0.92f);

    selectedFxModule = FxModule::guard;
    resized();
    repaint();
    setFxRackStatusOverride("Sends reset | tails killed, Guard remains on");
}

void NateVSTAudioProcessorEditor::beginMomentaryFxAction(MomentaryFxAction action)
{
    if (action == MomentaryFxAction::none || activeMomentaryFxAction != MomentaryFxAction::none)
        return;

    fxMomentarySnapshot = captureFxMomentarySnapshot();
    activeMomentaryFxAction = action;

    switch (action)
    {
        case MomentaryFxAction::delay:
            applyDelayThrow();
            setFxRackStatusOverride("Holding delay throw | release to restore");
            break;

        case MomentaryFxAction::space:
            applySpaceThrow();
            setFxRackStatusOverride("Holding space throw | release to restore");
            break;

        case MomentaryFxAction::pump:
            applyPumpDrop();
            setFxRackStatusOverride("Holding pump drop | release to restore");
            break;

        case MomentaryFxAction::mute:
            applyMomentaryMuteDrop();
            break;

        case MomentaryFxAction::none:
            break;
    }
}

void NateVSTAudioProcessorEditor::endMomentaryFxAction(MomentaryFxAction action)
{
    if (action == MomentaryFxAction::none || activeMomentaryFxAction != action)
        return;

    restoreFxMomentarySnapshot(fxMomentarySnapshot);
    fxMomentarySnapshot = {};
    activeMomentaryFxAction = MomentaryFxAction::none;
    resized();
    repaint();
    setFxRackStatusOverride("Momentary FX restored");
}

void NateVSTAudioProcessorEditor::applyMomentaryMuteDrop()
{
    setPlainParameterValue(Parameters::ID::outputGain, -24.0f);
    setPlainParameterValue(Parameters::ID::fxGuardEnabled, 1.0f);
    setPlainParameterValue(Parameters::ID::fxGuardPush, 0.0f);
    setPlainParameterValue(Parameters::ID::fxGuardGlue, 0.0f);
    setPlainParameterValue(Parameters::ID::fxGuardPunch, 0.0f);
    setPlainParameterValue(Parameters::ID::fxGuardClipMix, 1.0f);
    setPlainParameterValue(Parameters::ID::fxGuardCeiling, 0.92f);

    selectedFxModule = FxModule::guard;
    resized();
    repaint();
    setFxRackStatusOverride("Holding mute drop | release to restore");
}

void NateVSTAudioProcessorEditor::updateFxPresetBox(bool force)
{
    if (! force && fxPresetBoxModule == selectedFxModule && fxPresetBox.getNumItems() > 0)
        return;

    fxPresetBoxModule = selectedFxModule;
    fxPresetBox.clear(juce::dontSendNotification);

    auto addPreset = [this] (int presetId, const juce::String& name)
    {
        fxPresetBox.addItem(name, presetId);
    };

    switch (selectedFxModule)
    {
        case FxModule::tone:
            addPreset(1, "Garage Low Cut");
            addPreset(2, "Warm Top");
            addPreset(3, "Dark Warehouse");
            break;

        case FxModule::eq:
            addPreset(1, "Bass Tuck");
            addPreset(2, "Stab Presence");
            addPreset(3, "Club Trim");
            break;

        case FxModule::distortion:
            addPreset(1, "Controlled Grit");
            addPreset(2, "Rubber Bass");
            addPreset(3, "Warehouse Clip");
            break;

        case FxModule::bitcrush:
            addPreset(1, "Perc Dirt");
            addPreset(2, "Dub Edge");
            addPreset(3, "Digital Tight");
            break;

        case FxModule::pump:
            addPreset(1, "House Quarter");
            addPreset(2, "UKG Bounce");
            addPreset(3, "Tight Sixteenth");
            break;

        case FxModule::tremolo:
            addPreset(1, "Garage Skank");
            addPreset(2, "Wide Chop");
            addPreset(3, "Subtle Pulse");
            break;

        case FxModule::ring:
            addPreset(1, "Vocal Edge");
            addPreset(2, "Metal Blip");
            addPreset(3, "Dark Sidebands");
            break;

        case FxModule::comb:
            addPreset(1, "Garage Resonator");
            addPreset(2, "Perc Notch");
            addPreset(3, "Pluck Body");
            break;

        case FxModule::phaser:
            addPreset(1, "Light Sweep");
            addPreset(2, "Dub Swirl");
            addPreset(3, "Tech Pulse");
            break;

        case FxModule::flanger:
            addPreset(1, "Short Comb");
            addPreset(2, "Garage Drift");
            addPreset(3, "Metal Jet");
            break;

        case FxModule::chorus:
            addPreset(1, "Wide Organ");
            addPreset(2, "Tight Stab");
            addPreset(3, "Warm Pad");
            break;

        case FxModule::delay:
            addPreset(1, "Garage Throw");
            addPreset(2, "Short Slap");
            addPreset(3, "Dub Repeat");
            break;

        case FxModule::reverb:
            addPreset(1, "Short Room");
            addPreset(2, "Plate Wash");
            addPreset(3, "Dark Space");
            break;

        case FxModule::width:
            addPreset(1, "Mono-Safe Width");
            addPreset(2, "Wide Stab");
            addPreset(3, "Club Narrow");
            break;

        case FxModule::guard:
            addPreset(1, "Club Glue");
            addPreset(2, "Hot Clip");
            addPreset(3, "Punch Safe");
            break;
    }

    fxPresetBox.setTextWhenNothingSelected("Module Preset");
    fxPresetBox.setSelectedId(0, juce::dontSendNotification);
    fxApplyPresetButton.setEnabled(fxPresetBox.getNumItems() > 0);
}

void NateVSTAudioProcessorEditor::applySelectedFxPreset()
{
    const auto presetId = fxPresetBox.getSelectedId();
    if (presetId <= 0)
        return;

    applyFxModulePreset(selectedFxModule, presetId);
}

void NateVSTAudioProcessorEditor::applyFxModulePreset(FxModule module, int presetId)
{
    const auto presetName = fxPresetBox.getText().isNotEmpty() ? fxPresetBox.getText() : juce::String("FX preset");

    auto set = [this] (const juce::String& parameterID, float value)
    {
        setPlainParameterValue(parameterID, value);
    };

    selectedFxModule = module;
    set(fxEnabledParameterID(module), 1.0f);

    switch (module)
    {
        case FxModule::tone:
            if (presetId == 1)
            {
                set(Parameters::ID::fxToneTilt, 0.08f);
                set(Parameters::ID::fxToneLowCut, 92.0f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxToneTilt, 0.22f);
                set(Parameters::ID::fxToneLowCut, 48.0f);
            }
            else
            {
                set(Parameters::ID::fxToneTilt, -0.30f);
                set(Parameters::ID::fxToneLowCut, 70.0f);
            }
            break;

        case FxModule::eq:
            if (presetId == 1)
            {
                set(Parameters::ID::fxEqLowGain, -2.6f);
                set(Parameters::ID::fxEqMidGain, 0.4f);
                set(Parameters::ID::fxEqHighGain, 1.2f);
                set(Parameters::ID::fxEqTrim, -0.8f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxEqLowGain, -1.2f);
                set(Parameters::ID::fxEqMidGain, 1.8f);
                set(Parameters::ID::fxEqHighGain, 2.4f);
                set(Parameters::ID::fxEqTrim, -1.2f);
            }
            else
            {
                set(Parameters::ID::fxEqLowGain, -0.8f);
                set(Parameters::ID::fxEqMidGain, -1.0f);
                set(Parameters::ID::fxEqHighGain, 0.8f);
                set(Parameters::ID::fxEqTrim, -1.8f);
            }
            break;

        case FxModule::distortion:
            set(Parameters::ID::fxDistortionAmount, presetId == 1 ? 0.22f : (presetId == 2 ? 0.38f : 0.56f));
            set(Parameters::ID::fxDistortionBassSafe, presetId == 1 ? 0.20f : (presetId == 2 ? 0.48f : 0.72f));
            break;

        case FxModule::bitcrush:
            if (presetId == 1)
            {
                set(Parameters::ID::fxBitcrushBits, 10.0f);
                set(Parameters::ID::fxBitcrushDownsample, 2.0f);
                set(Parameters::ID::fxBitcrushMix, 0.18f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxBitcrushBits, 8.0f);
                set(Parameters::ID::fxBitcrushDownsample, 4.0f);
                set(Parameters::ID::fxBitcrushMix, 0.24f);
            }
            else
            {
                set(Parameters::ID::fxBitcrushBits, 12.0f);
                set(Parameters::ID::fxBitcrushDownsample, 3.0f);
                set(Parameters::ID::fxBitcrushMix, 0.14f);
            }
            break;

        case FxModule::pump:
            if (presetId == 1)
            {
                set(Parameters::ID::fxPumpRate, 0.0f);
                set(Parameters::ID::fxPumpCurve, 0.0f);
                set(Parameters::ID::fxPumpDepth, 0.44f);
                set(Parameters::ID::fxPumpShape, 0.56f);
                set(Parameters::ID::fxPumpPhase, 0.0f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxPumpRate, 1.0f);
                set(Parameters::ID::fxPumpCurve, 2.0f);
                set(Parameters::ID::fxPumpDepth, 0.34f);
                set(Parameters::ID::fxPumpShape, 0.64f);
                set(Parameters::ID::fxPumpPhase, 0.08f);
            }
            else
            {
                set(Parameters::ID::fxPumpRate, 3.0f);
                set(Parameters::ID::fxPumpCurve, 4.0f);
                set(Parameters::ID::fxPumpDepth, 0.26f);
                set(Parameters::ID::fxPumpShape, 0.48f);
                set(Parameters::ID::fxPumpPhase, 0.0f);
            }
            break;

        case FxModule::tremolo:
            if (presetId == 1)
            {
                set(Parameters::ID::fxTremoloRate, 2.0f);
                set(Parameters::ID::fxTremoloDepth, 0.30f);
                set(Parameters::ID::fxTremoloPan, 0.46f);
                set(Parameters::ID::fxTremoloShape, 0.64f);
                set(Parameters::ID::fxTremoloPhase, 0.18f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxTremoloRate, 3.0f);
                set(Parameters::ID::fxTremoloDepth, 0.44f);
                set(Parameters::ID::fxTremoloPan, 0.78f);
                set(Parameters::ID::fxTremoloShape, 0.70f);
                set(Parameters::ID::fxTremoloPhase, 0.28f);
            }
            else
            {
                set(Parameters::ID::fxTremoloRate, 1.0f);
                set(Parameters::ID::fxTremoloDepth, 0.18f);
                set(Parameters::ID::fxTremoloPan, 0.20f);
                set(Parameters::ID::fxTremoloShape, 0.42f);
                set(Parameters::ID::fxTremoloPhase, 0.0f);
            }
            break;

        case FxModule::ring:
            if (presetId == 1)
            {
                set(Parameters::ID::fxRingFrequency, 92.0f);
                set(Parameters::ID::fxRingDepth, 0.18f);
                set(Parameters::ID::fxRingMix, 0.08f);
                set(Parameters::ID::fxRingBias, 0.60f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxRingFrequency, 420.0f);
                set(Parameters::ID::fxRingDepth, 0.34f);
                set(Parameters::ID::fxRingMix, 0.14f);
                set(Parameters::ID::fxRingBias, 0.42f);
            }
            else
            {
                set(Parameters::ID::fxRingFrequency, 58.0f);
                set(Parameters::ID::fxRingDepth, 0.26f);
                set(Parameters::ID::fxRingMix, 0.10f);
                set(Parameters::ID::fxRingBias, 0.34f);
            }
            break;

        case FxModule::comb:
            if (presetId == 1)
            {
                set(Parameters::ID::fxCombFrequency, 280.0f);
                set(Parameters::ID::fxCombFeedback, 0.20f);
                set(Parameters::ID::fxCombDamping, 0.56f);
                set(Parameters::ID::fxCombMix, 0.08f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxCombFrequency, 620.0f);
                set(Parameters::ID::fxCombFeedback, -0.24f);
                set(Parameters::ID::fxCombDamping, 0.42f);
                set(Parameters::ID::fxCombMix, 0.10f);
            }
            else
            {
                set(Parameters::ID::fxCombFrequency, 170.0f);
                set(Parameters::ID::fxCombFeedback, 0.32f);
                set(Parameters::ID::fxCombDamping, 0.48f);
                set(Parameters::ID::fxCombMix, 0.12f);
            }
            break;

        case FxModule::phaser:
            if (presetId == 1)
            {
                set(Parameters::ID::fxPhaserRate, 0.32f);
                set(Parameters::ID::fxPhaserDepth, 0.34f);
                set(Parameters::ID::fxPhaserMix, 0.16f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxPhaserRate, 0.18f);
                set(Parameters::ID::fxPhaserDepth, 0.56f);
                set(Parameters::ID::fxPhaserMix, 0.24f);
            }
            else
            {
                set(Parameters::ID::fxPhaserRate, 0.78f);
                set(Parameters::ID::fxPhaserDepth, 0.28f);
                set(Parameters::ID::fxPhaserMix, 0.14f);
            }
            break;

        case FxModule::flanger:
            if (presetId == 1)
            {
                set(Parameters::ID::fxFlangerRate, 0.16f);
                set(Parameters::ID::fxFlangerDepth, 0.28f);
                set(Parameters::ID::fxFlangerFeedback, 0.24f);
                set(Parameters::ID::fxFlangerMix, 0.10f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxFlangerRate, 0.24f);
                set(Parameters::ID::fxFlangerDepth, 0.40f);
                set(Parameters::ID::fxFlangerFeedback, 0.12f);
                set(Parameters::ID::fxFlangerMix, 0.16f);
            }
            else
            {
                set(Parameters::ID::fxFlangerRate, 0.46f);
                set(Parameters::ID::fxFlangerDepth, 0.52f);
                set(Parameters::ID::fxFlangerFeedback, 0.42f);
                set(Parameters::ID::fxFlangerMix, 0.18f);
            }
            break;

        case FxModule::chorus:
            if (presetId == 1)
            {
                set(Parameters::ID::fxChorusRate, 0.28f);
                set(Parameters::ID::fxChorusDepth, 0.42f);
                set(Parameters::ID::fxChorusMix, 0.24f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxChorusRate, 0.42f);
                set(Parameters::ID::fxChorusDepth, 0.24f);
                set(Parameters::ID::fxChorusMix, 0.14f);
            }
            else
            {
                set(Parameters::ID::fxChorusRate, 0.18f);
                set(Parameters::ID::fxChorusDepth, 0.54f);
                set(Parameters::ID::fxChorusMix, 0.28f);
            }
            break;

        case FxModule::delay:
            if (presetId == 1)
            {
                set(Parameters::ID::fxDelaySync, 1.0f);
                set(Parameters::ID::fxDelayRate, 2.0f);
                set(Parameters::ID::fxDelayTime, 0.31f);
                set(Parameters::ID::fxDelayFeedback, 0.54f);
                set(Parameters::ID::fxDelayMix, 0.34f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxDelaySync, 1.0f);
                set(Parameters::ID::fxDelayRate, 4.0f);
                set(Parameters::ID::fxDelayTime, 0.11f);
                set(Parameters::ID::fxDelayFeedback, 0.20f);
                set(Parameters::ID::fxDelayMix, 0.16f);
            }
            else
            {
                set(Parameters::ID::fxDelaySync, 1.0f);
                set(Parameters::ID::fxDelayRate, 0.0f);
                set(Parameters::ID::fxDelayTime, 0.42f);
                set(Parameters::ID::fxDelayFeedback, 0.62f);
                set(Parameters::ID::fxDelayMix, 0.28f);
            }
            break;

        case FxModule::reverb:
            if (presetId == 1)
            {
                set(Parameters::ID::fxReverbSize, 0.24f);
                set(Parameters::ID::fxReverbDamping, 0.62f);
                set(Parameters::ID::fxReverbMix, 0.12f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxReverbSize, 0.54f);
                set(Parameters::ID::fxReverbDamping, 0.42f);
                set(Parameters::ID::fxReverbMix, 0.24f);
            }
            else
            {
                set(Parameters::ID::fxReverbSize, 0.74f);
                set(Parameters::ID::fxReverbDamping, 0.28f);
                set(Parameters::ID::fxReverbMix, 0.30f);
            }
            break;

        case FxModule::width:
            if (presetId == 1)
            {
                set(Parameters::ID::fxWidthAmount, 1.12f);
                set(Parameters::ID::fxWidthMonoCutoff, 145.0f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxWidthAmount, 1.42f);
                set(Parameters::ID::fxWidthMonoCutoff, 135.0f);
            }
            else
            {
                set(Parameters::ID::fxWidthAmount, 0.92f);
                set(Parameters::ID::fxWidthMonoCutoff, 175.0f);
            }
            break;

        case FxModule::guard:
            if (presetId == 1)
            {
                set(Parameters::ID::fxGuardPush, 0.06f);
                set(Parameters::ID::fxGuardGlue, 0.34f);
                set(Parameters::ID::fxGuardPunch, 0.18f);
                set(Parameters::ID::fxGuardClipMix, 0.86f);
                set(Parameters::ID::fxGuardCeiling, 0.90f);
            }
            else if (presetId == 2)
            {
                set(Parameters::ID::fxGuardPush, 0.16f);
                set(Parameters::ID::fxGuardGlue, 0.22f);
                set(Parameters::ID::fxGuardPunch, 0.08f);
                set(Parameters::ID::fxGuardClipMix, 1.0f);
                set(Parameters::ID::fxGuardCeiling, 0.88f);
            }
            else
            {
                set(Parameters::ID::fxGuardPush, 0.02f);
                set(Parameters::ID::fxGuardGlue, 0.12f);
                set(Parameters::ID::fxGuardPunch, 0.28f);
                set(Parameters::ID::fxGuardClipMix, 0.72f);
                set(Parameters::ID::fxGuardCeiling, 0.94f);
            }
            break;
    }

    updateFxRackControls();
    resized();
    repaint();
    setFxRackStatusOverride(presetName + " loaded | " + fxModuleName(module));
}

NateVSTAudioProcessorEditor::FxMomentarySnapshot NateVSTAudioProcessorEditor::captureFxMomentarySnapshot() const
{
    FxMomentarySnapshot snapshot;
    snapshot.selectedModule = selectedFxModule;
    snapshot.valid = true;

    for (size_t index = 0; index < momentaryFxParameterIDs.size(); ++index)
        snapshot.values[index] = readPlainParameterValue(momentaryFxParameterIDs[index], 0.0f);

    return snapshot;
}

void NateVSTAudioProcessorEditor::restoreFxMomentarySnapshot(const FxMomentarySnapshot& snapshot)
{
    if (! snapshot.valid)
        return;

    for (size_t index = 0; index < momentaryFxParameterIDs.size(); ++index)
        setPlainParameterValue(momentaryFxParameterIDs[index], snapshot.values[index]);

    fxDelayRateBox.setSelectedItemIndex(juce::roundToInt(readPlainParameterValue(Parameters::ID::fxDelayRate, 1.0f)), juce::dontSendNotification);
    fxPumpRateBox.setSelectedItemIndex(juce::roundToInt(readPlainParameterValue(Parameters::ID::fxPumpRate, 0.0f)), juce::dontSendNotification);
    fxPumpCurveBox.setSelectedItemIndex(juce::roundToInt(readPlainParameterValue(Parameters::ID::fxPumpCurve, 0.0f)), juce::dontSendNotification);
    selectedFxModule = snapshot.selectedModule;
}

void NateVSTAudioProcessorEditor::setFxRackStatusOverride(const juce::String& message)
{
    fxRackStatusOverride = message;
    fxRackStatusOverrideUntilMs = juce::Time::getMillisecondCounterHiRes() + fxRackStatusOverrideMs;
    fxRackStatusLabel.setText(fxRackStatusOverride, juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::updateFxRackControls()
{
    if (! shouldShowFxModule(selectedFxModule))
        selectedFxModule = FxModule::guard;

    const auto moduleOrder = fxModuleOrder();
    for (const auto module : moduleOrder)
    {
        auto& button = fxSlotButton(module);
        const auto isEnabled = isFxModuleEnabled(module);
        button.setState(fxModuleName(module),
                        fxModuleSummary(module),
                        fxOrderPosition(module),
                        isEnabled,
                        module == selectedFxModule,
                        module == FxModule::guard);
    }

    if (fxRackStatusOverride.isNotEmpty()
        && juce::Time::getMillisecondCounterHiRes() < fxRackStatusOverrideUntilMs)
    {
        fxRackStatusLabel.setText(fxRackStatusOverride, juce::dontSendNotification);
    }
    else
    {
        fxRackStatusOverride.clear();
        fxRackStatusLabel.setText("#" + juce::String(fxOrderPosition(selectedFxModule)).paddedLeft('0', 2)
                                      + " " + fxModuleName(selectedFxModule)
                                      + " | " + fxModuleSummary(selectedFxModule),
                                  juce::dontSendNotification);
    }
    const auto selectedPosition = fxOrderPosition(selectedFxModule);
    const auto canMoveSelected = selectedFxModule != FxModule::guard;
    const auto hasVisibleMoveTarget = [&] (int direction)
    {
        if (! canMoveSelected)
            return false;

        const auto position = selectedPosition - 1;
        const auto lastMovablePosition = static_cast<int>(moduleOrder.size()) - 2;
        const auto step = direction < 0 ? -1 : 1;
        auto targetPosition = position + step;

        while (targetPosition >= 0 && targetPosition <= lastMovablePosition)
        {
            if (shouldShowFxModule(moduleOrder[static_cast<size_t>(targetPosition)]))
                return true;

            targetPosition += step;
        }

        return false;
    };

    updateFxPresetBox();
    fxMoveUpButton.setEnabled(hasVisibleMoveTarget(-1));
    fxMoveDownButton.setEnabled(hasVisibleMoveTarget(1));
    fxRemoveButton.setEnabled(selectedFxModule != FxModule::guard);
}

std::array<NateVSTAudioProcessorEditor::FxModule, 15> NateVSTAudioProcessorEditor::fxDefaultModuleOrder() const
{
    return {
        FxModule::tone,
        FxModule::eq,
        FxModule::distortion,
        FxModule::bitcrush,
        FxModule::pump,
        FxModule::tremolo,
        FxModule::ring,
        FxModule::comb,
        FxModule::phaser,
        FxModule::flanger,
        FxModule::chorus,
        FxModule::delay,
        FxModule::reverb,
        FxModule::width,
        FxModule::guard
    };
}

std::array<NateVSTAudioProcessorEditor::FxModule, 15> NateVSTAudioProcessorEditor::fxModuleOrder() const
{
    auto order = fxDefaultModuleOrder();
    std::array<bool, 15> used {};
    auto writeIndex = size_t { 0 };

    for (size_t slotIndex = 0; slotIndex < Parameters::ID::fxOrder.size(); ++slotIndex)
    {
        const auto fallback = static_cast<float>(slotIndex);
        auto moduleIndex = static_cast<int>(slotIndex);
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(Parameters::ID::fxOrder[slotIndex]))
            moduleIndex = static_cast<int>(std::round(value->load()));
        else
            moduleIndex = static_cast<int>(fallback);

        moduleIndex = juce::jlimit(0, 14, moduleIndex);
        const auto module = fxModuleFromIndex(moduleIndex);
        if (module == FxModule::guard || used[static_cast<size_t>(moduleIndex)])
            continue;

        order[writeIndex++] = module;
        used[static_cast<size_t>(moduleIndex)] = true;
    }

    for (const auto module : fxDefaultModuleOrder())
    {
        const auto moduleIndex = fxModuleIndex(module);
        if (module == FxModule::guard || used[static_cast<size_t>(moduleIndex)])
            continue;

        order[writeIndex++] = module;
        used[static_cast<size_t>(moduleIndex)] = true;
    }

    while (writeIndex < order.size() - 1)
        order[writeIndex++] = FxModule::guard;

    order.back() = FxModule::guard;
    return order;
}

void NateVSTAudioProcessorEditor::setFxModuleOrder(const std::array<FxModule, 15>& order)
{
    auto normalised = order;
    normalised.back() = FxModule::guard;

    for (size_t slotIndex = 0; slotIndex < Parameters::ID::fxOrder.size(); ++slotIndex)
        setPlainParameterValue(Parameters::ID::fxOrder[slotIndex], static_cast<float>(fxModuleIndex(normalised[slotIndex])));
}

int NateVSTAudioProcessorEditor::fxOrderPosition(FxModule module) const
{
    const auto order = fxModuleOrder();
    for (size_t index = 0; index < order.size(); ++index)
        if (order[index] == module)
            return static_cast<int>(index + 1);

    return 0;
}

int NateVSTAudioProcessorEditor::fxModuleIndex(FxModule module) const
{
    return static_cast<int>(module);
}

NateVSTAudioProcessorEditor::FxModule NateVSTAudioProcessorEditor::fxModuleFromIndex(int index) const
{
    switch (juce::jlimit(0, 14, index))
    {
        case 0: return FxModule::tone;
        case 1: return FxModule::eq;
        case 2: return FxModule::distortion;
        case 3: return FxModule::bitcrush;
        case 4: return FxModule::pump;
        case 5: return FxModule::tremolo;
        case 6: return FxModule::ring;
        case 7: return FxModule::comb;
        case 8: return FxModule::phaser;
        case 9: return FxModule::flanger;
        case 10: return FxModule::chorus;
        case 11: return FxModule::delay;
        case 12: return FxModule::reverb;
        case 13: return FxModule::width;
        case 14:
        default: return FxModule::guard;
    }
}

bool NateVSTAudioProcessorEditor::isFxModuleEnabled(FxModule module) const
{
    if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(fxEnabledParameterID(module)))
        return value->load() >= 0.5f;

    return false;
}

bool NateVSTAudioProcessorEditor::shouldShowFxModule(FxModule module) const
{
    return module == FxModule::guard || module == selectedFxModule || isFxModuleEnabled(module);
}

juce::String NateVSTAudioProcessorEditor::fxEnabledParameterID(FxModule module) const
{
    switch (module)
    {
        case FxModule::tone: return Parameters::ID::fxToneEnabled;
        case FxModule::eq: return Parameters::ID::fxEqEnabled;
        case FxModule::distortion: return Parameters::ID::fxDistortionEnabled;
        case FxModule::bitcrush: return Parameters::ID::fxBitcrushEnabled;
        case FxModule::pump: return Parameters::ID::fxPumpEnabled;
        case FxModule::tremolo: return Parameters::ID::fxTremoloEnabled;
        case FxModule::ring: return Parameters::ID::fxRingEnabled;
        case FxModule::comb: return Parameters::ID::fxCombEnabled;
        case FxModule::phaser: return Parameters::ID::fxPhaserEnabled;
        case FxModule::flanger: return Parameters::ID::fxFlangerEnabled;
        case FxModule::chorus: return Parameters::ID::fxChorusEnabled;
        case FxModule::delay: return Parameters::ID::fxDelayEnabled;
        case FxModule::reverb: return Parameters::ID::fxReverbEnabled;
        case FxModule::width: return Parameters::ID::fxWidthEnabled;
        case FxModule::guard: return Parameters::ID::fxGuardEnabled;
    }

    return {};
}

juce::String NateVSTAudioProcessorEditor::fxModuleName(FxModule module) const
{
    switch (module)
    {
        case FxModule::tone: return "Tone";
        case FxModule::eq: return "EQ";
        case FxModule::distortion: return "Drive";
        case FxModule::bitcrush: return "Crush";
        case FxModule::pump: return "Pump";
        case FxModule::tremolo: return "Tremolo";
        case FxModule::ring: return "Ring Mod";
        case FxModule::comb: return "Comb";
        case FxModule::phaser: return "Phaser";
        case FxModule::flanger: return "Flanger";
        case FxModule::chorus: return "Chorus";
        case FxModule::delay: return "Delay";
        case FxModule::reverb: return "Reverb";
        case FxModule::width: return "Width";
        case FxModule::guard: return "Guard";
    }

    return {};
}

juce::String NateVSTAudioProcessorEditor::fxModuleSummary(FxModule module) const
{
    switch (module)
    {
        case FxModule::tone: return "tilt and low cut";
        case FxModule::eq: return "low mid high trim";
        case FxModule::distortion:
        {
            const auto bassSafe = readPlainParameterValue(Parameters::ID::fxDistortionBassSafe, 0.0f);
            return bassSafe > 0.05f
                ? "saturation | bass safe " + juce::String(static_cast<int>(std::round(bassSafe * 100.0f))) + "%"
                : "saturation amount";
        }
        case FxModule::bitcrush: return "bits downsample mix";
        case FxModule::pump:
        {
            const auto choices = Parameters::pumpCurveChoices();
            const auto curveIndex = juce::jlimit(0,
                                                 choices.size() - 1,
                                                 juce::roundToInt(readPlainParameterValue(Parameters::ID::fxPumpCurve, 0.0f)));
            return "sync " + choices[curveIndex] + " duck";
        }
        case FxModule::tremolo: return "sync trem pan";
        case FxModule::ring: return "metallic sidebands";
        case FxModule::comb: return "tuned resonance";
        case FxModule::phaser: return "rate depth mix";
        case FxModule::flanger: return "short delay feedback";
        case FxModule::chorus: return "rate depth mix";
        case FxModule::delay:
        {
            if (readPlainParameterValue(Parameters::ID::fxDelaySync, 0.0f) >= 0.5f)
            {
                const auto choices = Parameters::delayRateChoices();
                const auto rateIndex = juce::jlimit(0,
                                                     choices.size() - 1,
                                                     juce::roundToInt(readPlainParameterValue(Parameters::ID::fxDelayRate, 1.0f)));
                return "sync " + choices[rateIndex] + " feedback mix send";
            }

            return "time feedback mix send";
        }
        case FxModule::reverb: return "size damping mix send";
        case FxModule::width: return "mono bass width";
        case FxModule::guard:
        {
            const auto glue = readPlainParameterValue(Parameters::ID::fxGuardGlue, 0.0f);
            const auto punch = readPlainParameterValue(Parameters::ID::fxGuardPunch, 0.0f);
            auto peakLeft = 0.0f;
            auto peakRight = 0.0f;
            auto rmsLeft = 0.0f;
            auto rmsRight = 0.0f;
            auto guardDrive = 0.0f;
            auto guardReduction = 0.0f;
            auto guardActive = false;
            audioProcessor.getOutputMeterLevels(peakLeft, peakRight, rmsLeft, rmsRight);
            audioProcessor.getGuardMeterLevels(guardDrive, guardReduction, guardActive);
            juce::ignoreUnused(guardDrive);
            auto summary = guardSafetySummary(juce::jmax(peakLeft, peakRight), guardReduction, guardActive);
            if (glue > 0.01f || punch > 0.01f)
                summary = "glue " + juce::String(juce::roundToInt(glue * 100.0f))
                    + " punch " + juce::String(juce::roundToInt(punch * 100.0f));
            return summary;
        }
    }

    return {};
}

UI::FxRackRow& NateVSTAudioProcessorEditor::fxSlotButton(FxModule module)
{
    switch (module)
    {
        case FxModule::tone: return fxToneSlotButton;
        case FxModule::eq: return fxEqSlotButton;
        case FxModule::distortion: return fxDistortionSlotButton;
        case FxModule::bitcrush: return fxBitcrushSlotButton;
        case FxModule::pump: return fxPumpSlotButton;
        case FxModule::tremolo: return fxTremoloSlotButton;
        case FxModule::ring: return fxRingSlotButton;
        case FxModule::comb: return fxCombSlotButton;
        case FxModule::phaser: return fxPhaserSlotButton;
        case FxModule::flanger: return fxFlangerSlotButton;
        case FxModule::chorus: return fxChorusSlotButton;
        case FxModule::delay: return fxDelaySlotButton;
        case FxModule::reverb: return fxReverbSlotButton;
        case FxModule::width: return fxWidthSlotButton;
        case FxModule::guard: return fxGuardSlotButton;
    }

    return fxGuardSlotButton;
}

float NateVSTAudioProcessorEditor::readPlainParameterValue(const juce::String& parameterID, float fallback) const
{
    if (const auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
        return value->load();

    return fallback;
}

void NateVSTAudioProcessorEditor::setPlainParameterValue(const juce::String& parameterID, float plainValue)
{
    if (auto* parameter = audioProcessor.getValueTreeState().getParameter(parameterID))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
        parameter->endChangeGesture();
    }
}

float NateVSTAudioProcessorEditor::macroAssignmentAmountForRoute(int sourceIndex, int destinationIndex, float fallback) const
{
    for (size_t index = 0; index < Parameters::ID::modMatrixSource.size(); ++index)
    {
        const auto currentSource = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixSource[index], 0.0f));
        const auto currentDestination = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixDestination[index], 0.0f));
        const auto amount = readPlainParameterValue(Parameters::ID::modMatrixAmount[index], 0.0f);

        if (currentSource == sourceIndex && currentDestination == destinationIndex && std::abs(amount) > 0.001f)
            return amount;
    }

    return fallback;
}

juce::String NateVSTAudioProcessorEditor::formattedParameterValue(const juce::String& parameterID, double plainValue) const
{
    if (auto* parameter = audioProcessor.getValueTreeState().getParameter(parameterID))
    {
        auto text = parameter->getText(parameter->convertTo0to1(static_cast<float>(plainValue)), 64).trim();
        if (text.isNotEmpty())
            return text;
    }

    return juce::String(plainValue, 2);
}

int NateVSTAudioProcessorEditor::modulationDestinationIndexForParameter(const juce::String& parameterID) const
{
    if (parameterID == Parameters::ID::filterCutoff) return 1;
    if (parameterID == Parameters::ID::filterResonance) return 2;
    if (parameterID == Parameters::ID::filterEnvAmount) return 3;
    if (parameterID == Parameters::ID::driveAmount) return 4;
    if (parameterID == Parameters::ID::osc2Tune) return 5;
    if (parameterID == Parameters::ID::osc2Level) return 6;
    if (parameterID == Parameters::ID::fxPumpDepth) return 7;
    if (parameterID == Parameters::ID::fxDelayMix) return 8;
    if (parameterID == Parameters::ID::fxReverbMix) return 9;
    if (parameterID == Parameters::ID::fxWidthAmount) return 10;
    if (parameterID == Parameters::ID::fxDistortionAmount) return 11;
    if (parameterID == Parameters::ID::sampleStart) return 12;
    if (parameterID == Parameters::ID::sampleMix) return 13;
    if (parameterID == Parameters::ID::sampleTranspose) return 14;
    if (parameterID == Parameters::ID::samplePitchRamp) return 15;
    if (parameterID == Parameters::ID::sampleStutterRepeats) return 16;
    if (parameterID == Parameters::ID::oscWarp) return 17;
    if (parameterID == Parameters::ID::oscWavetablePosition) return 18;
    if (parameterID == Parameters::ID::osc2WavetablePosition) return 19;
    if (parameterID == Parameters::ID::fxSendDelay) return 20;
    if (parameterID == Parameters::ID::fxSendReverb) return 21;

    return 0;
}

juce::String NateVSTAudioProcessorEditor::modulationSummaryForParameter(const juce::String& parameterID) const
{
    const auto destinationIndex = modulationDestinationIndexForParameter(parameterID);
    if (destinationIndex <= 0)
        return "No MOD target";

    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    if (! juce::isPositiveAndBelow(destinationIndex, destinationChoices.size()))
        return "No MOD target";

    juce::StringArray routes;
    auto summedDepth = 0.0f;

    for (size_t index = 0; index < Parameters::ID::modMatrixDestination.size(); ++index)
    {
        const auto sourceIndex = juce::jlimit(0,
                                             sourceChoices.size() - 1,
                                             juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixSource[index], 0.0f)));
        const auto currentDestination = juce::jlimit(0,
                                                     destinationChoices.size() - 1,
                                                     juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixDestination[index], 0.0f)));
        const auto amount = readPlainParameterValue(Parameters::ID::modMatrixAmount[index], 0.0f);
        const auto enabled = readPlainParameterValue(Parameters::ID::modMatrixEnabled[index], 1.0f) >= 0.5f;

        if (! enabled || sourceIndex <= 0 || currentDestination != destinationIndex || std::abs(amount) <= 0.001f)
            continue;

        summedDepth += amount;
        const auto percent = juce::roundToInt(amount * 100.0f);
        routes.add(sourceChoices[sourceIndex] + " " + (percent >= 0 ? "+" : "") + juce::String(percent) + "%");
    }

    if (routes.isEmpty())
        return "No routes";

    const auto summedPercent = juce::roundToInt(juce::jlimit(-1.0f, 1.0f, summedDepth) * 100.0f);
    auto routeText = routes.joinIntoString(", ");
    if (routes.size() > 3)
        routeText = routes[0] + ", " + routes[1] + ", " + routes[2] + " +" + juce::String(routes.size() - 3);

    return juce::String(routes.size()) + " route" + (routes.size() == 1 ? " " : "s ")
        + routeText + " | Sum " + (summedPercent >= 0 ? "+" : "") + juce::String(summedPercent) + "%";
}

void NateVSTAudioProcessorEditor::updateSelectedControlInspector(const juce::String& labelText,
                                                                 const juce::String& parameterID,
                                                                 double plainValue)
{
    selectedControlName = labelText;
    selectedControlParameterID = parameterID;
    selectedControlPlainValue = plainValue;

    const auto valueText = formattedParameterValue(parameterID, plainValue);
    const auto routeText = modulationSummaryForParameter(parameterID);
    const auto isAutomatable = audioProcessor.getValueTreeState().getParameter(parameterID) != nullptr;
    const auto parameterText = isAutomatable ? parameterID : juce::String("local control");
    const auto summary = labelText + " " + valueText + " | " + parameterText + " | " + routeText;
    const auto hasActiveRoutes = ! routeText.startsWith("No ");

    selectedControlStatusLabel.setText(summary, juce::dontSendNotification);
    selectedControlStatusLabel.setTooltip(labelText + "\nValue: " + valueText
                                          + "\nAutomation ID: " + parameterText
                                          + "\n" + routeText);
    selectedControlStatusLabel.setColour(juce::Label::textColourId,
                                         hasActiveRoutes ? juce::Colour(0xffd9fff1) : juce::Colour(0xffa8b6b8));
    selectedControlStatusLabel.setColour(juce::Label::backgroundColourId,
                                         hasActiveRoutes ? juce::Colour(0xee10221e) : juce::Colour(0xee101619));
    updateSelectedControlActionState();
}

void NateVSTAudioProcessorEditor::updateSelectedControlActionState()
{
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto destinationIndex = modulationDestinationIndexForParameter(selectedControlParameterID);
    const auto canTarget = juce::isPositiveAndBelow(destinationIndex, destinationChoices.size());
    const auto sourceIndex = juce::jlimit(1,
                                         sourceChoices.size() - 1,
                                         modInspectorSourceBox.getSelectedId() - 1);
    const auto sourceName = juce::isPositiveAndBelow(sourceIndex, sourceChoices.size())
        ? sourceChoices[sourceIndex]
        : juce::String("selected source");
    const auto destinationName = canTarget ? destinationChoices[destinationIndex] : juce::String("selected control");

    selectedControlAddModButton.setEnabled(canTarget);
    selectedControlOpenModButton.setEnabled(canTarget);
    selectedControlAddModButton.setTooltip(canTarget
                                               ? "Add " + sourceName + " to " + destinationName + " using the next free MOD slot"
                                               : "Touch a control that is available in the MOD matrix");
    selectedControlOpenModButton.setTooltip(canTarget
                                                ? "Open MOD focused on " + destinationName
                                                : "Touch a control that is available in the MOD matrix");
}

void NateVSTAudioProcessorEditor::captureGlobalEdit(const juce::String& label)
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();
    audioProcessor.captureGlobalEditState(label);
    refreshGlobalEditControls();
}

void NateVSTAudioProcessorEditor::triggerGlobalUndo()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();
    if (! audioProcessor.undoGlobalEdit())
    {
        setRandomStatus("No edit undo");
        refreshGlobalEditControls();
        return;
    }

    clearPresetCompareState();
    refreshAfterGlobalEditRestore("Undo edit");
}

void NateVSTAudioProcessorEditor::triggerGlobalRedo()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();
    if (! audioProcessor.redoGlobalEdit())
    {
        setRandomStatus("No edit redo");
        refreshGlobalEditControls();
        return;
    }

    clearPresetCompareState();
    refreshAfterGlobalEditRestore("Redo edit");
}

void NateVSTAudioProcessorEditor::refreshGlobalEditControls()
{
    const auto canUndo = audioProcessor.canUndoGlobalEdit();
    const auto canRedo = audioProcessor.canRedoGlobalEdit();
    const auto summary = audioProcessor.getGlobalEditHistorySummary();

    undoEditButton.setEnabled(canUndo);
    redoEditButton.setEnabled(canRedo);
    undoEditButton.setTooltip(canUndo ? "Undo the last captured sound-design edit\n" + summary
                                      : "No captured sound-design edit to undo");
    redoEditButton.setTooltip(canRedo ? "Redo the last undone sound-design edit\n" + summary
                                      : "No captured sound-design edit to redo");
}

void NateVSTAudioProcessorEditor::refreshAfterGlobalEditRestore(const juce::String& statusText)
{
    updateSegmentedSelectors();
    updateLfoCurveDisplay();
    updatePumpCurveDisplay();
    updateWavetableDisplay();
    updateRandomMorphPad();
    updateHouseLayerRackDisplay();
    updateFilterResponseDisplay();
    updateModMatrixRows();
    updateModInspectorStatus();
    updateMacroAssignmentEditorStatus();
    updateModDestinationIndicators();
    updatePerformanceSnapshotButtons();
    updatePerformanceXYPad();
    updateSequencerSceneButtons();
    updateSequencerGridContext();
    updateSampleNameLabel();
    updateSampleSliceButtons();
    updateSampleSliceEditorStatus();
    updateSampleWaveformDisplay();
    updateFxRackControls();
    if (selectedControlParameterID.isNotEmpty())
        updateSelectedControlInspector(selectedControlName,
                                       selectedControlParameterID,
                                       readPlainParameterValue(selectedControlParameterID, 0.0f));
    refreshGlobalEditControls();
    setRandomStatus(statusText);
    repaint();
}

void NateVSTAudioProcessorEditor::addModRouteForSelectedControl()
{
    const auto destinationIndex = modulationDestinationIndexForParameter(selectedControlParameterID);
    if (destinationIndex <= 0)
    {
        updateSelectedControlActionState();
        return;
    }

    setModInspectorDestination(destinationIndex);
    addInspectedModRoute();

    if (selectedControlParameterID.isNotEmpty())
        updateSelectedControlInspector(selectedControlName, selectedControlParameterID, selectedControlPlainValue);
}

void NateVSTAudioProcessorEditor::focusSelectedControlModDestination()
{
    const auto destinationIndex = modulationDestinationIndexForParameter(selectedControlParameterID);
    if (destinationIndex <= 0)
    {
        updateSelectedControlActionState();
        return;
    }

    setModInspectorDestination(destinationIndex);
    activeModWorkflowPage = ModWorkflowPage::matrix;
    setActivePanel(Panel::mod);
}

int NateVSTAudioProcessorEditor::selectedMacroAssignmentSourceIndex() const
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto selectedSource = modMacroAssignSourceBox.getSelectedId() - 1;
    return juce::jlimit(firstMacroModSourceIndex,
                        juce::jmin(lastMacroModSourceIndex, sourceChoices.size() - 1),
                        selectedSource);
}

int NateVSTAudioProcessorEditor::selectedMacroAssignmentDestinationIndex() const
{
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    const auto selectedDestination = modMacroAssignDestinationBox.getSelectedId() - 1;
    return juce::jlimit(1, destinationChoices.size() - 1, selectedDestination);
}

void NateVSTAudioProcessorEditor::setActivePanel(Panel panel)
{
    activePanel = panel;
    activeFocusOverlay = FocusOverlay::none;
    updatePanelVisibility();
    resized();
    repaint();
    returnKeyboardFocusToPiano();
}

void NateVSTAudioProcessorEditor::openMacroFocusOverlay()
{
    activeFocusOverlay = FocusOverlay::macroEditor;
    updatePerformanceXYPad();
    updateMacroAssignmentEditorStatus();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::openSampleChopFocusOverlay()
{
    activeFocusOverlay = FocusOverlay::sampleChopEditor;
    updateSampleSliceButtons();
    updateSampleWaveformDisplay();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::openSourceLayerFocusOverlay()
{
    activeFocusOverlay = FocusOverlay::sourceLayerEditor;
    updateHouseLayerRackDisplay();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::openSequencerFocusOverlay()
{
    activeFocusOverlay = FocusOverlay::sequencerEditor;
    updateSequencerGridContext();
    repaintSequencerGrids();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::closeFocusOverlay()
{
    activeFocusOverlay = FocusOverlay::none;
    resized();
    repaint();
}

juce::Rectangle<int> NateVSTAudioProcessorEditor::focusOverlayBounds() const
{
    auto bounds = getLocalBounds().reduced(16);
    bounds.removeFromTop(42);
    bounds.removeFromTop(14);
    bounds.removeFromBottom(pianoKeyboardHeight);
    bounds.removeFromBottom(10);
    return bounds.reduced(10);
}

void NateVSTAudioProcessorEditor::layoutFocusOverlay()
{
    const auto showMacroEditor = activeFocusOverlay == FocusOverlay::macroEditor;
    const auto showSampleChopEditor = activeFocusOverlay == FocusOverlay::sampleChopEditor;
    const auto showSourceLayerEditor = activeFocusOverlay == FocusOverlay::sourceLayerEditor;
    const auto showSequencerEditor = activeFocusOverlay == FocusOverlay::sequencerEditor;
    const auto showOverlay = showMacroEditor || showSampleChopEditor || showSourceLayerEditor || showSequencerEditor;

    focusOverlayPanel.setVisible(showOverlay);
    focusOverlayTitleLabel.setVisible(showOverlay);
    focusOverlayCloseButton.setVisible(showOverlay);
    expandedMacroPerformanceMap.setVisible(showMacroEditor);
    expandedMacroAssignmentPad.setVisible(showMacroEditor);
    expandedSampleWaveformDisplay.setVisible(showSampleChopEditor);
    sampleChopPanel.setVisible(showSampleChopEditor);
    expandedHouseLayerRackDisplay.setVisible(showSourceLayerEditor);
    expandedSequencerGrid.setVisible(showSequencerEditor);
    sequencerSceneChainLengthButton.setVisible(showSequencerEditor);

    if (! showOverlay)
        return;

    focusOverlayPanel.setBounds(focusOverlayBounds());

    const auto overlayOrigin = focusOverlayPanel.getBounds().getPosition();
    auto panelBounds = focusOverlayPanel.getPanelBounds().translated(overlayOrigin.x, overlayOrigin.y);
    auto header = panelBounds.reduced(18, 14).removeFromTop(28);
    focusOverlayCloseButton.setBounds(header.removeFromRight(74).reduced(2, 1));
    focusOverlayTitleLabel.setText(showSampleChopEditor ? "SAMPLE CHOP FOCUS"
                                                        : showSourceLayerEditor ? "HOUSE SOURCE LAYERS"
                                                                                : showSequencerEditor ? "SEQ PATTERN EDITOR"
                                                                                                      : "EXPANDED MACRO EDITOR",
                                   juce::dontSendNotification);
    focusOverlayTitleLabel.setBounds(header.reduced(2, 1));

    auto content = focusOverlayPanel.getContentBounds().translated(overlayOrigin.x, overlayOrigin.y);
    if (showMacroEditor)
    {
        const auto macroShapeWidth = juce::jlimit(230, 330, content.getWidth() / 3);
        expandedMacroPerformanceMap.setBounds(content.removeFromLeft(macroShapeWidth).reduced(6, 4));
        expandedMacroAssignmentPad.setBounds(content.reduced(6, 4));
    }
    else if (showSampleChopEditor)
    {
        auto waveformArea = content.removeFromTop(juce::jmax(190, content.getHeight() - 116));
        expandedSampleWaveformDisplay.setBounds(waveformArea.reduced(6, 4));
        sampleChopPanel.setBounds(content.removeFromTop(sampleChopPanel.focusHeight()));
    }
    else if (showSourceLayerEditor)
    {
        expandedHouseLayerRackDisplay.setBounds(content.reduced(8, 8));
    }
    else if (showSequencerEditor)
    {
        auto chainRow = content.removeFromTop(38).withTrimmedTop(2);
        sequencerSceneChainLengthButton.setBounds(chainRow.removeFromLeft(104).reduced(4));
        expandedSequencerGrid.setBounds(content.reduced(8, 8));
    }

    focusOverlayPanel.toFront(false);
    focusOverlayTitleLabel.toFront(false);
    expandedMacroPerformanceMap.toFront(false);
    expandedMacroAssignmentPad.toFront(false);
    expandedSampleWaveformDisplay.toFront(false);
    sampleChopPanel.toFront(false);
    expandedHouseLayerRackDisplay.toFront(false);
    expandedSequencerGrid.toFront(false);
    sequencerSceneChainLengthButton.toFront(false);
    focusOverlayCloseButton.toFront(false);
}

void NateVSTAudioProcessorEditor::setActiveRandomLabPage(RandomLabPage page)
{
    activeRandomLabPage = page;
    updateRandomLabPageButtons();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::setActiveModWorkflowPage(ModWorkflowPage page)
{
    activeModWorkflowPage = page;
    updateModWorkflowButtons();
    resized();
    repaint();
    returnKeyboardFocusToPiano();
}

void NateVSTAudioProcessorEditor::updatePanelVisibility()
{
    hidePanelComponents();
    updateTabButtons();
    updateRandomLabPageButtons();
    updateModWorkflowButtons();
}

void NateVSTAudioProcessorEditor::updateTabButtons()
{
    homeTabButton.setToggleState(activePanel == Panel::home, juce::dontSendNotification);
    synthTabButton.setToggleState(activePanel == Panel::synth, juce::dontSendNotification);
    labTabButton.setToggleState(activePanel == Panel::lab, juce::dontSendNotification);
    modTabButton.setToggleState(activePanel == Panel::mod, juce::dontSendNotification);
    sampleTabButton.setToggleState(activePanel == Panel::sample, juce::dontSendNotification);
    sequencerTabButton.setToggleState(activePanel == Panel::sequencer, juce::dontSendNotification);
    effectsTabButton.setToggleState(activePanel == Panel::effects, juce::dontSendNotification);
    libraryTabButton.setToggleState(activePanel == Panel::library, juce::dontSendNotification);
    infoTabButton.setToggleState(activePanel == Panel::info, juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::updateRandomLabPageButtons()
{
    for (size_t index = 0; index < randomLabPageButtons.size(); ++index)
        randomLabPageButtons[index].setToggleState(activeRandomLabPage == static_cast<RandomLabPage>(index),
                                                   juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::updateModWorkflowButtons()
{
    const std::array<ModWorkflowPage, 4> modWorkflowPages {
        ModWorkflowPage::matrix,
        ModWorkflowPage::sources,
        ModWorkflowPage::macros,
        ModWorkflowPage::curves
    };

    for (size_t index = 0; index < modWorkflowButtons.size(); ++index)
        modWorkflowButtons[index].setToggleState(activeModWorkflowPage == modWorkflowPages[index],
                                                 juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::updateRandomRecipeInfo()
{
    auto recipeName = recipeBox.getText().trim();
    if (recipeName.isEmpty())
    {
        const auto choices = Parameters::randomRecipeChoices();
        const auto index = juce::roundToInt(readPlainParameterValue(Parameters::ID::randomRecipe, 0.0f));
        recipeName = choices.isEmpty() ? juce::String("Deep House Bass")
                                        : (juce::isPositiveAndBelow(index, choices.size()) ? choices[index] : choices[0]);
    }

    const auto text = randomRecipeInfoText(recipeName);
    randomRecipeInfoLabel.setText(text, juce::dontSendNotification);
    randomRecipeInfoLabel.setTooltip(recipeName + "\n" + text);
}

void NateVSTAudioProcessorEditor::updateInfoDetail()
{
    const auto topicId = juce::jmax(1, infoTopicBox.getSelectedId());
    const auto detailText = infoDetailTextForTopic(topicId);
    infoDetailEditor.setText(detailText, juce::dontSendNotification);
    infoDetailEditor.setTooltip(infoTopicBox.getText() + "\n" + detailText);
}

juce::String NateVSTAudioProcessorEditor::infoDetailTextForTopic(int topicId) const
{
    switch (topicId)
    {
        case 2:
            return "Random Lab is the fastest way to make usable starting points. Pick a genre recipe, choose the scope, lock sections you want preserved, then Generate or Mutate.\n\nUse section rolls when a patch is close but one area is wrong: Source for oscillator/sample character, Filter for tone, FX for space/drive, Seq for groove, and Macros for performance movement.\n\nCandidate slots let you cue, recall, promote to A/B snapshots, rate, favorite, and save without losing the current sound.";

        case 3:
            return "MOD is where motion becomes visible. Source meters show activity for LFOs, macros, random sources, velocity, and envelopes. The matrix routes a source to a destination with bipolar amount control.\n\nTouch a knob first, then use CONTROL MOD+ to add a route quickly. Use the inspector to focus a destination, clear routes, or open the MOD panel from any sound-design page.\n\nFor house and UKG, start with small macro amounts on cutoff, drive, pump, delay mix, sample start, and wavetable position.";

        case 4:
            return "FX is a slot-based rack. Add modules from the dropdown, reorder them, remove weak links, and select a slot to edit only that module.\n\nFor club patches, useful chains are Tone -> Drive -> Pump -> Delay -> Reverb -> Width -> Guard. Guard belongs near the end for output safety, glue, punch, and clip control.\n\nThrow buttons are performance moves: send delay, send space, pump drop, dry reset, and hold actions for fills and breakdowns.";

        case 5:
            return "LIBRARY stores the patch with category, pack, author, key, BPM, rating, favorite, notes, and searchable metadata.\n\nUse categories like UKG/Bass, UKG/Chops, House/Chords, Tech House/Bass, Minimal/Plucks, and Techno/Stabs so the browser behaves like a production crate.\n\nThe browser can filter by favorites, rating, generated status, category, tag, pack, tempo, and macro-rich patches. Macro previews help decide if a preset has usable performance movement before loading it.";

        case 6:
            return "UKG targets: 130-136 BPM, swung 16ths, short organ stabs, vocal chops, dred/Reese bass, clean mono subs, delay throws, tight room space, and shuffled call-response movement.\n\nHouse and tech-house targets: 120-130 BPM, rolling bass, warm filtered chords, restrained drive, pump, short plates, hook stabs, and macro-friendly cutoff/space control.\n\nMinimal and techno targets: sparse blips, darker stabs, resonant motion, noise layers, tighter envelopes, Guard glue/clip safety, and controlled width.";

        case 7:
            return "Undo Edit and Redo Edit sit in the CONTROL strip. They restore full sound-design snapshots across synth parameters, modulation routes, sequencer steps, FX rack edits, sample edits, preset loads, and performance snapshots.\n\nKnobs support normal drag, fine drag with Shift or Cmd, double-click reset, text entry, and disabled scroll-wheel movement to avoid accidental jumps.\n\nThe CONTROL strip also shows the touched parameter, value, automation ID, and modulation summary.";

        case 8:
            return "Planned high-value additions: deeper wavetable import/editing, more MSEG/function generators, per-step modulation lanes, transient sample slicing, time-stretch/warp, formant-safe vocal chops, multiband distortion, compressor/clipper metering, preset audio previews, and per-section preset save/load.\n\nThe current UI direction is to keep each major work area focused and move deeper explanations or secondary tools behind panels, dropdowns, and inspectors.";

        case 1:
        default:
            return "Start on HOME to hear and balance the patch. Use the bottom keyboard for quick auditioning, the output meter for level, and the Low End Assistant for mono/sub guidance.\n\nMove to LAB when you want a fresh idea, SYNTH or SAMPLE when the source needs detail, MOD when it needs motion, SEQ when it needs rhythm, FX when it needs club processing, and LIBRARY when it is worth saving.\n\nUse INFO any time the UI feels dense and you want a quick reminder of what each area is for.";
    }
}

void NateVSTAudioProcessorEditor::hidePanelComponents()
{
    auto hide = [] (std::initializer_list<juce::Component*> components)
    {
        for (auto* component : components)
            component->setVisible(false);
    };

    hide({
        &homeSectionLabel, &homeEngineLabel, &homeShapeLabel, &homeLabLabel, &homeLibraryLabel,
        &synthSectionLabel, &synthSourceLabel, &synthVoiceLabel, &synthFilterLabel, &synthAmpLabel,
        &randomSectionLabel, &modSectionLabel, &modSourceLabel, &modMacroLabel, &modLfoLabel, &modLfo2Label, &modEnvelopeLabel, &modMatrixLabel,
        &modMatrixStatusLabel, &modInspectorLabel, &modInspectorStatusLabel, &modMatrixSourceHeader, &modMatrixDestinationHeader, &modMatrixAmountHeader,
        &modMatrixSourceHeaderB, &modMatrixDestinationHeaderB, &modMatrixAmountHeaderB, &modMacroAssignLabel, &modMacroAssignStatusLabel, &macroAssignmentPad, &modRouteMapDisplay,
        &sampleSectionLabel, &sampleSourceLabel, &sampleChopLabel, &sampleShapeLabel, &sequencerSectionLabel,
        &hostSyncStatusLabel, &futureSectionLabel, &librarySectionLabel, &libraryFindLabel, &libraryBrowserLabel, &librarySaveLabel, &libraryInspectorLabel, &infoSectionLabel, &infoAboutLabel, &infoWorkflowLabel, &infoDetailsLabel, &infoFocusLabel, &sampleNameLabel, &presetStatusLabel, &presetBrowserHeaderLabel, &randomStatusLabel, &randomRecipeInfoLabel, &performanceStatusLabel, &focusOverlayTitleLabel, &sequencerRootValueLabel, &sequencerStepEditorLabel,
        &waveformBox, &osc2WaveBox, &wavetableToolBox, &wavetableDrawModeBox, &noiseTypeBox, &filterModeBox, &filterCharacterBox, &filterSlopeBox, &recipeBox, &randomScopeBox, &randomSectionActionBox, &randomLockActionBox, &sequencerRateBox, &sequencerGrooveBox, &sequencerScaleBox, &sequencerChordBox, &sequencerVoicingBox, &sequencerPatternBox, &sequencerGrooveTransformBox, &sequencerLaneViewBox, &sequencerLockDestinationBox, &sampleModeBox, &sampleEngineBox, &sampleSliceStyleBox, &sampleStutterRateBox, &presetBox, &presetCategoryBox,
        &presetFilterBox, &presetTagBox, &presetSortBox, &presetBrowserPackFilterBox, &presetRatingBox, &candidateRatingBox, &presetPackBox, &presetKeyBox, &presetBpmBox, &infoTopicBox, &fxAddBox, &fxPresetBox, &fxDelayRateBox, &fxPumpRateBox, &fxPumpCurveBox, &fxTremoloRateBox, &modInspectorDestinationBox, &modInspectorSourceBox, &modMacroAssignSourceBox, &modMacroAssignDestinationBox, &lfo1ShapeBox, &lfo1SyncRateBox, &lfo2ShapeBox, &lfo2SyncRateBox, &lfoCurvePresetBox, &lfoCurveActionBox,
        &monoButton, &sampleEnabledButton, &sampleReverseButton, &sampleStutterEnabledButton, &sequencerEnabledButton, &sequencerChordMemoryButton,
        &fxDistortionEnabledButton, &fxBitcrushEnabledButton, &fxPumpEnabledButton, &fxTremoloEnabledButton, &fxRingEnabledButton, &fxCombEnabledButton, &fxChorusEnabledButton, &fxDelayEnabledButton, &fxDelaySyncButton, &fxReverbEnabledButton, &fxWidthEnabledButton,
        &fxToneEnabledButton, &fxEqEnabledButton, &fxPhaserEnabledButton, &fxGuardEnabledButton,
        &fxFlangerEnabledButton,
        &randomLockPitchButton, &randomLockEnvelopeButton, &randomLockFilterButton, &randomLockSourceButton,
        &randomLockSampleButton, &randomLockFxButton, &randomLockOutputButton, &randomLockSequencerButton,
        &lfo1SyncButton, &lfo1RetriggerButton, &lfo2SyncButton, &lfo2RetriggerButton,
        &homeMacroExpandButton, &modMacroExpandButton, &sampleChopExpandButton, &sourceLayerExpandButton, &sequencerExpandButton, &focusOverlayCloseButton,
        &lfoCurveInvertButton, &lfoCurveReverseButton, &lfoCurveSmoothButton,
        &lfoCurveQuantizeButton, &lfoCurveRandomButton, &lfoCurveGarageButton,
        &generateButton, &mutateButton, &variationButton, &wildMutateButton, &undoRandomButton, &redoRandomButton,
        &recallSnapshotAButton, &captureSnapshotAButton, &recallSnapshotBButton, &captureSnapshotBButton,
        &recallSnapshotCButton, &captureSnapshotCButton, &recallSnapshotDButton, &captureSnapshotDButton,
        &loadSampleButton, &clearSampleButton, &sampleChopPanel, &sampleRecorderPanel,
        &randomCutButton, &ukgChopButton, &randomSequencerButton, &mutateSequencerButton, &undoSequencerButton, &clearSequencerButton,
        &bassPatternButton, &stabPatternButton, &ukgPatternButton, &applyPatternButton, &copySequencerButton,
        &rotateSequencerLeftButton, &rotateSequencerRightButton, &exportSequencerMidiButton, &exportSequencerChainButton, &sequencerSceneChainLiveButton, &sequencerSceneChainLengthButton, &applyGrooveTransformButton,
        &sineWaveButton, &sawWaveButton, &squareWaveButton, &triangleWaveButton, &wavetableWaveButton, &organWaveButton, &housePianoWaveButton, &customWaveButton, &waveEditorFocusButton,
        &osc2SineWaveButton, &osc2SawWaveButton, &osc2SquareWaveButton, &osc2TriangleWaveButton, &osc2WavetableWaveButton, &osc2OrganWaveButton, &osc2HousePianoWaveButton, &osc2CustomWaveButton,
        &lowpassFilterButton, &bandpassFilterButton, &highpassFilterButton,
        &rateEighthButton, &rateSixteenthButton, &rateThirtySecondButton, &sequencerRootDownButton, &sequencerRootUpButton,
        &previousPresetButton, &nextPresetButton,
        &savePresetButton, &loadPresetButton, &auditionPresetButton, &warmPresetPreviewsButton, &refreshPresetsButton, &favoritePresetButton, &comparePresetButton, &revertPresetButton, &candidateFavoriteButton,
        &saveCandidateButton,
        &promoteCandidateAButton, &promoteCandidateBButton,
        &fxMoveUpButton, &fxMoveDownButton, &fxResetOrderButton,
        &fxThrowDelayButton, &fxThrowSpaceButton, &fxThrowPumpButton, &fxThrowDryButton,
        &fxHoldDelayButton, &fxHoldSpaceButton, &fxHoldPumpButton, &fxMuteDropButton,
        &fxApplyPresetButton, &modInspectorAddButton, &modInspectorClearButton, &infoOpenLabButton, &infoOpenModButton, &infoOpenFxButton, &infoOpenLibraryButton, &modMacroAssignAddButton, &modMacroAssignReplaceButton, &modMacroAssignClearButton,
        &fxRemoveButton, &fxToneSlotButton, &fxEqSlotButton, &fxDistortionSlotButton, &fxBitcrushSlotButton, &fxPumpSlotButton, &fxTremoloSlotButton, &fxRingSlotButton, &fxCombSlotButton, &fxPhaserSlotButton, &fxFlangerSlotButton, &fxChorusSlotButton,
        &fxDelaySlotButton, &fxReverbSlotButton, &fxWidthSlotButton, &fxGuardSlotButton,
        &presetNameEditor, &presetSearchEditor, &presetAuthorEditor, &presetNotesEditor, &presetNotesTemplateBox, &randomCandidateDetailEditor, &infoAboutEditor, &infoWorkflowEditor, &infoDetailEditor, &presetBrowserList, &fxRackStatusLabel,
        &homeOverviewDisplay, &homeSignalFlowDisplay, &homeSessionDisplay, &outputOscilloscopeDisplay, &outputSpectrumDisplay, &stereoFieldDisplay, &clubMonitorDisplay, &presetCrateMapDisplay, &presetLibrarySummary, &presetSaveSummary, &randomMorphPad, &lowEndAssistant, &focusOverlayPanel, &macroPerformanceMap, &expandedMacroPerformanceMap, &macroAssignmentPad, &expandedMacroAssignmentPad, &performanceXYPad, &sampleWaveformDisplay, &expandedSampleWaveformDisplay, &wavetableDisplay, &houseLayerRackDisplay, &expandedHouseLayerRackDisplay, &filterResponseDisplay, &lfoCurveDisplay, &pumpCurveDisplay, &sequencerGrid, &expandedSequencerGrid
    });

    for (auto& slider : lfoCurveSliders)
        slider.setVisible(false);

    for (auto& button : randomLabPageButtons)
        button.setVisible(false);

    for (auto& button : randomSectionRollButtons)
        button.setVisible(false);

    for (auto& button : randomCandidateButtons)
        button.setVisible(false);

    for (auto& button : randomCandidateAuditionButtons)
        button.setVisible(false);

    for (auto& button : presetQuickFilterButtons)
        button.setVisible(false);

    for (auto& button : modWorkflowButtons)
        button.setVisible(false);

    for (auto& button : sequencerSceneRecallButtons)
        button.setVisible(false);

    for (auto& button : sequencerSceneCaptureButtons)
        button.setVisible(false);

    for (auto& button : sequencerStepEditorButtons)
        button.setVisible(false);

    for (auto& label : modSourceRows)
        label.setVisible(false);

    for (size_t index = 0; index < modSlotRows.size(); ++index)
    {
        modMatrixRows[index].setVisible(false);
        modSlotRows[index].setVisible(false);
        modSourceBoxes[index].setVisible(false);
        modDestinationBoxes[index].setVisible(false);
        modSlotEnabledButtons[index].setVisible(false);
        modSlotDuplicateButtons[index].setVisible(false);
        modSlotDeleteButtons[index].setVisible(false);
        setSliderVisible(modAmountSliders[index], modAmountLabels[index], false);
    }

    setSliderVisible(octaveSlider, octaveLabel, false);
    setSliderVisible(tuneSlider, tuneLabel, false);
    setSliderVisible(osc1LevelSlider, osc1LevelLabel, false);
    setSliderVisible(osc2OctaveSlider, osc2OctaveLabel, false);
    setSliderVisible(osc2TuneSlider, osc2TuneLabel, false);
    setSliderVisible(osc2LevelSlider, osc2LevelLabel, false);
    setSliderVisible(subLevelSlider, subLevelLabel, false);
    setSliderVisible(noiseLevelSlider, noiseLevelLabel, false);
    setSliderVisible(noiseDecaySlider, noiseDecayLabel, false);
    setSliderVisible(oscWarpSlider, oscWarpLabel, false);
    setSliderVisible(oscWavetablePositionSlider, oscWavetablePositionLabel, false);
    setSliderVisible(osc2WavetablePositionSlider, osc2WavetablePositionLabel, false);
    setSliderVisible(unisonVoicesSlider, unisonVoicesLabel, false);
    setSliderVisible(unisonDetuneSlider, unisonDetuneLabel, false);
    setSliderVisible(unisonBlendSlider, unisonBlendLabel, false);
    setSliderVisible(unisonSpreadSlider, unisonSpreadLabel, false);
    setSliderVisible(glideSlider, glideLabel, false);
    setSliderVisible(macroToneSlider, macroToneLabel, false);
    setSliderVisible(macroDirtSlider, macroDirtLabel, false);
    setSliderVisible(macroMotionSlider, macroMotionLabel, false);
    setSliderVisible(macroSpaceSlider, macroSpaceLabel, false);
    setSliderVisible(macroWeightSlider, macroWeightLabel, false);
    setSliderVisible(macroBounceSlider, macroBounceLabel, false);
    setSliderVisible(macroWarpSlider, macroWarpLabel, false);
    setSliderVisible(macroThrowSlider, macroThrowLabel, false);
    modMacroAssignAmountSlider.setVisible(false);
    setSliderVisible(lfo1RateSlider, lfo1RateLabel, false);
    setSliderVisible(lfo1DepthSlider, lfo1DepthLabel, false);
    setSliderVisible(lfo1PhaseSlider, lfo1PhaseLabel, false);
    setSliderVisible(lfo2RateSlider, lfo2RateLabel, false);
    setSliderVisible(lfo2DepthSlider, lfo2DepthLabel, false);
    setSliderVisible(lfo2PhaseSlider, lfo2PhaseLabel, false);
    setSliderVisible(modEnv1AttackSlider, modEnv1AttackLabel, false);
    setSliderVisible(modEnv1DecaySlider, modEnv1DecayLabel, false);
    setSliderVisible(modEnv1SustainSlider, modEnv1SustainLabel, false);
    setSliderVisible(modEnv1ReleaseSlider, modEnv1ReleaseLabel, false);
    setSliderVisible(modEnv1DepthSlider, modEnv1DepthLabel, false);
    setSliderVisible(attackSlider, attackLabel, false);
    setSliderVisible(decaySlider, decayLabel, false);
    setSliderVisible(sustainSlider, sustainLabel, false);
    setSliderVisible(releaseSlider, releaseLabel, false);
    setSliderVisible(cutoffSlider, cutoffLabel, false);
    setSliderVisible(resonanceSlider, resonanceLabel, false);
    setSliderVisible(filterEnvSlider, filterEnvLabel, false);
    setSliderVisible(driveSlider, driveLabel, false);
    setSliderVisible(outputSlider, outputLabel, false);
    setSliderVisible(randomAmountSlider, randomAmountLabel, false);
    setSliderVisible(randomChaosSlider, randomChaosLabel, false);
    setSliderVisible(brightnessSlider, brightnessLabel, false);
    setSliderVisible(driveBiasSlider, driveBiasLabel, false);
    setSliderVisible(motionBiasSlider, motionBiasLabel, false);
    for (size_t index = 0; index < randomSectionIntensitySliders.size(); ++index)
        setSliderVisible(randomSectionIntensitySliders[index], randomSectionIntensityLabels[index], false);
    setSliderVisible(sampleStartSlider, sampleStartLabel, false);
    setSliderVisible(sampleEndSlider, sampleEndLabel, false);
    setSliderVisible(sampleTransposeSlider, sampleTransposeLabel, false);
    setSliderVisible(samplePitchRampSlider, samplePitchRampLabel, false);
    setSliderVisible(sampleGainSlider, sampleGainLabel, false);
    setSliderVisible(sampleMixSlider, sampleMixLabel, false);
    setSliderVisible(sampleStutterRepeatsSlider, sampleStutterRepeatsLabel, false);
    setSliderVisible(sampleGrainSizeSlider, sampleGrainSizeLabel, false);
    setSliderVisible(sampleGrainSpraySlider, sampleGrainSprayLabel, false);
    setSliderVisible(sampleSpectralFreezeSlider, sampleSpectralFreezeLabel, false);
    setSliderVisible(sequencerRootSlider, sequencerRootLabel, false);
    setSliderVisible(sequencerGateSlider, sequencerGateLabel, false);
    setSliderVisible(sequencerSwingSlider, sequencerSwingLabel, false);
    setSliderVisible(sequencerChordStrumSlider, sequencerChordStrumLabel, false);
    setSliderVisible(sequencerAccentSlider, sequencerAccentLabel, false);
    setSliderVisible(sequencerOctaveSlider, sequencerOctaveLabel, false);
    setSliderVisible(sequencerProbabilitySlider, sequencerProbabilityLabel, false);
    setSliderVisible(sequencerRandomSlider, sequencerRandomLabel, false);
    setSliderVisible(sequencerLockDepthSlider, sequencerLockDepthLabel, false);
    setSliderVisible(fxDistortionAmountSlider, fxDistortionAmountLabel, false);
    setSliderVisible(fxDistortionBassSafeSlider, fxDistortionBassSafeLabel, false);
    setSliderVisible(fxBitcrushBitsSlider, fxBitcrushBitsLabel, false);
    setSliderVisible(fxBitcrushDownsampleSlider, fxBitcrushDownsampleLabel, false);
    setSliderVisible(fxBitcrushMixSlider, fxBitcrushMixLabel, false);
    setSliderVisible(fxPumpDepthSlider, fxPumpDepthLabel, false);
    setSliderVisible(fxPumpShapeSlider, fxPumpShapeLabel, false);
    setSliderVisible(fxPumpPhaseSlider, fxPumpPhaseLabel, false);
    setSliderVisible(fxTremoloDepthSlider, fxTremoloDepthLabel, false);
    setSliderVisible(fxTremoloPanSlider, fxTremoloPanLabel, false);
    setSliderVisible(fxTremoloShapeSlider, fxTremoloShapeLabel, false);
    setSliderVisible(fxTremoloPhaseSlider, fxTremoloPhaseLabel, false);
    setSliderVisible(fxRingFrequencySlider, fxRingFrequencyLabel, false);
    setSliderVisible(fxRingDepthSlider, fxRingDepthLabel, false);
    setSliderVisible(fxRingMixSlider, fxRingMixLabel, false);
    setSliderVisible(fxRingBiasSlider, fxRingBiasLabel, false);
    setSliderVisible(fxCombFrequencySlider, fxCombFrequencyLabel, false);
    setSliderVisible(fxCombFeedbackSlider, fxCombFeedbackLabel, false);
    setSliderVisible(fxCombDampingSlider, fxCombDampingLabel, false);
    setSliderVisible(fxCombMixSlider, fxCombMixLabel, false);
    setSliderVisible(fxChorusRateSlider, fxChorusRateLabel, false);
    setSliderVisible(fxChorusDepthSlider, fxChorusDepthLabel, false);
    setSliderVisible(fxChorusMixSlider, fxChorusMixLabel, false);
    setSliderVisible(fxDelayTimeSlider, fxDelayTimeLabel, false);
    setSliderVisible(fxDelayFeedbackSlider, fxDelayFeedbackLabel, false);
    setSliderVisible(fxDelayMixSlider, fxDelayMixLabel, false);
    setSliderVisible(fxSendDelaySlider, fxSendDelayLabel, false);
    setSliderVisible(fxReverbSizeSlider, fxReverbSizeLabel, false);
    setSliderVisible(fxReverbDampingSlider, fxReverbDampingLabel, false);
    setSliderVisible(fxReverbMixSlider, fxReverbMixLabel, false);
    setSliderVisible(fxSendReverbSlider, fxSendReverbLabel, false);
    setSliderVisible(fxWidthAmountSlider, fxWidthAmountLabel, false);
    setSliderVisible(fxWidthMonoCutoffSlider, fxWidthMonoCutoffLabel, false);
    setSliderVisible(fxToneTiltSlider, fxToneTiltLabel, false);
    setSliderVisible(fxToneLowCutSlider, fxToneLowCutLabel, false);
    setSliderVisible(fxEqLowGainSlider, fxEqLowGainLabel, false);
    setSliderVisible(fxEqMidGainSlider, fxEqMidGainLabel, false);
    setSliderVisible(fxEqHighGainSlider, fxEqHighGainLabel, false);
    setSliderVisible(fxEqTrimSlider, fxEqTrimLabel, false);
    setSliderVisible(fxPhaserRateSlider, fxPhaserRateLabel, false);
    setSliderVisible(fxPhaserDepthSlider, fxPhaserDepthLabel, false);
    setSliderVisible(fxPhaserMixSlider, fxPhaserMixLabel, false);
    setSliderVisible(fxFlangerRateSlider, fxFlangerRateLabel, false);
    setSliderVisible(fxFlangerDepthSlider, fxFlangerDepthLabel, false);
    setSliderVisible(fxFlangerFeedbackSlider, fxFlangerFeedbackLabel, false);
    setSliderVisible(fxFlangerMixSlider, fxFlangerMixLabel, false);
    setSliderVisible(fxGuardPushSlider, fxGuardPushLabel, false);
    setSliderVisible(fxGuardGlueSlider, fxGuardGlueLabel, false);
    setSliderVisible(fxGuardPunchSlider, fxGuardPunchLabel, false);
    setSliderVisible(fxGuardClipMixSlider, fxGuardClipMixLabel, false);
    setSliderVisible(fxGuardCeilingSlider, fxGuardCeilingLabel, false);
}

void NateVSTAudioProcessorEditor::setSliderVisible(juce::Slider& slider, juce::Label& label, bool shouldBeVisible)
{
    slider.setVisible(shouldBeVisible);
    label.setVisible(shouldBeVisible);
}

void NateVSTAudioProcessorEditor::setChoiceParameter(const juce::String& parameterID, int choiceIndex)
{
    captureGlobalEdit("Select " + parameterID);
    if (auto* parameter = audioProcessor.getValueTreeState().getParameter(parameterID))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(choiceIndex)));
        parameter->endChangeGesture();
    }

    if (parameterID == Parameters::ID::oscWave)
        waveformBox.setSelectedItemIndex(choiceIndex, juce::dontSendNotification);
    else if (parameterID == Parameters::ID::osc2Wave)
        osc2WaveBox.setSelectedItemIndex(choiceIndex, juce::dontSendNotification);
    else if (parameterID == Parameters::ID::filterMode)
        filterModeBox.setSelectedItemIndex(choiceIndex, juce::dontSendNotification);
    else if (parameterID == Parameters::ID::sequencerRate)
        sequencerRateBox.setSelectedItemIndex(choiceIndex, juce::dontSendNotification);

    updateSegmentedSelectors();
    if (parameterID == Parameters::ID::oscWave || parameterID == Parameters::ID::osc2Wave)
        updateWavetableDisplay();
    returnKeyboardFocusToPiano();
}

void NateVSTAudioProcessorEditor::updateSegmentedSelectors()
{
    auto getChoiceIndex = [this] (const juce::ComboBox& sourceBox, const juce::String& parameterID, int fallback)
    {
        const auto selectedIndex = sourceBox.getSelectedItemIndex();
        if (selectedIndex >= 0)
            return selectedIndex;

        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return static_cast<int>(value->load());

        return fallback;
    };

    const auto waveformIndex = getChoiceIndex(waveformBox, Parameters::ID::oscWave, 1);
    sineWaveButton.setToggleState(waveformIndex == 0, juce::dontSendNotification);
    sawWaveButton.setToggleState(waveformIndex == 1, juce::dontSendNotification);
    squareWaveButton.setToggleState(waveformIndex == 2, juce::dontSendNotification);
    triangleWaveButton.setToggleState(waveformIndex == 3, juce::dontSendNotification);
    wavetableWaveButton.setToggleState(waveformIndex == 4, juce::dontSendNotification);
    organWaveButton.setToggleState(waveformIndex == 5, juce::dontSendNotification);
    housePianoWaveButton.setToggleState(waveformIndex == 6, juce::dontSendNotification);
    customWaveButton.setToggleState(waveformIndex == 7, juce::dontSendNotification);

    const auto osc2WaveformIndex = getChoiceIndex(osc2WaveBox, Parameters::ID::osc2Wave, 1);
    osc2SineWaveButton.setToggleState(osc2WaveformIndex == 0, juce::dontSendNotification);
    osc2SawWaveButton.setToggleState(osc2WaveformIndex == 1, juce::dontSendNotification);
    osc2SquareWaveButton.setToggleState(osc2WaveformIndex == 2, juce::dontSendNotification);
    osc2TriangleWaveButton.setToggleState(osc2WaveformIndex == 3, juce::dontSendNotification);
    osc2WavetableWaveButton.setToggleState(osc2WaveformIndex == 4, juce::dontSendNotification);
    osc2OrganWaveButton.setToggleState(osc2WaveformIndex == 5, juce::dontSendNotification);
    osc2HousePianoWaveButton.setToggleState(osc2WaveformIndex == 6, juce::dontSendNotification);
    osc2CustomWaveButton.setToggleState(osc2WaveformIndex == 7, juce::dontSendNotification);

    const auto filterModeIndex = getChoiceIndex(filterModeBox, Parameters::ID::filterMode, 0);
    lowpassFilterButton.setToggleState(filterModeIndex == 0, juce::dontSendNotification);
    bandpassFilterButton.setToggleState(filterModeIndex == 1, juce::dontSendNotification);
    highpassFilterButton.setToggleState(filterModeIndex == 2, juce::dontSendNotification);

    const auto rateIndex = getChoiceIndex(sequencerRateBox, Parameters::ID::sequencerRate, 1);
    rateEighthButton.setToggleState(rateIndex == 0, juce::dontSendNotification);
    rateSixteenthButton.setToggleState(rateIndex == 1, juce::dontSendNotification);
    rateThirtySecondButton.setToggleState(rateIndex == 2, juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::updateLfoCurveDisplay()
{
    std::array<float, 8> values {};
    for (size_t index = 0; index < values.size(); ++index)
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(Parameters::ID::lfo1Curve[index]))
            values[index] = value->load();

    auto shapeIndex = 0;
    if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(Parameters::ID::lfo1Shape))
        shapeIndex = static_cast<int>(value->load() + 0.5f);

    lfoCurveDisplay.setValues(values, shapeIndex == 5);

    auto phase = readPlainParameterValue(Parameters::ID::lfo1Phase, 0.0f);
    const auto syncEnabled = readPlainParameterValue(Parameters::ID::lfo1Sync, 1.0f) >= 0.5f;
    const auto syncRateIndex = juce::roundToInt(readPlainParameterValue(Parameters::ID::lfo1SyncRate, 1.0f));
    const auto hostStatus = audioProcessor.getHostSyncStatus();

    if (syncEnabled && hostStatus.ppqAvailable)
    {
        phase += static_cast<float>(std::fmod(hostStatus.ppqPosition * lfoCyclesPerBeatForUi(syncRateIndex), 1.0));
    }
    else
    {
        const auto bpm = juce::jlimit(20.0, 300.0, hostStatus.bpm);
        const auto rateHz = syncEnabled
            ? static_cast<float>((bpm / 60.0) * lfoCyclesPerBeatForUi(syncRateIndex))
            : readPlainParameterValue(Parameters::ID::lfo1Rate, 1.0f);
        phase += static_cast<float>(std::fmod((juce::Time::getMillisecondCounterHiRes() * 0.001) * rateHz, 1.0));
    }

    phase = std::fmod(phase + 1.0f, 1.0f);
    lfoCurveDisplay.setPhase(phase, shapeIndex == 5);
}

void NateVSTAudioProcessorEditor::applyLfoCurvePreset(int presetId)
{
    const auto values = lfoCurvePresetValues(presetId);
    captureGlobalEdit("Load LFO curve");
    setPlainParameterValue(Parameters::ID::lfo1Shape, 5.0f);

    for (size_t index = 0; index < values.size(); ++index)
        setPlainParameterValue(Parameters::ID::lfo1Curve[index], values[index]);

    updateLfoCurveDisplay();
    updateModDestinationIndicators();
    modMatrixStatusLabel.setText("Loaded LFO curve: " + lfoCurvePresetBox.getText(),
                                 juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::applyLfoCurveTool(LfoCurveTool tool)
{
    std::array<float, 8> values {};
    for (size_t index = 0; index < values.size(); ++index)
        values[index] = readPlainParameterValue(Parameters::ID::lfo1Curve[index], 0.0f);

    auto statusText = juce::String("Edited MSEG");
    auto presetId = 1;
    captureGlobalEdit("Edit MSEG curve");

    switch (tool)
    {
        case LfoCurveTool::invert:
            for (auto& value : values)
                value = -value;
            statusText = "Inverted MSEG curve";
            break;

        case LfoCurveTool::reverse:
            std::reverse(values.begin(), values.end());
            statusText = "Reversed MSEG curve";
            break;

        case LfoCurveTool::smooth:
        {
            auto smoothed = values;
            for (size_t index = 0; index < values.size(); ++index)
            {
                const auto previous = values[(index + values.size() - 1) % values.size()];
                const auto next = values[(index + 1) % values.size()];
                smoothed[index] = juce::jlimit(-1.0f, 1.0f, (previous + (values[index] * 2.0f) + next) * 0.25f);
            }
            values = smoothed;
            statusText = "Smoothed MSEG curve";
            break;
        }

        case LfoCurveTool::quantize:
            for (auto& value : values)
                value = juce::jlimit(-1.0f, 1.0f, std::round(value * 4.0f) * 0.25f);
            statusText = "Quantized MSEG curve";
            break;

        case LfoCurveTool::randomize:
        {
            juce::Random random { static_cast<juce::int64> (juce::Time::getMillisecondCounterHiRes() * 1000.0) };
            auto previous = (random.nextFloat() * 1.6f) - 0.8f;
            for (auto& value : values)
            {
                const auto target = (random.nextFloat() * 2.0f) - 1.0f;
                previous = (previous * 0.42f) + (target * 0.58f);
                value = juce::jlimit(-1.0f, 1.0f, previous);
            }
            statusText = "Generated controlled random MSEG";
            break;
        }

        case LfoCurveTool::garage:
            values = lfoCurvePresetValues(9);
            presetId = 9;
            statusText = "Loaded UKG swing MSEG";
            break;
    }

    setPlainParameterValue(Parameters::ID::lfo1Shape, 5.0f);
    lfoCurvePresetBox.setSelectedId(presetId, juce::dontSendNotification);

    for (size_t index = 0; index < values.size(); ++index)
        setPlainParameterValue(Parameters::ID::lfo1Curve[index], values[index]);

    updateLfoCurveDisplay();
    updateModDestinationIndicators();
    updateSelectedControlInspector("MSEG P1", Parameters::ID::lfo1Curve[0], values[0]);
    modMatrixStatusLabel.setText(statusText, juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::applySelectedLfoCurveAction()
{
    const auto selectedId = lfoCurveActionBox.getSelectedId();
    if (selectedId <= 1)
        return;

    switch (selectedId)
    {
        case 2: applyLfoCurveTool(LfoCurveTool::invert); break;
        case 3: applyLfoCurveTool(LfoCurveTool::reverse); break;
        case 4: applyLfoCurveTool(LfoCurveTool::smooth); break;
        case 5: applyLfoCurveTool(LfoCurveTool::quantize); break;
        case 6: applyLfoCurveTool(LfoCurveTool::randomize); break;
        case 7: applyLfoCurveTool(LfoCurveTool::garage); break;
        default: break;
    }

    lfoCurveActionBox.setSelectedId(1, juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::updatePumpCurveDisplay()
{
    std::array<float, 8> customCurve {};
    for (size_t index = 0; index < customCurve.size(); ++index)
        customCurve[index] = readPlainParameterValue(Parameters::ID::fxPumpCustomCurve[index], 0.0f);

    const auto bounce = readPlainParameterValue(Parameters::ID::macroBounce, 0.0f);
    const auto moduleEnabled = readPlainParameterValue(Parameters::ID::fxPumpEnabled, 0.0f) >= 0.5f;

    pumpCurveDisplay.setState(
        juce::roundToInt(readPlainParameterValue(Parameters::ID::fxPumpCurve, 0.0f)),
        juce::jlimit(0.0f, 1.0f, (moduleEnabled ? readPlainParameterValue(Parameters::ID::fxPumpDepth, 0.35f) : 0.0f) + (bounce * 0.5f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::fxPumpShape, 0.45f) + (bounce * 0.16f)),
        readPlainParameterValue(Parameters::ID::fxPumpPhase, 0.0f),
        juce::roundToInt(readPlainParameterValue(Parameters::ID::fxPumpRate, 0.0f)),
        moduleEnabled || bounce > 0.001f,
        customCurve);

    auto pumpPhase = 0.0f;
    auto pumpGain = 1.0f;
    auto pumpReduction = 0.0f;
    auto pumpActive = false;
    audioProcessor.getPumpMeterLevels(pumpPhase, pumpGain, pumpReduction, pumpActive);

    const auto hostSync = audioProcessor.getHostSyncStatus();
    const auto sourceText = hostSync.ppqAvailable ? juce::String("HOST")
                                                  : juce::String("INT");
    pumpCurveDisplay.setLiveState(pumpPhase,
                                  pumpGain,
                                  pumpReduction,
                                  pumpActive && (moduleEnabled || bounce > 0.001f),
                                  sourceText);
}

void NateVSTAudioProcessorEditor::updateWavetableDisplay()
{
    if (! wavetableDisplay.isVisible())
        return;

    const auto osc1Wave = juce::roundToInt(readPlainParameterValue(Parameters::ID::oscWave, 1.0f));
    const auto osc2Wave = juce::roundToInt(readPlainParameterValue(Parameters::ID::osc2Wave, 1.0f));
    const auto osc1IsCustom = osc1Wave == 7;
    const auto osc2IsCustom = osc2Wave == 7;
    UI::WavetableDisplay::CustomPointArray osc1CustomPoints {};
    UI::WavetableDisplay::CustomPointArray osc2CustomPoints {};

    if (osc1IsCustom)
    {
        for (size_t index = 0; index < osc1CustomPoints.size(); ++index)
            osc1CustomPoints[index] = readPlainParameterValue(Parameters::ID::oscCustomWave[index], 0.5f);
    }

    if (osc2IsCustom)
    {
        for (size_t index = 0; index < osc2CustomPoints.size(); ++index)
            osc2CustomPoints[index] = readPlainParameterValue(Parameters::ID::osc2CustomWave[index], 0.5f);
    }

    auto osc1WtModAmount = 0.0f;
    auto osc2WtModAmount = 0.0f;
    auto wtRouteCount = 0;
    juce::StringArray wtSources;
    juce::StringArray sourceChoices;
    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    for (size_t index = 0; index < Parameters::ID::modMatrixDestination.size(); ++index)
    {
        const auto sourceIndex = juce::roundToInt(readParameter(Parameters::ID::modMatrixSource[index], 0.0f));
        const auto destinationIndex = juce::roundToInt(readParameter(Parameters::ID::modMatrixDestination[index], 0.0f));
        const auto amount = readParameter(Parameters::ID::modMatrixAmount[index], 0.0f);
        const auto enabled = readParameter(Parameters::ID::modMatrixEnabled[index], 1.0f) >= 0.5f;

        if (! enabled || sourceIndex <= 0)
            continue;

        if (destinationIndex == 18)
            osc1WtModAmount += amount;
        else if (destinationIndex == 19)
            osc2WtModAmount += amount;
        else
            continue;

        ++wtRouteCount;
        if (sourceChoices.isEmpty())
            sourceChoices = Parameters::modulationSourceChoices();

        if (juce::isPositiveAndBelow(sourceIndex, sourceChoices.size()))
            wtSources.addIfNotAlreadyThere(sourceChoices[sourceIndex]);
    }

    wavetableDisplay.setState(
        readPlainParameterValue(Parameters::ID::oscWavetablePosition, 0.0f),
        readPlainParameterValue(Parameters::ID::osc2WavetablePosition, 0.35f),
        osc1Wave == 4 || osc1IsCustom,
        osc2Wave == 4 || osc2IsCustom,
        juce::jlimit(-1.0f, 1.0f, osc1WtModAmount),
        juce::jlimit(-1.0f, 1.0f, osc2WtModAmount),
        wtRouteCount,
        wtSources.joinIntoString(", "),
        readPlainParameterValue(Parameters::ID::oscWarp, 0.0f),
        osc1CustomPoints,
        osc2CustomPoints,
        osc1IsCustom,
        osc2IsCustom);
}

bool NateVSTAudioProcessorEditor::wavetableTargetIsOsc2() const
{
    const auto osc1IsCustom = juce::roundToInt(readPlainParameterValue(Parameters::ID::oscWave, 1.0f)) == 7;
    const auto osc2IsCustom = juce::roundToInt(readPlainParameterValue(Parameters::ID::osc2Wave, 1.0f)) == 7;
    return osc2IsCustom && ! osc1IsCustom;
}

UI::WavetableDisplay::CustomPointArray NateVSTAudioProcessorEditor::readCustomWaveFrame(bool targetOsc2,
                                                                                        size_t frameIndex) const
{
    UI::WavetableDisplay::CustomPointArray values {};

    if (frameIndex == 0)
    {
        const auto& pointIDs = targetOsc2 ? Parameters::ID::osc2CustomWave : Parameters::ID::oscCustomWave;
        for (size_t index = 0; index < values.size(); ++index)
            values[index] = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(pointIDs[index], 0.5f));

        return values;
    }

    const auto safeFrame = juce::jlimit<size_t>(1, Parameters::customWaveMorphFrameCount - 1, frameIndex);
    for (size_t index = 0; index < values.size(); ++index)
        values[index] = juce::jlimit(0.0f,
                                     1.0f,
                                     readPlainParameterValue(Parameters::customWaveMorphFrameParameterID(targetOsc2, safeFrame, index),
                                                             0.5f));

    return values;
}

UI::WavetableDisplay::CustomPointArray NateVSTAudioProcessorEditor::readMorphedCustomWaveFrame(bool targetOsc2) const
{
    const auto position = juce::jlimit(0.0f,
                                      1.0f,
                                      readPlainParameterValue(targetOsc2 ? Parameters::ID::osc2WavetablePosition
                                                                         : Parameters::ID::oscWavetablePosition,
                                                              targetOsc2 ? 0.35f : 0.0f));
    const auto framePosition = position * static_cast<float>(Parameters::customWaveMorphFrameCount - 1);
    const auto lowerFrame = juce::jlimit<size_t>(0,
                                                 Parameters::customWaveMorphFrameCount - 1,
                                                 static_cast<size_t>(std::floor(framePosition)));
    const auto upperFrame = juce::jlimit<size_t>(0,
                                                 Parameters::customWaveMorphFrameCount - 1,
                                                 lowerFrame + 1);
    const auto mix = juce::jlimit(0.0f, 1.0f, framePosition - static_cast<float>(lowerFrame));
    const auto smoothMix = mix * mix * (3.0f - (2.0f * mix));
    const auto lower = readCustomWaveFrame(targetOsc2, lowerFrame);
    const auto upper = readCustomWaveFrame(targetOsc2, upperFrame);
    UI::WavetableDisplay::CustomPointArray values {};

    for (size_t index = 0; index < values.size(); ++index)
        values[index] = juce::jlimit(0.0f, 1.0f, lower[index] + ((upper[index] - lower[index]) * smoothMix));

    return values;
}

void NateVSTAudioProcessorEditor::writeCustomWaveFrame(bool targetOsc2,
                                                       size_t frameIndex,
                                                       const UI::WavetableDisplay::CustomPointArray& values,
                                                       const juce::String& editLabel)
{
    const auto safeFrame = juce::jlimit<size_t>(0, Parameters::customWaveMorphFrameCount - 1, frameIndex);
    const auto& pointIDs = targetOsc2 ? Parameters::ID::osc2CustomWave : Parameters::ID::oscCustomWave;
    const auto waveID = targetOsc2 ? juce::String(Parameters::ID::osc2Wave)
                                   : juce::String(Parameters::ID::oscWave);
    auto& waveBox = targetOsc2 ? osc2WaveBox : waveformBox;

    captureGlobalEdit(editLabel);
    setPlainParameterValue(waveID, 7.0f);
    waveBox.setSelectedItemIndex(7, juce::dontSendNotification);

    for (size_t index = 0; index < values.size(); ++index)
    {
        const auto value = juce::jlimit(0.0f, 1.0f, values[index]);
        if (safeFrame == 0)
            setPlainParameterValue(pointIDs[index], value);
        else
            setPlainParameterValue(Parameters::customWaveMorphFrameParameterID(targetOsc2, safeFrame, index), value);
    }

    updateSegmentedSelectors();
    updateWavetableDisplay();
    updateSelectedControlInspector(targetOsc2 ? "O2 Custom" : "O1 Custom",
                                   safeFrame == 0 ? pointIDs[0]
                                                  : Parameters::customWaveMorphFrameParameterID(targetOsc2, safeFrame, 0),
                                   values[0]);
}

void NateVSTAudioProcessorEditor::writeCustomWaveFrameSet(
    bool targetOsc2,
    const std::array<UI::WavetableDisplay::CustomPointArray, Parameters::customWaveMorphFrameCount>& frames,
    const juce::String& editLabel,
    const juce::String& statusText)
{
    const auto& pointIDs = targetOsc2 ? Parameters::ID::osc2CustomWave : Parameters::ID::oscCustomWave;
    const auto waveID = targetOsc2 ? juce::String(Parameters::ID::osc2Wave)
                                   : juce::String(Parameters::ID::oscWave);
    auto& waveBox = targetOsc2 ? osc2WaveBox : waveformBox;

    captureGlobalEdit(editLabel);
    setPlainParameterValue(waveID, 7.0f);
    waveBox.setSelectedItemIndex(7, juce::dontSendNotification);

    for (size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex)
    {
        for (size_t pointIndex = 0; pointIndex < frames[frameIndex].size(); ++pointIndex)
        {
            const auto value = juce::jlimit(0.0f, 1.0f, frames[frameIndex][pointIndex]);
            if (frameIndex == 0)
                setPlainParameterValue(pointIDs[pointIndex], value);
            else
                setPlainParameterValue(Parameters::customWaveMorphFrameParameterID(targetOsc2, frameIndex, pointIndex), value);
        }
    }

    updateSegmentedSelectors();
    updateWavetableDisplay();
    updateSelectedControlInspector(targetOsc2 ? "O2 Custom Stack" : "O1 Custom Stack",
                                   pointIDs[0],
                                   frames[0][0]);
    setRandomStatus(statusText);
    returnKeyboardFocusToPiano();
}

void NateVSTAudioProcessorEditor::importSingleCycleWave(bool targetOsc2)
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Import single-cycle WAV",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.aif;*.aiff");

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [this, targetOsc2] (const juce::FileChooser& chooser)
                             {
                                 const auto file = chooser.getResult();
                                 if (! file.existsAsFile())
                                     return;

                                 juce::AudioFormatManager manager;
                                 manager.registerBasicFormats();
                                 std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(file));
                                 if (reader == nullptr || reader->lengthInSamples <= 1)
                                 {
                                     setRandomStatus("Wave import failed");
                                     return;
                                 }

                                 const auto readSamples = juce::jlimit(2,
                                                                       262144,
                                                                       static_cast<int>(reader->lengthInSamples));
                                 juce::AudioBuffer<float> buffer(1, readSamples);
                                 buffer.clear();
                                 reader->read(&buffer, 0, readSamples, 0, true, false);

                                 UI::WavetableDisplay::CustomPointArray values {};
                                 std::array<float, UI::WavetableDisplay::customPointCount> bipolar {};
                                 auto mean = 0.0f;
                                 for (size_t index = 0; index < bipolar.size(); ++index)
                                 {
                                     const auto position = (static_cast<float>(index) / static_cast<float>(bipolar.size()))
                                         * static_cast<float>(readSamples - 1);
                                     const auto lowerIndex = juce::jlimit(0, readSamples - 1, static_cast<int>(std::floor(position)));
                                     const auto upperIndex = juce::jlimit(0, readSamples - 1, lowerIndex + 1);
                                     const auto mix = position - static_cast<float>(lowerIndex);
                                     const auto lower = buffer.getSample(0, lowerIndex);
                                     const auto upper = buffer.getSample(0, upperIndex);
                                     bipolar[index] = lower + ((upper - lower) * mix);
                                     mean += bipolar[index];
                                 }

                                 mean /= static_cast<float>(bipolar.size());
                                 auto peak = 0.0f;
                                 for (auto& sample : bipolar)
                                 {
                                     sample -= mean;
                                     peak = juce::jmax(peak, std::abs(sample));
                                 }

                                 if (peak <= 0.000001f)
                                 {
                                     setRandomStatus("Wave import silent");
                                     return;
                                 }

                                 for (size_t index = 0; index < values.size(); ++index)
                                     values[index] = juce::jlimit(0.0f, 1.0f, 0.5f + ((bipolar[index] / peak) * 0.5f));

                                 writeCustomWaveFrame(targetOsc2, 0, values, "Import single-cycle wave");
                                 setRandomStatus("Imported " + file.getFileNameWithoutExtension() + " to "
                                                 + juce::String(targetOsc2 ? "O2" : "O1") + " custom wave");
                                 returnKeyboardFocusToPiano();
                             });
}

void NateVSTAudioProcessorEditor::exportSingleCycleWave(bool targetOsc2)
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Export single-cycle WAV",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile("Nate VST Custom Wave.wav"),
        "*.wav");

    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode
                                 | juce::FileBrowserComponent::canSelectFiles
                                 | juce::FileBrowserComponent::warnAboutOverwriting,
                             [this, targetOsc2] (const juce::FileChooser& chooser)
                             {
                                 auto file = chooser.getResult();
                                 if (file == juce::File{})
                                     return;

                                 if (! file.hasFileExtension(".wav"))
                                     file = file.withFileExtension(".wav");

                                 constexpr auto sampleCount = 2048;
                                 auto values = readMorphedCustomWaveFrame(targetOsc2);
                                 juce::AudioBuffer<float> buffer(1, sampleCount);
                                 for (auto sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
                                 {
                                     const auto phase = static_cast<float>(sampleIndex) / static_cast<float>(sampleCount);
                                     const auto scaled = phase * static_cast<float>(values.size());
                                     const auto lowerIndex = static_cast<size_t>(std::floor(scaled)) % values.size();
                                     const auto upperIndex = (lowerIndex + 1) % values.size();
                                     const auto mix = scaled - std::floor(scaled);
                                     const auto value = values[lowerIndex] + ((values[upperIndex] - values[lowerIndex]) * mix);
                                     buffer.setSample(0, sampleIndex, (juce::jlimit(0.0f, 1.0f, value) * 2.0f) - 1.0f);
                                 }

                                 juce::WavAudioFormat format;
                                 std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
                                 if (stream == nullptr)
                                 {
                                     setRandomStatus("Wave export failed");
                                     return;
                                 }

                                 const auto writerOptions = juce::AudioFormatWriterOptions{}
                                     .withSampleRate(44100.0)
                                     .withNumChannels(1)
                                     .withBitsPerSample(24);
                                 auto writer = format.createWriterFor(stream, writerOptions);
                                 if (writer == nullptr)
                                 {
                                     setRandomStatus("Wave export failed");
                                     return;
                                 }

                                 const auto ok = writer->writeFromAudioSampleBuffer(buffer, 0, sampleCount);
                                 setRandomStatus(ok ? "Exported custom wave WAV" : "Wave export failed");
                                 returnKeyboardFocusToPiano();
                             });
}

void NateVSTAudioProcessorEditor::storeCustomWaveFrame(bool targetOsc2, size_t frameIndex)
{
    const auto values = readMorphedCustomWaveFrame(targetOsc2);
    writeCustomWaveFrame(targetOsc2, frameIndex, values, "Store custom wave frame");
    setRandomStatus("Stored " + juce::String(targetOsc2 ? "O2" : "O1")
                    + " frame " + juce::String(static_cast<int>(frameIndex + 1)));
}

void NateVSTAudioProcessorEditor::loadCustomWaveFrame(bool targetOsc2, size_t frameIndex)
{
    const auto values = readCustomWaveFrame(targetOsc2, frameIndex);
    writeCustomWaveFrame(targetOsc2, 0, values, "Load custom wave frame");
    setRandomStatus("Loaded " + juce::String(targetOsc2 ? "O2" : "O1")
                    + " frame " + juce::String(static_cast<int>(frameIndex + 1)));
}

void NateVSTAudioProcessorEditor::bakeCurrentCustomWaveMorph(bool targetOsc2)
{
    const auto values = readMorphedCustomWaveFrame(targetOsc2);
    writeCustomWaveFrame(targetOsc2, 0, values, "Bake custom wave morph");
    setRandomStatus("Baked " + juce::String(targetOsc2 ? "O2" : "O1") + " WT position into frame 1");
}

void NateVSTAudioProcessorEditor::applySelectedWavetableTool()
{
    const auto selectedId = wavetableToolBox.getSelectedId();
    if (selectedId <= 1)
        return;

    const auto osc1IsCustom = juce::roundToInt(readPlainParameterValue(Parameters::ID::oscWave, 1.0f)) == 7;
    const auto osc2IsCustom = juce::roundToInt(readPlainParameterValue(Parameters::ID::osc2Wave, 1.0f)) == 7;
    const auto targetOsc2 = osc2IsCustom && ! osc1IsCustom;
    const auto& pointIDs = targetOsc2 ? Parameters::ID::osc2CustomWave : Parameters::ID::oscCustomWave;
    const auto waveID = targetOsc2 ? juce::String(Parameters::ID::osc2Wave)
                                   : juce::String(Parameters::ID::oscWave);
    auto& waveBox = targetOsc2 ? osc2WaveBox : waveformBox;

    std::array<float, Parameters::ID::oscCustomWave.size()> values {};
    for (size_t index = 0; index < values.size(); ++index)
        values[index] = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(pointIDs[index], 0.5f));

    auto normalizeValues = [&values]
    {
        auto minValue = values.front();
        auto maxValue = values.front();
        for (const auto value : values)
        {
            minValue = juce::jmin(minValue, value);
            maxValue = juce::jmax(maxValue, value);
        }

        const auto range = maxValue - minValue;
        if (range <= 0.001f)
        {
            values.fill(0.5f);
            return;
        }

        for (auto& value : values)
            value = juce::jlimit(0.0f, 1.0f, (value - minValue) / range);
    };

    auto writeShape = [&values] (auto&& valueForPhase)
    {
        const auto lastIndex = std::max<size_t>(1, values.size() - 1);
        for (size_t index = 0; index < values.size(); ++index)
        {
            const auto phase = static_cast<float>(index) / static_cast<float>(lastIndex);
            values[index] = juce::jlimit(0.0f, 1.0f, valueForPhase(phase));
        }
    };

    auto applyValues = [&]
    {
        captureGlobalEdit("Wave tool " + wavetableToolBox.getText());
        setPlainParameterValue(waveID, 7.0f);
        waveBox.setSelectedItemIndex(7, juce::dontSendNotification);
        for (size_t index = 0; index < values.size(); ++index)
            setPlainParameterValue(pointIDs[index], juce::jlimit(0.0f, 1.0f, values[index]));

        updateSegmentedSelectors();
        updateWavetableDisplay();
        updateSelectedControlInspector(targetOsc2 ? "O2 Custom" : "O1 Custom",
                                       pointIDs[0],
                                       values[0]);
        setRandomStatus("Wave tool: " + wavetableToolBox.getText());
        returnKeyboardFocusToPiano();
    };

    auto changed = true;
    switch (selectedId)
    {
        case 2:
            for (size_t index = 0; index < values.size() / 2; ++index)
                values[values.size() - 1 - index] = values[index];
            break;

        case 3:
            normalizeValues();
            break;

        case 4:
        {
            const auto source = values;
            for (size_t index = 0; index < values.size(); ++index)
            {
                const auto left = source[index == 0 ? 0 : index - 1];
                const auto right = source[index + 1 >= source.size() ? source.size() - 1 : index + 1];
                values[index] = (left * 0.25f) + (source[index] * 0.50f) + (right * 0.25f);
            }
            break;
        }

        case 5:
        {
            const auto source = values;
            for (size_t index = 0; index < values.size(); ++index)
            {
                const auto left = source[index == 0 ? 0 : index - 1];
                const auto right = source[index + 1 >= source.size() ? source.size() - 1 : index + 1];
                const auto neighbourAverage = (left + right) * 0.5f;
                values[index] = juce::jlimit(0.0f, 1.0f, source[index] + ((source[index] - neighbourAverage) * 0.65f));
            }
            break;
        }

        case 6:
        {
            juce::Random random;
            auto previous = 0.0f;
            for (auto& value : values)
            {
                const auto target = (random.nextFloat() * 2.0f) - 1.0f;
                previous = (previous * 0.45f) + (target * 0.55f);
                value = juce::jlimit(0.0f, 1.0f, 0.5f + (previous * 0.45f));
            }
            normalizeValues();
            break;
        }

        case 7:
            writeShape([] (float phase)
            {
                const auto wave = std::sin(juce::MathConstants<float>::twoPi * phase)
                    + (0.45f * std::sin(juce::MathConstants<float>::twoPi * phase * 2.0f + 0.35f))
                    + (0.28f * std::sin(juce::MathConstants<float>::twoPi * phase * 3.0f + 1.15f))
                    + (0.18f * std::sin(juce::MathConstants<float>::twoPi * phase * 5.0f));
                return 0.5f + (wave * 0.28f);
            });
            normalizeValues();
            break;

        case 8:
            writeShape([] (float phase) { return 0.5f + (0.5f * std::sin(juce::MathConstants<float>::twoPi * phase)); });
            break;

        case 9:
            writeShape([] (float phase) { return phase; });
            break;

        case 10:
            writeShape([] (float phase) { return phase < 0.5f ? 1.0f : 0.0f; });
            break;

        case 11:
            writeShape([] (float phase) { return phase < 0.5f ? phase * 2.0f : 2.0f - (phase * 2.0f); });
            break;

        case 12:
        {
            juce::StringArray parts;
            for (const auto value : values)
                parts.add(juce::String(value, 4));
            juce::SystemClipboard::copyTextToClipboard(parts.joinIntoString(","));
            setRandomStatus("Copied custom wave points");
            changed = false;
            break;
        }

        case 13:
        {
            const auto clipboardText = juce::SystemClipboard::getTextFromClipboard();
            juce::StringArray tokens;
            tokens.addTokens(clipboardText, ",; \n\r\t", "\"'");
            tokens.removeEmptyStrings();
            if (tokens.size() < static_cast<int>(values.size()))
            {
                setRandomStatus("Paste needs 16 wave values");
                changed = false;
                break;
            }

            auto hasNegative = false;
            auto looksPercent = false;
            for (size_t index = 0; index < values.size(); ++index)
            {
                const auto raw = tokens[static_cast<int>(index)].getFloatValue();
                hasNegative = hasNegative || raw < 0.0f;
                looksPercent = looksPercent || raw > 1.0f;
                values[index] = raw;
            }

            for (auto& value : values)
            {
                if (hasNegative)
                    value = (value + 1.0f) * 0.5f;
                else if (looksPercent)
                    value *= 0.01f;
                value = juce::jlimit(0.0f, 1.0f, value);
            }
            break;
        }

        case 14:
            for (auto& value : values)
                value = 1.0f - value;
            break;

        case 15:
            std::reverse(values.begin(), values.end());
            break;

        case 16:
            std::rotate(values.begin(), values.begin() + 1, values.end());
            break;

        case 17:
            std::rotate(values.rbegin(), values.rbegin() + 1, values.rend());
            break;

        case 18:
            for (auto& value : values)
                value = std::round(juce::jlimit(0.0f, 1.0f, value) * 4.0f) * 0.25f;
            break;

        case 19:
            for (auto& value : values)
            {
                const auto bipolar = (value * 2.0f) - 1.0f;
                value = juce::jlimit(0.0f, 1.0f, std::abs(bipolar));
            }
            break;

        case 20:
        {
            auto mean = 0.0f;
            for (const auto value : values)
                mean += value;
            mean /= static_cast<float>(values.size());
            for (auto& value : values)
                value = juce::jlimit(0.0f, 1.0f, value - mean + 0.5f);
            break;
        }

        case 21:
            writeShape([] (float phase)
            {
                auto wave = 0.0f;
                for (auto harmonic = 1; harmonic <= 9; ++harmonic)
                {
                    const auto weight = harmonic == 1 ? 0.72f : 0.52f / std::sqrt(static_cast<float>(harmonic));
                    wave += std::sin(juce::MathConstants<float>::twoPi * phase * static_cast<float>(harmonic)
                                     + (static_cast<float>(harmonic % 3) * 0.21f)) * weight;
                }
                return 0.5f + (wave * 0.18f);
            });
            normalizeValues();
            break;

        case 22:
            for (auto& value : values)
            {
                const auto bipolar = ((value * 2.0f) - 1.0f) * 1.85f;
                value = 0.5f + (std::tanh(bipolar) * 0.5f);
            }
            break;

        case 23:
        {
            const auto lastIndex = std::max<size_t>(1, values.size() - 1);
            for (size_t index = 0; index < values.size(); ++index)
            {
                const auto phase = static_cast<float>(index) / static_cast<float>(lastIndex);
                const auto window = std::sin(juce::MathConstants<float>::pi * phase);
                values[index] = 0.5f + ((values[index] - 0.5f) * window);
            }
            break;
        }

        case 24:
            importSingleCycleWave(targetOsc2);
            changed = false;
            break;

        case 25:
            exportSingleCycleWave(targetOsc2);
            changed = false;
            break;

        case 26:
            writeShape([] (float phase)
            {
                auto wave = 0.0f;
                for (auto harmonic = 1; harmonic <= 15; harmonic += 2)
                    wave += std::sin(juce::MathConstants<float>::twoPi * phase * static_cast<float>(harmonic))
                        / static_cast<float>(harmonic);
                return 0.5f + (wave * 0.38f);
            });
            normalizeValues();
            break;

        case 27:
            writeShape([] (float phase)
            {
                auto wave = 0.0f;
                for (auto harmonic = 2; harmonic <= 16; harmonic += 2)
                    wave += std::sin(juce::MathConstants<float>::twoPi * phase * static_cast<float>(harmonic))
                        / static_cast<float>(harmonic);
                return 0.5f + (wave * 0.46f);
            });
            normalizeValues();
            break;

        case 28:
            writeShape([] (float phase)
            {
                auto wave = 0.0f;
                for (auto harmonic = 1; harmonic <= 16; ++harmonic)
                {
                    const auto h = static_cast<float>(harmonic);
                    const auto tilt = std::pow(h / 16.0f, 0.35f);
                    wave += std::sin(juce::MathConstants<float>::twoPi * phase * h) * tilt / std::sqrt(h);
                }
                return 0.5f + (wave * 0.20f);
            });
            normalizeValues();
            break;

        case 29:
            writeShape([] (float phase)
            {
                auto wave = 0.0f;
                for (auto harmonic = 1; harmonic <= 16; ++harmonic)
                {
                    const auto h = static_cast<float>(harmonic);
                    wave += std::sin(juce::MathConstants<float>::twoPi * phase * h) / std::pow(h, 1.38f);
                }
                return 0.5f + (wave * 0.36f);
            });
            normalizeValues();
            break;

        case 30:
            writeShape([] (float phase)
            {
                auto wave = 0.0f;
                for (auto harmonic = 2; harmonic <= 12; harmonic += 2)
                {
                    const auto h = static_cast<float>(harmonic);
                    wave += std::sin(juce::MathConstants<float>::twoPi * phase * h + (0.08f * h))
                        / std::sqrt(h);
                }
                return 0.5f + (wave * 0.28f);
            });
            normalizeValues();
            break;

        case 31:
            {
                auto& random = juce::Random::getSystemRandom();
                std::array<float, 16> amplitudes {};
                std::array<float, 16> offsets {};
                for (size_t harmonic = 0; harmonic < amplitudes.size(); ++harmonic)
                {
                    amplitudes[harmonic] = random.nextFloat() * (harmonic < 3 ? 0.72f : 0.42f)
                        / std::sqrt(static_cast<float>(harmonic + 1));
                    offsets[harmonic] = random.nextFloat();
                }

                writeShape([amplitudes, offsets] (float phase)
                {
                    auto wave = 0.0f;
                    for (size_t harmonic = 0; harmonic < amplitudes.size(); ++harmonic)
                    {
                        const auto h = static_cast<float>(harmonic + 1);
                        wave += std::sin(juce::MathConstants<float>::twoPi * ((phase * h) + offsets[harmonic]))
                            * amplitudes[harmonic];
                    }
                    return 0.5f + (wave * 0.32f);
                });
                normalizeValues();
            }
            break;

        case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39:
            storeCustomWaveFrame(targetOsc2, static_cast<size_t>(selectedId - 32));
            changed = false;
            break;

        case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47:
            loadCustomWaveFrame(targetOsc2, static_cast<size_t>(selectedId - 40));
            changed = false;
            break;

        case 48:
            bakeCurrentCustomWaveMorph(targetOsc2);
            changed = false;
            break;

        case 49:
            writeCustomWaveFrameSet(targetOsc2,
                                    Synth::WavetableFrameRecipes::currentSweep(values),
                                    "Generate current wave sweep",
                                    "Generated " + juce::String(targetOsc2 ? "O2" : "O1") + " current-wave frame sweep");
            changed = false;
            break;

        case 50:
            writeCustomWaveFrameSet(targetOsc2,
                                    Synth::WavetableFrameRecipes::classicHouseStack(),
                                    "Generate classic house wave stack",
                                    "Generated " + juce::String(targetOsc2 ? "O2" : "O1") + " classic house frame stack");
            changed = false;
            break;

        case 51:
            writeCustomWaveFrameSet(targetOsc2,
                                    Synth::WavetableFrameRecipes::raveSweep(),
                                    "Generate rave wave sweep",
                                    "Generated " + juce::String(targetOsc2 ? "O2" : "O1") + " rave frame sweep");
            changed = false;
            break;

        default:
            changed = false;
            break;
    }

    if (changed)
        applyValues();

    wavetableToolBox.setSelectedId(1, juce::dontSendNotification);
    returnKeyboardFocusToPiano();
}

void NateVSTAudioProcessorEditor::updateHouseLayerRackDisplay()
{
    const auto osc1Wave = juce::roundToInt(readPlainParameterValue(Parameters::ID::oscWave, 1.0f));
    const auto osc2Wave = juce::roundToInt(readPlainParameterValue(Parameters::ID::osc2Wave, 1.0f));
    const auto osc1 = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::osc1Level, 1.0f));
    const auto osc2 = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::osc2Level, 0.0f));
    const auto macroWeight = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::macroWeight, 0.0f));
    const auto sub = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::subLevel, 0.0f) + (macroWeight * 0.32f));
    const auto noise = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::noiseLevel, 0.0f));
    const auto noiseChoices = Parameters::noiseTypeChoices();
    const auto noiseIndex = juce::jlimit(0, noiseChoices.size() - 1, juce::roundToInt(readPlainParameterValue(Parameters::ID::noiseType, 0.0f)));
    const auto noiseSource = noiseChoices[noiseIndex];
    const auto sampleName = audioProcessor.getLoadedSampleName();
    const auto sampleEnabled = readPlainParameterValue(Parameters::ID::sampleEnabled, 0.0f) >= 0.5f;
    const auto sampleGain = juce::Decibels::decibelsToGain(readPlainParameterValue(Parameters::ID::sampleGain, -6.0f));
    const auto sampleLoaded = audioProcessor.hasLoadedSample();
    const auto sampleMissing = audioProcessor.hasMissingSampleReference();
    const auto sampleMix = sampleEnabled && sampleLoaded
        ? juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::sampleMix, 0.0f) * sampleGain)
        : 0.0f;
    const auto playbackMode = juce::jlimit(0, 2, juce::roundToInt(readPlainParameterValue(Parameters::ID::samplePlaybackMode, 0.0f)));
    const auto sampleModeName = playbackMode == 2 ? juce::String("Slice Keys")
                              : playbackMode == 1 ? juce::String("One Shot")
                                                  : juce::String("Gate");
    const auto sliceChoices = Parameters::sampleSliceStyleChoices();
    const auto sliceIndex = juce::jlimit(0, sliceChoices.size() - 1, juce::roundToInt(readPlainParameterValue(Parameters::ID::sampleSliceStyle, 0.0f)));
    const auto activeThreshold = 0.04f;

    UI::HouseLayerRackDisplay::State state;
    state.layers = {
        UI::HouseLayerRackDisplay::Layer {
            "SUB",
            macroWeight > activeThreshold ? "Mono +W" : "Mono",
            juce::String("Bass-safe low layer") + (macroWeight > activeThreshold ? juce::String(" with Weight macro") : juce::String()),
            sub,
            sub > activeThreshold,
            true,
            false
        },
        UI::HouseLayerRackDisplay::Layer {
            bodyRoleForWave(osc1Wave),
            sourceNameForWave(osc1Wave),
            "Main source body",
            osc1,
            osc1 > activeThreshold,
            false,
            osc1Wave == 4
        },
        UI::HouseLayerRackDisplay::Layer {
            characterRoleForWave(osc2Wave),
            sourceNameForWave(osc2Wave),
            "Upper color or stab layer",
            osc2,
            osc2 > activeThreshold,
            false,
            osc2Wave == 4
        },
        UI::HouseLayerRackDisplay::Layer {
            "NOISE",
            noiseSource,
            noiseSource + " transient and dust layer",
            noise,
            noise > activeThreshold,
            false,
            true
        },
        UI::HouseLayerRackDisplay::Layer {
            "CHOP",
            sampleLoaded ? sampleModeName : (sampleMissing ? juce::String("Missing") : juce::String("Empty")),
            sampleLoaded ? (sampleModeName + " | " + sliceChoices[sliceIndex] + " slice style")
                         : (sampleMissing ? juce::String("Relink missing sample before chop playback")
                                          : juce::String("Load a sample to enable chop layer")),
            sampleMix,
            sampleMix > activeThreshold,
            false,
            true
        }
    };

    const auto activeLayers = std::count_if(state.layers.begin(),
                                            state.layers.end(),
                                            [activeThreshold] (const UI::HouseLayerRackDisplay::Layer& layer)
                                            {
                                                return layer.active && layer.level > activeThreshold;
                                            });
    const auto hasHouseSource = osc1Wave == 5 || osc1Wave == 6 || osc2Wave == 5 || osc2Wave == 6;
    state.summary = hasHouseSource ? "House source color active"
                  : activeLayers >= 3 ? "Layered house source"
                  : sampleMissing ? "Sample missing - relink needed"
                  : sampleLoaded && sampleMix > activeThreshold ? "Sample-led house layer"
                  : "House layer rack";

    expandedHouseLayerRackDisplay.setState(state);
    houseLayerRackDisplay.setState(std::move(state));
}

void NateVSTAudioProcessorEditor::focusHouseLayer(size_t layerIndex)
{
    struct LayerTarget
    {
        const char* label = "";
        const char* parameterID = "";
        float fallback = 0.0f;
    };

    static constexpr std::array<LayerTarget, UI::HouseLayerRackDisplay::layerCount> targets {
        LayerTarget { "Layer Sub", Parameters::ID::subLevel, 0.0f },
        LayerTarget { "Layer Body", Parameters::ID::osc1Level, 1.0f },
        LayerTarget { "Layer Character", Parameters::ID::osc2Level, 0.0f },
        LayerTarget { "Layer Transient", Parameters::ID::noiseLevel, 0.0f },
        LayerTarget { "Layer Chop", Parameters::ID::sampleMix, 0.0f }
    };

    if (layerIndex >= targets.size())
        return;

    const auto& target = targets[layerIndex];
    const auto value = readPlainParameterValue(target.parameterID, target.fallback);
    updateSelectedControlInspector(target.label, target.parameterID, value);
    updateSelectedControlActionState();
}

void NateVSTAudioProcessorEditor::beginHouseLayerLevelEdit(size_t layerIndex)
{
    static constexpr std::array<const char*, UI::HouseLayerRackDisplay::layerCount> labels {
        "Layer Sub",
        "Layer Body",
        "Layer Character",
        "Layer Transient",
        "Layer Chop"
    };

    const auto safeIndex = juce::jlimit<size_t>(0, labels.size() - 1, layerIndex);
    captureGlobalEdit("Edit " + juce::String(labels[safeIndex]));
}

void NateVSTAudioProcessorEditor::setHouseLayerLevel(size_t layerIndex, float level)
{
    struct LayerTarget
    {
        const char* label = "";
        const char* parameterID = "";
    };

    static constexpr std::array<LayerTarget, UI::HouseLayerRackDisplay::layerCount> targets {
        LayerTarget { "Layer Sub", Parameters::ID::subLevel },
        LayerTarget { "Layer Body", Parameters::ID::osc1Level },
        LayerTarget { "Layer Character", Parameters::ID::osc2Level },
        LayerTarget { "Layer Transient", Parameters::ID::noiseLevel },
        LayerTarget { "Layer Chop", Parameters::ID::sampleMix }
    };

    if (layerIndex >= targets.size())
        return;

    level = juce::jlimit(0.0f, 1.0f, level);
    const auto& target = targets[layerIndex];

    if (layerIndex == 4 && level > 0.001f)
        setPlainParameterValue(Parameters::ID::sampleEnabled, 1.0f);

    setPlainParameterValue(target.parameterID, level);
    updateSelectedControlInspector(target.label, target.parameterID, level);
    updateHouseLayerRackDisplay();
    updateHomeOverviewDisplay();
    updateHomeSignalFlowDisplay();
}

void NateVSTAudioProcessorEditor::updateFilterResponseDisplay()
{
    UI::FilterResponseDisplay::State state;
    state.cutoffHz = readPlainParameterValue(Parameters::ID::filterCutoff, 1800.0f);
    state.resonance = readPlainParameterValue(Parameters::ID::filterResonance, 0.45f);
    state.envAmount = readPlainParameterValue(Parameters::ID::filterEnvAmount, 0.15f);
    state.drive = juce::jlimit(0.0f, 1.0f,
                               readPlainParameterValue(Parameters::ID::driveAmount, 0.18f)
                                   + (readPlainParameterValue(Parameters::ID::macroDirt, 0.0f) * 0.35f)
                                   + (readPlainParameterValue(Parameters::ID::macroWarp, 0.0f) * 0.16f));
    state.mode = juce::roundToInt(readPlainParameterValue(Parameters::ID::filterMode, 0.0f));
    state.character = juce::roundToInt(readPlainParameterValue(Parameters::ID::filterCharacter, 0.0f));
    state.slope = juce::roundToInt(readPlainParameterValue(Parameters::ID::filterSlope, 0.0f));

    const auto sourceChoices = Parameters::modulationSourceChoices();
    juce::StringArray filterSources;
    auto filterRouteCount = 0;
    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    for (size_t index = 0; index < Parameters::ID::modMatrixDestination.size(); ++index)
    {
        const auto sourceIndex = juce::roundToInt(readParameter(Parameters::ID::modMatrixSource[index], 0.0f));
        const auto destinationIndex = juce::roundToInt(readParameter(Parameters::ID::modMatrixDestination[index], 0.0f));
        const auto amount = readParameter(Parameters::ID::modMatrixAmount[index], 0.0f);
        const auto enabled = readParameter(Parameters::ID::modMatrixEnabled[index], 1.0f) >= 0.5f;

        if (! enabled || sourceIndex <= 0)
            continue;

        if (destinationIndex == 1)
            state.cutoffModAmount += amount;
        else if (destinationIndex == 2)
            state.resonanceModAmount += amount;
        else if (destinationIndex == 3)
            state.envModAmount += amount;
        else if (destinationIndex == 4)
            state.driveModAmount += amount;
        else
            continue;

        ++filterRouteCount;
        if (juce::isPositiveAndBelow(sourceIndex, sourceChoices.size()))
            filterSources.addIfNotAlreadyThere(sourceChoices[sourceIndex]);
    }

    state.cutoffModAmount = juce::jlimit(-1.0f, 1.0f, state.cutoffModAmount);
    state.resonanceModAmount = juce::jlimit(-1.0f, 1.0f, state.resonanceModAmount);
    state.envModAmount = juce::jlimit(-1.0f, 1.0f, state.envModAmount);
    state.driveModAmount = juce::jlimit(-1.0f, 1.0f, state.driveModAmount);
    state.modRouteCount = filterRouteCount;
    state.modSourceSummary = filterSources.joinIntoString(", ");

    filterResponseDisplay.setState(state);
}

void NateVSTAudioProcessorEditor::updateHostSyncStatus()
{
    const auto status = audioProcessor.getHostSyncStatus();
    const auto bpm = juce::roundToInt(juce::jlimit(20.0, 300.0, status.bpm));
    juce::String text;
    juce::String tooltip;
    auto textColour = juce::Colour(0xff7d8b90);
    auto background = juce::Colour(0x22141a1d);
    auto outline = juce::Colour(0xff263035);

    if (status.positionAvailable && status.ppqAvailable && status.playing)
    {
        text = "LOCK " + juce::String(bpm) + " | PLAY";
        tooltip = "Host BPM and PPQ are available. SEQ, Pump, Tremolo, and synced Delay can follow Ableton transport phase. PPQ "
            + juce::String(status.ppqPosition, 2);
        textColour = juce::Colour(0xff8ee6c9);
        background = juce::Colour(0x243bcfa7);
        outline = juce::Colour(0xff3bcfa7);
    }
    else if (status.positionAvailable)
    {
        text = "HOST " + juce::String(bpm) + (status.playing ? " | NO PPQ" : " | STOP");
        tooltip = status.playing
            ? "Host tempo is available, but PPQ phase is not. Tempo-synced movement uses internal phase fallback."
            : "Host tempo is available and transport is stopped. The sequencer waits; tempo-synced FX can audition from internal phase.";
        textColour = juce::Colour(0xffffc29a);
        background = juce::Colour(0x22ff8a4d);
        outline = juce::Colour(0xff705846);
    }
    else
    {
        text = "INT " + juce::String(bpm) + " | FREE";
        tooltip = "No host tempo or transport position has reached the audio engine yet. Nate VST is using its internal fallback tempo.";
    }

    hostSyncStatusLabel.setText(text, juce::dontSendNotification);
    hostSyncStatusLabel.setTooltip(tooltip);
    hostSyncStatusLabel.setColour(juce::Label::textColourId, textColour);
    hostSyncStatusLabel.setColour(juce::Label::backgroundColourId, background);
    hostSyncStatusLabel.setColour(juce::Label::outlineColourId, outline);
}

void NateVSTAudioProcessorEditor::updateModMatrixRows()
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    std::vector<int> sourceRouteCounts(static_cast<size_t>(sourceChoices.size()), 0);
    std::vector<float> sourceDepths(static_cast<size_t>(sourceChoices.size()), 0.0f);
    std::vector<UI::ModRouteMapDisplay::Route> routeMapRoutes;
    auto activeRouteCount = 0;
    juce::String firstActiveRoute;

    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    auto choiceName = [] (const juce::StringArray& choices, int index)
    {
        if (juce::isPositiveAndBelow(index, choices.size()))
            return choices[index];

        return juce::String("Off");
    };

    for (size_t index = 0; index < modMatrixRows.size(); ++index)
    {
        const auto sourceIndex = juce::jlimit(0,
                                             sourceChoices.size() - 1,
                                             static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixSource[index], 0.0f))));
        const auto destinationIndex = juce::jlimit(0,
                                                  destinationChoices.size() - 1,
                                                  static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixDestination[index], 0.0f))));
        const auto amount = readParameter(Parameters::ID::modMatrixAmount[index], 0.0f);
        const auto enabled = readParameter(Parameters::ID::modMatrixEnabled[index], 1.0f) >= 0.5f;
        const auto sourceText = choiceName(sourceChoices, sourceIndex);
        const auto destinationText = choiceName(destinationChoices, destinationIndex);
        const auto isConfiguredRoute = sourceIndex > 0 && destinationIndex > 0 && std::abs(amount) > 0.001f;
        const auto isActiveRoute = enabled && isConfiguredRoute;
        const auto shapeSummary = isConfiguredRoute ? modRouteShapeSummary(index) : juce::String {};

        modMatrixRows[index].setState(static_cast<int>(index + 1), sourceText, destinationText, amount, enabled, shapeSummary);
        if (isConfiguredRoute)
            routeMapRoutes.push_back({ sourceText, destinationText, amount, enabled });
        modSlotEnabledButtons[index].setButtonText(enabled ? "On" : "Off");
        modSlotEnabledButtons[index].setEnabled(isConfiguredRoute);
        modSlotDuplicateButtons[index].setEnabled(isConfiguredRoute);
        modSlotDeleteButtons[index].setEnabled(isConfiguredRoute);

        if (isActiveRoute)
        {
            ++activeRouteCount;
            if (juce::isPositiveAndBelow(sourceIndex, sourceChoices.size()))
            {
                sourceRouteCounts[static_cast<size_t>(sourceIndex)] += 1;
                sourceDepths[static_cast<size_t>(sourceIndex)] = juce::jlimit(-1.0f,
                                                                              1.0f,
                                                                              sourceDepths[static_cast<size_t>(sourceIndex)] + amount);
            }

            if (firstActiveRoute.isEmpty())
            {
                const auto percent = juce::roundToInt(amount * 100.0f);
                firstActiveRoute = sourceText + " -> " + destinationText
                    + " " + (percent >= 0 ? "+" : "") + juce::String(percent);
            }
        }
    }

    const auto statusText = activeRouteCount == 0
        ? juce::String("No active routes")
        : juce::String(activeRouteCount) + " active | " + firstActiveRoute;
    modMatrixStatusLabel.setText(statusText, juce::dontSendNotification);
    modRouteMapDisplay.setRoutes(std::move(routeMapRoutes));

    for (size_t rowIndex = 0; rowIndex < modSourceRows.size(); ++rowIndex)
    {
        const auto sourceIndex = rowIndex + 1;
        const auto sourceChoiceIndex = static_cast<int>(sourceIndex);
        const auto routeCount = juce::isPositiveAndBelow(sourceChoiceIndex, sourceChoices.size())
            ? sourceRouteCounts[sourceIndex]
            : 0;
        const auto depth = juce::isPositiveAndBelow(sourceChoiceIndex, sourceChoices.size())
            ? sourceDepths[sourceIndex]
            : 0.0f;
        const auto activity = modulationSourceActivityForUi(sourceChoiceIndex);
        const auto text = modSourceSummaryText(rowIndex);

        juce::String routeSummary;
        if (routeCount > 0)
        {
            const auto percent = juce::roundToInt(depth * 100.0f);
            routeSummary = juce::String(routeCount) + " route" + (routeCount == 1 ? "" : "s")
                + " " + (percent >= 0 ? "+" : "") + juce::String(percent) + "%";
        }
        else
        {
            routeSummary = "no active routes";
        }

        auto& meter = modSourceRows[rowIndex];
        meter.setState(text,
                       routeCount,
                       depth,
                       activity,
                       juce::isPositiveAndBelow(sourceChoiceIndex, sourceChoices.size())
                           ? sourceChoices[sourceChoiceIndex] + ": " + routeSummary
                                + " | activity " + juce::String(juce::roundToInt(activity * 100.0f)) + "%"
                           : juce::String("No source"));
    }
}

float NateVSTAudioProcessorEditor::modulationSourceActivityForUi(int sourceIndex) const
{
    auto lfoPhase = [this] (bool syncEnabled, int syncRateIndex, float freeRateHz, float phaseOffset)
    {
        auto phase = phaseOffset;
        const auto hostStatus = audioProcessor.getHostSyncStatus();

        if (syncEnabled && hostStatus.ppqAvailable)
        {
            phase += static_cast<float>(std::fmod(hostStatus.ppqPosition * lfoCyclesPerBeatForUi(syncRateIndex), 1.0));
        }
        else
        {
            const auto bpm = juce::jlimit(20.0, 300.0, hostStatus.bpm);
            const auto rateHz = syncEnabled
                ? static_cast<float>((bpm / 60.0) * lfoCyclesPerBeatForUi(syncRateIndex))
                : freeRateHz;
            phase += static_cast<float>(std::fmod((juce::Time::getMillisecondCounterHiRes() * 0.001) * rateHz, 1.0));
        }

        return std::fmod(phase + 1.0f, 1.0f);
    };

    std::array<float, 8> curveValues {};
    for (size_t index = 0; index < curveValues.size(); ++index)
        curveValues[index] = readPlainParameterValue(Parameters::ID::lfo1Curve[index], 0.0f);

    auto bipolarActivity = [] (float value, float scale = 1.0f)
    {
        return juce::jlimit(0.0f, 1.0f, std::abs(value) * juce::jlimit(0.0f, 1.0f, scale));
    };

    auto macroActivity = [this] (const juce::String& parameterID)
    {
        return juce::jlimit(0.0f, 1.0f, readPlainParameterValue(parameterID, 0.0f));
    };

    switch (sourceIndex)
    {
        case 1:
        {
            const auto phase = lfoPhase(readPlainParameterValue(Parameters::ID::lfo1Sync, 1.0f) >= 0.5f,
                                        juce::roundToInt(readPlainParameterValue(Parameters::ID::lfo1SyncRate, 1.0f)),
                                        readPlainParameterValue(Parameters::ID::lfo1Rate, 1.0f),
                                        readPlainParameterValue(Parameters::ID::lfo1Phase, 0.0f));
            const auto value = lfoShapeValueForUi(juce::roundToInt(readPlainParameterValue(Parameters::ID::lfo1Shape, 0.0f)),
                                                  phase,
                                                  curveValues);
            return bipolarActivity(value, readPlainParameterValue(Parameters::ID::lfo1Depth, 0.45f));
        }

        case 2:
            return juce::jlimit(0.0f,
                                1.0f,
                                readPlainParameterValue(Parameters::ID::modEnv1Depth, 0.4f)
                                    * juce::jmax(0.25f, readPlainParameterValue(Parameters::ID::modEnv1Sustain, 0.55f)));

        case 3:
            return 0.72f;

        case 4: return macroActivity(Parameters::ID::macroTone);
        case 5: return macroActivity(Parameters::ID::macroDirt);
        case 6: return macroActivity(Parameters::ID::macroMotion);
        case 7: return macroActivity(Parameters::ID::macroSpace);
        case 8: return macroActivity(Parameters::ID::macroWeight);
        case 9: return macroActivity(Parameters::ID::macroBounce);
        case 10: return macroActivity(Parameters::ID::macroWarp);
        case 11: return macroActivity(Parameters::ID::macroThrow);

        case 12:
        {
            const auto phase = lfoPhase(true, 2, 2.0f, 0.0f);
            return phase < 0.5f ? 0.82f : 0.28f;
        }

        case 13:
            return 0.5f + (0.5f * std::sin(static_cast<float>(juce::Time::getMillisecondCounterHiRes() * 0.0011)));

        case 14:
            return juce::jlimit(0.0f,
                                1.0f,
                                0.26f
                                    + (0.38f * std::abs(std::sin(static_cast<float>(juce::Time::getMillisecondCounterHiRes() * 0.0017))))
                                    + (0.24f * std::abs(std::sin(static_cast<float>(juce::Time::getMillisecondCounterHiRes() * 0.0041)))));

        case 15:
        {
            const auto phase = lfoPhase(readPlainParameterValue(Parameters::ID::lfo2Sync, 1.0f) >= 0.5f,
                                        juce::roundToInt(readPlainParameterValue(Parameters::ID::lfo2SyncRate, 3.0f)),
                                        readPlainParameterValue(Parameters::ID::lfo2Rate, 1.5f),
                                        readPlainParameterValue(Parameters::ID::lfo2Phase, 0.25f));
            const auto value = lfoShapeValueForUi(juce::roundToInt(readPlainParameterValue(Parameters::ID::lfo2Shape, 1.0f)),
                                                  phase,
                                                  curveValues);
            return bipolarActivity(value, readPlainParameterValue(Parameters::ID::lfo2Depth, 0.25f));
        }

        case 16:
        case 17:
        case 18:
            return 0.0f;

        case 19:
            return 0.5f;

        case 20:
        {
            const auto phase = lfoPhase(readPlainParameterValue(Parameters::ID::stepLfoSync, 1.0f) >= 0.5f,
                                        juce::roundToInt(readPlainParameterValue(Parameters::ID::stepLfoSyncRate, 3.0f)),
                                        readPlainParameterValue(Parameters::ID::stepLfoRate, 2.0f),
                                        0.0f);
            const auto stepIndex = juce::jlimit(0,
                                                static_cast<int>(Parameters::ID::stepLfoValue.size() - 1),
                                                static_cast<int>(std::floor(phase * static_cast<float>(Parameters::ID::stepLfoValue.size()))));
            const auto value = readPlainParameterValue(Parameters::ID::stepLfoValue[static_cast<size_t>(stepIndex)], 0.0f);
            return bipolarActivity(value, readPlainParameterValue(Parameters::ID::stepLfoDepth, 0.55f));
        }

        default:
            return 0.0f;
    }
}

void NateVSTAudioProcessorEditor::updateModInspectorStatus()
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    const auto selectedDestination = juce::jlimit(1,
                                                 destinationChoices.size() - 1,
                                                 modInspectorDestinationBox.getSelectedId() - 1);
    const auto destinationName = destinationChoices[selectedDestination];
    juce::StringArray routes;
    auto summedDepth = 0.0f;

    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    for (size_t index = 0; index < Parameters::ID::modMatrixDestination.size(); ++index)
    {
        const auto sourceIndex = juce::jlimit(0,
                                             sourceChoices.size() - 1,
                                             static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixSource[index], 0.0f))));
        const auto destinationIndex = juce::jlimit(0,
                                                  destinationChoices.size() - 1,
                                                  static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixDestination[index], 0.0f))));
        const auto amount = readParameter(Parameters::ID::modMatrixAmount[index], 0.0f);
        const auto enabled = readParameter(Parameters::ID::modMatrixEnabled[index], 1.0f) >= 0.5f;

        if (sourceIndex <= 0 || destinationIndex != selectedDestination || std::abs(amount) <= 0.001f)
            continue;

        if (enabled)
            summedDepth += amount;

        const auto percent = juce::roundToInt(amount * 100.0f);
        routes.add("S" + juce::String(static_cast<int>(index + 1)) + " "
                   + sourceChoices[sourceIndex] + " "
                   + (percent >= 0 ? "+" : "") + juce::String(percent) + "%"
                   + (enabled ? "" : " off"));
    }

    if (routes.isEmpty())
    {
        modInspectorStatusLabel.setText(destinationName + ": no active routes", juce::dontSendNotification);
        modInspectorStatusLabel.setTooltip("No modulation routes currently target " + destinationName);
        modInspectorClearButton.setEnabled(false);
        return;
    }

    const auto summedPercent = juce::roundToInt(juce::jlimit(-1.0f, 1.0f, summedDepth) * 100.0f);
    const auto summary = destinationName + " | " + routes.joinIntoString(" | ")
        + " | Sum " + (summedPercent >= 0 ? "+" : "") + juce::String(summedPercent) + "%";
    modInspectorStatusLabel.setText(summary, juce::dontSendNotification);
    modInspectorStatusLabel.setTooltip(summary);
    modInspectorClearButton.setEnabled(true);
}

void NateVSTAudioProcessorEditor::updateMacroAssignmentEditorStatus()
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    const auto sourceIndex = selectedMacroAssignmentSourceIndex();
    const auto destinationIndex = selectedMacroAssignmentDestinationIndex();
    const auto sourceName = sourceChoices[sourceIndex];
    juce::StringArray routes;
    auto selectedRouteExists = false;

    for (size_t index = 0; index < Parameters::ID::modMatrixSource.size(); ++index)
    {
        const auto currentSource = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixSource[index], 0.0f));
        const auto currentDestination = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixDestination[index], 0.0f));
        const auto amount = readPlainParameterValue(Parameters::ID::modMatrixAmount[index], 0.0f);
        const auto enabled = readPlainParameterValue(Parameters::ID::modMatrixEnabled[index], 1.0f) >= 0.5f;

        if (currentSource != sourceIndex
            || currentDestination <= 0
            || ! juce::isPositiveAndBelow(currentDestination, destinationChoices.size())
            || std::abs(amount) <= 0.001f)
        {
            continue;
        }

        if (currentDestination == destinationIndex)
            selectedRouteExists = true;

        const auto percent = juce::roundToInt(amount * 100.0f);
        routes.add(destinationChoices[currentDestination]
                   + " " + (percent >= 0 ? "+" : "") + juce::String(percent)
                   + (enabled ? "" : " off"));
    }

    const auto targetAmount = juce::roundToInt(modMacroAssignAmountSlider.getValue());
    const auto targetText = destinationChoices[destinationIndex]
        + " " + (targetAmount >= 0 ? "+" : "") + juce::String(targetAmount) + "%";
    const auto routeCountText = routes.isEmpty()
        ? juce::String("no routes")
        : juce::String(routes.size()) + (routes.size() == 1 ? " route" : " routes");
    const auto summary = sourceName + " | " + routeCountText + " | target " + targetText;
    const auto tooltip = routes.isEmpty()
        ? summary
        : sourceName + ": " + routes.joinIntoString(", ") + " | target " + targetText;

    modMacroAssignStatusLabel.setText(summary, juce::dontSendNotification);
    modMacroAssignStatusLabel.setTooltip(tooltip + " | Drag the assignment pad to add or update the selected route");
    modMacroAssignAddButton.setButtonText(selectedRouteExists ? "Update" : "Add");
    modMacroAssignClearButton.setEnabled(! routes.isEmpty());
    updateMacroAssignmentPad();
}

void NateVSTAudioProcessorEditor::updateMacroAssignmentPad()
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    const auto selectedSourceIndex = selectedMacroAssignmentSourceIndex();
    const auto selectedDestinationIndex = selectedMacroAssignmentDestinationIndex();

    UI::MacroAssignmentPad::State state;
    state.selectedSourceIndex = selectedSourceIndex;
    state.selectedDestinationIndex = selectedDestinationIndex;
    state.targetAmount = static_cast<float>(modMacroAssignAmountSlider.getValue() / 100.0);
    state.summary = modMacroAssignStatusLabel.getTooltip();

    for (size_t index = 0; index < state.macroNames.size(); ++index)
    {
        const auto sourceIndex = firstMacroModSourceIndex + static_cast<int>(index);
        state.macroSourceIndices[index] = sourceIndex;
        state.macroNames[index] = juce::isPositiveAndBelow(sourceIndex, sourceChoices.size())
            ? sourceChoices[sourceIndex]
            : ("Macro " + juce::String(static_cast<int>(index + 1)));
    }

    for (auto destinationIndex = 1; destinationIndex < destinationChoices.size(); ++destinationIndex)
    {
        UI::MacroAssignmentPad::Destination destination;
        destination.index = destinationIndex;
        destination.name = destinationChoices[destinationIndex];

        for (size_t slotIndex = 0; slotIndex < Parameters::ID::modMatrixSource.size(); ++slotIndex)
        {
            const auto currentSource = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixSource[slotIndex], 0.0f));
            const auto currentDestination = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixDestination[slotIndex], 0.0f));
            const auto amount = readPlainParameterValue(Parameters::ID::modMatrixAmount[slotIndex], 0.0f);

            if (currentSource == selectedSourceIndex
                && currentDestination == destinationIndex
                && std::abs(amount) > 0.001f)
            {
                destination.assigned = true;
                destination.amount = amount;
                destination.enabled = readPlainParameterValue(Parameters::ID::modMatrixEnabled[slotIndex], 1.0f) >= 0.5f;
                break;
            }
        }

        state.destinations.push_back(destination);
    }

    for (size_t slotIndex = 0; slotIndex < Parameters::ID::modMatrixSource.size(); ++slotIndex)
    {
        const auto currentSource = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixSource[slotIndex], 0.0f));
        const auto currentDestination = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixDestination[slotIndex], 0.0f));
        const auto amount = readPlainParameterValue(Parameters::ID::modMatrixAmount[slotIndex], 0.0f);

        if (currentDestination <= 0 || std::abs(amount) <= 0.001f)
            continue;

        const auto macroIndex = currentSource - firstMacroModSourceIndex;
        if (juce::isPositiveAndBelow(macroIndex, state.macroRouteCounts.size()))
            ++state.macroRouteCounts[static_cast<size_t>(macroIndex)];
    }

    macroAssignmentPad.setState(state);
    expandedMacroAssignmentPad.setState(state);
}

void NateVSTAudioProcessorEditor::setModInspectorDestination(int destinationIndex)
{
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    destinationIndex = juce::jlimit(1, destinationChoices.size() - 1, destinationIndex);
    modInspectorDestinationBox.setSelectedId(destinationIndex + 1, juce::dontSendNotification);
    updateModInspectorStatus();
}

void NateVSTAudioProcessorEditor::addInspectedModRoute()
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    auto sourceIndex = juce::jlimit(1,
                                    sourceChoices.size() - 1,
                                    modInspectorSourceBox.getSelectedId() - 1);
    const auto destinationIndex = juce::jlimit(1,
                                              destinationChoices.size() - 1,
                                              modInspectorDestinationBox.getSelectedId() - 1);
    if (destinationUsesGlobalModulationSources(destinationIndex) && (sourceIndex == 2 || sourceIndex == 3))
    {
        sourceIndex = 1;
        modInspectorSourceBox.setSelectedId(sourceIndex + 1, juce::dontSendNotification);
    }

    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    auto targetSlot = -1;
    for (size_t index = 0; index < Parameters::ID::modMatrixSource.size(); ++index)
    {
        const auto currentSource = static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixSource[index], 0.0f)));
        const auto currentDestination = static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixDestination[index], 0.0f)));
        const auto currentAmount = readParameter(Parameters::ID::modMatrixAmount[index], 0.0f);

        if (currentSource <= 0 || currentDestination <= 0 || std::abs(currentAmount) <= 0.001f)
        {
            targetSlot = static_cast<int>(index);
            break;
        }
    }

    if (targetSlot < 0)
    {
        modMatrixStatusLabel.setText("No free modulation slot", juce::dontSendNotification);
        return;
    }

    auto defaultAmount = 0.28f;
    if (destinationUsesGlobalModulationSources(destinationIndex))
        defaultAmount = 0.24f;
    else if (sourceIndex == 1 || sourceIndex == 2)
        defaultAmount = 0.35f;
    else if (sourceIndex == 3)
        defaultAmount = 0.20f;
    else if (sourceIndex >= 4)
        defaultAmount = 0.30f;

    const auto slotIndex = static_cast<size_t>(targetSlot);
    captureGlobalEdit("Add mod route");
    setPlainParameterValue(Parameters::ID::modMatrixSource[slotIndex], static_cast<float>(sourceIndex));
    setPlainParameterValue(Parameters::ID::modMatrixDestination[slotIndex], static_cast<float>(destinationIndex));
    setPlainParameterValue(Parameters::ID::modMatrixAmount[slotIndex], defaultAmount);
    setPlainParameterValue(Parameters::ID::modMatrixEnabled[slotIndex], 1.0f);
    resetModRouteShape(slotIndex);

    updateModMatrixRows();
    updateModDestinationIndicators();
    updateModInspectorStatus();
    updateMacroAssignmentEditorStatus();

    const auto percent = juce::roundToInt(defaultAmount * 100.0f);
    modMatrixStatusLabel.setText("Added S" + juce::String(targetSlot + 1) + " "
                                     + sourceChoices[sourceIndex] + " -> " + destinationChoices[destinationIndex]
                                     + " +" + juce::String(percent) + "%",
                                 juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::addMacroAssignment(bool replaceExisting)
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    const auto sourceIndex = selectedMacroAssignmentSourceIndex();
    const auto destinationIndex = selectedMacroAssignmentDestinationIndex();
    auto amount = static_cast<float>(modMacroAssignAmountSlider.getValue() / 100.0);

    if (std::abs(amount) < 0.001f)
        amount = 0.01f;

    if (replaceExisting)
        captureGlobalEdit("Replace macro assignment");

    auto targetSlot = -1;
    auto firstEmptySlot = -1;

    for (size_t index = 0; index < Parameters::ID::modMatrixSource.size(); ++index)
    {
        const auto currentSource = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixSource[index], 0.0f));
        const auto currentDestination = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixDestination[index], 0.0f));
        const auto currentAmount = readPlainParameterValue(Parameters::ID::modMatrixAmount[index], 0.0f);
        const auto isEmpty = currentSource <= 0 || currentDestination <= 0 || std::abs(currentAmount) <= 0.001f;

        if (! replaceExisting && currentSource == sourceIndex && currentDestination == destinationIndex)
            targetSlot = static_cast<int>(index);

        if (replaceExisting && currentSource == sourceIndex)
        {
            if (targetSlot < 0)
                targetSlot = static_cast<int>(index);

            setPlainParameterValue(Parameters::ID::modMatrixSource[index], 0.0f);
            setPlainParameterValue(Parameters::ID::modMatrixDestination[index], 0.0f);
            setPlainParameterValue(Parameters::ID::modMatrixAmount[index], 0.0f);
            setPlainParameterValue(Parameters::ID::modMatrixEnabled[index], 1.0f);
            resetModRouteShape(index);
            continue;
        }

        if (firstEmptySlot < 0 && isEmpty)
            firstEmptySlot = static_cast<int>(index);
    }

    if (targetSlot < 0)
        targetSlot = firstEmptySlot;

    if (targetSlot < 0)
    {
        modMatrixStatusLabel.setText("No free modulation slot for " + sourceChoices[sourceIndex], juce::dontSendNotification);
        updateMacroAssignmentEditorStatus();
        return;
    }

    const auto slotIndex = static_cast<size_t>(targetSlot);
    if (! replaceExisting)
        captureGlobalEdit("Edit macro assignment");
    setPlainParameterValue(Parameters::ID::modMatrixSource[slotIndex], static_cast<float>(sourceIndex));
    setPlainParameterValue(Parameters::ID::modMatrixDestination[slotIndex], static_cast<float>(destinationIndex));
    setPlainParameterValue(Parameters::ID::modMatrixAmount[slotIndex], amount);
    setPlainParameterValue(Parameters::ID::modMatrixEnabled[slotIndex], 1.0f);

    modInspectorSourceBox.setSelectedId(sourceIndex + 1, juce::dontSendNotification);
    modInspectorDestinationBox.setSelectedId(destinationIndex + 1, juce::dontSendNotification);

    updateModMatrixRows();
    updateModDestinationIndicators();
    updateModInspectorStatus();
    updateMacroAssignmentEditorStatus();

    const auto percent = juce::roundToInt(amount * 100.0f);
    modMatrixStatusLabel.setText(juce::String(replaceExisting ? "Replaced " : "Assigned ")
                                     + sourceChoices[sourceIndex] + " -> " + destinationChoices[destinationIndex]
                                     + " " + (percent >= 0 ? "+" : "") + juce::String(percent) + "%",
                                 juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::clearSelectedMacroAssignments()
{
    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto sourceIndex = selectedMacroAssignmentSourceIndex();
    auto clearedCount = 0;
    auto hasRoutesToClear = false;

    for (size_t index = 0; index < Parameters::ID::modMatrixSource.size(); ++index)
        if (juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixSource[index], 0.0f)) == sourceIndex)
            hasRoutesToClear = true;

    if (hasRoutesToClear)
        captureGlobalEdit("Clear macro assignments");

    for (size_t index = 0; index < Parameters::ID::modMatrixSource.size(); ++index)
    {
        const auto currentSource = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixSource[index], 0.0f));
        if (currentSource != sourceIndex)
            continue;

        setPlainParameterValue(Parameters::ID::modMatrixSource[index], 0.0f);
        setPlainParameterValue(Parameters::ID::modMatrixDestination[index], 0.0f);
        setPlainParameterValue(Parameters::ID::modMatrixAmount[index], 0.0f);
        setPlainParameterValue(Parameters::ID::modMatrixEnabled[index], 1.0f);
        resetModRouteShape(index);
        ++clearedCount;
    }

    updateModMatrixRows();
    updateModDestinationIndicators();
    updateModInspectorStatus();
    updateMacroAssignmentEditorStatus();

    modMatrixStatusLabel.setText(clearedCount > 0
                                     ? "Cleared " + juce::String(clearedCount) + " " + sourceChoices[sourceIndex] + " route"
                                        + (clearedCount == 1 ? "" : "s")
                                     : sourceChoices[sourceIndex] + " had no routes",
                                 juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::duplicateModRoute(size_t slotIndex)
{
    if (slotIndex >= Parameters::ID::modMatrixSource.size())
        return;

    const auto sourceChoices = Parameters::modulationSourceChoices();
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    const auto sourceIndex = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixSource[slotIndex], 0.0f));
    const auto destinationIndex = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixDestination[slotIndex], 0.0f));
    const auto amount = readPlainParameterValue(Parameters::ID::modMatrixAmount[slotIndex], 0.0f);
    const auto enabled = readPlainParameterValue(Parameters::ID::modMatrixEnabled[slotIndex], 1.0f);

    if (sourceIndex <= 0 || destinationIndex <= 0 || std::abs(amount) <= 0.001f)
    {
        modMatrixStatusLabel.setText("Slot " + juce::String(static_cast<int>(slotIndex + 1)) + " has no route to duplicate",
                                     juce::dontSendNotification);
        return;
    }

    auto targetSlot = -1;
    for (size_t index = 0; index < Parameters::ID::modMatrixSource.size(); ++index)
    {
        if (index == slotIndex)
            continue;

        const auto currentSource = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixSource[index], 0.0f));
        const auto currentDestination = juce::roundToInt(readPlainParameterValue(Parameters::ID::modMatrixDestination[index], 0.0f));
        const auto currentAmount = readPlainParameterValue(Parameters::ID::modMatrixAmount[index], 0.0f);
        if (currentSource <= 0 || currentDestination <= 0 || std::abs(currentAmount) <= 0.001f)
        {
            targetSlot = static_cast<int>(index);
            break;
        }
    }

    if (targetSlot < 0)
    {
        modMatrixStatusLabel.setText("No free modulation slot to duplicate S" + juce::String(static_cast<int>(slotIndex + 1)),
                                     juce::dontSendNotification);
        return;
    }

    const auto targetIndex = static_cast<size_t>(targetSlot);
    captureGlobalEdit("Duplicate mod route");
    setPlainParameterValue(Parameters::ID::modMatrixSource[targetIndex], static_cast<float>(sourceIndex));
    setPlainParameterValue(Parameters::ID::modMatrixDestination[targetIndex], static_cast<float>(destinationIndex));
    setPlainParameterValue(Parameters::ID::modMatrixAmount[targetIndex], amount);
    setPlainParameterValue(Parameters::ID::modMatrixEnabled[targetIndex], enabled >= 0.5f ? 1.0f : 0.0f);
    setPlainParameterValue(Parameters::ID::modMatrixPolarity[targetIndex],
                           readPlainParameterValue(Parameters::ID::modMatrixPolarity[slotIndex], 0.0f));
    setPlainParameterValue(Parameters::ID::modMatrixCurve[targetIndex],
                           readPlainParameterValue(Parameters::ID::modMatrixCurve[slotIndex], 0.0f));
    setPlainParameterValue(Parameters::ID::modMatrixRangeMin[targetIndex],
                           readPlainParameterValue(Parameters::ID::modMatrixRangeMin[slotIndex], -1.0f));
    setPlainParameterValue(Parameters::ID::modMatrixRangeMax[targetIndex],
                           readPlainParameterValue(Parameters::ID::modMatrixRangeMax[slotIndex], 1.0f));
    setPlainParameterValue(Parameters::ID::modMatrixSlew[targetIndex],
                           readPlainParameterValue(Parameters::ID::modMatrixSlew[slotIndex], 0.0f));

    updateModMatrixRows();
    updateModDestinationIndicators();
    updateModInspectorStatus();
    updateMacroAssignmentEditorStatus();

    const auto sourceName = juce::isPositiveAndBelow(sourceIndex, sourceChoices.size()) ? sourceChoices[sourceIndex] : juce::String("Source");
    const auto destinationName = juce::isPositiveAndBelow(destinationIndex, destinationChoices.size()) ? destinationChoices[destinationIndex] : juce::String("Destination");
    const auto percent = juce::roundToInt(amount * 100.0f);
    modMatrixStatusLabel.setText("Duplicated S" + juce::String(static_cast<int>(slotIndex + 1))
                                     + " to S" + juce::String(targetSlot + 1)
                                     + " | " + sourceName + " -> " + destinationName
                                     + " " + (percent >= 0 ? "+" : "") + juce::String(percent) + "%",
                                 juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::deleteModRoute(size_t slotIndex)
{
    if (slotIndex >= Parameters::ID::modMatrixSource.size())
        return;

    captureGlobalEdit("Delete mod route");
    setPlainParameterValue(Parameters::ID::modMatrixSource[slotIndex], 0.0f);
    setPlainParameterValue(Parameters::ID::modMatrixDestination[slotIndex], 0.0f);
    setPlainParameterValue(Parameters::ID::modMatrixAmount[slotIndex], 0.0f);
    setPlainParameterValue(Parameters::ID::modMatrixEnabled[slotIndex], 1.0f);
    resetModRouteShape(slotIndex);

    updateModMatrixRows();
    updateModDestinationIndicators();
    updateModInspectorStatus();
    updateMacroAssignmentEditorStatus();

    modMatrixStatusLabel.setText("Deleted S" + juce::String(static_cast<int>(slotIndex + 1)),
                                 juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::clearInspectedModRoutes()
{
    const auto destinationChoices = Parameters::modulationDestinationChoices();
    const auto selectedDestination = juce::jlimit(1,
                                                 destinationChoices.size() - 1,
                                                 modInspectorDestinationBox.getSelectedId() - 1);
    auto clearedCount = 0;

    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    auto capturedUndo = false;
    for (size_t index = 0; index < Parameters::ID::modMatrixDestination.size(); ++index)
    {
        const auto destinationIndex = static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixDestination[index], 0.0f)));
        if (destinationIndex != selectedDestination)
            continue;

        if (! capturedUndo)
        {
            captureGlobalEdit("Clear mod destination");
            capturedUndo = true;
        }

        setPlainParameterValue(Parameters::ID::modMatrixSource[index], 0.0f);
        setPlainParameterValue(Parameters::ID::modMatrixDestination[index], 0.0f);
        setPlainParameterValue(Parameters::ID::modMatrixAmount[index], 0.0f);
        setPlainParameterValue(Parameters::ID::modMatrixEnabled[index], 1.0f);
        resetModRouteShape(index);
        ++clearedCount;
    }

    updateModMatrixRows();
    updateModDestinationIndicators();
    updateModInspectorStatus();
    updateMacroAssignmentEditorStatus();

    if (clearedCount > 0)
        modMatrixStatusLabel.setText("Cleared " + juce::String(clearedCount)
                                         + " route" + (clearedCount == 1 ? "" : "s")
                                         + " to " + destinationChoices[selectedDestination],
                                     juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::updateModDestinationIndicators()
{
    std::array<float, 22> destinationDepths {};
    std::array<int, 22> destinationRouteCounts {};
    std::array<juce::StringArray, 22> destinationSources {};
    const auto sourceChoices = Parameters::modulationSourceChoices();

    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    for (size_t index = 0; index < Parameters::ID::modMatrixDestination.size(); ++index)
    {
        const auto sourceIndex = static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixSource[index], 0.0f)));
        const auto destinationIndex = static_cast<int>(std::round(readParameter(Parameters::ID::modMatrixDestination[index], 0.0f)));
        const auto amount = readParameter(Parameters::ID::modMatrixAmount[index], 0.0f);
        const auto enabled = readParameter(Parameters::ID::modMatrixEnabled[index], 1.0f) >= 0.5f;

        if (! enabled || sourceIndex <= 0 || destinationIndex <= 0 || destinationIndex >= static_cast<int>(destinationDepths.size()))
            continue;

        destinationDepths[static_cast<size_t>(destinationIndex)] += amount;
        destinationRouteCounts[static_cast<size_t>(destinationIndex)] += 1;
        if (juce::isPositiveAndBelow(sourceIndex, sourceChoices.size()))
            destinationSources[static_cast<size_t>(destinationIndex)].addIfNotAlreadyThere(sourceChoices[sourceIndex]);
    }

    auto setIndicator = [] (juce::Slider& slider, float amount, int routeCount, const juce::StringArray& sources)
    {
        amount = juce::jlimit(-1.0f, 1.0f, amount);

        auto& properties = slider.getProperties();
        const auto previous = static_cast<float>(static_cast<double>(properties.getWithDefault("modAmount", 0.0)));
        const auto previousCount = static_cast<int>(properties.getWithDefault("modRouteCount", 0));
        const auto previousSources = properties.getWithDefault("modSourceSummary", {}).toString();
        const auto sourceSummary = sources.joinIntoString(", ");
        const auto baseTooltip = properties.getWithDefault("baseTooltip", {}).toString();
        if (std::abs(previous - amount) < 0.001f
            && previousCount == routeCount
            && previousSources == sourceSummary)
            return;

        properties.set("modAmount", amount);
        properties.set("modRouteCount", routeCount);
        properties.set("modSourceSummary", sourceSummary);
        const auto modulationTooltip = routeCount > 0
            ? "Modulated by " + sourceSummary
                + " | Sum " + (amount >= 0.0f ? "+" : "") + juce::String(juce::roundToInt(amount * 100.0f)) + "%"
            : juce::String {};
        slider.setTooltip(modulationTooltip.isNotEmpty() && baseTooltip.isNotEmpty()
                              ? baseTooltip + "\n" + modulationTooltip
                              : modulationTooltip.isNotEmpty() ? modulationTooltip : baseTooltip);
        slider.repaint();
    };

    setIndicator(cutoffSlider, destinationDepths[1], destinationRouteCounts[1], destinationSources[1]);
    setIndicator(resonanceSlider, destinationDepths[2], destinationRouteCounts[2], destinationSources[2]);
    setIndicator(filterEnvSlider, destinationDepths[3], destinationRouteCounts[3], destinationSources[3]);
    setIndicator(driveSlider, destinationDepths[4], destinationRouteCounts[4], destinationSources[4]);
    setIndicator(osc2TuneSlider, destinationDepths[5], destinationRouteCounts[5], destinationSources[5]);
    setIndicator(osc2LevelSlider, destinationDepths[6], destinationRouteCounts[6], destinationSources[6]);
    setIndicator(fxPumpDepthSlider, destinationDepths[7], destinationRouteCounts[7], destinationSources[7]);
    setIndicator(fxDelayMixSlider, destinationDepths[8], destinationRouteCounts[8], destinationSources[8]);
    setIndicator(fxReverbMixSlider, destinationDepths[9], destinationRouteCounts[9], destinationSources[9]);
    setIndicator(fxWidthAmountSlider, destinationDepths[10], destinationRouteCounts[10], destinationSources[10]);
    setIndicator(fxDistortionAmountSlider, destinationDepths[11], destinationRouteCounts[11], destinationSources[11]);
    setIndicator(sampleStartSlider, destinationDepths[12], destinationRouteCounts[12], destinationSources[12]);
    setIndicator(sampleMixSlider, destinationDepths[13], destinationRouteCounts[13], destinationSources[13]);
    setIndicator(sampleTransposeSlider, destinationDepths[14], destinationRouteCounts[14], destinationSources[14]);
    setIndicator(samplePitchRampSlider, destinationDepths[15], destinationRouteCounts[15], destinationSources[15]);
    setIndicator(sampleStutterRepeatsSlider, destinationDepths[16], destinationRouteCounts[16], destinationSources[16]);
    setIndicator(oscWarpSlider, destinationDepths[17], destinationRouteCounts[17], destinationSources[17]);
    setIndicator(oscWavetablePositionSlider, destinationDepths[18], destinationRouteCounts[18], destinationSources[18]);
    setIndicator(osc2WavetablePositionSlider, destinationDepths[19], destinationRouteCounts[19], destinationSources[19]);
    setIndicator(fxSendDelaySlider, destinationDepths[20], destinationRouteCounts[20], destinationSources[20]);
    setIndicator(fxSendReverbSlider, destinationDepths[21], destinationRouteCounts[21], destinationSources[21]);

    if (selectedControlParameterID.isNotEmpty())
        updateSelectedControlInspector(selectedControlName, selectedControlParameterID, selectedControlPlainValue);
}

void NateVSTAudioProcessorEditor::updateOutputMeter()
{
    auto peakLeft = 0.0f;
    auto peakRight = 0.0f;
    auto rmsLeft = 0.0f;
    auto rmsRight = 0.0f;
    audioProcessor.getOutputMeterLevels(peakLeft, peakRight, rmsLeft, rmsRight);

    displayedPeakLeft = smoothMeterValue(displayedPeakLeft, peakLeft);
    displayedPeakRight = smoothMeterValue(displayedPeakRight, peakRight);
    displayedRmsLeft = smoothMeterValue(displayedRmsLeft, rmsLeft);
    displayedRmsRight = smoothMeterValue(displayedRmsRight, rmsRight);
    outputMeter.setLevels(displayedPeakLeft, displayedPeakRight, displayedRmsLeft, displayedRmsRight);
}

void NateVSTAudioProcessorEditor::updateOutputSpectrumDisplay()
{
    audioProcessor.getOutputSpectrumSnapshot(outputSpectrumSnapshot);

    UI::OutputSpectrumDisplay::BandArray nextBands {};
    const auto sampleRate = audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 44100.0;
    const auto peak = juce::jlimit(0.0f, 1.0f, juce::jmax(displayedPeakLeft, displayedPeakRight));
    const auto active = peak > 0.0025f;

    for (size_t index = 0; index < nextBands.size(); ++index)
    {
        const auto target = active ? goertzelBandLevel(outputSpectrumSnapshot, sampleRate, spectrumBandFrequencies[index])
                                   : 0.0f;
        const auto coefficient = target > displayedSpectrumBands[index] ? 0.54f : 0.16f;
        displayedSpectrumBands[index] += (target - displayedSpectrumBands[index]) * coefficient;
        nextBands[index] = displayedSpectrumBands[index];
    }

    outputSpectrumDisplay.setLevels(nextBands, peak, active);
}

void NateVSTAudioProcessorEditor::updateOutputOscilloscopeDisplay()
{
    UI::OutputOscilloscopeDisplay::SampleArray nextSamples {};

    auto peak = 0.0f;
    auto rms = 0.0f;
    auto maxStep = 0.0f;
    auto previous = outputSpectrumSnapshot.front();

    for (const auto sample : outputSpectrumSnapshot)
    {
        const auto absSample = std::abs(sample);
        peak = juce::jmax(peak, absSample);
        rms += sample * sample;
        maxStep = juce::jmax(maxStep, std::abs(sample - previous));
        previous = sample;
    }

    rms = std::sqrt(rms / static_cast<float>(outputSpectrumSnapshot.size()));
    peak = juce::jlimit(0.0f, 1.0f, peak);
    const auto active = peak > 0.0025f;
    const auto transient = active
        ? juce::jlimit(0.0f, 1.0f, ((peak - (rms * 1.55f)) * 1.85f) + (maxStep * 0.20f))
        : 0.0f;

    for (size_t index = 0; index < nextSamples.size(); ++index)
    {
        const auto start = (index * outputSpectrumSnapshot.size()) / nextSamples.size();
        const auto end = ((index + 1) * outputSpectrumSnapshot.size()) / nextSamples.size();
        auto selected = 0.0f;

        for (auto sourceIndex = start; sourceIndex < end; ++sourceIndex)
        {
            const auto sample = outputSpectrumSnapshot[sourceIndex];
            if (std::abs(sample) > std::abs(selected))
                selected = sample;
        }

        const auto coefficient = active ? 0.50f : 0.20f;
        displayedScopeSamples[index] += (juce::jlimit(-1.0f, 1.0f, selected) - displayedScopeSamples[index]) * coefficient;
        nextSamples[index] = displayedScopeSamples[index];
    }

    displayedScopeTransient += (transient - displayedScopeTransient) * (transient > displayedScopeTransient ? 0.54f : 0.18f);
    outputOscilloscopeDisplay.setSamples(nextSamples, peak, displayedScopeTransient, active);
}

void NateVSTAudioProcessorEditor::updateStereoFieldDisplay()
{
    auto correlation = 0.0f;
    auto width = 0.0f;
    auto balance = 0.0f;
    auto lowStereoRisk = 0.0f;
    audioProcessor.getStereoFieldLevels(correlation, width, balance, lowStereoRisk);

    displayedStereoCorrelation += (correlation - displayedStereoCorrelation) * 0.38f;
    displayedStereoWidth += (width - displayedStereoWidth) * 0.34f;
    displayedStereoBalance += (balance - displayedStereoBalance) * 0.34f;
    displayedLowStereoRisk += (lowStereoRisk - displayedLowStereoRisk) * 0.36f;

    UI::StereoFieldDisplay::State state;
    state.correlation = displayedStereoCorrelation;
    state.width = displayedStereoWidth;
    state.balance = displayedStereoBalance;
    state.lowStereoRisk = displayedLowStereoRisk;
    state.active = juce::jmax(displayedPeakLeft, displayedPeakRight) > 0.0025f;
    stereoFieldDisplay.setState(state);
}

void NateVSTAudioProcessorEditor::updateClubMonitorDisplay()
{
    auto subRms = 0.0f;
    auto lowStereoRisk = 0.0f;
    auto lowEndPeak = 0.0f;
    audioProcessor.getLowEndMeterLevels(subRms, lowStereoRisk, lowEndPeak);

    auto guardDrive = 0.0f;
    auto guardReduction = 0.0f;
    auto guardActive = false;
    audioProcessor.getGuardMeterLevels(guardDrive, guardReduction, guardActive);
    juce::ignoreUnused(guardDrive);

    auto pumpPhase = 0.0f;
    auto pumpGain = 1.0f;
    auto pumpReduction = 0.0f;
    auto pumpActive = false;
    audioProcessor.getPumpMeterLevels(pumpPhase, pumpGain, pumpReduction, pumpActive);
    juce::ignoreUnused(pumpPhase, pumpGain);

    UI::ClubMonitorDisplay::State state;
    state.subRms = subRms;
    state.lowStereoRisk = juce::jmax(lowStereoRisk, displayedLowStereoRisk);
    state.outputPeak = juce::jlimit(0.0f,
                                    1.0f,
                                    std::max({ lowEndPeak, displayedPeakLeft, displayedPeakRight }));
    state.guardReduction = guardReduction;
    state.pumpReduction = pumpReduction;
    state.guardActive = guardActive || readPlainParameterValue(Parameters::ID::fxGuardEnabled, 0.0f) >= 0.5f;
    state.pumpActive = pumpActive || readPlainParameterValue(Parameters::ID::fxPumpEnabled, 0.0f) >= 0.5f;
    state.active = state.outputPeak > 0.0025f || state.subRms > 0.0002f || state.lowStereoRisk > 0.01f;
    clubMonitorDisplay.setState(state);
}

void NateVSTAudioProcessorEditor::updateLowEndAssistant()
{
    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    auto subRms = 0.0f;
    auto lowStereoRisk = 0.0f;
    auto outputPeak = 0.0f;
    audioProcessor.getLowEndMeterLevels(subRms, lowStereoRisk, outputPeak);

    const auto rootNote = juce::jlimit(0, 127, juce::roundToInt(readParameter(Parameters::ID::sequencerRoot, 36.0f)));
    const auto rootName = juce::MidiMessage::getMidiNoteName(rootNote, true, true, 3);
    const auto rootHz = juce::MidiMessage::getMidiNoteInHertz(rootNote);
    const auto unisonVoiceCount = juce::jlimit(1, 8, juce::roundToInt(readParameter(Parameters::ID::unisonVoices, 1.0f)));
    const auto unisonSpreadAmount = readParameter(Parameters::ID::unisonSpread, 0.0f);

    UI::LowEndAssistant::State state;
    state.rootText = rootName + " " + juce::String(static_cast<int>(std::round(rootHz))) + "Hz";
    state.subRms = subRms;
    state.lowStereoRisk = lowStereoRisk;
    state.outputPeak = outputPeak;
    state.monoCrossoverHz = readParameter(Parameters::ID::fxWidthMonoCutoff, 120.0f);
    state.monoEnabled = readParameter(Parameters::ID::monoMode, 0.0f) >= 0.5f;
    state.widthEnabled = readParameter(Parameters::ID::fxWidthEnabled, 0.0f) >= 0.5f;
    state.guardEnabled = readParameter(Parameters::ID::fxGuardEnabled, 0.0f) >= 0.5f;

    if (! state.monoEnabled && ! state.widthEnabled && lowStereoRisk >= 0.28f)
    {
        state.phaseText = "LOW SIDE";
        state.guidanceText = "Mono lows before club play";
        state.guidanceLevel = 2;
    }
    else if (! state.monoEnabled && ! state.widthEnabled && lowStereoRisk >= 0.16f)
    {
        state.phaseText = "LOW SIDE";
        state.guidanceText = "Width off: watch bass sides";
        state.guidanceLevel = 1;
    }
    else if (rootHz < 40.0)
    {
        state.phaseText = "ROOT LOW";
        state.guidanceText = "Raise root for club subs";
        state.guidanceLevel = 1;
    }
    else if (rootHz > 82.0)
    {
        state.phaseText = "ROOT HIGH";
        state.guidanceText = "Root above sub sweet spot";
        state.guidanceLevel = 1;
    }
    else if (! state.guardEnabled && outputPeak >= 0.9f)
    {
        state.phaseText = "HEADROOM";
        state.guidanceText = "Guard off near ceiling";
        state.guidanceLevel = 1;
    }
    else if (state.monoEnabled && unisonVoiceCount > 1 && unisonSpreadAmount > 0.05f)
    {
        state.phaseText = "SPREAD LOCK";
        state.guidanceText = "Mono is collapsing spread";
        state.guidanceLevel = 0;
    }
    else
    {
        state.phaseText = "RESET OK";
        state.guidanceText = "Phase resets on note start";
        state.guidanceLevel = 0;
    }

    lowEndAssistant.setState(state);
}

void NateVSTAudioProcessorEditor::updatePerformanceSnapshotButtons()
{
    const auto hasSnapshotA = audioProcessor.hasPerformanceSnapshot(0);
    const auto hasSnapshotB = audioProcessor.hasPerformanceSnapshot(1);
    const auto hasSnapshotC = audioProcessor.hasPerformanceSnapshot(2);
    const auto hasSnapshotD = audioProcessor.hasPerformanceSnapshot(3);

    recallSnapshotAButton.setEnabled(hasSnapshotA);
    recallSnapshotBButton.setEnabled(hasSnapshotB);
    recallSnapshotCButton.setEnabled(hasSnapshotC);
    recallSnapshotDButton.setEnabled(hasSnapshotD);
    performanceStatusLabel.setText(juce::String("Scenes ")
                                       + (hasSnapshotA ? "A" : "-")
                                       + (hasSnapshotB ? "B" : "-")
                                       + (hasSnapshotC ? "C" : "-")
                                       + (hasSnapshotD ? "D" : "-"),
                                   juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::updatePerformanceXYPad()
{
    auto readParameter = [this] (const juce::String& parameterID)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return 0.0f;
    };

    UI::MacroPerformanceMap::ValueArray macroValues {};
    for (size_t index = 0; index < macroValues.size(); ++index)
        macroValues[index] = readParameter(macroPerformanceParameterIDs[index]);

    performanceXYPad.setValues(macroValues[2], macroValues[3]);
    macroPerformanceMap.setValues(macroValues);
    expandedMacroPerformanceMap.setValues(macroValues);
}

void NateVSTAudioProcessorEditor::updateHomeOverviewDisplay()
{
    UI::HomeOverviewDisplay::State state;

    state.sources = {
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::osc1Level, 1.0f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::osc2Level, 0.0f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::subLevel, 0.0f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::noiseLevel, 0.0f))
    };

    state.macros = {
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::macroTone, 0.0f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::macroDirt, 0.0f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::macroMotion, 0.0f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::macroSpace, 0.0f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::macroWeight, 0.0f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::macroBounce, 0.0f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::macroWarp, 0.0f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::macroThrow, 0.0f))
    };

    const auto cutoffHz = juce::jlimit(20.0f, 20000.0f, readPlainParameterValue(Parameters::ID::filterCutoff, 1000.0f));
    state.cutoff = juce::jlimit(0.0f, 1.0f, std::log(cutoffHz / 20.0f) / std::log(1000.0f));
    state.drive = juce::jlimit(0.0f, 1.0f,
                               readPlainParameterValue(Parameters::ID::driveAmount, 0.0f)
                                   + (readPlainParameterValue(Parameters::ID::fxDistortionAmount, 0.0f) * 0.35f)
                                   + (readPlainParameterValue(Parameters::ID::fxDistortionBassSafe, 0.0f) * 0.10f)
                                   + (readPlainParameterValue(Parameters::ID::fxGuardPush, 0.0f) * 0.25f));

    auto pumpPhase = 0.0f;
    auto pumpGain = 1.0f;
    auto pumpReduction = 0.0f;
    auto pumpActive = false;
    audioProcessor.getPumpMeterLevels(pumpPhase, pumpGain, pumpReduction, pumpActive);
    state.pumpReduction = pumpActive ? juce::jlimit(0.0f, 1.0f, pumpReduction) : 0.0f;

    auto guardDrive = 0.0f;
    auto guardReduction = 0.0f;
    auto guardActive = false;
    audioProcessor.getGuardMeterLevels(guardDrive, guardReduction, guardActive);
    state.guardReduction = guardActive ? juce::jlimit(0.0f, 1.0f, guardReduction) : 0.0f;

    state.outputPeak = juce::jlimit(0.0f, 1.0f, juce::jmax(displayedPeakLeft, displayedPeakRight));
    state.delaySend = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::fxSendDelay, 0.0f));
    state.reverbSend = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::fxSendReverb, 0.0f));

    const auto osc1Wave = juce::roundToInt(readPlainParameterValue(Parameters::ID::oscWave, 1.0f));
    const auto osc2Level = state.sources[1];
    const auto sampleEnabled = readPlainParameterValue(Parameters::ID::sampleEnabled, 0.0f) >= 0.5f;
    if (sampleEnabled)
        state.sourceName = "Sample + Synth";
    else if (osc1Wave == 4)
        state.sourceName = osc2Level > 0.04f ? "Dual Wavetable" : "Wavetable Synth";
    else if (osc1Wave == 5)
        state.sourceName = osc2Level > 0.04f ? "Dual Organ" : "Drawbar Organ";
    else if (osc1Wave == 6)
        state.sourceName = osc2Level > 0.04f ? "Layered Piano" : "House Piano";
    else
        state.sourceName = osc2Level > 0.04f ? "Dual Synth" : "Synth Engine";

    auto recipeName = recipeBox.getText().trim();
    state.recipeName = recipeName.isNotEmpty() ? recipeName : juce::String("House");

    const auto rms = juce::jmax(displayedRmsLeft, displayedRmsRight);
    if (state.outputPeak >= 0.985f)
        state.safetyName = "CLIP";
    else if (state.outputPeak >= 0.9f)
        state.safetyName = "HOT";
    else if (rms < 0.025f && state.outputPeak < 0.08f)
        state.safetyName = "LOW";
    else
        state.safetyName = "SAFE";

    homeOverviewDisplay.setState(state);
}

void NateVSTAudioProcessorEditor::updateHomeSignalFlowDisplay()
{
    UI::HomeSignalFlowDisplay::State state;

    const auto osc1 = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::osc1Level, 1.0f));
    const auto osc2 = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::osc2Level, 0.0f));
    const auto sub = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::subLevel, 0.0f));
    const auto noise = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::noiseLevel, 0.0f));
    const auto sampleEnabled = readPlainParameterValue(Parameters::ID::sampleEnabled, 0.0f) >= 0.5f;
    const auto sampleMix = sampleEnabled ? juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::sampleMix, 0.0f)) : 0.0f;
    const auto osc1Wave = juce::roundToInt(readPlainParameterValue(Parameters::ID::oscWave, 1.0f));
    const auto sourceAmount = juce::jlimit(0.0f, 1.0f, std::max({ osc1, osc2, sub, noise, sampleMix }));
    const auto activeSources = (osc1 > 0.04f ? 1 : 0)
        + (osc2 > 0.04f ? 1 : 0)
        + (sub > 0.04f ? 1 : 0)
        + (noise > 0.04f ? 1 : 0)
        + (sampleMix > 0.04f ? 1 : 0);

    auto sourceDetail = sampleEnabled ? juce::String("Sample+") : juce::String("Synth");
    if (! sampleEnabled)
        sourceDetail = activeSources > 2 ? juce::String("Layered")
                     : activeSources > 1 ? juce::String("Dual")
                     : osc1Wave == 5 ? juce::String("Organ")
                     : osc1Wave == 6 ? juce::String("Piano")
                     : juce::String("Synth");

    const auto filterMode = juce::jlimit(0, 2, juce::roundToInt(readPlainParameterValue(Parameters::ID::filterMode, 0.0f)));
    const auto filterCharacter = juce::jlimit(0, 3, juce::roundToInt(readPlainParameterValue(Parameters::ID::filterCharacter, 0.0f)));
    const auto cutoffHz = juce::jlimit(20.0f, 20000.0f, readPlainParameterValue(Parameters::ID::filterCutoff, 1000.0f));
    const auto cutoffAmount = juce::jlimit(0.0f, 1.0f, std::log(cutoffHz / 20.0f) / std::log(1000.0f));
    const std::array<const char*, 3> filterModes { "LP", "BP", "HP" };
    const std::array<const char*, 4> filterCharacters { "Clean", "Warm", "Acid", "Dirty" };

    auto pumpPhase = 0.0f;
    auto pumpGain = 1.0f;
    auto pumpReduction = 0.0f;
    auto pumpActive = false;
    audioProcessor.getPumpMeterLevels(pumpPhase, pumpGain, pumpReduction, pumpActive);

    const auto macroMotion = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::macroMotion, 0.0f));
    const auto lfo1Depth = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::lfo1Depth, 0.0f));
    const auto lfo2Depth = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::lfo2Depth, 0.0f));
    const auto seqEnabled = readPlainParameterValue(Parameters::ID::sequencerEnabled, 0.0f) >= 0.5f;
    const auto motionAmount = juce::jlimit(0.0f,
                                           1.0f,
                                           std::max({ macroMotion,
                                                      lfo1Depth,
                                                      lfo2Depth,
                                                      pumpActive ? pumpReduction : 0.0f,
                                                      seqEnabled ? 0.52f : 0.0f }));
    const auto motionDetail = seqEnabled ? juce::String("SEQ")
                           : pumpActive ? juce::String("PUMP")
                           : lfo1Depth > 0.05f ? juce::String("LFO")
                           : macroMotion > 0.05f ? juce::String("MACRO")
                           : juce::String("Idle");

    auto activeFx = 0;
    auto fxEnergy = 0.0f;
    auto addFx = [this, &activeFx, &fxEnergy] (const char* enabledID, const char* amountID, float fallback)
    {
        if (readPlainParameterValue(enabledID, 0.0f) < 0.5f)
            return;

        ++activeFx;
        fxEnergy = juce::jmax(fxEnergy, juce::jlimit(0.0f, 1.0f, readPlainParameterValue(amountID, fallback)));
    };

    if (readPlainParameterValue(Parameters::ID::fxDistortionEnabled, 0.0f) >= 0.5f)
    {
        ++activeFx;
        fxEnergy = juce::jmax(fxEnergy,
                              juce::jlimit(0.0f,
                                           1.0f,
                                           readPlainParameterValue(Parameters::ID::fxDistortionAmount, 0.2f)
                                               + (readPlainParameterValue(Parameters::ID::fxDistortionBassSafe, 0.0f) * 0.12f)));
    }
    addFx(Parameters::ID::fxBitcrushEnabled, Parameters::ID::fxBitcrushMix, 0.25f);
    addFx(Parameters::ID::fxPumpEnabled, Parameters::ID::fxPumpDepth, 0.35f);
    addFx(Parameters::ID::fxTremoloEnabled, Parameters::ID::fxTremoloDepth, 0.28f);
    addFx(Parameters::ID::fxRingEnabled, Parameters::ID::fxRingMix, 0.18f);
    addFx(Parameters::ID::fxCombEnabled, Parameters::ID::fxCombMix, 0.16f);
    addFx(Parameters::ID::fxChorusEnabled, Parameters::ID::fxChorusMix, 0.25f);
    addFx(Parameters::ID::fxDelayEnabled, Parameters::ID::fxDelayMix, 0.2f);
    addFx(Parameters::ID::fxReverbEnabled, Parameters::ID::fxReverbMix, 0.2f);
    addFx(Parameters::ID::fxWidthEnabled, Parameters::ID::fxWidthAmount, 0.15f);
    addFx(Parameters::ID::fxPhaserEnabled, Parameters::ID::fxPhaserMix, 0.22f);
    addFx(Parameters::ID::fxFlangerEnabled, Parameters::ID::fxFlangerMix, 0.18f);
    addFx(Parameters::ID::fxToneEnabled, Parameters::ID::fxToneTilt, 0.2f);
    addFx(Parameters::ID::fxEqEnabled, Parameters::ID::fxEqHighGain, 0.2f);

    const auto sendAmount = juce::jlimit(0.0f, 1.0f, juce::jmax(readPlainParameterValue(Parameters::ID::fxSendDelay, 0.0f),
                                                               readPlainParameterValue(Parameters::ID::fxSendReverb, 0.0f)));
    fxEnergy = juce::jlimit(0.0f, 1.0f, juce::jmax(fxEnergy, sendAmount));
    if (sendAmount > 0.01f)
        ++activeFx;

    auto guardDrive = 0.0f;
    auto guardReduction = 0.0f;
    auto guardActive = false;
    audioProcessor.getGuardMeterLevels(guardDrive, guardReduction, guardActive);
    const auto guardEnabled = readPlainParameterValue(Parameters::ID::fxGuardEnabled, 0.0f) >= 0.5f;
    const auto guardAmount = guardEnabled ? juce::jlimit(0.0f, 1.0f, juce::jmax(readPlainParameterValue(Parameters::ID::fxGuardPush, 0.0f), guardReduction)) : 0.0f;

    const auto outputPeak = juce::jlimit(0.0f, 1.0f, juce::jmax(displayedPeakLeft, displayedPeakRight));
    const auto rms = juce::jmax(displayedRmsLeft, displayedRmsRight);
    auto safetyName = juce::String("SAFE");
    if (outputPeak >= 0.985f)
        safetyName = "CLIP";
    else if (outputPeak >= 0.9f)
        safetyName = "HOT";
    else if (rms < 0.025f && outputPeak < 0.08f)
        safetyName = "LOW";

    state.nodes[0] = { "SOURCE", sourceDetail, sourceAmount, sourceAmount > 0.04f };
    state.nodes[1] = { "FILTER", juce::String(filterModes[static_cast<size_t>(filterMode)]) + " " + filterCharacters[static_cast<size_t>(filterCharacter)], cutoffAmount, true };
    state.nodes[2] = { "MOTION", motionDetail, motionAmount, motionAmount > 0.05f };
    state.nodes[3] = { "FX", activeFx > 0 ? juce::String(activeFx) + " ON" : juce::String("Dry"), fxEnergy, activeFx > 0 };
    state.nodes[4] = { "GUARD", guardEnabled ? (guardActive ? "Catch" : "Ready") : "Off", guardAmount, guardEnabled };
    state.nodes[5] = { "OUT", safetyName, outputPeak, true };

    homeSignalFlowDisplay.setState(state);
}

void NateVSTAudioProcessorEditor::updateHomeSessionDisplay()
{
    UI::HomeSessionDisplay::State state;
    const auto library = audioProcessor.getPresetLibrary();
    const auto selectedName = presetBox.getText().trim();
    const auto osc2 = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::osc2Level, 0.0f));
    const auto sub = juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::subLevel, 0.0f));
    const auto sampleEnabled = readPlainParameterValue(Parameters::ID::sampleEnabled, 0.0f) >= 0.5f;
    const auto sampleMix = sampleEnabled ? juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::sampleMix, 0.0f)) : 0.0f;
    const auto osc1Wave = juce::roundToInt(readPlainParameterValue(Parameters::ID::oscWave, 1.0f));

    state.visibleCount = static_cast<int>(visiblePresetBrowserPresets.size());
    state.totalCount = static_cast<int>(library.size());
    state.filterName = presetFilterBox.getText().trim();
    state.selectedName = selectedName.isNotEmpty() ? selectedName : juce::String("Choose a sound");
    state.sourceName = sampleEnabled ? juce::String("Sample")
                    : osc1Wave == 4 ? juce::String("WT")
                    : osc1Wave == 5 ? juce::String("Organ")
                    : osc1Wave == 6 ? juce::String("Piano")
                    : osc2 > 0.05f ? juce::String("Dual")
                    : juce::String("Synth");
    state.selectedMeta = state.sourceName + " | Use Library for full search";
    state.roleName = sub > 0.34f ? juce::String("Bass")
                   : sampleEnabled && sampleMix > 0.1f ? juce::String("Chop")
                   : juce::String("Patch");
    juce::String selectedSearchText;

    for (const auto& preset : library)
    {
        if (preset.name != selectedName)
            continue;

        const auto category = preset.folder.isNotEmpty() ? preset.folder : preset.category;
        selectedSearchText = presetSearchText(preset);
        state.selectedMeta = preset.pack + " | " + preset.key + " | " + formatPresetBpm(preset.bpm);
        if (textContainsAny(selectedSearchText, { "Bass", "Sub", "Reese", "Dred", "Rubber", "Log Drum" }))
            state.roleName = "Bass";
        else if (textContainsAny(selectedSearchText, { "Chord", "Stab", "Organ", "Keys", "Dub" }))
            state.roleName = "Chord/Stab";
        else if (textContainsAny(selectedSearchText, { "Lead" }))
            state.roleName = "Lead";
        else if (textContainsAny(selectedSearchText, { "Pluck", "Bell", "Ping", "Marimba" }))
            state.roleName = "Pluck";
        else if (textContainsAny(selectedSearchText, { "Chop", "Vocal", "Sample", "Slice" }))
            state.roleName = "Chop";
        else if (category.isNotEmpty())
            state.roleName = category.fromLastOccurrenceOf("/", false, true);
        state.rating = preset.rating;
        state.favorite = preset.isFavorite;
        state.factory = preset.isFactory;
        state.generated = preset.source.equalsIgnoreCase("Generated");
        break;
    }

    state.compareReady = hasPresetCompareSnapshots();
    state.sequencerActive = readPlainParameterValue(Parameters::ID::sequencerEnabled, 0.0f) >= 0.5f;

    auto pumpPhase = 0.0f;
    auto pumpGain = 1.0f;
    auto pumpReduction = 0.0f;
    audioProcessor.getPumpMeterLevels(pumpPhase, pumpGain, pumpReduction, state.pumpActive);

    auto guardDrive = 0.0f;
    auto guardReduction = 0.0f;
    audioProcessor.getGuardMeterLevels(guardDrive, guardReduction, state.guardActive);
    state.guardActive = state.guardActive || readPlainParameterValue(Parameters::ID::fxGuardEnabled, 0.0f) >= 0.5f;

    const auto outputPeak = juce::jlimit(0.0f, 1.0f, juce::jmax(displayedPeakLeft, displayedPeakRight));
    const auto rms = juce::jmax(displayedRmsLeft, displayedRmsRight);
    if (outputPeak >= 0.985f)
        state.safetyName = "CLIP";
    else if (outputPeak >= 0.9f)
        state.safetyName = "HOT";
    else if (rms < 0.025f && outputPeak < 0.08f)
        state.safetyName = "LOW";
    else
        state.safetyName = "SAFE";

    state.performanceValues = {
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::macroTone, 0.0f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::macroDirt, 0.0f)
            + (readPlainParameterValue(Parameters::ID::driveAmount, 0.0f) * 0.4f)
            + (readPlainParameterValue(Parameters::ID::fxDistortionAmount, 0.0f) * 0.35f)
            + (readPlainParameterValue(Parameters::ID::fxDistortionBassSafe, 0.0f) * 0.10f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::macroMotion, 0.0f)
            + (readPlainParameterValue(Parameters::ID::lfo1Depth, 0.0f) * 0.3f)
            + (readPlainParameterValue(Parameters::ID::lfo2Depth, 0.0f) * 0.22f)
            + (state.sequencerActive ? 0.16f : 0.0f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::macroSpace, 0.0f)
            + (readPlainParameterValue(Parameters::ID::fxSendDelay, 0.0f) * 0.4f)
            + (readPlainParameterValue(Parameters::ID::fxSendReverb, 0.0f) * 0.4f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::macroWeight, 0.0f)
            + (sub * 0.45f)
            + (sampleMix * 0.2f)),
        juce::jlimit(0.0f, 1.0f, readPlainParameterValue(Parameters::ID::macroBounce, 0.0f)
            + (readPlainParameterValue(Parameters::ID::fxPumpDepth, 0.0f) * 0.42f)
            + (state.pumpActive ? pumpReduction * 0.35f : 0.0f))
    };

    const auto activeCandidate = audioProcessor.getActiveRandomCandidateIndex();
    state.candidateReady = audioProcessor.hasRandomCandidate(activeCandidate);
    state.candidateName = state.candidateReady
        ? audioProcessor.getRandomCandidateSummary(activeCandidate)
        : juce::String("No candidate");

    homeSessionDisplay.setState(state);
}

void NateVSTAudioProcessorEditor::updateSequencerSceneButtons()
{
    const std::array<juce::String, 4> sceneLabels { "A", "B", "Fill", "Drop" };
    const auto liveChainEnabled = audioProcessor.isSequencerSceneChainPlaybackEnabled();
    const auto liveChainLength = audioProcessor.getSequencerSceneChainPlaybackLength();
    const auto chainBars = audioProcessor.getSequencerSceneChainClipBars();
    sequencerSceneChainLiveButton.setToggleState(liveChainEnabled, juce::dontSendNotification);
    sequencerSceneChainLiveButton.setButtonText(liveChainEnabled
                                                   ? "Live " + juce::String(juce::jmax(1, liveChainLength))
                                                   : "Live");
    sequencerSceneChainLiveButton.setTooltip(liveChainEnabled
                                                 ? "Live scene-chain playback is following captured scene steps"
                                                 : "Play captured A/B/Fill/Drop scene steps as a live 2/4-bar chain");
    sequencerSceneChainLengthButton.setButtonText(chainBars == 0 ? "Auto" : juce::String(chainBars) + " Bar");
    sequencerSceneChainLengthButton.setTooltip(chainBars == 0
                                                   ? "Auto chain length follows captured scenes; click for forced 2 bars"
                                                   : "Forced " + juce::String(chainBars) + "-bar scene-chain MIDI/live length; click to cycle");

    for (size_t index = 0; index < sequencerSceneRecallButtons.size(); ++index)
    {
        const auto slot = static_cast<int>(index);
        const auto hasScene = audioProcessor.hasSequencerPatternScene(slot);
        const auto summary = audioProcessor.getSequencerPatternSceneSummary(slot);
        sequencerSceneRecallButtons[index].setEnabled(hasScene);
        sequencerSceneRecallButtons[index].setButtonText(sceneLabels[index] + (hasScene ? "*" : ""));
        sequencerSceneRecallButtons[index].setTooltip(summary);
        sequencerSceneCaptureButtons[index].setTooltip("Capture current pattern to " + sceneLabels[index] + " | " + summary);
    }
}

void NateVSTAudioProcessorEditor::updateSequencerGridContext()
{
    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    const auto root = juce::roundToInt(readParameter(Parameters::ID::sequencerRoot, 36.0f));
    const auto octaveOffset = juce::roundToInt(readParameter(Parameters::ID::sequencerOctave, 0.0f)) * 12;
    const auto scaleMode = juce::roundToInt(readParameter(Parameters::ID::sequencerScale, 0.0f));
    const auto displayRoot = juce::jlimit(0, 127, root + octaveOffset);
    sequencerGrid.setRootNote(displayRoot);
    sequencerGrid.setScaleMode(scaleMode);
    expandedSequencerGrid.setRootNote(displayRoot);
    expandedSequencerGrid.setScaleMode(scaleMode);
    updateSequencerRootStepper();
    updateSequencerStepEditor();
}

void NateVSTAudioProcessorEditor::repaintSequencerGrids()
{
    sequencerGrid.repaint();
    expandedSequencerGrid.repaint();
}

void NateVSTAudioProcessorEditor::updateSequencerStepEditor()
{
    auto selectedIndex = sequencerGrid.getSelectedStepIndex();
    if (selectedIndex < 0)
        selectedIndex = expandedSequencerGrid.getSelectedStepIndex();

    const auto hasSelection = selectedIndex >= 0;
    for (auto& button : sequencerStepEditorButtons)
        button.setEnabled(hasSelection);

    if (! hasSelection)
    {
        sequencerStepEditorLabel.setText("Select a step", juce::dontSendNotification);
        sequencerStepEditorLabel.setTooltip("Click a piano-roll step or lane cell, then edit it here");
        return;
    }

    const auto step = audioProcessor.getSequencerStep(selectedIndex);
    const auto root = juce::roundToInt(readPlainParameterValue(Parameters::ID::sequencerRoot, 36.0f));
    const auto octave = juce::roundToInt(readPlainParameterValue(Parameters::ID::sequencerOctave, 0.0f)) * 12;
    const auto midiNote = juce::jlimit(0, 127, root + octave + step.noteOffset);
    const auto noteName = juce::MidiMessage::getMidiNoteName(midiNote, true, true, 3);
    const auto label = "S" + juce::String(selectedIndex + 1)
        + (step.enabled ? " " + noteName : " empty")
        + " | V" + juce::String(juce::roundToInt(step.velocity * 100.0f))
        + " P" + juce::String(juce::roundToInt(step.probability * 100.0f))
        + " L" + juce::String(juce::roundToInt(step.length * 100.0f));

    sequencerStepEditorLabel.setText(label, juce::dontSendNotification);
    sequencerStepEditorLabel.setTooltip("Selected step editor: " + label
                                        + " | Ratchet " + juce::String(step.ratchet) + "x"
                                        + (step.slide ? " | Slide" : "")
                                        + " | Lock " + juce::String(juce::roundToInt(step.lock * 100.0f)) + "%");
    sequencerStepEditorButtons[10].setButtonText("Rat " + juce::String(step.ratchet) + "x");
    sequencerStepEditorButtons[11].setButtonText(step.slide ? "Slide On" : "Slide");
    sequencerStepEditorButtons[12].setButtonText(step.lock > 0.01f ? "Lock " + juce::String(juce::roundToInt(step.lock * 100.0f)) : "Lock");
}

void NateVSTAudioProcessorEditor::setSelectedSequencerStep(Sequencer::Step step)
{
    auto selectedIndex = sequencerGrid.getSelectedStepIndex();
    if (selectedIndex < 0)
        selectedIndex = expandedSequencerGrid.getSelectedStepIndex();
    if (selectedIndex < 0)
        selectedIndex = 0;

    step.enabled = true;
    audioProcessor.setSequencerStep(selectedIndex, step);
    if (sequencerGrid.getSelectedStepIndex() != selectedIndex)
        sequencerGrid.selectStep(selectedIndex);
    if (expandedSequencerGrid.getSelectedStepIndex() != selectedIndex)
        expandedSequencerGrid.selectStep(selectedIndex);

    updateSequencerStepEditor();
    repaintSequencerGrids();
    returnKeyboardFocusToPiano();
}

void NateVSTAudioProcessorEditor::adjustSelectedSequencerStepNote(int semitones)
{
    auto step = sequencerGrid.getSelectedStepIndex() >= 0
        ? sequencerGrid.getSelectedStepValue()
        : expandedSequencerGrid.getSelectedStepValue();

    step.noteOffset = juce::jlimit(Sequencer::PatternSequencer::minNoteOffset,
                                  Sequencer::PatternSequencer::maxNoteOffset,
                                  step.noteOffset + semitones);
    if (step.velocity <= 0.0f)
        step.velocity = 0.82f;
    if (step.probability <= 0.0f)
        step.probability = 1.0f;
    if (step.length <= 0.0f)
        step.length = 1.0f;

    setSelectedSequencerStep(step);
    setRandomStatus("Step note " + juce::String(semitones > 0 ? "+" : "") + juce::String(semitones));
}

void NateVSTAudioProcessorEditor::adjustSelectedSequencerStepValue(int fieldIndex, float delta)
{
    auto step = sequencerGrid.getSelectedStepIndex() >= 0
        ? sequencerGrid.getSelectedStepValue()
        : expandedSequencerGrid.getSelectedStepValue();

    switch (fieldIndex)
    {
        case 0: step.velocity = juce::jlimit(0.1f, 1.0f, step.velocity + delta); break;
        case 1: step.probability = juce::jlimit(0.0f, 1.0f, step.probability + delta); break;
        case 2: step.length = juce::jlimit(0.1f, 1.0f, step.length + delta); break;
        default: break;
    }

    setSelectedSequencerStep(step);
}

void NateVSTAudioProcessorEditor::toggleSelectedSequencerStepFlag(int flagIndex)
{
    auto step = sequencerGrid.getSelectedStepIndex() >= 0
        ? sequencerGrid.getSelectedStepValue()
        : expandedSequencerGrid.getSelectedStepValue();

    switch (flagIndex)
    {
        case 0:
            step.ratchet = step.ratchet >= Sequencer::PatternSequencer::maxRatchet
                ? Sequencer::PatternSequencer::minRatchet
                : step.ratchet + 1;
            if (step.ratchet > 1)
                step.slide = false;
            break;

        case 1:
            step.slide = ! step.slide;
            if (step.slide)
                step.ratchet = 1;
            break;

        case 2:
            step.lock = step.lock < 0.05f ? 0.5f : (step.lock < 0.75f ? 1.0f : 0.0f);
            break;

        default:
            break;
    }

    setSelectedSequencerStep(step);
}

void NateVSTAudioProcessorEditor::timerCallback()
{
    updatePresetAudition();
    if (pendingOverwritePresetName.isNotEmpty()
        && juce::Time::getMillisecondCounterHiRes() > pendingOverwriteUntilMs)
    {
        clearPresetOverwriteWarning();
    }

    if (activeRandomCandidateAuditionNote >= 0
        && juce::Time::getMillisecondCounterHiRes() >= randomCandidateAuditionNoteOffMs)
    {
        releaseRandomCandidateAudition(true);
    }

    if (anyComponentVisible(sineWaveButton,
                            sawWaveButton,
                            squareWaveButton,
                            triangleWaveButton,
                            wavetableWaveButton,
                            organWaveButton,
                            housePianoWaveButton,
                            customWaveButton,
                            rateEighthButton,
                            rateSixteenthButton,
                            rateThirtySecondButton))
    {
        updateSegmentedSelectors();
    }

    if (lfoCurveDisplay.isVisible())
        updateLfoCurveDisplay();
    if (pumpCurveDisplay.isVisible())
        updatePumpCurveDisplay();
    if (wavetableDisplay.isVisible())
        updateWavetableDisplay();
    if (randomMorphPad.isVisible())
        updateRandomMorphPad();
    if (anyComponentVisible(houseLayerRackDisplay, expandedHouseLayerRackDisplay))
        updateHouseLayerRackDisplay();
    if (filterResponseDisplay.isVisible())
        updateFilterResponseDisplay();
    if (hostSyncStatusLabel.isVisible())
        updateHostSyncStatus();

    const auto modWorkflowVisible = anyComponentVisible(modRouteMapDisplay,
                                                        modInspectorStatusLabel,
                                                        modMacroAssignStatusLabel,
                                                        macroAssignmentPad,
                                                        expandedMacroAssignmentPad);
    if (modWorkflowVisible)
    {
        updateModMatrixRows();
        updateModInspectorStatus();
        updateMacroAssignmentEditorStatus();
    }
    updateModDestinationIndicators();

    updateOutputMeter();
    if (outputSpectrumDisplay.isVisible())
        updateOutputSpectrumDisplay();
    if (outputOscilloscopeDisplay.isVisible())
        updateOutputOscilloscopeDisplay();
    if (stereoFieldDisplay.isVisible())
        updateStereoFieldDisplay();
    if (clubMonitorDisplay.isVisible())
        updateClubMonitorDisplay();
    if (lowEndAssistant.isVisible())
        updateLowEndAssistant();
    if (anyComponentVisible(recallSnapshotAButton,
                            captureSnapshotAButton,
                            recallSnapshotBButton,
                            captureSnapshotBButton,
                            recallSnapshotCButton,
                            captureSnapshotCButton,
                            recallSnapshotDButton,
                            captureSnapshotDButton))
    {
        updatePerformanceSnapshotButtons();
    }
    if (performanceXYPad.isVisible())
        updatePerformanceXYPad();
    if (homeOverviewDisplay.isVisible())
        updateHomeOverviewDisplay();
    if (homeSignalFlowDisplay.isVisible())
        updateHomeSignalFlowDisplay();

    const auto sequencerVisible = anyComponentVisible(sequencerGrid,
                                                      expandedSequencerGrid,
                                                      sequencerRootValueLabel,
                                                      sequencerSceneChainLiveButton);
    if (sequencerVisible)
    {
        updateSequencerSceneButtons();
        updateSequencerGridContext();
        updateSequencerRootStepper();
    }

    if (anyComponentVisible(sampleWaveformDisplay,
                            expandedSampleWaveformDisplay,
                            sampleChopPanel,
                            sampleRecorderPanel))
    {
        updateSampleSliceButtons();
        updateSampleRecorderStatus();
        updateSampleWaveformDisplay();
    }

    updateKeyboardRangeLabel();
    if (fxRackStatusLabel.isVisible())
        updateFxRackControls();
    refreshGlobalEditControls();
}

int NateVSTAudioProcessorEditor::getNumRows()
{
    return static_cast<int>(visiblePresetBrowserPresets.size());
}

void NateVSTAudioProcessorEditor::paintListBoxItem(int rowNumber,
                                                    juce::Graphics& g,
                                                    int width,
                                                    int height,
                                                    bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= getNumRows())
        return;

    const auto& preset = visiblePresetBrowserPresets[static_cast<size_t>(rowNumber)];
    const auto rowFill = rowIsSelected
        ? juce::Colour(0xff21342f)
        : (rowNumber % 2 == 0 ? juce::Colour(0xff101619) : juce::Colour(0xff121b1e));

    g.fillAll(rowFill);
    g.setColour(rowIsSelected ? juce::Colour(0xff8ee6c9) : juce::Colour(0xff253036));
    g.drawHorizontalLine(height - 1, 0.0f, static_cast<float>(width));

    auto row = juce::Rectangle<int>(0, 0, width, height).reduced(9, 4);
    const auto categoryText = preset.folder.isNotEmpty() ? preset.folder : preset.category;
    const auto ratingText = preset.rating > 0 ? juce::String(preset.rating) + "/5" : juce::String("-");
    const auto sourcePrefix = preset.isFavorite ? juce::String("F ") : juce::String();
    const auto nameColour = preset.isFavorite ? juce::Colour(0xffffd27a) : juce::Colour(0xffedf7f4);
    const auto layout = presetBrowserRowLayoutForWidth(width, height);

    auto infoArea = row.removeFromRight(layout.infoWidth);
    row.removeFromRight(layout.compact ? 4 : 6);
    auto previewArea = row.removeFromRight(layout.previewWidth);
    row.removeFromRight(layout.compact ? 4 : 6);
    auto nameArea = row;

    auto marker = nameArea.removeFromLeft(4).toFloat().reduced(0.0f, 2.0f);
    g.setColour(preset.isFavorite ? juce::Colour(0xffffd27a)
                                  : (preset.isFactory ? juce::Colour(0xff7bb7ff) : juce::Colour(0xff8ee6c9)));
    g.fillRoundedRectangle(marker, 2.0f);
    nameArea.removeFromLeft(5);

    g.setFont(juce::FontOptions(layout.compact ? 10.7f : 11.5f, juce::Font::bold));
    g.setColour(nameColour);
    g.drawFittedText(sourcePrefix + preset.name,
                     nameArea.removeFromTop(layout.compact ? 13 : 15).reduced(1, 0),
                     juce::Justification::centredLeft,
                     1,
                     0.62f);

    const auto roleText = presetInspectorRoleName(inferPresetAuditionRole(&preset));
    const auto metaText = roleText + " | " + categoryText + " | " + preset.pack + " | "
        + preset.key + " " + formatPresetBpm(preset.bpm);
    g.setFont(juce::FontOptions(layout.compact ? 8.0f : 8.8f, juce::Font::plain));
    g.setColour(juce::Colour(0xff9dafb2));
    g.drawFittedText(metaText,
                     nameArea.reduced(1, 0),
                     juce::Justification::centredLeft,
                     1,
                     0.48f);

    drawPresetPreviewLevelBadge(g, previewArea.removeFromTop(layout.compact ? 13 : 15), preset);
    g.setFont(juce::FontOptions(7.7f, juce::Font::bold));
    g.setColour(juce::Colour(0xffb8c6c5));
    g.drawFittedText(roleText,
                     previewArea.reduced(1, 0),
                     juce::Justification::centred,
                     1,
                     0.50f);

    auto pill = infoArea.removeFromTop(layout.compact ? 13 : 15).toFloat().reduced(0.0f, 1.0f);
    g.setColour(preset.isFactory ? juce::Colour(0xff182b3b) : juce::Colour(0xff1d2d28));
    g.fillRoundedRectangle(pill, 4.0f);
    g.setColour(preset.isFactory ? juce::Colour(0xff7bb7ff) : juce::Colour(0xff8ee6c9));
    g.drawRoundedRectangle(pill, 4.0f, 1.0f);
    g.setFont(juce::FontOptions(7.8f, juce::Font::bold));
    g.drawFittedText(preset.isFactory ? "FCT" : "USER", pill.toNearestInt(), juce::Justification::centred, 1);

    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffc5d1d0));
    g.drawFittedText(ratingText,
                     infoArea.reduced(1, 0),
                     juce::Justification::centred,
                     1,
                     0.58f);
}

juce::String NateVSTAudioProcessorEditor::getNameForRow(int rowNumber)
{
    if (rowNumber < 0 || rowNumber >= getNumRows())
        return {};

    return visiblePresetBrowserPresets[static_cast<size_t>(rowNumber)].name;
}

void NateVSTAudioProcessorEditor::selectedRowsChanged(int lastRowSelected)
{
    if (ignorePresetBrowserSelection
        || lastRowSelected < 0
        || lastRowSelected >= getNumRows())
    {
        return;
    }

    const auto& preset = visiblePresetBrowserPresets[static_cast<size_t>(lastRowSelected)];
    presetBox.setSelectedItemIndex(lastRowSelected, juce::dontSendNotification);
    presetNameEditor.setText(preset.name, juce::dontSendNotification);
    updateFavoritePresetButton();
    presetStatusLabel.setText("Selected " + preset.name + " | " + preset.pack + " | "
                                  + preset.key + " | " + formatPresetBpm(preset.bpm)
                                  + " | " + presetPreviewSummaryText(preset)
                                  + " | " + presetMacroPreviewText(preset),
                              juce::dontSendNotification);
    updatePresetLibrarySummary();
}

void NateVSTAudioProcessorEditor::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (row < 0 || row >= getNumRows())
        return;

    selectedRowsChanged(row);
    loadSelectedPreset();
}

void NateVSTAudioProcessorEditor::refreshPresetList()
{
    const auto previousSelection = presetBox.getText();
    presetBox.clear(juce::dontSendNotification);
    visiblePresetBrowserPresets.clear();

    const auto library = audioProcessor.getPresetLibrary();
    const auto recentNames = audioProcessor.getRecentPresetNames();
    auto filter = presetFilterBox.getText().trim();
    if (filter.isEmpty())
        filter = "All";

    auto tagFilter = presetTagBox.getText().trim();
    if (tagFilter.isEmpty())
        tagFilter = "All Tags";

    auto packFilter = presetBrowserPackFilterBox.getText().trim();
    if (packFilter.isEmpty())
        packFilter = "All Packs";

    auto sortMode = presetSortBox.getText().trim();
    if (sortMode.isEmpty())
        sortMode = "Name";

    for (size_t index = 0; index < presetQuickFilterButtons.size(); ++index)
    {
        auto active = false;
            switch (index)
            {
                case 0: active = filter == "All" && tagFilter == "All Tags"; break;
                case 1: active = filter == "Favorites"; break;
                case 2: active = filter == "Recent"; break;
                case 3: active = filter == "Similar"; break;
                case 4: active = filter == "Bass"; break;
                case 5: active = filter == "Lead"; break;
                case 6: active = filter == "Chord"; break;
                case 7: active = filter == "Pad"; break;
                case 8: active = filter == "FX"; break;
                case 9: active = filter == "Sequence"; break;
                default: break;
            }
        presetQuickFilterButtons[index].setToggleState(active, juce::dontSendNotification);
    }

    const auto searchText = presetSearchEditor.getText().trim();
    juce::StringArray searchTerms;
    searchTerms.addTokens(searchText, " ", "\"");
    searchTerms.removeEmptyStrings();

    auto matchesSearch = [&searchTerms] (const NateVSTAudioProcessor::PresetInfo& preset)
    {
        if (searchTerms.isEmpty())
            return true;

        const auto searchable = presetSearchText(preset) + " "
            + presetMacroPreviewText(preset) + " Macro Macros Performance";

        for (const auto& term : searchTerms)
            if (! searchable.containsIgnoreCase(term))
                return false;

        return true;
    };

    auto matchesTag = [&tagFilter] (const NateVSTAudioProcessor::PresetInfo& preset)
    {
        if (tagFilter == "All Tags")
            return true;

        return preset.tags.containsIgnoreCase(tagFilter)
            || preset.category.equalsIgnoreCase(tagFilter)
            || preset.pack.equalsIgnoreCase(tagFilter)
            || preset.key.equalsIgnoreCase(tagFilter)
            || preset.author.equalsIgnoreCase(tagFilter)
            || preset.notes.containsIgnoreCase(tagFilter)
            || preset.name.containsIgnoreCase(tagFilter);
    };

    auto matchesPack = [&packFilter] (const NateVSTAudioProcessor::PresetInfo& preset)
    {
        return packFilter == "All Packs" || preset.pack.equalsIgnoreCase(packFilter);
    };

    auto nextItemId = 1;
    auto addPreset = [this, &nextItemId] (const NateVSTAudioProcessor::PresetInfo& preset)
    {
        presetBox.addItem(preset.name, nextItemId++);
        visiblePresetBrowserPresets.push_back(preset);
    };

    auto sortPresets = [&sortMode] (std::vector<NateVSTAudioProcessor::PresetInfo>& presetsToSort)
    {
        std::stable_sort(presetsToSort.begin(),
                         presetsToSort.end(),
                         [&sortMode] (const auto& left, const auto& right)
                         {
                             if (sortMode == "Rating" && left.rating != right.rating)
                                 return left.rating > right.rating;

                             if (sortMode == "Newest" && left.lastModifiedMs != right.lastModifiedMs)
                                 return left.lastModifiedMs > right.lastModifiedMs;

                             if (sortMode == "Category")
                             {
                                 const auto categoryCompare = left.category.compareIgnoreCase(right.category);
                                 if (categoryCompare != 0)
                                     return categoryCompare < 0;
                             }

                             if (sortMode == "Pack")
                             {
                                 const auto packCompare = left.pack.compareIgnoreCase(right.pack);
                                 if (packCompare != 0)
                                     return packCompare < 0;
                             }

                             if (sortMode == "BPM" && left.bpm != right.bpm)
                                 return left.bpm > 0 && (right.bpm == 0 || left.bpm < right.bpm);

                             if (sortMode == "Key")
                             {
                                 const auto keyCompare = left.key.compareIgnoreCase(right.key);
                                 if (keyCompare != 0)
                                     return keyCompare < 0;
                             }

                             if (sortMode == "Author")
                             {
                                 const auto authorCompare = left.author.compareIgnoreCase(right.author);
                                 if (authorCompare != 0)
                                     return authorCompare < 0;
                             }

                             if (sortMode == "Source")
                             {
                                 const auto sourceCompare = left.source.compareIgnoreCase(right.source);
                                 if (sourceCompare != 0)
                                     return sourceCompare < 0;
                             }

                             if (sortMode == "Macros" && std::abs(left.macroIntensity - right.macroIntensity) > 0.001f)
                                 return left.macroIntensity > right.macroIntensity;

                             return left.name.compareIgnoreCase(right.name) < 0;
                         });
    };

    auto findPreset = [&library] (const juce::String& name) -> const NateVSTAudioProcessor::PresetInfo*
    {
        for (const auto& preset : library)
            if (preset.name == name)
                return &preset;

        return nullptr;
    };

    NateVSTAudioProcessor::PresetInfo similarReferencePreset;
    auto hasSimilarReference = false;
    if (const auto* preset = findPreset(previousSelection))
    {
        similarReferencePreset = *preset;
        hasSimilarReference = true;
    }

    auto matchesSimilarPreset = [&similarReferencePreset, hasSimilarReference] (const NateVSTAudioProcessor::PresetInfo& preset)
    {
        if (! hasSimilarReference || preset.name == similarReferencePreset.name)
            return false;

        auto score = 0;
        if (inferPresetAuditionRole(&preset) == inferPresetAuditionRole(&similarReferencePreset))
            score += 3;
        if (preset.pack.isNotEmpty() && preset.pack.equalsIgnoreCase(similarReferencePreset.pack))
            score += 2;
        if (preset.category.equalsIgnoreCase(similarReferencePreset.category)
            || preset.folder.equalsIgnoreCase(similarReferencePreset.folder))
            score += 2;

        juce::StringArray referenceTerms;
        referenceTerms.addTokens(similarReferencePreset.tags + " " + similarReferencePreset.category + " " + similarReferencePreset.folder,
                                 ",/; ",
                                 "\"'");
        referenceTerms.removeEmptyStrings();
        const auto searchable = presetSearchText(preset);
        for (const auto& term : referenceTerms)
        {
            if (term.length() >= 3 && searchable.containsIgnoreCase(term))
            {
                ++score;
                if (score >= 4)
                    break;
            }
        }

        return score >= 3;
    };

    if (filter == "Recent")
    {
        for (const auto& recentName : recentNames)
            if (const auto* preset = findPreset(recentName))
                if (matchesPack(*preset) && matchesTag(*preset) && matchesSearch(*preset))
                    addPreset(*preset);
    }
    else
    {
        std::vector<NateVSTAudioProcessor::PresetInfo> visiblePresets;
        for (const auto& preset : library)
        {
            const auto leafCategory = preset.category.fromLastOccurrenceOf("/", false, true);
            const auto matchesFilter = filter == "All"
                || (filter == "Favorites" && preset.isFavorite)
                || (filter == "Rated" && preset.rating > 0)
                || (filter == "5 Stars" && preset.rating == 5)
                || (filter == "4+ Stars" && preset.rating >= 4)
                || (filter == "Macro Rich" && preset.macroIntensity >= 0.30f)
                || (filter == "Generated" && (preset.source.equalsIgnoreCase("Generated")
                                               || preset.tags.containsIgnoreCase("Generated")))
                || (filter == "User" && ! preset.isFactory)
                || (filter == "Factory" && preset.isFactory)
                || (filter == "Similar" && matchesSimilarPreset(preset))
                || (filter == "120-124 BPM" && preset.bpm >= 120 && preset.bpm <= 124)
                || (filter == "125-128 BPM" && preset.bpm >= 125 && preset.bpm <= 128)
                || (filter == "129-132 BPM" && preset.bpm >= 129 && preset.bpm <= 132)
                || (filter == "133+ BPM" && preset.bpm >= 133)
                || presetMatchesSmartCrate(preset, filter)
                || preset.category.equalsIgnoreCase(filter)
                || leafCategory.equalsIgnoreCase(filter)
                || preset.pack.equalsIgnoreCase(filter)
                || preset.key.equalsIgnoreCase(filter)
                || preset.author.equalsIgnoreCase(filter)
                || preset.folder.startsWithIgnoreCase(filter + "/")
                || preset.tags.containsIgnoreCase(filter)
                || preset.notes.containsIgnoreCase(filter)
                || preset.name.containsIgnoreCase(filter);

            if (matchesFilter && matchesPack(preset) && matchesTag(preset) && matchesSearch(preset))
                visiblePresets.push_back(preset);
        }

        sortPresets(visiblePresets);
        for (const auto& preset : visiblePresets)
            addPreset(preset);
    }

    auto previousIndex = -1;
    for (auto index = 0; index < presetBox.getNumItems(); ++index)
    {
        if (presetBox.getItemText(index) == previousSelection)
        {
            previousIndex = index;
            break;
        }
    }
    if (previousIndex >= 0)
        presetBox.setSelectedItemIndex(previousIndex, juce::dontSendNotification);
    else if (presetBox.getNumItems() > 0)
        presetBox.setSelectedItemIndex(0, juce::dontSendNotification);

    presetBrowserList.updateContent();
    auto selectedBrowserRow = -1;
    for (auto index = 0; index < getNumRows(); ++index)
    {
        if (visiblePresetBrowserPresets[static_cast<size_t>(index)].name == presetBox.getText())
        {
            selectedBrowserRow = index;
            break;
        }
    }

    ignorePresetBrowserSelection = true;
    if (selectedBrowserRow >= 0)
        presetBrowserList.selectRow(selectedBrowserRow);
    else
        presetBrowserList.deselectAllRows();
    ignorePresetBrowserSelection = false;

    const auto* selectedPreset = findPreset(presetBox.getText());
    auto previewReadyCount = 0;
    auto previewStaleCount = 0;
    for (const auto& preset : visiblePresetBrowserPresets)
    {
        if (preset.previewAvailable && ! preset.previewStale)
            ++previewReadyCount;
        else if (preset.previewStale)
            ++previewStaleCount;
    }

    auto statusText = juce::String(presetBox.getNumItems()) + " presets | Filter: " + filter;
    if (tagFilter != "All Tags")
        statusText += " | Tag: " + tagFilter;
    if (packFilter != "All Packs")
        statusText += " | Pack: " + packFilter;
    statusText += " | Sort: " + sortMode;
    if (searchText.isNotEmpty())
        statusText += " | Search: " + searchText;
    if (selectedPreset != nullptr)
        statusText += " | " + presetPreviewSummaryText(*selectedPreset) + " | " + presetMacroPreviewText(*selectedPreset);
    if (previewReadyCount > 0 || previewStaleCount > 0)
        statusText += " | Previews: " + juce::String(previewReadyCount) + " ready"
            + (previewStaleCount > 0 ? " / " + juce::String(previewStaleCount) + " stale" : juce::String());
    statusText += " | User: " + audioProcessor.getPresetDirectory().getFullPathName();
    presetStatusLabel.setText(statusText, juce::dontSendNotification);
    presetStatusLabel.setTooltip(selectedPreset != nullptr
                                     ? presetPreviewSummaryText(*selectedPreset) + " | "
                                           + presetMacroPreviewText(*selectedPreset) + " | " + selectedPreset->pack + " | " + selectedPreset->key + " | " + formatPresetBpm(selectedPreset->bpm)
                                           + (selectedPreset->notes.trim().isNotEmpty() ? " | " + selectedPreset->notes.replaceCharacter('\n', ' ') : juce::String())
                                     : juce::String("Preset browser status"));
    presetBox.setTooltip(selectedPreset != nullptr
                             ? selectedPreset->name + " | " + presetMacroPreviewText(*selectedPreset)
                             : juce::String("Select a preset"));
    updateFavoritePresetButton();
    updatePresetLibrarySummary();
}

void NateVSTAudioProcessorEditor::updatePresetCrateMapDisplay()
{
    UI::PresetCrateMapDisplay::State state;
    const auto library = audioProcessor.getPresetLibrary();
    juce::StringArray folders;
    juce::StringArray packs;
    state.totalCount = static_cast<int>(library.size());
    state.visibleCount = static_cast<int>(visiblePresetBrowserPresets.size());
    state.filterName = presetFilterBox.getText().trim();
    state.tagName = presetTagBox.getText().trim();
    state.searchText = presetSearchEditor.getText().trim();

    for (const auto& preset : library)
    {
        if (preset.isFavorite)
            ++state.favoriteCount;
        if (preset.rating > 0)
            ++state.ratedCount;
        if (preset.isFactory)
            ++state.factoryCount;
        else
            ++state.userCount;
        if (preset.source.equalsIgnoreCase("Generated") || preset.tags.containsIgnoreCase("Generated"))
            ++state.generatedCount;
        if (preset.macroIntensity >= 0.30f)
            ++state.macroRichCount;

        const auto folder = preset.folder.trim().isNotEmpty() ? preset.folder.trim() : preset.category.trim();
        if (folder.isNotEmpty())
            folders.addIfNotAlreadyThere(folder);
        if (preset.pack.trim().isNotEmpty())
            packs.addIfNotAlreadyThere(preset.pack.trim());

        const auto searchable = presetSearchText(preset);
        if (textContainsAny(searchable, { "UKG", "Garage", "House", "Tech House", "Techno", "Minimal", "Bass House", "Amapiano", "Hardgroove", "Future Garage", "Speed Garage", "Deep Tech", "Indie Dance", "Italo Disco", "Balearic House", "Acid House", "Nu Disco", "Afro Tech", "Afro Melodic", "Progressive House", "Hard House", "Peak Time Techno", "Detroit Techno", "Melodic Techno", "Minimal FM", "Deep Minimal", "Lo-Fi House", "French House", "Soulful House", "Garage House", "Microhouse", "Raw Techno", "Tribal Tech House", "Breaks House", "Chicago House", "Classic House", "Funky House", "Melodic House", "Romanian Minimal", "Dub Techno", "Electro Breaks" }))
            ++state.styleCount;
    }

    state.folderCount = folders.size();
    state.packCount = packs.size();
    presetCrateMapDisplay.setState(state);
}

void NateVSTAudioProcessorEditor::updatePresetLibrarySummary()
{
    UI::PresetLibrarySummary::State state;
    const auto library = audioProcessor.getPresetLibrary();
    state.totalCount = static_cast<int>(library.size());
    state.visibleCount = static_cast<int>(visiblePresetBrowserPresets.size());

    for (const auto& preset : library)
    {
        if (preset.isFavorite)
            ++state.favoriteCount;
        if (preset.rating > 0)
            ++state.ratedCount;
        if (preset.isFactory)
            ++state.factoryCount;
        else
            ++state.userCount;
    }

    const auto selectedName = presetBox.getText().trim();
    for (const auto& preset : library)
    {
        if (preset.name != selectedName)
            continue;

        state.hasSelection = true;
        state.selectedFavorite = preset.isFavorite;
        state.selectedFactory = preset.isFactory;
        state.selectedName = preset.name;
        state.selectedCategory = preset.folder.isNotEmpty() ? preset.folder : preset.category;
        state.selectedPack = preset.pack;
        state.selectedKey = preset.key;
        state.selectedBpm = formatPresetBpm(preset.bpm);
        state.selectedRating = preset.rating > 0 ? juce::String(preset.rating) + "/5" : juce::String("Unrated");
        state.selectedSource = preset.source;
        state.selectedNotes = preset.notes;
        state.selectedRole = presetInspectorRoleName(inferPresetAuditionRole(&preset));
        state.selectedTraits = presetInspectorTraits(preset);
        state.selectedCue = presetInspectorCue(preset);
        state.macroValues = preset.macroValues;
        state.profileValues = presetInspectorProfile(preset);
        break;
    }

    if (! state.hasSelection)
    {
        state.selectedName = state.totalCount > 0 ? "Choose a preset" : "No presets found";
        state.selectedCategory = "All folders";
        state.selectedPack = juce::String(state.totalCount) + " total";
        state.selectedRating = juce::String(state.favoriteCount) + " fav";
        state.selectedRole = "Library";
        state.selectedTraits = juce::String(state.ratedCount) + " rated | " + juce::String(state.factoryCount) + " factory";
        state.selectedCue = juce::String(state.visibleCount) + " visible | " + juce::String(state.totalCount) + " total";
    }

    presetLibrarySummary.setState(state);
    updatePresetCrateMapDisplay();
    updatePresetSaveSummary();
    updateHomeSessionDisplay();
}

void NateVSTAudioProcessorEditor::updatePresetSaveSummary()
{
    UI::PresetSaveSummary::State state;

    auto presetName = presetNameEditor.getText().trim();
    if (presetName.isEmpty())
        presetName = presetBox.getText().trim();

    state.hasName = presetName.isNotEmpty();
    state.name = state.hasName ? presetName : juce::String("Untitled patch");
    state.category = presetCategoryBox.getText().trim();
    state.author = presetAuthorEditor.getText().trim();
    state.pack = presetPackBox.getText().trim();
    state.key = presetKeyBox.getText().trim();
    state.bpm = presetBpmBox.getText().trim();
    state.notesCharacters = presetNotesEditor.getText().length();
    state.generated = currentPresetDraftIsGenerated;

    if (state.hasName)
    {
        state.overwriteExists = audioProcessor.userPresetExists(presetName, state.category);
        state.overwriteArmed = isPresetOverwriteArmed(presetName, state.category);
    }

    presetSaveSummary.setState(state);
}

void NateVSTAudioProcessorEditor::applyPresetQuickFilter(size_t index)
{
    auto filter = juce::String("All");
    auto tag = juce::String("All Tags");
    juce::String label = "All presets";

    switch (index)
    {
        case 1:
            filter = "Favorites";
            label = "Favorites";
            break;
        case 2:
            filter = "Recent";
            label = "Recent";
            break;
        case 3:
            filter = "Similar";
            label = "Similar presets";
            break;
        case 4:
            filter = "Bass";
            label = "Bass";
            break;
        case 5:
            filter = "Lead";
            label = "Lead";
            break;
        case 6:
            filter = "Chord";
            label = "Chord";
            break;
        case 7:
            filter = "Pad";
            label = "Pad";
            break;
        case 8:
            filter = "FX";
            label = "FX";
            break;
        case 9:
            filter = "Sequence";
            label = "Sequence";
            break;
        case 0:
        default:
            break;
    }

    presetFilterBox.setText(filter, juce::dontSendNotification);
    presetTagBox.setText(tag, juce::dontSendNotification);
    refreshPresetList();
    presetStatusLabel.setText("Quick filter: " + label, juce::dontSendNotification);
}

bool NateVSTAudioProcessorEditor::isPresetOverwriteArmed(const juce::String& presetName, const juce::String& category) const
{
    return pendingOverwritePresetName == presetName.trim()
        && pendingOverwriteCategory == category.trim()
        && juce::Time::getMillisecondCounterHiRes() <= pendingOverwriteUntilMs;
}

void NateVSTAudioProcessorEditor::armPresetOverwrite(const juce::String& presetName, const juce::String& category)
{
    pendingOverwritePresetName = presetName.trim();
    pendingOverwriteCategory = category.trim();
    pendingOverwriteUntilMs = juce::Time::getMillisecondCounterHiRes() + presetOverwriteConfirmMs;
    savePresetButton.setButtonText("Overwrite");
    savePresetButton.setTooltip("Press again to replace " + pendingOverwritePresetName + " in "
                                    + (pendingOverwriteCategory.isNotEmpty() ? pendingOverwriteCategory : juce::String("User")));
    updatePresetSaveSummary();
}

void NateVSTAudioProcessorEditor::clearPresetOverwriteWarning()
{
    pendingOverwritePresetName.clear();
    pendingOverwriteCategory.clear();
    pendingOverwriteUntilMs = 0.0;
    savePresetButton.setButtonText("Save");
    savePresetButton.setTooltip("Save the current patch into the selected category folder");
    updatePresetSaveSummary();
}

void NateVSTAudioProcessorEditor::saveCurrentPreset()
{
    releaseRandomCandidateAudition(false);

    auto presetName = presetNameEditor.getText().trim();
    if (presetName.isEmpty())
        presetName = presetBox.getText().trim();

    if (presetName.isEmpty())
    {
        clearPresetOverwriteWarning();
        presetStatusLabel.setText("Preset name required", juce::dontSendNotification);
        return;
    }

    NateVSTAudioProcessor::PresetSaveOptions options;
    options.category = presetCategoryBox.getText().trim();
    options.author = presetAuthorEditor.getText().trim();
    options.pack = presetPackBox.getText().trim();
    options.key = presetKeyBox.getText().trim();
    options.bpm = parsePresetBpm(presetBpmBox.getText());
    options.generated = currentPresetDraftIsGenerated;
    options.generatedRecipe = currentGeneratedPresetRecipe;
    if (options.generated)
    {
        options.notes = presetNotesIsRandomDraft
            ? generatedPresetNotes(options.category, options.generatedRecipe, audioProcessor.getActiveRandomCandidateIndex())
            : presetNotesEditor.getText().trim();
    }
    else
    {
        options.notes = presetNotesEditor.getText().trim();
    }

    if (audioProcessor.userPresetExists(presetName, options.category))
    {
        if (! isPresetOverwriteArmed(presetName, options.category))
        {
            armPresetOverwrite(presetName, options.category);
            presetStatusLabel.setText("Preset exists in "
                                          + (options.category.trim().isNotEmpty() ? options.category.trim() : juce::String("User"))
                                          + " | press Overwrite to replace",
                                      juce::dontSendNotification);
            return;
        }
    }
    else
    {
        clearPresetOverwriteWarning();
    }

    if (audioProcessor.savePreset(presetName, options))
    {
        clearPresetOverwriteWarning();
        auto storedName = juce::File::createLegalFileName(presetName);
        if (storedName.isEmpty())
            storedName = "Untitled";

        presetStatusLabel.setText(options.generated ? "Saved generated " + storedName : "Saved " + storedName,
                                  juce::dontSendNotification);
        refreshPresetList();
        presetBox.setText(storedName, juce::dontSendNotification);
        presetNameEditor.setText(storedName, juce::dontSendNotification);
        presetNameIsRandomDraft = false;
        presetNotesIsRandomDraft = false;
        currentPresetDraftIsGenerated = false;
        currentGeneratedPresetRecipe.clear();
        updateFavoritePresetButton();
        updatePresetSaveSummary();
        return;
    }

    clearPresetOverwriteWarning();
    presetStatusLabel.setText("Preset name required", juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::saveActiveRandomCandidatePreset()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto activeSlot = audioProcessor.getActiveRandomCandidateIndex();
    if (! audioProcessor.hasRandomCandidate(activeSlot))
    {
        presetStatusLabel.setText("Generate or cue a candidate first", juce::dontSendNotification);
        updateRandomCandidateButtons();
        return;
    }

    auto presetName = presetNameEditor.getText().trim();
    if (presetName.isEmpty())
    {
        presetName = "Candidate " + juce::String(activeSlot + 1) + " "
            + juce::String(static_cast<int>(juce::Time::getMillisecondCounter() % 1000000));
        presetNameEditor.setText(presetName, juce::dontSendNotification);
        presetNameIsRandomDraft = true;
    }

    NateVSTAudioProcessor::PresetSaveOptions options;
    options.category = presetCategoryBox.getText().trim();
    options.author = presetAuthorEditor.getText().trim();
    options.pack = presetPackBox.getText().trim();
    options.key = presetKeyBox.getText().trim();
    options.bpm = parsePresetBpm(presetBpmBox.getText());
    options.generated = true;
    options.generatedRecipe = currentGeneratedPresetRecipe.trim().isNotEmpty()
        ? currentGeneratedPresetRecipe
        : audioProcessor.getRandomCandidateSummary(activeSlot);
    options.notes = presetNotesIsRandomDraft || presetNotesEditor.getText().trim().isEmpty()
        ? generatedPresetNotes(options.category, options.generatedRecipe, activeSlot)
        : presetNotesEditor.getText().trim();

    if (audioProcessor.saveRandomCandidatePreset(activeSlot, presetName, options))
    {
        auto storedName = juce::File::createLegalFileName(presetName);
        if (storedName.isEmpty())
            storedName = "Untitled";

        const auto candidateRating = juce::jlimit(0, 5, candidateRatingBox.getSelectedId() - 1);
        const auto favoriteCandidate = candidateFavoriteButton.getToggleState();
        auto statusText = "Saved candidate " + storedName;

        if (audioProcessor.setPresetFavorite(storedName, favoriteCandidate) && favoriteCandidate)
            statusText += " | Starred";

        if (audioProcessor.setPresetRating(storedName, candidateRating) && candidateRating > 0)
            statusText += " | " + juce::String(candidateRating) + " star" + (candidateRating == 1 ? "" : "s");

        presetStatusLabel.setText(statusText, juce::dontSendNotification);
        refreshPresetList();
        presetBox.setText(storedName, juce::dontSendNotification);
        presetNameEditor.setText(storedName, juce::dontSendNotification);
        presetNameIsRandomDraft = false;
        presetNotesIsRandomDraft = false;
        currentPresetDraftIsGenerated = false;
        currentGeneratedPresetRecipe.clear();
        updateFavoritePresetButton();
        updateRandomCandidateButtons();
        return;
    }

    presetStatusLabel.setText("Candidate save skipped", juce::dontSendNotification);
    updateRandomCandidateButtons();
}

void NateVSTAudioProcessorEditor::loadSelectedPreset()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto presetName = presetBox.getText().trim();
    const auto canLoadPreset = presetName.isNotEmpty() && audioProcessor.getPresetNames().contains(presetName);
    if (canLoadPreset)
    {
        capturePresetCompareBefore(presetName);
        captureGlobalEdit("Load preset " + presetName);
    }

    if (audioProcessor.loadPreset(presetName))
    {
        capturePresetCompareLoaded();
        refreshPresetList();
        presetBox.setText(presetName, juce::dontSendNotification);
        presetNameEditor.setText(presetName, juce::dontSendNotification);
        for (const auto& preset : audioProcessor.getPresetLibrary())
        {
            if (preset.name == presetName)
            {
                presetNotesEditor.setText(preset.notes, juce::dontSendNotification);
                break;
            }
        }
        presetNameIsRandomDraft = false;
        presetNotesIsRandomDraft = false;
        currentPresetDraftIsGenerated = false;
        currentGeneratedPresetRecipe.clear();
        presetStatusLabel.setText("Loaded " + presetName + " | Compare ready", juce::dontSendNotification);
        updateFavoritePresetButton();
        updatePresetCompareButtons();
        updateSampleNameLabel();
        repaintSequencerGrids();
        return;
    }

    if (canLoadPreset)
        clearPresetCompareState();
    presetStatusLabel.setText("Select a preset to load", juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::loadPresetByOffset(int offset)
{
    const auto presetCount = presetBox.getNumItems();
    if (presetCount == 0)
    {
        presetStatusLabel.setText("No presets saved", juce::dontSendNotification);
        return;
    }

    auto selectedIndex = presetBox.getSelectedItemIndex();
    if (selectedIndex < 0)
        selectedIndex = 0;
    else
        selectedIndex = (selectedIndex + offset + presetCount) % presetCount;

    presetBox.setSelectedItemIndex(selectedIndex, juce::dontSendNotification);
    loadSelectedPreset();
}

void NateVSTAudioProcessorEditor::auditionSelectedPreset()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto presetName = presetBox.getText().trim();
    if (presetName.isEmpty())
    {
        presetStatusLabel.setText("Select a preset to audition", juce::dontSendNotification);
        return;
    }

    NateVSTAudioProcessor::PresetInfo auditionPresetInfo;
    auto hasAuditionPresetInfo = false;
    for (const auto& preset : audioProcessor.getPresetLibrary())
    {
        if (preset.name != presetName)
            continue;

        auditionPresetInfo = preset;
        hasAuditionPresetInfo = true;
        break;
    }

    if (! audioProcessor.startPresetPreviewPlayback(presetName))
    {
        presetStatusLabel.setText("Preview render failed for " + presetName, juce::dontSendNotification);
        return;
    }

    refreshPresetList();
    presetBox.setText(presetName, juce::dontSendNotification);
    presetNameEditor.setText(presetName, juce::dontSendNotification);
    updateFavoritePresetButton();

    const auto previewInfo = audioProcessor.getPresetPreviewInfo(presetName);
    const auto roleText = hasAuditionPresetInfo ? presetInspectorRoleName(inferPresetAuditionRole(&auditionPresetInfo))
                                                : juce::String("Preset");
    presetStatusLabel.setText("Previewing " + presetName + " | " + roleText + " | "
                                  + (previewInfo.status.isNotEmpty() ? previewInfo.status : juce::String("Rendered preview")),
                              juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::warmVisiblePresetPreviews()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();
    audioProcessor.stopPresetPreviewPlayback();

    static constexpr auto maxWarmCount = 8;
    const auto selectedName = presetBox.getText().trim();
    juce::StringArray presetNames;
    if (selectedName.isNotEmpty())
        presetNames.addIfNotAlreadyThere(selectedName);

    for (const auto& preset : visiblePresetBrowserPresets)
    {
        if (presetNames.size() >= maxWarmCount)
            break;

        const auto ready = preset.previewAvailable
            && ! preset.previewStale
            && preset.previewPeak > 0.001f
            && preset.previewRms > 0.00005f
            && std::isfinite(preset.previewPeak)
            && std::isfinite(preset.previewRms);

        if (! ready)
            presetNames.addIfNotAlreadyThere(preset.name);
    }

    if (presetNames.isEmpty())
    {
        for (const auto& preset : visiblePresetBrowserPresets)
        {
            presetNames.addIfNotAlreadyThere(preset.name);
            if (presetNames.size() >= maxWarmCount)
                break;
        }
    }

    if (presetNames.isEmpty())
    {
        presetStatusLabel.setText("No visible presets to warm", juce::dontSendNotification);
        return;
    }

    const auto result = audioProcessor.ensurePresetPreviews(presetNames, false);
    refreshPresetList();
    if (selectedName.isNotEmpty())
    {
        presetBox.setText(selectedName, juce::dontSendNotification);
        presetNameEditor.setText(selectedName, juce::dontSendNotification);
    }

    updateFavoritePresetButton();
    presetStatusLabel.setText(result.status + " | Warmed visible crate previews", juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::updatePresetAudition()
{
    if (presetAuditionNotes.empty())
        return;

    const auto nowMs = juce::Time::getMillisecondCounterHiRes();
    auto& keyboardState = audioProcessor.getMidiKeyboardState();

    for (auto& event : presetAuditionNotes)
    {
        if (! event.started && nowMs >= event.startMs)
        {
            keyboardState.noteOn(1, event.note, event.velocity);
            event.started = true;
        }

        if (event.started && ! event.stopped && nowMs >= event.stopMs)
        {
            keyboardState.noteOff(1, event.note, 0.0f);
            event.stopped = true;
        }
    }

    presetAuditionNotes.erase(std::remove_if(presetAuditionNotes.begin(),
                                             presetAuditionNotes.end(),
                                             [] (const auto& event) { return event.stopped; }),
                              presetAuditionNotes.end());
}

void NateVSTAudioProcessorEditor::releasePresetAuditionNote()
{
    audioProcessor.stopPresetPreviewPlayback();

    if (presetAuditionNotes.empty())
        return;

    auto& keyboardState = audioProcessor.getMidiKeyboardState();
    for (auto& event : presetAuditionNotes)
        if (event.started && ! event.stopped)
            keyboardState.noteOff(1, event.note, 0.0f);

    presetAuditionNotes.clear();
}

juce::String NateVSTAudioProcessorEditor::startPresetAuditionPhrase(const NateVSTAudioProcessor::PresetInfo* presetInfo, int rootNote)
{
    presetAuditionNotes.clear();

    const auto role = inferPresetAuditionRole(presetInfo);
    const auto nowMs = juce::Time::getMillisecondCounterHiRes();

    auto addNote = [this, nowMs] (int note, double startOffsetMs, double durationMs, float velocity)
    {
        PresetAuditionNote event;
        event.note = juce::jlimit(0, 127, note);
        event.startMs = nowMs + juce::jmax(0.0, startOffsetMs);
        event.stopMs = event.startMs + juce::jmax(60.0, durationMs);
        event.velocity = juce::jlimit(0.05f, 1.0f, velocity);
        presetAuditionNotes.push_back(event);
    };

    switch (role)
    {
        case PresetAuditionRole::bass:
        {
            const auto base = foldMidiNoteToRange(rootNote, 36, 48);
            addNote(base, 0.0, 150.0, 0.92f);
            addNote(foldMidiNoteToRange(base + 7, 36, 52), 210.0, 130.0, 0.74f);
            addNote(base, 390.0, 145.0, 0.86f);
            addNote(foldMidiNoteToRange(base + 10, 36, 52), 610.0, 150.0, 0.78f);
            addNote(foldMidiNoteToRange(base + 12, 36, 52), 850.0, 260.0, 0.90f);
            break;
        }

        case PresetAuditionRole::chord:
        {
            const auto base = foldMidiNoteToRange(rootNote, 48, 60);
            for (const auto interval : { 0, 3, 7, 10 })
                addNote(foldMidiNoteToRange(base + interval, 48, 76), 0.0, 240.0, 0.78f);
            for (const auto interval : { 5, 8, 12 })
                addNote(foldMidiNoteToRange(base + interval, 48, 76), 420.0, 210.0, 0.68f);
            for (const auto interval : { 7, 10, 14 })
                addNote(foldMidiNoteToRange(base + interval, 48, 76), 780.0, 300.0, 0.72f);
            break;
        }

        case PresetAuditionRole::chop:
        {
            const auto base = foldMidiNoteToRange(rootNote + 12, 60, 72);
            addNote(base, 0.0, 95.0, 0.82f);
            addNote(foldMidiNoteToRange(base + 3, 60, 76), 145.0, 90.0, 0.70f);
            addNote(foldMidiNoteToRange(base + 7, 60, 76), 310.0, 110.0, 0.80f);
            addNote(foldMidiNoteToRange(base + 10, 60, 76), 500.0, 95.0, 0.68f);
            addNote(base, 650.0, 170.0, 0.86f);
            break;
        }

        case PresetAuditionRole::sequence:
        {
            const auto base = foldMidiNoteToRange(rootNote, 48, 60);
            const std::array<int, 6> intervals { 0, 7, 10, 12, 7, 15 };
            for (size_t index = 0; index < intervals.size(); ++index)
                addNote(foldMidiNoteToRange(base + intervals[index], 48, 76),
                        static_cast<double>(index) * 165.0,
                        105.0,
                        index % 2 == 0 ? 0.82f : 0.70f);
            break;
        }

        case PresetAuditionRole::fx:
        {
            const auto note = foldMidiNoteToRange(rootNote + 24, 72, 84);
            addNote(note, 0.0, 1180.0, 0.78f);
            addNote(foldMidiNoteToRange(note - 12, 60, 72), 360.0, 520.0, 0.46f);
            break;
        }

        case PresetAuditionRole::lead:
        case PresetAuditionRole::pluck:
        {
            const auto base = foldMidiNoteToRange(rootNote + 12, 60, 72);
            const std::array<int, 5> intervals { 0, 3, 7, 10, 12 };
            for (size_t index = 0; index < intervals.size(); ++index)
                addNote(foldMidiNoteToRange(base + intervals[index], 60, 84),
                        static_cast<double>(index) * 170.0,
                        role == PresetAuditionRole::pluck ? 105.0 : 180.0,
                        index == 0 ? 0.84f : 0.72f);
            break;
        }

        case PresetAuditionRole::general:
        default:
        {
            const auto base = foldMidiNoteToRange(rootNote, 48, 72);
            addNote(base, 0.0, 260.0, presetAuditionVelocity);
            addNote(foldMidiNoteToRange(base + 7, 48, 76), 360.0, 220.0, 0.72f);
            addNote(foldMidiNoteToRange(base + 12, 48, 84), 700.0, 280.0, 0.78f);
            break;
        }
    }

    updatePresetAudition();
    return presetAuditionRoleName(role);
}

bool NateVSTAudioProcessorEditor::hasPresetCompareSnapshots() const
{
    return presetCompareBeforeSnapshot.getSize() > 0 && presetCompareLoadedSnapshot.getSize() > 0;
}

bool NateVSTAudioProcessorEditor::capturePresetCompareBefore(const juce::String& presetName)
{
    juce::MemoryBlock snapshot;
    audioProcessor.getStateInformation(snapshot);

    if (snapshot.getSize() == 0)
    {
        clearPresetCompareState();
        return false;
    }

    presetCompareBeforeSnapshot = snapshot;
    presetCompareLoadedSnapshot.setSize(0);
    presetCompareName = presetName;
    presetCompareShowingLoaded = true;
    updatePresetCompareButtons();
    return true;
}

void NateVSTAudioProcessorEditor::capturePresetCompareLoaded()
{
    if (presetCompareBeforeSnapshot.getSize() == 0)
    {
        clearPresetCompareState();
        return;
    }

    juce::MemoryBlock snapshot;
    audioProcessor.getStateInformation(snapshot);

    if (snapshot.getSize() == 0)
    {
        clearPresetCompareState();
        return;
    }

    presetCompareLoadedSnapshot = snapshot;
    presetCompareShowingLoaded = true;
    updatePresetCompareButtons();
}

void NateVSTAudioProcessorEditor::clearPresetCompareState()
{
    presetCompareBeforeSnapshot.setSize(0);
    presetCompareLoadedSnapshot.setSize(0);
    presetCompareName.clear();
    presetCompareShowingLoaded = true;
    updatePresetCompareButtons();
}

void NateVSTAudioProcessorEditor::updatePresetCompareButtons()
{
    const auto hasSnapshots = hasPresetCompareSnapshots();
    comparePresetButton.setEnabled(hasSnapshots);
    revertPresetButton.setEnabled(hasSnapshots);
    comparePresetButton.setButtonText(presetCompareShowingLoaded ? "Before" : "Loaded");

    if (! hasSnapshots)
    {
        comparePresetButton.setTooltip("Load a preset to compare it against the sound that was active before loading");
        revertPresetButton.setTooltip("Restore becomes available after a Library preset load");
        updateHomeSessionDisplay();
        return;
    }

    comparePresetButton.setTooltip(presetCompareShowingLoaded
                                       ? "Hear the sound that was active before loading " + presetCompareName
                                       : "Return to the loaded preset " + presetCompareName);
    revertPresetButton.setTooltip("Restore the sound from before loading " + presetCompareName + " and clear this compare pair");
    updateHomeSessionDisplay();
}

void NateVSTAudioProcessorEditor::togglePresetCompare()
{
    if (! hasPresetCompareSnapshots())
    {
        presetStatusLabel.setText("Load a preset first to compare", juce::dontSendNotification);
        updatePresetCompareButtons();
        return;
    }

    if (presetCompareShowingLoaded)
    {
        presetCompareShowingLoaded = false;
        restorePresetCompareSnapshot(presetCompareBeforeSnapshot, "Comparing previous sound before " + presetCompareName);
        return;
    }

    presetCompareShowingLoaded = true;
    restorePresetCompareSnapshot(presetCompareLoadedSnapshot, "Comparing loaded " + presetCompareName);
}

void NateVSTAudioProcessorEditor::revertPresetCompare()
{
    if (! hasPresetCompareSnapshots())
    {
        presetStatusLabel.setText("No preset load to revert", juce::dontSendNotification);
        updatePresetCompareButtons();
        return;
    }

    const auto presetName = presetCompareName;
    juce::MemoryBlock snapshot = presetCompareBeforeSnapshot;
    clearPresetCompareState();
    restorePresetCompareSnapshot(snapshot, "Reverted load of " + presetName);
}

void NateVSTAudioProcessorEditor::restorePresetCompareSnapshot(const juce::MemoryBlock& snapshot, const juce::String& statusText)
{
    if (snapshot.getSize() == 0)
    {
        clearPresetCompareState();
        presetStatusLabel.setText("Preset compare state unavailable", juce::dontSendNotification);
        return;
    }

    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();
    audioProcessor.setStateInformation(snapshot.getData(), static_cast<int>(snapshot.getSize()));
    refreshAfterGlobalEditRestore(statusText);
    presetStatusLabel.setText(statusText, juce::dontSendNotification);
    updatePresetCompareButtons();
}

void NateVSTAudioProcessorEditor::toggleFavoritePreset()
{
    const auto presetName = presetBox.getText().trim();
    if (presetName.isEmpty())
    {
        presetStatusLabel.setText("Select a preset to favorite", juce::dontSendNotification);
        return;
    }

    const auto shouldBeFavorite = ! audioProcessor.isPresetFavorite(presetName);
    if (! audioProcessor.setPresetFavorite(presetName, shouldBeFavorite))
    {
        presetStatusLabel.setText("Favorite update failed", juce::dontSendNotification);
        return;
    }

    refreshPresetList();
    if (shouldBeFavorite || presetFilterBox.getText() != "Favorites")
        presetBox.setText(presetName, juce::dontSendNotification);

    updateFavoritePresetButton();
    presetStatusLabel.setText(juce::String(shouldBeFavorite ? "Favorited " : "Unfavorited ") + presetName,
                              juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::setSelectedPresetRating()
{
    const auto presetName = presetBox.getText().trim();
    if (presetName.isEmpty())
        return;

    const auto selectedId = presetRatingBox.getSelectedId();
    if (selectedId <= 0)
        return;

    const auto rating = juce::jlimit(0, 5, selectedId - 1);
    if (! audioProcessor.setPresetRating(presetName, rating))
    {
        presetStatusLabel.setText("Rating update failed", juce::dontSendNotification);
        return;
    }

    refreshPresetList();
    if ((rating > 0 || presetFilterBox.getText() != "Rated")
        && (rating == 5 || presetFilterBox.getText() != "5 Stars")
        && (rating >= 4 || presetFilterBox.getText() != "4+ Stars"))
    {
        presetBox.setText(presetName, juce::dontSendNotification);
    }

    updateFavoritePresetButton();
    presetStatusLabel.setText(rating > 0
                                  ? "Rated " + presetName + " | " + juce::String(rating) + " star" + (rating == 1 ? "" : "s")
                                  : "Cleared rating for " + presetName,
                              juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::updateFavoritePresetButton()
{
    const auto presetName = presetBox.getText().trim();
    auditionPresetButton.setEnabled(presetName.isNotEmpty());
    warmPresetPreviewsButton.setEnabled(! visiblePresetBrowserPresets.empty());
    favoritePresetButton.setEnabled(presetName.isNotEmpty());
    favoritePresetButton.setToggleState(presetName.isNotEmpty() && audioProcessor.isPresetFavorite(presetName),
                                        juce::dontSendNotification);
    presetRatingBox.setEnabled(presetName.isNotEmpty());
    presetRatingBox.setSelectedId(presetName.isNotEmpty() ? audioProcessor.getPresetRating(presetName) + 1 : 1,
                                  juce::dontSendNotification);

    if (presetName.isEmpty())
    {
        updatePresetSaveSummary();
        return;
    }

    for (const auto& preset : audioProcessor.getPresetLibrary())
    {
        if (preset.name != presetName)
            continue;

        presetCategoryBox.setText(preset.folder.isNotEmpty() ? preset.folder : preset.category,
                                  juce::dontSendNotification);
        presetAuthorEditor.setText(preset.author, juce::dontSendNotification);
        presetPackBox.setText(preset.pack, juce::dontSendNotification);
        presetKeyBox.setText(preset.key, juce::dontSendNotification);
        presetBpmBox.setText(formatPresetBpm(preset.bpm), juce::dontSendNotification);
        const auto preview = presetMacroPreviewText(preset);
        const auto renderedPreview = presetPreviewSummaryText(preset);
        presetBox.setTooltip(preset.name + " | " + renderedPreview + " | " + preview);
        auditionPresetButton.setTooltip("Play rendered audio preview for " + preset.name + " without loading the patch");
        warmPresetPreviewsButton.setTooltip("Render previews for the selected preset and visible crate rows");
        presetStatusLabel.setTooltip(renderedPreview + " | " + preview + " | " + preset.pack + " | " + preset.key + " | " + formatPresetBpm(preset.bpm));
        break;
    }

    updatePresetLibrarySummary();
    updatePresetSaveSummary();
}
