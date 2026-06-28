#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <typeinfo>
#include <utility>
#include <vector>

namespace
{
constexpr auto editorMinWidth = 940;
constexpr auto editorMinHeight = 710;
constexpr auto editorDefaultWidth = 1040;
constexpr auto editorDefaultHeight = 760;
constexpr auto editorMaxWidth = 1440;
constexpr auto editorMaxHeight = 980;
constexpr auto pianoKeyboardHeight = 58;
constexpr auto keyboardControlsWidth = 214;
constexpr auto keyboardLowestNote = 24;
constexpr auto keyboardHighestNote = 96;
constexpr auto keyboardInitialLowestNote = 36;
constexpr auto keyboardMaxLowestVisibleNote = 84;
constexpr auto modTopRowHeight = 148;
constexpr auto modGeneratorRowHeight = 158;
constexpr auto modPanelGap = 6;
constexpr auto firstMacroModSourceIndex = 4;
constexpr auto presetAuditionDurationMs = 720.0;
constexpr auto presetAuditionVelocity = 0.86f;
constexpr auto candidateAuditionDurationMs = 680.0;
constexpr auto candidateAuditionVelocity = 0.88f;
constexpr auto fxRackStatusOverrideMs = 2200.0;
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

juce::Colour backgroundColour()
{
    return juce::Colour(0xff0d1113);
}

juce::Colour panelColour()
{
    return juce::Colour(0xff141a1d);
}

juce::StringArray presetCategoryChoices()
{
    return {
        "User",
        "Bass",
        "Stab",
        "Lead",
        "House",
        "House/Bass",
        "House/Chords",
        "Tech House",
        "Tech House/Bass",
        "Techno",
        "Techno/Stabs",
        "Minimal",
        "Minimal/Plucks",
        "UKG",
        "UKG/Bass",
        "UKG/Chops",
        "UKG/Organ",
        "UKG/Stabs",
        "UKG/Bells",
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
        "Bass",
        "Stab",
        "Lead",
        "House",
        "Tech House",
        "Techno",
        "Minimal",
        "UKG",
        "FX",
        "Sequence",
        "Sample",
        "Project Pack",
        "Factory Pack",
        "UKG Essentials",
        "UKG Basslines",
        "Garage Chops",
        "House Tools",
        "Tech House Tools",
        "Minimal Tools",
        "Techno Tools",
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
        "House Tools",
        "Tech House Tools",
        "Minimal Tools",
        "Techno Tools",
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

void drawPresetMacroValueStrip(juce::Graphics& g,
                               juce::Rectangle<int> area,
                               const NateVSTAudioProcessor::PresetInfo& preset)
{
    static constexpr std::array<const char*, 8> labels { "T", "D", "M", "S", "W", "B", "Wr", "Th" };

    area = area.reduced(2, 2);
    if (area.getWidth() <= 0 || area.getHeight() <= 0)
        return;

    const auto cellWidth = juce::jmax(16, area.getWidth() / static_cast<int>(labels.size()));
    g.setFont(juce::FontOptions(area.getWidth() < 182 ? 8.2f : 8.8f, juce::Font::bold));

    for (size_t index = 0; index < labels.size(); ++index)
    {
        auto cell = area.removeFromLeft(index + 1 == labels.size() ? area.getWidth() : cellWidth).reduced(1, 1);
        if (cell.getWidth() <= 0)
            continue;

        const auto value = juce::jlimit(0.0f, 1.0f, preset.macroValues[index]);
        const auto fill = juce::Colour(0xff8ee6c9).interpolatedWith(juce::Colour(0xffffd27a), juce::jlimit(0.0f, 1.0f, value * 0.8f));
        auto meter = cell.reduced(1, 1).toFloat();
        const auto activeHeight = meter.getHeight() * value;

        g.setColour(juce::Colour(0xff172024));
        g.fillRoundedRectangle(cell.toFloat(), 3.0f);

        if (activeHeight > 0.5f)
        {
            auto active = meter.withY(meter.getBottom() - activeHeight).withHeight(activeHeight);
            g.setColour(fill.withAlpha(0.62f));
            g.fillRoundedRectangle(active, 2.0f);
        }

        g.setColour(value >= 0.30f ? fill : juce::Colour(0xff728083));
        g.drawRoundedRectangle(cell.toFloat(), 3.0f, value >= 0.30f ? 1.1f : 0.8f);
        g.setColour(value >= 0.30f ? juce::Colour(0xffedf7f4) : juce::Colour(0xffa8b6b8));
        g.drawFittedText(juce::String(labels[index]) + juce::String(juce::roundToInt(value * 100.0f)),
                         cell.reduced(1, 0),
                         juce::Justification::centred,
                         1,
                         0.42f);
    }
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
        "House",
        "Tech House",
        "Techno",
        "Minimal",
        "UKG"
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
    static const std::array<const char*, 15> sourceTexts {
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
        "LFO 2: secondary groove motion"
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
    static const std::array<RandomRecipeInfo, 11> recipes {
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
        RandomRecipeInfo { "UKG Dred Bass", "UKG / Speed Garage", "132-136 BPM", "Dred/Reese bass, darker 2-step pressure", "mono sub, detuned upper bass, slow filter pull" }
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
      pianoKeyboard(processorToUse.getMidiKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&lookAndFeel);
    setSize(editorDefaultWidth, editorDefaultHeight);
    setResizeLimits(editorMinWidth, editorMinHeight, editorMaxWidth, editorMaxHeight);
    setResizable(true, true);

    titleLabel.setText("Nate VST", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffedf7f4));
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(outputMeter);
    addAndMakeVisible(outputSpectrumDisplay);
    addAndMakeVisible(homeOverviewDisplay);
    addAndMakeVisible(lowEndAssistant);

    pianoKeyboard.setAvailableRange(keyboardLowestNote, keyboardHighestNote);
    pianoKeyboard.setLowestVisibleKey(keyboardInitialLowestNote);
    pianoKeyboard.setKeyWidth(18.0f);
    pianoKeyboard.setScrollButtonsVisible(true);
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffd9e3df));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff151b1f));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xff2a363c));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour(0x338ee6c9));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0xaa8ee6c9));
    pianoKeyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, juce::Colour(0xff253037));
    addAndMakeVisible(pianoKeyboard);

    keyboardOctaveDownButton.setTooltip("Shift the audition keyboard down one octave");
    keyboardOctaveDownButton.onClick = [this] { shiftKeyboardOctave(-12); };
    addAndMakeVisible(keyboardOctaveDownButton);

    keyboardOctaveUpButton.setTooltip("Shift the audition keyboard up one octave");
    keyboardOctaveUpButton.onClick = [this] { shiftKeyboardOctave(12); };
    addAndMakeVisible(keyboardOctaveUpButton);

    keyboardPanicButton.setTooltip("Stop held keyboard, chord-memory, synth, and sample voices");
    keyboardPanicButton.onClick = [this]
    {
        releasePresetAuditionNote();
        audioProcessor.panicAllNotesOff();
        setRandomStatus("Panic: all notes off");
    };
    addAndMakeVisible(keyboardPanicButton);

    keyboardRangeLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    keyboardRangeLabel.setJustificationType(juce::Justification::centred);
    keyboardRangeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdce7e4));
    addAndMakeVisible(keyboardRangeLabel);
    updateKeyboardRangeLabel();

    configureSectionLabel(homeSectionLabel, "HOME");
    configureSectionLabel(homeEngineLabel, "PERFORM");
    configureSectionLabel(homeShapeLabel, "MACROS");
    configureSectionLabel(homeLabLabel, "RANDOM LAB");
    configureSectionLabel(homeLibraryLabel, "PRESET RECALL");
    configureSectionLabel(synthSectionLabel, "SYNTH");
    configureSectionLabel(synthSourceLabel, "SOURCE MIX");
    configureSectionLabel(synthVoiceLabel, "PITCH + VOICE");
    configureSectionLabel(synthFilterLabel, "FILTER DRIVE");
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
    configureSectionLabel(libraryFindLabel, "FIND");
    configureSectionLabel(libraryBrowserLabel, "BROWSER");
    configureSectionLabel(librarySaveLabel, "SAVE PATCH");
    configureSectionLabel(libraryInspectorLabel, "INSPECT");
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

    sampleSliceStatusLabel.setText("S1 default | 0-13% | pitch 0 st | gain -6 dB", juce::dontSendNotification);
    sampleSliceStatusLabel.setJustificationType(juce::Justification::centredLeft);
    sampleSliceStatusLabel.setFont(juce::FontOptions(11.0f));
    sampleSliceStatusLabel.setMinimumHorizontalScale(0.62f);
    sampleSliceStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    sampleSliceStatusLabel.setTooltip("Selected slice memory: store or recall region, pitch, gain, reverse, choke, and stutter edits");
    addAndMakeVisible(sampleSliceStatusLabel);

    presetStatusLabel.setJustificationType(juce::Justification::centredLeft);
    presetStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(presetStatusLabel);

    presetBrowserHeaderLabel.setText("PRESET CRATE        SOURCE / KEY / BPM / RATING        PERFORMANCE MACROS", juce::dontSendNotification);
    presetBrowserHeaderLabel.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    presetBrowserHeaderLabel.setJustificationType(juce::Justification::centredLeft);
    presetBrowserHeaderLabel.setMinimumHorizontalScale(0.58f);
    presetBrowserHeaderLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8ee6c9));
    presetBrowserHeaderLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff101619));
    presetBrowserHeaderLabel.setTooltip("Visible preset browser columns");
    addAndMakeVisible(presetBrowserHeaderLabel);

    presetBrowserList.setModel(this);
    presetBrowserList.setRowHeight(38);
    presetBrowserList.setMultipleSelectionEnabled(false);
    presetBrowserList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff101619));
    presetBrowserList.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff344047));
    presetBrowserList.setColour(juce::ListBox::textColourId, juce::Colour(0xffdce7e4));
    presetBrowserList.setOutlineThickness(1);
    presetBrowserList.setTooltip("Click a preset row to select it. Double-click a row to load it.");
    addAndMakeVisible(presetBrowserList);
    addAndMakeVisible(presetLibrarySummary);

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
    };
    addAndMakeVisible(performanceXYPad);

    sampleWaveformDisplay.onRangeChange = [this] (float start, float end)
    {
        setPlainParameterValue(Parameters::ID::sampleStart, start);
        setPlainParameterValue(Parameters::ID::sampleEnd, end);
        setPlainParameterValue(Parameters::ID::sampleEnabled, 1.0f);
        updateSampleSliceButtons();
    };
    addAndMakeVisible(sampleWaveformDisplay);
    addAndMakeVisible(wavetableDisplay);
    addAndMakeVisible(filterResponseDisplay);

    fxRackStatusLabel.setJustificationType(juce::Justification::centredLeft);
    fxRackStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
    addAndMakeVisible(fxRackStatusLabel);

    for (size_t index = 0; index < modSourceRows.size(); ++index)
    {
        auto& meter = modSourceRows[index];
        meter.setComponentID("ModSource" + juce::String(static_cast<int>(index + 1)));
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

    presetNameEditor.setTextToShowWhenEmpty("Preset name", juce::Colour(0xff617078));
    presetNameEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff101619));
    presetNameEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff344047));
    presetNameEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff8ee6c9));
    presetNameEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xffdce7e4));
    presetNameEditor.onTextChange = [this] { presetNameIsRandomDraft = false; };
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
    presetNotesEditor.onTextChange = [this] { presetNotesIsRandomDraft = false; };
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
    addAndMakeVisible(waveformBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::oscWave, waveformBox));

    osc2WaveBox.addItemList(Parameters::waveformChoices(), 1);
    addAndMakeVisible(osc2WaveBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::osc2Wave, osc2WaveBox));

    noiseTypeBox.addItemList(Parameters::noiseTypeChoices(), 1);
    noiseTypeBox.setTextWhenNothingSelected("Noise Type");
    noiseTypeBox.setTooltip("Choose the noise source color for attack ticks, air, vinyl texture, and digital grit");
    addAndMakeVisible(noiseTypeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::noiseType, noiseTypeBox));

    filterModeBox.addItemList(Parameters::filterModeChoices(), 1);
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
    addAndMakeVisible(recipeBox);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.getValueTreeState(), Parameters::ID::randomRecipe, recipeBox));
    recipeBox.onChange = [this] { updateRandomRecipeInfo(); };

    randomScopeBox.addItemList({ "All", "Source", "Env", "Filter", "Sample", "FX", "Seq", "Macros" }, 1);
    randomScopeBox.setSelectedId(1, juce::dontSendNotification);
    randomScopeBox.setTextWhenNothingSelected("Scope");
    randomScopeBox.setTooltip("Limit Generate, Vary, Mutate, and Wild to one patch section");
    addAndMakeVisible(randomScopeBox);

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
    sequencerGrooveTransformBox.setSelectedId(1, juce::dontSendNotification);
    sequencerGrooveTransformBox.setTooltip("Choose a timing transform or genre groove template for the current sequence");
    addAndMakeVisible(sequencerGrooveTransformBox);

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
    addAndMakeVisible(presetCategoryBox);

    presetFilterBox.addItemList(presetFilterChoices(), 1);
    presetFilterBox.setSelectedItemIndex(0, juce::dontSendNotification);
    addAndMakeVisible(presetFilterBox);

    presetTagBox.addItemList(presetTagChoices(), 1);
    presetTagBox.setSelectedItemIndex(0, juce::dontSendNotification);
    addAndMakeVisible(presetTagBox);

    presetSortBox.addItemList(presetSortChoices(), 1);
    presetSortBox.setSelectedItemIndex(0, juce::dontSendNotification);
    presetSortBox.setTextWhenNothingSelected("Sort");
    presetSortBox.setTooltip("Sort the visible library by name, rating, newest save, category, or source");
    addAndMakeVisible(presetSortBox);

    presetRatingBox.addItem("No Stars", 1);
    presetRatingBox.addItem("1 Star", 2);
    presetRatingBox.addItem("2 Stars", 3);
    presetRatingBox.addItem("3 Stars", 4);
    presetRatingBox.addItem("4 Stars", 5);
    presetRatingBox.addItem("5 Stars", 6);
    presetRatingBox.setSelectedId(1, juce::dontSendNotification);
    presetRatingBox.setTextWhenNothingSelected("Rating");
    presetRatingBox.setTooltip("Rate the selected preset from 1 to 5 stars");
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
    addAndMakeVisible(presetPackBox);

    presetKeyBox.addItemList(presetKeyChoices(), 1);
    presetKeyBox.setSelectedId(1, juce::dontSendNotification);
    presetKeyBox.setEditableText(true);
    presetKeyBox.setTextWhenNothingSelected("Key");
    presetKeyBox.setTooltip("Store the musical key for browser search and sorting");
    addAndMakeVisible(presetKeyBox);

    presetBpmBox.addItemList(presetBpmChoices(), 1);
    presetBpmBox.setSelectedId(7, juce::dontSendNotification);
    presetBpmBox.setEditableText(true);
    presetBpmBox.setTextWhenNothingSelected("BPM");
    presetBpmBox.setTooltip("Store the target tempo for browser search, filtering, and sorting");
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
        configureHorizontalSlider(modAmountSliders[index],
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
        sequencerGrid.repaint();
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
        sequencerGrid.repaint();
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
            sequencerGrid.repaint();
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
            sequencerGrid.repaint();
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
    };
    randomCutButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        captureGlobalEdit("Random sample cut");
        setRandomStatus(audioProcessor.randomizeSampleCut() ? "Sample randomized" : "Sample skipped");
        updateSampleWaveformDisplay();
    };
    ukgChopButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        captureGlobalEdit("UKG chop");
        if (audioProcessor.randomizeUkgVocalChop())
        {
            sequencerGrid.repaint();
            updateSampleWaveformDisplay();
            setRandomStatus("UKG chop ready");
        }
        else
        {
            setRandomStatus("UKG chop skipped");
        }
    };
    for (size_t index = 0; index < sampleSliceButtons.size(); ++index)
    {
        auto& button = sampleSliceButtons[index];
        button.setButtonText(juce::String(static_cast<int>(index + 1)));
        button.setTooltip("Select and audition sample slice " + juce::String(static_cast<int>(index + 1)));
        button.onClick = [this, index] { selectSampleSlice(index); };
        addAndMakeVisible(button);
    }
    sampleSliceStoreButton.setTooltip("Store the current sample region, pitch, gain, pan, chance, reverse, choke, and stutter settings into this slice");
    sampleSliceStoreButton.onClick = [this] { storeSelectedSampleSliceSettings(); };
    sampleSliceRecallButton.setTooltip("Recall this slice's stored region, pitch, gain, pan, chance, reverse, choke, and stutter settings");
    sampleSliceRecallButton.onClick = [this] { recallSelectedSampleSliceSettings(); };
    sampleSliceDiceButton.setTooltip("Create a UKG-style random edit for this slice and store it");
    sampleSliceDiceButton.onClick = [this] { randomizeSelectedSampleSliceSettings(); };
    sampleSliceReverseEditButton.setTooltip("Toggle reverse for the selected slice and store it");
    sampleSliceReverseEditButton.onClick = [this] { toggleSelectedSampleSliceReverse(); };
    sampleSliceChokeButton.setTooltip("Toggle slice-key choke for this slice, so choked slices cut each other off in Slice Keys mode");
    sampleSliceChokeButton.onClick = [this] { toggleSelectedSampleSliceChoke(); };
    sampleSlicePanButton.setTooltip("Cycle this slice between center, left, and right pan for Slice Keys playback");
    sampleSlicePanButton.onClick = [this] { cycleSelectedSampleSlicePan(); };
    sampleSliceGhostButton.setTooltip("Toggle ghost probability for this slice so Slice Keys can skip it sometimes");
    sampleSliceGhostButton.onClick = [this] { toggleSelectedSampleSliceGhost(); };
    randomSequencerButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        if (audioProcessor.randomizeSequencerPattern())
        {
            sequencerGrid.repaint();
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
            sequencerGrid.repaint();
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
            sequencerGrid.repaint();
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
        sequencerGrid.repaint();
    };
    bassPatternButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        audioProcessor.applySequencerPatternPreset(0);
        sequencerGrid.repaint();
    };
    stabPatternButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        audioProcessor.applySequencerPatternPreset(1);
        sequencerGrid.repaint();
    };
    ukgPatternButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        audioProcessor.applySequencerPatternPreset(2);
        sequencerGrid.repaint();
    };
    applyPatternButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        const auto selectedId = sequencerPatternBox.getSelectedId();
        audioProcessor.applySequencerPatternPreset(juce::jmax(1, selectedId) - 1);
        sequencerGrid.repaint();
    };
    copySequencerButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        audioProcessor.copySequencerFirstHalfToSecondHalf();
        sequencerGrid.repaint();
    };
    rotateSequencerLeftButton.setTooltip("Shift the whole sequencer pattern one step earlier");
    rotateSequencerLeftButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        audioProcessor.rotateSequencerPattern(-1);
        sequencerGrid.repaint();
        setRandomStatus("Sequence rotated left");
    };
    rotateSequencerRightButton.setTooltip("Shift the whole sequencer pattern one step later");
    rotateSequencerRightButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        audioProcessor.rotateSequencerPattern(1);
        sequencerGrid.repaint();
        setRandomStatus("Sequence rotated right");
    };
    exportSequencerMidiButton.setTooltip("Export the current sequencer pattern as a MIDI clip");
    exportSequencerMidiButton.onClick = [this] { exportSequencerMidiClip(); };
    applyGrooveTransformButton.setTooltip("Apply the selected groove transform to the current sequence");
    applyGrooveTransformButton.onClick = [this]
    {
        releaseRandomCandidateAudition(false);
        releasePresetAuditionNote();
        const auto selectedId = sequencerGrooveTransformBox.getSelectedId();
        const auto transformIndex = juce::jmax(1, selectedId) - 1;
        if (audioProcessor.applySequencerGrooveTransform(transformIndex))
        {
            sequencerGrid.repaint();
            updateSegmentedSelectors();
            setRandomStatus(sequencerGrooveTransformBox.getText() + " shaped");
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
                sequencerGrid.repaint();
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
    osc2SineWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::osc2Wave, 0); };
    osc2SawWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::osc2Wave, 1); };
    osc2SquareWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::osc2Wave, 2); };
    osc2TriangleWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::osc2Wave, 3); };
    osc2WavetableWaveButton.onClick = [this] { setChoiceParameter(Parameters::ID::osc2Wave, 4); };
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
    refreshPresetsButton.onClick = [this] { refreshPresetList(); };
    favoritePresetButton.onClick = [this] { toggleFavoritePreset(); };
    const std::array<juce::String, 8> quickFilterLabels { "All", "Fav", "House", "Tech", "Min", "UKG", "Techno", "Factory" };
    for (size_t index = 0; index < presetQuickFilterButtons.size(); ++index)
    {
        auto& button = presetQuickFilterButtons[index];
        button.setButtonText(quickFilterLabels[index]);
        button.setTooltip("Quick Library filter: " + quickFilterLabels[index]);
        button.setClickingTogglesState(false);
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
    lfoCurvePresetBox.onChange = [this]
    {
        const auto selectedId = lfoCurvePresetBox.getSelectedId();
        if (selectedId > 1)
            applyLfoCurvePreset(selectedId);
    };
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
        button.onClick = [this, index]
        {
            setActiveRandomLabPage(static_cast<RandomLabPage>(index));
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
    addAndMakeVisible(loadSampleButton);
    addAndMakeVisible(clearSampleButton);
    addAndMakeVisible(randomCutButton);
    addAndMakeVisible(ukgChopButton);
    addAndMakeVisible(sampleSliceStoreButton);
    addAndMakeVisible(sampleSliceRecallButton);
    addAndMakeVisible(sampleSliceDiceButton);
    addAndMakeVisible(sampleSliceReverseEditButton);
    addAndMakeVisible(sampleSliceChokeButton);
    addAndMakeVisible(sampleSlicePanButton);
    addAndMakeVisible(sampleSliceGhostButton);
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
    addAndMakeVisible(sineWaveButton);
    addAndMakeVisible(sawWaveButton);
    addAndMakeVisible(squareWaveButton);
    addAndMakeVisible(triangleWaveButton);
    addAndMakeVisible(wavetableWaveButton);
    addAndMakeVisible(osc2SineWaveButton);
    addAndMakeVisible(osc2SawWaveButton);
    addAndMakeVisible(osc2SquareWaveButton);
    addAndMakeVisible(osc2TriangleWaveButton);
    addAndMakeVisible(osc2WavetableWaveButton);
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
    addAndMakeVisible(refreshPresetsButton);
    addAndMakeVisible(favoritePresetButton);
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

    sequencerGrid.setCallbacks(
        [this] (int index) { return audioProcessor.getSequencerStep(index); },
        [this] (int index, Sequencer::Step step) { audioProcessor.setSequencerStep(index, step); });
    addAndMakeVisible(sequencerGrid);
    updateSampleNameLabel();
    updateSampleSliceButtons();
    refreshPresetList();
    setActivePanel(Panel::home);
    startTimerHz(12);
}

NateVSTAudioProcessorEditor::~NateVSTAudioProcessorEditor()
{
    restoreFxMomentarySnapshot(fxMomentarySnapshot);
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();
    stopTimer();
    presetBrowserList.setModel(nullptr);
    setLookAndFeel(nullptr);
}

void NateVSTAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(backgroundColour());

    auto bounds = getLocalBounds().reduced(16);
    const auto topArea = bounds.removeFromTop(48);
    const auto keyboardArea = bounds.removeFromBottom(pianoKeyboardHeight);
    bounds.removeFromBottom(10);
    const auto contentArea = bounds.reduced(0, 8);

    g.setColour(panelColour());
    g.fillRoundedRectangle(topArea.toFloat(), 7.0f);
    g.fillRoundedRectangle(contentArea.toFloat(), 7.0f);
    g.fillRoundedRectangle(keyboardArea.toFloat(), 7.0f);

    g.setColour(juce::Colour(0xff293339));
    g.drawRoundedRectangle(topArea.toFloat(), 7.0f, 1.0f);
    g.drawRoundedRectangle(contentArea.toFloat(), 7.0f, 1.0f);
    g.drawRoundedRectangle(keyboardArea.toFloat(), 7.0f, 1.0f);
    g.drawVerticalLine(keyboardArea.getX() + keyboardControlsWidth,
                       static_cast<float>(keyboardArea.getY() + 9),
                       static_cast<float>(keyboardArea.getBottom() - 9));

    if (activePanel == Panel::home)
    {
        auto homeContent = contentArea.reduced(18).withTrimmedTop(36);
        auto topRow = homeContent.removeFromTop(258);
        const auto macroColumnWidth = juce::jlimit(286, 340, topRow.getWidth() / 3);
        auto engineArea = topRow.removeFromLeft(306).reduced(5);
        auto shapeArea = topRow.removeFromRight(macroColumnWidth).reduced(5);
        auto overviewArea = topRow.reduced(5);
        auto bottomRow = homeContent.withTrimmedTop(14);
        auto labArea = bottomRow.removeFromLeft(330).reduced(5);
        auto libraryArea = bottomRow.reduced(5);

        for (auto area : { engineArea, overviewArea, shapeArea, labArea, libraryArea })
        {
            g.setColour(juce::Colour(0xff101619));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff2b363c));
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }
    }

    if (activePanel == Panel::synth)
    {
        auto synthContent = contentArea.reduced(18).withTrimmedTop(36);
        synthContent.removeFromTop(44);
        synthContent.removeFromTop(44);
        synthContent.removeFromTop(8);

        auto topCards = synthContent.removeFromTop((synthContent.getHeight() - 10) / 2);
        synthContent.removeFromTop(10);
        auto bottomCards = synthContent;
        auto sourceArea = topCards.removeFromLeft(330).reduced(5);
        auto voiceArea = topCards.reduced(5);
        auto filterArea = bottomCards.removeFromLeft(330).reduced(5);
        auto ampArea = bottomCards.reduced(5);

        for (auto area : { sourceArea, voiceArea, filterArea, ampArea })
        {
            g.setColour(juce::Colour(0xff101619));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff2b363c));
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }
    }

    if (activePanel == Panel::mod)
    {
        auto modContent = contentArea.reduced(18).withTrimmedTop(36);
        auto topRow = modContent.removeFromTop(modTopRowHeight);
        auto sourceArea = topRow.removeFromLeft(300).reduced(5);
        auto macroArea = topRow.reduced(5);
        modContent.removeFromTop(modPanelGap);
        auto controlsRow = modContent.removeFromTop(modGeneratorRowHeight);
        auto lfoArea = controlsRow.removeFromLeft(450).reduced(5);
        auto envelopeArea = controlsRow.reduced(5);
        modContent.removeFromTop(modPanelGap);
        auto matrixArea = modContent.reduced(5);

        for (auto area : { sourceArea, macroArea, lfoArea, envelopeArea, matrixArea })
        {
            g.setColour(juce::Colour(0xff101619));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff2b363c));
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }
    }

    if (activePanel == Panel::sample)
    {
        auto sampleContent = contentArea.reduced(18);
        sampleContent.removeFromTop(28);
        auto sourceArea = sampleContent.removeFromTop(88).reduced(5);
        auto chopArea = sampleContent.removeFromTop(192).reduced(5);
        auto shapeArea = sampleContent.reduced(5);

        for (auto area : { sourceArea, chopArea, shapeArea })
        {
            g.setColour(juce::Colour(0xff101619));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff2b363c));
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }
    }

    if (activePanel == Panel::sequencer)
    {
        auto sequencerContent = contentArea.reduced(18).withTrimmedTop(36);
        auto controlArea = sequencerContent.removeFromTop(262).reduced(5);
        auto gridArea = sequencerContent.reduced(5, 7);

        for (auto area : { controlArea, gridArea })
        {
            g.setColour(juce::Colour(0xff101619));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff2b363c));
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
            g.setColour(juce::Colour(0xff101619));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff2b363c));
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }

        g.setColour(juce::Colour(0xff879299));
        g.setFont(12.0f);
        g.drawText("RACK", rackArea.removeFromTop(24).reduced(12, 0), juce::Justification::centredLeft);
        g.drawText(fxModuleName(selectedFxModule).toUpperCase(), detailArea.removeFromTop(24).reduced(14, 0), juce::Justification::centredLeft);
    }

    if (activePanel == Panel::library)
    {
        auto libraryContent = contentArea.reduced(18).withTrimmedTop(36);
        auto leftArea = libraryContent.removeFromLeft(236).reduced(5);
        auto inspectorArea = libraryContent.removeFromRight(318).reduced(5);
        auto browserArea = libraryContent.reduced(5);

        for (auto area : { leftArea, browserArea, inspectorArea })
        {
            g.setColour(juce::Colour(0xff0f1619));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff2b363c));
            g.drawRoundedRectangle(area.toFloat(), 6.0f, 1.0f);
        }

        auto saveArea = inspectorArea.reduced(10).removeFromTop(314);
        auto inspectArea = inspectorArea.reduced(10).withTrimmedTop(326);
        auto drawInset = [&g] (juce::Rectangle<int> area, juce::Colour accent)
        {
            auto inset = area.toFloat();
            g.setColour(juce::Colour(0xff0b1114));
            g.fillRoundedRectangle(inset, 5.0f);
            g.setColour(juce::Colour(0xff26343a));
            g.drawRoundedRectangle(inset, 5.0f, 1.0f);
            g.setColour(accent.withAlpha(0.56f));
            g.fillRoundedRectangle(inset.removeFromTop(3.0f), 2.0f);
        };

        drawInset(saveArea, juce::Colour(0xff8ee6c9));
        drawInset(inspectArea, juce::Colour(0xffffc36b));

        auto browserHeader = browserArea.removeFromTop(56).reduced(10, 8);
        g.setColour(juce::Colour(0xff121c20));
        g.fillRoundedRectangle(browserHeader.toFloat(), 5.0f);
        g.setColour(juce::Colour(0xff324148));
        g.drawRoundedRectangle(browserHeader.toFloat(), 5.0f, 1.0f);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff74868b));
        g.drawFittedText("SEARCH, AUDITION, RATE, AND SAVE PRODUCTION PATCHES",
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
            g.setColour(juce::Colour(0xff101619));
            g.fillRoundedRectangle(area.toFloat(), 6.0f);
            g.setColour(juce::Colour(0xff2b363c));
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
    keyboardOctaveDownButton.setBounds(keyboardControlArea.removeFromLeft(42).reduced(2, 3));
    keyboardRangeLabel.setBounds(keyboardControlArea.removeFromLeft(52).reduced(2, 3));
    keyboardOctaveUpButton.setBounds(keyboardControlArea.removeFromLeft(42).reduced(2, 3));
    keyboardPanicButton.setBounds(keyboardControlArea.removeFromLeft(62).reduced(2, 3));
    pianoKeyboard.setBounds(keyboardArea.reduced(8, 6));
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
            wildMutateButton.setVisible(true);
            undoRandomButton.setVisible(true);
            redoRandomButton.setVisible(true);
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
            outputSpectrumDisplay.setVisible(true);
            lowEndAssistant.setVisible(true);
            performanceXYPad.setVisible(true);
            performanceStatusLabel.setVisible(true);
            recallSnapshotAButton.setVisible(true);
            captureSnapshotAButton.setVisible(true);
            recallSnapshotBButton.setVisible(true);
            captureSnapshotBButton.setVisible(true);

            homeSectionLabel.setBounds(content.removeFromTop(28));
            auto dashboard = content.withTrimmedTop(8);
            auto topRow = dashboard.removeFromTop(258);
            const auto macroColumnWidth = juce::jlimit(286, 340, topRow.getWidth() / 3);
            auto performArea = topRow.removeFromLeft(306).reduced(18, 12);
            auto macroArea = topRow.removeFromRight(macroColumnWidth).reduced(18, 12);
            auto overviewArea = topRow.reduced(18, 12);
            auto bottomRow = dashboard.withTrimmedTop(14);
            auto labArea = bottomRow.removeFromLeft(330).reduced(18, 12);
            auto libraryArea = bottomRow.reduced(18, 12);

            homeEngineLabel.setBounds(performArea.removeFromTop(24));
            setSliderVisible(subLevelSlider, subLevelLabel, true);
            setSliderVisible(cutoffSlider, cutoffLabel, true);
            setSliderVisible(driveSlider, driveLabel, true);
            setSliderVisible(outputSlider, outputLabel, true);
            layoutKnobRow(performArea.removeFromTop(92).withTrimmedTop(6), { &subLevelSlider, &cutoffSlider, &driveSlider, &outputSlider });
            lowEndAssistant.setBounds(performArea.removeFromTop(62).reduced(2, 4));

            homeOverviewDisplay.setBounds(overviewArea.removeFromTop(166));
            outputSpectrumDisplay.setBounds(overviewArea.withTrimmedTop(8));

            homeShapeLabel.setBounds(macroArea.removeFromTop(24));
            setSliderVisible(macroToneSlider, macroToneLabel, true);
            setSliderVisible(macroDirtSlider, macroDirtLabel, true);
            setSliderVisible(macroWeightSlider, macroWeightLabel, true);
            setSliderVisible(macroBounceSlider, macroBounceLabel, true);
            setSliderVisible(macroWarpSlider, macroWarpLabel, true);
            setSliderVisible(macroThrowSlider, macroThrowLabel, true);
            auto macroControlArea = macroArea.removeFromTop(132).withTrimmedTop(4);
            const auto xyWidth = juce::jlimit(104, 128, macroControlArea.getWidth() / 2);
            performanceXYPad.setBounds(macroControlArea.removeFromRight(xyWidth).reduced(4, 0));
            layoutKnobRow(macroControlArea.removeFromTop(60), {
                &macroToneSlider,
                &macroDirtSlider,
                &macroWeightSlider
            });
            layoutKnobRow(macroControlArea.withTrimmedTop(4), {
                &macroBounceSlider,
                &macroWarpSlider,
                &macroThrowSlider
            });
            auto snapshotRow = macroArea.removeFromTop(40).withTrimmedTop(5);
            performanceStatusLabel.setBounds(snapshotRow.removeFromLeft(juce::jlimit(76, 112, snapshotRow.getWidth() / 3)).reduced(4, 4));
            const auto snapshotButtonWidth = juce::jmax(1, snapshotRow.getWidth() / 4);
            recallSnapshotAButton.setBounds(snapshotRow.removeFromLeft(snapshotButtonWidth).reduced(3, 4));
            captureSnapshotAButton.setBounds(snapshotRow.removeFromLeft(snapshotButtonWidth).reduced(3, 4));
            recallSnapshotBButton.setBounds(snapshotRow.removeFromLeft(snapshotButtonWidth).reduced(3, 4));
            captureSnapshotBButton.setBounds(snapshotRow.reduced(3, 4));

            homeLabLabel.setBounds(labArea.removeFromTop(24));
            auto randomSelectRow = labArea.removeFromTop(38);
            recipeBox.setBounds(randomSelectRow.removeFromLeft(176).reduced(3, 4));
            randomScopeBox.setBounds(randomSelectRow.reduced(3, 4));
            auto labButtonRow = labArea.removeFromTop(36).withTrimmedTop(4);
            generateButton.setBounds(labButtonRow.removeFromLeft(82).reduced(3, 4));
            mutateButton.setBounds(labButtonRow.removeFromLeft(74).reduced(3, 4));
            variationButton.setBounds(labButtonRow.removeFromLeft(62).reduced(3, 4));
            wildMutateButton.setBounds(labButtonRow.removeFromLeft(56).reduced(3, 4));
            auto randomStatusRow = labArea.removeFromTop(34).withTrimmedTop(6);
            undoRandomButton.setBounds(randomStatusRow.removeFromLeft(58).reduced(3, 4));
            redoRandomButton.setBounds(randomStatusRow.removeFromLeft(58).reduced(3, 4));
            randomStatusLabel.setBounds(randomStatusRow.reduced(5, 4));

            homeLibraryLabel.setBounds(libraryArea.removeFromTop(24));
            auto loadRow = libraryArea.removeFromTop(42);
            previousPresetButton.setBounds(loadRow.removeFromLeft(42).reduced(3, 4));
            presetBox.setBounds(loadRow.removeFromLeft(juce::jmax(120, loadRow.getWidth() - 42)).reduced(3, 4));
            nextPresetButton.setBounds(loadRow.removeFromLeft(42).reduced(3, 4));
            auto actionRow = libraryArea.removeFromTop(42).withTrimmedTop(4);
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
            synthAmpLabel.setVisible(true);
            sineWaveButton.setVisible(true);
            sawWaveButton.setVisible(true);
            squareWaveButton.setVisible(true);
            triangleWaveButton.setVisible(true);
            wavetableWaveButton.setVisible(true);
            osc2SineWaveButton.setVisible(true);
            osc2SawWaveButton.setVisible(true);
            osc2SquareWaveButton.setVisible(true);
            osc2TriangleWaveButton.setVisible(true);
            osc2WavetableWaveButton.setVisible(true);
            lowpassFilterButton.setVisible(true);
            bandpassFilterButton.setVisible(true);
            highpassFilterButton.setVisible(true);
            noiseTypeBox.setVisible(true);
            filterCharacterBox.setVisible(true);
            filterSlopeBox.setVisible(true);
            monoButton.setVisible(true);
            synthSectionLabel.setBounds(content.removeFromTop(28));
            auto selectorRow = content.removeFromTop(44);
            auto waveRow = selectorRow.removeFromLeft(320);
            const auto waveButtonWidth = waveRow.getWidth() / 5;
            sineWaveButton.setBounds(waveRow.removeFromLeft(waveButtonWidth).reduced(3, 4));
            sawWaveButton.setBounds(waveRow.removeFromLeft(waveButtonWidth).reduced(3, 4));
            squareWaveButton.setBounds(waveRow.removeFromLeft(waveButtonWidth).reduced(3, 4));
            triangleWaveButton.setBounds(waveRow.removeFromLeft(waveButtonWidth).reduced(3, 4));
            wavetableWaveButton.setBounds(waveRow.reduced(3, 4));
            auto filterRow = selectorRow.removeFromLeft(180);
            const auto filterButtonWidth = filterRow.getWidth() / 3;
            lowpassFilterButton.setBounds(filterRow.removeFromLeft(filterButtonWidth).reduced(3, 4));
            bandpassFilterButton.setBounds(filterRow.removeFromLeft(filterButtonWidth).reduced(3, 4));
            highpassFilterButton.setBounds(filterRow.reduced(3, 4));
            filterCharacterBox.setBounds(selectorRow.removeFromLeft(126).reduced(4));
            filterSlopeBox.setBounds(selectorRow.removeFromLeft(82).reduced(4));
            monoButton.setBounds(selectorRow.removeFromLeft(90).reduced(4));
            auto osc2Row = content.removeFromTop(44).withTrimmedTop(2);
            const auto osc2WaveButtonWidth = osc2Row.getWidth() / 5;
            osc2SineWaveButton.setBounds(osc2Row.removeFromLeft(osc2WaveButtonWidth).reduced(3, 4));
            osc2SawWaveButton.setBounds(osc2Row.removeFromLeft(osc2WaveButtonWidth).reduced(3, 4));
            osc2SquareWaveButton.setBounds(osc2Row.removeFromLeft(osc2WaveButtonWidth).reduced(3, 4));
            osc2TriangleWaveButton.setBounds(osc2Row.removeFromLeft(osc2WaveButtonWidth).reduced(3, 4));
            osc2WavetableWaveButton.setBounds(osc2Row.reduced(3, 4));
            content.removeFromTop(8);
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
            auto cards = content;
            auto topCards = cards.removeFromTop((cards.getHeight() - 10) / 2);
            cards.removeFromTop(10);
            auto bottomCards = cards;

            auto sourceArea = topCards.removeFromLeft(330).reduced(18, 10);
            auto voiceArea = topCards.reduced(18, 10);
            auto filterArea = bottomCards.removeFromLeft(330).reduced(18, 10);
            auto ampArea = bottomCards.reduced(18, 10);

            synthSourceLabel.setBounds(sourceArea.removeFromTop(22));
            wavetableDisplay.setVisible(true);
            wavetableDisplay.setBounds(sourceArea.removeFromTop(48).reduced(2, 4));
            layoutKnobRow(sourceArea.removeFromTop(68).withTrimmedTop(3), {
                &osc1LevelSlider,
                &osc2LevelSlider,
                &subLevelSlider,
                &noiseLevelSlider
            });
            auto sourceTextureRow = sourceArea.removeFromTop(54).withTrimmedTop(2);
            noiseTypeBox.setBounds(sourceTextureRow.removeFromLeft(90).reduced(4, 8));
            layoutKnobRow(sourceTextureRow, {
                &noiseDecaySlider,
                &oscWavetablePositionSlider,
                &osc2WavetablePositionSlider
            });

            synthVoiceLabel.setBounds(voiceArea.removeFromTop(22));
            layoutKnobRow(voiceArea.removeFromTop(72).withTrimmedTop(3), {
                &octaveSlider,
                &tuneSlider,
                &osc2OctaveSlider,
                &osc2TuneSlider,
                &glideSlider,
                &oscWarpSlider
            });
            layoutKnobRow(voiceArea.removeFromTop(72).withTrimmedTop(5), {
                &unisonVoicesSlider,
                &unisonDetuneSlider,
                &unisonBlendSlider,
                &unisonSpreadSlider
            });

            synthFilterLabel.setBounds(filterArea.removeFromTop(22));
            filterResponseDisplay.setBounds(filterArea.removeFromTop(58).reduced(2, 4));
            layoutKnobRow(filterArea.removeFromTop(72).withTrimmedTop(3), {
                &cutoffSlider,
                &resonanceSlider,
                &filterEnvSlider,
                &driveSlider
            });

            synthAmpLabel.setBounds(ampArea.removeFromTop(22));
            layoutKnobRow(ampArea.removeFromTop(104).withTrimmedTop(5), {
                &attackSlider,
                &decaySlider,
                &sustainSlider,
                &releaseSlider,
                &outputSlider
            });
            break;
        }

        case Panel::lab:
        {
            randomSectionLabel.setVisible(true);
            randomSectionLabel.setBounds(content.removeFromTop(28));

            auto pageRow = content.removeFromTop(42).withTrimmedTop(4);
            const auto pageButtonWidth = pageRow.getWidth() / static_cast<int>(randomLabPageButtons.size());
            for (auto& button : randomLabPageButtons)
            {
                button.setVisible(true);
                button.setBounds(pageRow.removeFromLeft(pageButtonWidth).reduced(4));
            }
            updateRandomLabPageButtons();

            auto showRecipeControls = [&content, this] (bool includeGenerate)
            {
                recipeBox.setVisible(true);
                randomScopeBox.setVisible(true);
                if (includeGenerate)
                    generateButton.setVisible(true);

                auto row = content.removeFromTop(48).withTrimmedTop(4);
                recipeBox.setBounds(row.removeFromLeft(240).reduced(4));
                randomScopeBox.setBounds(row.removeFromLeft(132).reduced(4));
                if (includeGenerate)
                    generateButton.setBounds(row.removeFromLeft(112).reduced(4));
            };

            auto showSectionRolls = [&content, this]
            {
                auto row = content.removeFromTop(42).withTrimmedTop(6);
                const auto buttonWidth = row.getWidth() / static_cast<int>(randomSectionRollButtons.size());
                for (auto& button : randomSectionRollButtons)
                {
                    button.setVisible(true);
                    button.setBounds(row.removeFromLeft(buttonWidth).reduced(4));
                }
            };

            auto showLockRow = [&content, this]
            {
                randomLockPitchButton.setVisible(true);
                randomLockEnvelopeButton.setVisible(true);
                randomLockFilterButton.setVisible(true);
                randomLockSourceButton.setVisible(true);
                randomLockSampleButton.setVisible(true);
                randomLockFxButton.setVisible(true);
                randomLockOutputButton.setVisible(true);
                randomLockSequencerButton.setVisible(true);

                auto row = content.removeFromTop(44).withTrimmedTop(6);
                const auto buttonWidth = row.getWidth() / 8;
                randomLockPitchButton.setBounds(row.removeFromLeft(buttonWidth).reduced(4));
                randomLockEnvelopeButton.setBounds(row.removeFromLeft(buttonWidth).reduced(4));
                randomLockFilterButton.setBounds(row.removeFromLeft(buttonWidth).reduced(4));
                randomLockSourceButton.setBounds(row.removeFromLeft(buttonWidth).reduced(4));
                randomLockSampleButton.setBounds(row.removeFromLeft(buttonWidth).reduced(4));
                randomLockFxButton.setBounds(row.removeFromLeft(buttonWidth).reduced(4));
                randomLockOutputButton.setBounds(row.removeFromLeft(buttonWidth).reduced(4));
                randomLockSequencerButton.setBounds(row.reduced(4));
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
                auto row = content.removeFromTop(72).withTrimmedTop(6);
                const auto count = static_cast<int>(randomSectionIntensitySliders.size());
                const auto cellWidth = row.getWidth() / juce::jmax(1, count);

                for (size_t index = 0; index < randomSectionIntensitySliders.size(); ++index)
                {
                    auto cell = row.removeFromLeft(cellWidth).reduced(4, 2);
                    randomSectionIntensityLabels[index].setVisible(true);
                    randomSectionIntensitySliders[index].setVisible(true);
                    randomSectionIntensityLabels[index].setBounds(cell.removeFromTop(18));
                    randomSectionIntensitySliders[index].setBounds(cell.removeFromTop(26));
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
                    showRecipeControls(true);
                    showSectionRolls();
                    showRandomBiasKnobs();
                    showSectionIntensityControls();
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

                    auto row = content.removeFromTop(48).withTrimmedTop(4);
                    recipeBox.setBounds(row.removeFromLeft(220).reduced(4));
                    randomScopeBox.setBounds(row.removeFromLeft(118).reduced(4));
                    variationButton.setBounds(row.removeFromLeft(84).reduced(4));
                    mutateButton.setBounds(row.removeFromLeft(96).reduced(4));
                    wildMutateButton.setBounds(row.removeFromLeft(76).reduced(4));
                    undoRandomButton.setBounds(row.removeFromLeft(78).reduced(4));
                    redoRandomButton.setBounds(row.removeFromLeft(78).reduced(4));
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
                    showLockRow();
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
            modSourceLabel.setVisible(true);
            modMacroLabel.setVisible(true);
            modLfoLabel.setVisible(true);
            modLfo2Label.setVisible(true);
            modEnvelopeLabel.setVisible(true);
            modMatrixLabel.setVisible(true);
            lfo1ShapeBox.setVisible(true);
            lfo1SyncRateBox.setVisible(true);
            lfo2ShapeBox.setVisible(true);
            lfo2SyncRateBox.setVisible(true);
            lfoCurvePresetBox.setVisible(true);
            lfo1SyncButton.setVisible(true);
            lfo1RetriggerButton.setVisible(true);
            lfo2SyncButton.setVisible(true);
            lfo2RetriggerButton.setVisible(true);
            lfoCurveDisplay.setVisible(true);
            lfoCurveInvertButton.setVisible(true);
            lfoCurveReverseButton.setVisible(true);
            lfoCurveSmoothButton.setVisible(true);
            lfoCurveQuantizeButton.setVisible(true);
            lfoCurveRandomButton.setVisible(true);
            lfoCurveGarageButton.setVisible(true);

            modSectionLabel.setBounds(content.removeFromTop(28));

            for (auto& label : modSourceRows)
                label.setVisible(true);

            for (size_t index = 0; index < modSlotRows.size(); ++index)
            {
                modSlotRows[index].setVisible(true);
                modSourceBoxes[index].setVisible(true);
                modDestinationBoxes[index].setVisible(true);
                modAmountSliders[index].setVisible(true);
                modAmountLabels[index].setVisible(false);
                modMatrixRows[index].setVisible(true);
                modSlotEnabledButtons[index].setVisible(true);
                modSlotDuplicateButtons[index].setVisible(true);
                modSlotDeleteButtons[index].setVisible(true);
            }
            modMatrixStatusLabel.setVisible(true);
            modInspectorLabel.setVisible(true);
            modInspectorDestinationBox.setVisible(true);
            modInspectorSourceBox.setVisible(true);
            modInspectorStatusLabel.setVisible(true);
            modInspectorAddButton.setVisible(true);
            modInspectorClearButton.setVisible(true);
            modMacroAssignLabel.setVisible(true);
            modMacroAssignStatusLabel.setVisible(true);
            modMacroAssignSourceBox.setVisible(true);
            modMacroAssignDestinationBox.setVisible(true);
            modMacroAssignAmountSlider.setVisible(true);
            modMacroAssignAddButton.setVisible(true);
            modMacroAssignReplaceButton.setVisible(true);
            modMacroAssignClearButton.setVisible(true);
            modMatrixSourceHeader.setVisible(true);
            modMatrixDestinationHeader.setVisible(true);
            modMatrixAmountHeader.setVisible(true);
            modMatrixSourceHeaderB.setVisible(true);
            modMatrixDestinationHeaderB.setVisible(true);
            modMatrixAmountHeaderB.setVisible(true);

            auto modContent = content.withTrimmedTop(8);
            auto topRow = modContent.removeFromTop(modTopRowHeight);
            auto sourceArea = topRow.removeFromLeft(300).reduced(18, 8);
            auto macroArea = topRow.reduced(18, 8);

            modSourceLabel.setBounds(sourceArea.removeFromTop(18));
            auto sourceListArea = sourceArea.withTrimmedTop(2);
            auto leftSources = sourceListArea.removeFromLeft(sourceListArea.getWidth() / 2);
            auto rightSources = sourceListArea;
            const auto sourceColumnRows = (modSourceRows.size() + 1) / 2;
            const auto sourceRowHeight = juce::jmax(10,
                                                    sourceListArea.getHeight()
                                                        / juce::jmax(1, static_cast<int>(sourceColumnRows)));
            for (size_t index = 0; index < modSourceRows.size(); ++index)
            {
                auto& column = index < sourceColumnRows ? leftSources : rightSources;
                modSourceRows[index].setBounds(column.removeFromTop(sourceRowHeight).reduced(3, 1));
            }

            auto macroHeader = macroArea.removeFromTop(18);
            modMacroLabel.setBounds(macroHeader.removeFromLeft(74));
            modMacroAssignStatusLabel.setBounds(macroHeader.reduced(3, 0));
            setSliderVisible(macroToneSlider, macroToneLabel, true);
            setSliderVisible(macroDirtSlider, macroDirtLabel, true);
            setSliderVisible(macroMotionSlider, macroMotionLabel, true);
            setSliderVisible(macroSpaceSlider, macroSpaceLabel, true);
            setSliderVisible(macroWeightSlider, macroWeightLabel, true);
            setSliderVisible(macroBounceSlider, macroBounceLabel, true);
            setSliderVisible(macroWarpSlider, macroWarpLabel, true);
            setSliderVisible(macroThrowSlider, macroThrowLabel, true);
            layoutKnobRow(macroArea.removeFromTop(42).withTrimmedTop(1), { &macroToneSlider, &macroDirtSlider, &macroMotionSlider, &macroSpaceSlider });
            layoutKnobRow(macroArea.removeFromTop(42).withTrimmedTop(1), { &macroWeightSlider, &macroBounceSlider, &macroWarpSlider, &macroThrowSlider });
            auto macroAssignRow = macroArea.removeFromTop(30).withTrimmedTop(4);
            modMacroAssignLabel.setBounds(macroAssignRow.removeFromLeft(48).withTrimmedTop(5));
            modMacroAssignSourceBox.setBounds(macroAssignRow.removeFromLeft(88).reduced(3, 3));
            modMacroAssignDestinationBox.setBounds(macroAssignRow.removeFromLeft(124).reduced(3, 3));
            modMacroAssignAmountSlider.setBounds(macroAssignRow.removeFromLeft(114).reduced(3, 4));
            modMacroAssignAddButton.setBounds(macroAssignRow.removeFromLeft(48).reduced(3, 3));
            modMacroAssignReplaceButton.setBounds(macroAssignRow.removeFromLeft(66).reduced(3, 3));
            modMacroAssignClearButton.setBounds(macroAssignRow.removeFromLeft(52).reduced(3, 3));

            modContent.removeFromTop(modPanelGap);
            auto generatorRow = modContent.removeFromTop(modGeneratorRowHeight);
            auto lfoArea = generatorRow.removeFromLeft(450).reduced(18, 8);
            auto rightGeneratorArea = generatorRow.reduced(18, 8);
            auto lfo2Area = rightGeneratorArea.removeFromLeft(212);
            auto envelopeArea = rightGeneratorArea.withTrimmedLeft(10);

            modLfoLabel.setBounds(lfoArea.removeFromTop(18));
            auto lfoModeRow = lfoArea.removeFromTop(24);
            lfo1ShapeBox.setBounds(lfoModeRow.removeFromLeft(86).reduced(3, 4));
            lfo1SyncRateBox.setBounds(lfoModeRow.removeFromLeft(78).reduced(3, 4));
            lfo1SyncButton.setBounds(lfoModeRow.removeFromLeft(58).reduced(3, 4));
            lfo1RetriggerButton.setBounds(lfoModeRow.removeFromLeft(72).reduced(3, 4));
            lfoCurvePresetBox.setBounds(lfoModeRow.removeFromLeft(130).reduced(3, 4));
            auto lfoToolRow = lfoArea.removeFromTop(24).withTrimmedTop(3);
            const auto lfoToolWidth = lfoToolRow.getWidth() / 6;
            lfoCurveInvertButton.setBounds(lfoToolRow.removeFromLeft(lfoToolWidth).reduced(2, 1));
            lfoCurveReverseButton.setBounds(lfoToolRow.removeFromLeft(lfoToolWidth).reduced(2, 1));
            lfoCurveSmoothButton.setBounds(lfoToolRow.removeFromLeft(lfoToolWidth).reduced(2, 1));
            lfoCurveQuantizeButton.setBounds(lfoToolRow.removeFromLeft(lfoToolWidth).reduced(2, 1));
            lfoCurveRandomButton.setBounds(lfoToolRow.removeFromLeft(lfoToolWidth).reduced(2, 1));
            lfoCurveGarageButton.setBounds(lfoToolRow.reduced(2, 1));
            lfoCurveDisplay.setBounds(lfoArea.removeFromTop(36).withTrimmedTop(2));

            setSliderVisible(lfo1RateSlider, lfo1RateLabel, true);
            setSliderVisible(lfo1DepthSlider, lfo1DepthLabel, true);
            setSliderVisible(lfo1PhaseSlider, lfo1PhaseLabel, true);
            layoutKnobRow(lfoArea.removeFromTop(40).withTrimmedTop(2), { &lfo1RateSlider, &lfo1DepthSlider, &lfo1PhaseSlider });

            modLfo2Label.setBounds(lfo2Area.removeFromTop(18));
            auto lfo2ModeRow = lfo2Area.removeFromTop(48);
            lfo2ShapeBox.setBounds(lfo2ModeRow.removeFromTop(24).reduced(3, 4));
            auto lfo2SyncRow = lfo2ModeRow;
            lfo2SyncRateBox.setBounds(lfo2SyncRow.removeFromLeft(72).reduced(3, 4));
            lfo2SyncButton.setBounds(lfo2SyncRow.removeFromLeft(55).reduced(3, 4));
            lfo2RetriggerButton.setBounds(lfo2SyncRow.removeFromLeft(70).reduced(3, 4));
            setSliderVisible(lfo2RateSlider, lfo2RateLabel, true);
            setSliderVisible(lfo2DepthSlider, lfo2DepthLabel, true);
            setSliderVisible(lfo2PhaseSlider, lfo2PhaseLabel, true);
            layoutKnobRow(lfo2Area.removeFromTop(58).withTrimmedTop(4), { &lfo2RateSlider, &lfo2DepthSlider, &lfo2PhaseSlider });

            modEnvelopeLabel.setBounds(envelopeArea.removeFromTop(18));
            setSliderVisible(modEnv1AttackSlider, modEnv1AttackLabel, true);
            setSliderVisible(modEnv1DecaySlider, modEnv1DecayLabel, true);
            setSliderVisible(modEnv1SustainSlider, modEnv1SustainLabel, true);
            setSliderVisible(modEnv1ReleaseSlider, modEnv1ReleaseLabel, true);
            setSliderVisible(modEnv1DepthSlider, modEnv1DepthLabel, true);
            layoutKnobRow(envelopeArea.removeFromTop(58).withTrimmedTop(2), {
                &modEnv1AttackSlider,
                &modEnv1DecaySlider,
                &modEnv1SustainSlider
            });
            layoutKnobRow(envelopeArea.removeFromTop(54).withTrimmedTop(2), {
                &modEnv1ReleaseSlider,
                &modEnv1DepthSlider
            });

            modContent.removeFromTop(modPanelGap);
            auto matrixArea = modContent.reduced(18, 0);
            auto matrixTitleRow = matrixArea.removeFromTop(28);
            modMatrixLabel.setBounds(matrixTitleRow.removeFromLeft(70).withTrimmedTop(5));
            modMatrixStatusLabel.setBounds(matrixTitleRow.removeFromLeft(150).reduced(3, 5));
            modInspectorStatusLabel.setBounds(matrixTitleRow.reduced(5, 5));

            auto inspectorRow = matrixArea.removeFromTop(30);
            modInspectorLabel.setBounds(inspectorRow.removeFromLeft(62).withTrimmedTop(6));
            modInspectorDestinationBox.setBounds(inspectorRow.removeFromLeft(150).reduced(3, 4));
            modInspectorSourceBox.setBounds(inspectorRow.removeFromLeft(138).reduced(3, 4));
            modInspectorAddButton.setBounds(inspectorRow.removeFromLeft(58).reduced(3, 4));
            modInspectorClearButton.setBounds(inspectorRow.removeFromLeft(66).reduced(3, 4));

            auto matrixHeaderRow = matrixArea.removeFromTop(18).reduced(3, 1);
            auto leftHeader = matrixHeaderRow.removeFromLeft((matrixHeaderRow.getWidth() - 10) / 2);
            matrixHeaderRow.removeFromLeft(10);
            auto rightHeader = matrixHeaderRow;
            auto placeHeader = [] (juce::Rectangle<int> header,
                                   juce::Label& source,
                                   juce::Label& destination,
                                   juce::Label& amount)
            {
                header.removeFromLeft(26);
                header.removeFromRight(74);
                source.setBounds(header.removeFromLeft(100).reduced(5, 0));
                destination.setBounds(header.removeFromLeft(138).reduced(5, 0));
                amount.setBounds(header.reduced(5, 0));
            };
            placeHeader(leftHeader, modMatrixSourceHeader, modMatrixDestinationHeader, modMatrixAmountHeader);
            placeHeader(rightHeader, modMatrixSourceHeaderB, modMatrixDestinationHeaderB, modMatrixAmountHeaderB);

            auto leftBank = matrixArea.removeFromLeft((matrixArea.getWidth() - 10) / 2);
            matrixArea.removeFromLeft(10);
            auto rightBank = matrixArea;
            auto placeRouteRow = [this] (size_t index, juce::Rectangle<int>& bank, int rowsRemaining)
            {
                const auto rowHeight = juce::jmax(22, bank.getHeight() / rowsRemaining);
                auto rowBounds = bank.removeFromTop(rowHeight).reduced(3, 1);
                modMatrixRows[index].setBounds(rowBounds);

                auto row = rowBounds.reduced(2, 2);
                modSlotRows[index].setBounds(row.removeFromLeft(26).reduced(2, 0));
                modSourceBoxes[index].setBounds(row.removeFromLeft(100).reduced(3, 0));
                modDestinationBoxes[index].setBounds(row.removeFromLeft(138).reduced(3, 0));
                auto actionArea = row.removeFromRight(74);
                modSlotEnabledButtons[index].setBounds(actionArea.removeFromLeft(31).reduced(1, 0));
                modSlotDuplicateButtons[index].setBounds(actionArea.removeFromLeft(22).reduced(1, 0));
                modSlotDeleteButtons[index].setBounds(actionArea.reduced(1, 0));
                modAmountSliders[index].setBounds(row.reduced(3, 0));
            };

            for (size_t index = 0; index < 4; ++index)
                placeRouteRow(index, leftBank, static_cast<int>(4 - index));

            for (size_t index = 4; index < modSlotRows.size(); ++index)
                placeRouteRow(index, rightBank, static_cast<int>(modSlotRows.size() - index));

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
            randomCutButton.setVisible(true);
            ukgChopButton.setVisible(true);
            sampleEnabledButton.setVisible(true);
            sampleReverseButton.setVisible(true);
            sampleModeBox.setVisible(true);
            sampleSliceStyleBox.setVisible(true);
            sampleStutterEnabledButton.setVisible(true);
            sampleStutterRateBox.setVisible(true);
            sampleWaveformDisplay.setVisible(true);
            for (auto& button : sampleSliceButtons)
                button.setVisible(true);
            sampleSliceStatusLabel.setVisible(true);
            sampleSliceStoreButton.setVisible(true);
            sampleSliceRecallButton.setVisible(true);
            sampleSliceDiceButton.setVisible(true);
            sampleSliceReverseEditButton.setVisible(true);
            sampleSliceChokeButton.setVisible(true);
            sampleSlicePanButton.setVisible(true);
            sampleSliceGhostButton.setVisible(true);
            sampleNameLabel.setVisible(true);
            sampleSectionLabel.setBounds(content.removeFromTop(28));
            sampleSourceLabel.setBounds(content.removeFromTop(16).withTrimmedLeft(4));
            auto actionRow = content.removeFromTop(38);
            loadSampleButton.setBounds(actionRow.removeFromLeft(90).reduced(4));
            clearSampleButton.setBounds(actionRow.removeFromLeft(90).reduced(4));
            randomCutButton.setBounds(actionRow.removeFromLeft(114).reduced(4));
            ukgChopButton.setBounds(actionRow.removeFromLeft(114).reduced(4));
            sampleEnabledButton.setBounds(actionRow.removeFromLeft(70).reduced(4));
            sampleReverseButton.setBounds(actionRow.removeFromLeft(70).reduced(4));
            sampleModeBox.setBounds(actionRow.removeFromLeft(112).reduced(4));
            auto stutterRow = content.removeFromTop(34).withTrimmedTop(2);
            sampleNameLabel.setBounds(stutterRow.removeFromLeft(330).reduced(8, 4));
            sampleSliceStyleBox.setBounds(stutterRow.removeFromLeft(142).reduced(4));
            sampleStutterEnabledButton.setBounds(stutterRow.removeFromLeft(96).reduced(4));
            sampleStutterRateBox.setBounds(stutterRow.removeFromLeft(108).reduced(4));
            sampleChopLabel.setBounds(content.removeFromTop(16).withTrimmedLeft(4));
            sampleWaveformDisplay.setBounds(content.removeFromTop(100).reduced(4, 6));
            auto sliceRow = content.removeFromTop(34).withTrimmedTop(2);
            const auto sliceWidth = sliceRow.getWidth() / static_cast<int>(sampleSliceButtons.size());
            for (auto& button : sampleSliceButtons)
                button.setBounds(sliceRow.removeFromLeft(sliceWidth).reduced(4));
            auto sliceEditRow = content.removeFromTop(36).withTrimmedTop(2);
            sampleSliceStatusLabel.setBounds(sliceEditRow.removeFromLeft(318).reduced(8, 4));
            sampleSliceStoreButton.setBounds(sliceEditRow.removeFromLeft(72).reduced(4));
            sampleSliceRecallButton.setBounds(sliceEditRow.removeFromLeft(78).reduced(4));
            sampleSliceDiceButton.setBounds(sliceEditRow.removeFromLeft(64).reduced(4));
            sampleSliceReverseEditButton.setBounds(sliceEditRow.removeFromLeft(58).reduced(4));
            sampleSliceChokeButton.setBounds(sliceEditRow.removeFromLeft(70).reduced(4));
            sampleSlicePanButton.setBounds(sliceEditRow.removeFromLeft(58).reduced(4));
            sampleSliceGhostButton.setBounds(sliceEditRow.removeFromLeft(72).reduced(4));
            auto cutRow = content.removeFromTop(42).withTrimmedTop(4);
            setSliderVisible(sampleStartSlider, sampleStartLabel, true);
            setSliderVisible(sampleEndSlider, sampleEndLabel, true);
            sampleStartSlider.setBounds(cutRow.removeFromLeft(cutRow.getWidth() / 2).reduced(48, 6));
            sampleEndSlider.setBounds(cutRow.reduced(48, 6));
            sampleShapeLabel.setBounds(content.removeFromTop(16).withTrimmedLeft(4));
            setSliderVisible(sampleTransposeSlider, sampleTransposeLabel, true);
            setSliderVisible(samplePitchRampSlider, samplePitchRampLabel, true);
            setSliderVisible(sampleGainSlider, sampleGainLabel, true);
            setSliderVisible(sampleMixSlider, sampleMixLabel, true);
            setSliderVisible(sampleStutterRepeatsSlider, sampleStutterRepeatsLabel, true);
            layoutKnobRow(content.removeFromTop(78), { &sampleTransposeSlider, &sampleGainSlider, &sampleMixSlider });
            layoutKnobRow(content.removeFromTop(78).withTrimmedTop(4), { &samplePitchRampSlider, &sampleStutterRepeatsSlider });
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
            sequencerLockDestinationBox.setVisible(true);
            applyPatternButton.setVisible(true);
            copySequencerButton.setVisible(true);
            rotateSequencerLeftButton.setVisible(true);
            rotateSequencerRightButton.setVisible(true);
            exportSequencerMidiButton.setVisible(true);
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
            sequencerSectionLabel.setBounds(content.removeFromTop(28));
            auto timingRow = content.removeFromTop(44);
            sequencerEnabledButton.setBounds(timingRow.removeFromLeft(62).reduced(4));
            auto rateRow = timingRow.removeFromLeft(150);
            const auto rateButtonWidth = rateRow.getWidth() / 3;
            rateEighthButton.setBounds(rateRow.removeFromLeft(rateButtonWidth).reduced(3, 4));
            rateSixteenthButton.setBounds(rateRow.removeFromLeft(rateButtonWidth).reduced(3, 4));
            rateThirtySecondButton.setBounds(rateRow.reduced(3, 4));
            sequencerGrooveBox.setBounds(timingRow.removeFromLeft(124).reduced(4));
            sequencerScaleBox.setBounds(timingRow.removeFromLeft(112).reduced(4));
            sequencerChordBox.setBounds(timingRow.removeFromLeft(112).reduced(4));
            sequencerVoicingBox.setBounds(timingRow.removeFromLeft(104).reduced(4));
            sequencerChordMemoryButton.setBounds(timingRow.removeFromLeft(92).reduced(4));
            hostSyncStatusLabel.setBounds(timingRow.reduced(4));
            auto patternRow = content.removeFromTop(42).withTrimmedTop(2);
            sequencerPatternBox.setBounds(patternRow.removeFromLeft(230).reduced(4));
            applyPatternButton.setBounds(patternRow.removeFromLeft(76).reduced(4));
            randomSequencerButton.setBounds(patternRow.removeFromLeft(108).reduced(4));
            mutateSequencerButton.setBounds(patternRow.removeFromLeft(76).reduced(4));
            undoSequencerButton.setBounds(patternRow.removeFromLeft(76).reduced(4));
            clearSequencerButton.setBounds(patternRow.removeFromLeft(82).reduced(4));
            auto sceneRow = content.removeFromTop(32).withTrimmedTop(2);
            for (size_t index = 0; index < sequencerSceneRecallButtons.size(); ++index)
            {
                sequencerSceneRecallButtons[index].setBounds(sceneRow.removeFromLeft(62).reduced(4));
                sequencerSceneCaptureButtons[index].setBounds(sceneRow.removeFromLeft(78).reduced(4));
            }
            auto utilityRow = content.removeFromTop(32).withTrimmedTop(2);
            copySequencerButton.setBounds(utilityRow.removeFromLeft(72).reduced(4));
            rotateSequencerLeftButton.setBounds(utilityRow.removeFromLeft(58).reduced(4));
            rotateSequencerRightButton.setBounds(utilityRow.removeFromLeft(58).reduced(4));
            exportSequencerMidiButton.setBounds(utilityRow.removeFromLeft(72).reduced(4));
            sequencerLockDestinationBox.setBounds(utilityRow.removeFromLeft(132).reduced(4));
            sequencerGrooveTransformBox.setBounds(utilityRow.removeFromLeft(184).reduced(4));
            applyGrooveTransformButton.setBounds(utilityRow.removeFromLeft(76).reduced(4));
            setSliderVisible(sequencerRootSlider, sequencerRootLabel, true);
            setSliderVisible(sequencerGateSlider, sequencerGateLabel, true);
            setSliderVisible(sequencerSwingSlider, sequencerSwingLabel, true);
            setSliderVisible(sequencerChordStrumSlider, sequencerChordStrumLabel, true);
            setSliderVisible(sequencerAccentSlider, sequencerAccentLabel, true);
            setSliderVisible(sequencerOctaveSlider, sequencerOctaveLabel, true);
            setSliderVisible(sequencerProbabilitySlider, sequencerProbabilityLabel, true);
            setSliderVisible(sequencerRandomSlider, sequencerRandomLabel, true);
            setSliderVisible(sequencerLockDepthSlider, sequencerLockDepthLabel, true);
            layoutKnobRow(content.removeFromTop(84).withTrimmedTop(6), {
                &sequencerRootSlider,
                &sequencerGateSlider,
                &sequencerSwingSlider,
                &sequencerChordStrumSlider,
                &sequencerAccentSlider,
                &sequencerOctaveSlider,
                &sequencerProbabilitySlider,
                &sequencerRandomSlider,
                &sequencerLockDepthSlider
            });
            sequencerGrid.setBounds(content.reduced(4, 12));
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
                    layoutKnobRow(controlsArea.removeFromTop(150), { &fxDistortionAmountSlider });
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
            favoritePresetButton.setVisible(true);
            refreshPresetsButton.setVisible(true);
            presetStatusLabel.setVisible(true);
            presetBrowserHeaderLabel.setVisible(true);
            presetBrowserList.setVisible(true);
            presetLibrarySummary.setVisible(true);
            for (auto& button : presetQuickFilterButtons)
                button.setVisible(true);
            librarySectionLabel.setBounds(content.removeFromTop(28));
            auto libraryArea = content.withTrimmedTop(8);
            auto findArea = libraryArea.removeFromLeft(236).reduced(18, 14);
            auto inspectorArea = libraryArea.removeFromRight(318).reduced(18, 14);
            auto browserArea = libraryArea.reduced(18, 14);

            libraryFindLabel.setBounds(findArea.removeFromTop(24));
            presetSearchEditor.setBounds(findArea.removeFromTop(38).reduced(2, 4));
            auto quickRowA = findArea.removeFromTop(30).withTrimmedTop(2);
            auto quickRowB = findArea.removeFromTop(30).withTrimmedTop(2);
            for (size_t index = 0; index < 4; ++index)
                presetQuickFilterButtons[index].setBounds(quickRowA.removeFromLeft(quickRowA.getWidth() / static_cast<int>(4 - index)).reduced(2, 3));
            for (size_t index = 4; index < presetQuickFilterButtons.size(); ++index)
                presetQuickFilterButtons[index].setBounds(quickRowB.removeFromLeft(quickRowB.getWidth() / static_cast<int>(presetQuickFilterButtons.size() - index)).reduced(2, 3));

            auto modeRow = findArea.removeFromTop(72).withTrimmedTop(6);
            presetFilterBox.setBounds(modeRow.removeFromTop(34).reduced(2, 4));
            presetTagBox.setBounds(modeRow.reduced(2, 4));

            auto sortRefreshRow = findArea.removeFromTop(38).withTrimmedTop(3);
            refreshPresetsButton.setBounds(sortRefreshRow.removeFromRight(70).reduced(2, 4));
            presetSortBox.setBounds(sortRefreshRow.reduced(2, 4));

            auto selectedRow = findArea.removeFromTop(58).withTrimmedTop(8);
            previousPresetButton.setBounds(selectedRow.removeFromLeft(36).reduced(2, 4));
            nextPresetButton.setBounds(selectedRow.removeFromRight(36).reduced(2, 4));
            presetBox.setBounds(selectedRow.reduced(2, 4));
            auto loadActions = findArea.removeFromTop(40).withTrimmedTop(4);
            loadPresetButton.setBounds(loadActions.removeFromLeft(72).reduced(2, 4));
            auditionPresetButton.setBounds(loadActions.removeFromLeft(86).reduced(2, 4));
            favoritePresetButton.setBounds(loadActions.reduced(2, 4));
            presetStatusLabel.setBounds(findArea.reduced(2, 4));

            libraryBrowserLabel.setBounds(browserArea.removeFromTop(24));
            presetBrowserHeaderLabel.setBounds(browserArea.removeFromTop(34).reduced(6, 5));
            presetBrowserList.setBounds(browserArea.reduced(2, 5));

            auto saveArea = inspectorArea.removeFromTop(314).reduced(10, 8);
            librarySaveLabel.setBounds(saveArea.removeFromTop(24));
            auto nameRow = saveArea.removeFromTop(38).withTrimmedTop(2);
            presetNameEditor.setBounds(nameRow.reduced(2, 4));
            auto folderRow = saveArea.removeFromTop(36);
            presetCategoryBox.setBounds(folderRow.reduced(2, 4));
            auto metadataRow = saveArea.removeFromTop(36);
            presetAuthorEditor.setBounds(metadataRow.removeFromLeft(metadataRow.getWidth() / 2).reduced(2, 4));
            presetPackBox.setBounds(metadataRow.reduced(2, 4));
            auto keyRow = saveArea.removeFromTop(36);
            const auto keyCellWidth = keyRow.getWidth() / 3;
            presetKeyBox.setBounds(keyRow.removeFromLeft(keyCellWidth).reduced(2, 4));
            presetBpmBox.setBounds(keyRow.removeFromLeft(keyCellWidth).reduced(2, 4));
            presetRatingBox.setBounds(keyRow.reduced(2, 4));
            auto saveActionRow = saveArea.removeFromTop(38).withTrimmedTop(4);
            presetNotesTemplateBox.setBounds(saveActionRow.removeFromLeft(154).reduced(2, 4));
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

    const std::array<LayoutSizeAuditSpec, 4> layoutSizes {
        LayoutSizeAuditSpec { "Min", editorMinWidth, editorMinHeight },
        LayoutSizeAuditSpec { "Default", editorDefaultWidth, editorDefaultHeight },
        LayoutSizeAuditSpec { "Wide", 1180, 820 },
        LayoutSizeAuditSpec { "Max", editorMaxWidth, editorMaxHeight }
    };

    const auto originalPanel = activePanel;
    const auto originalFxModule = selectedFxModule;
    const auto originalRandomLabPage = activeRandomLabPage;
    const auto originalWidth = getWidth();
    const auto originalHeight = getHeight();
    juce::StringArray issues;

    auto auditCurrentLayout = [this, &issues] (const juce::String& panelName)
    {
        resized();
        appendVisibleLayoutIssues(*this, *this, panelName, {}, issues);
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
            else
            {
                auditCurrentLayout(layoutPrefix + panel.name);
            }
        }
    }

    setSize(originalWidth, originalHeight);
    activePanel = originalPanel;
    selectedFxModule = originalFxModule;
    activeRandomLabPage = originalRandomLabPage;
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
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    const auto tooltipText = controlFeelTooltip(labelText);
    slider.setTooltip(tooltipText);
    slider.getProperties().set("baseTooltip", tooltipText);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdce7e4));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff101619));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
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
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 62, 18);
    const auto tooltipText = controlFeelTooltip(labelText);
    slider.setTooltip(tooltipText);
    slider.getProperties().set("baseTooltip", tooltipText);
    slider.setColour(juce::Slider::trackColourId, juce::Colour(0xff8ee6c9));
    slider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff263035));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdce7e4));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff101619));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredLeft);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
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

    registerModulationMenuTarget(slider, labelText, parameterID);
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.getValueTreeState(), parameterID, slider));
}

void NateVSTAudioProcessorEditor::configureCompactHorizontalSlider(juce::Slider& slider,
                                                                    const juce::String& parameterID)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setMouseDragSensitivity(110);
    applyFineDragMode(slider, 0.38);
    slider.setSliderSnapsToMousePosition(false);
    slider.setScrollWheelEnabled(false);
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 42, 16);
    const auto tooltipText = juce::String("Curve point: drag to adjust, hold Shift or Cmd for fine movement, double-click to reset, or type a value.");
    slider.setTooltip(tooltipText);
    slider.getProperties().set("baseTooltip", tooltipText);
    slider.setNumDecimalPlacesToDisplay(2);
    slider.setColour(juce::Slider::trackColourId, juce::Colour(0xff8ee6c9));
    slider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff263035));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdce7e4));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff101619));
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
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 42, 16);
    slider.setNumDecimalPlacesToDisplay(2);
    const auto tooltipText = "Random Lab " + labelText
        + " strength: drag to scale how far generated values move this section, Shift or Cmd for fine movement, double-click to reset.";
    slider.setTooltip(tooltipText);
    slider.getProperties().set("baseTooltip", tooltipText);
    slider.setColour(juce::Slider::trackColourId, juce::Colour(0xff8ee6c9));
    slider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff263035));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdce7e4));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff101619));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centredLeft);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffa8b6b8));
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
        return;

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
    label.setColour(juce::Label::textColourId, juce::Colour(0xff8ee6c9));
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
    }
}

void NateVSTAudioProcessorEditor::updateSampleNameLabel()
{
    const auto sampleName = audioProcessor.getLoadedSampleName();
    sampleNameLabel.setText(sampleName.isNotEmpty() ? sampleName : "No sample", juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::selectSampleSlice(size_t sliceIndex)
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto sliceCount = static_cast<float>(sampleSliceButtons.size());
    if (sliceCount <= 0.0f)
        return;

    const auto safeIndex = juce::jlimit<size_t>(0, sampleSliceButtons.size() - 1, sliceIndex);
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
    const auto safeIndex = juce::jlimit<size_t>(0, sampleSliceButtons.size() - 1, sliceIndex);
    const auto styleIndex = juce::jlimit(0, 4, sampleSliceStyleBox.getSelectedItemIndex());
    const auto slicePosition = static_cast<int>(safeIndex);
    const std::array<float, 8> pitchLadder { -12.0f, -7.0f, -5.0f, 0.0f, 3.0f, 7.0f, 10.0f, 12.0f };
    const std::array<float, 8> garagePitch { -12.0f, 0.0f, 7.0f, -5.0f, 0.0f, 12.0f, 3.0f, -7.0f };
    const std::array<float, 8> garageRamp { 7.0f, 0.0f, -5.0f, 12.0f, 0.0f, -7.0f, 5.0f, -12.0f };

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
    const auto safeIndex = juce::jlimit<size_t>(0, sampleSliceButtons.size() - 1, sliceIndex);
    const auto sliceCount = static_cast<float>(sampleSliceButtons.size());
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
}

void NateVSTAudioProcessorEditor::recallSampleSliceSettings(size_t sliceIndex)
{
    const auto safeIndex = juce::jlimit<size_t>(0, sampleSliceButtons.size() - 1, sliceIndex);
    const auto sliceCount = static_cast<float>(sampleSliceButtons.size());
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

    const auto safeIndex = juce::jlimit<size_t>(0, sampleSliceButtons.size() - 1, selectedSampleSliceIndex);
    captureGlobalEdit("Recall sample slice");
    recallSampleSliceSettings(safeIndex);

    const auto didAudition = audioProcessor.triggerSampleSliceAudition(static_cast<int>(safeIndex));
    setRandomStatus("Recalled slice " + juce::String(static_cast<int>(safeIndex + 1)) + (didAudition ? " auditioned" : ""));
    updateSampleSliceButtons();
    updateSampleSliceEditorStatus();
    updateSampleWaveformDisplay();
}

void NateVSTAudioProcessorEditor::randomizeSelectedSampleSliceSettings()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto safeIndex = juce::jlimit<size_t>(0, sampleSliceButtons.size() - 1, selectedSampleSliceIndex);
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

    captureGlobalEdit("Dice sample slice");
    setPlainParameterValue(Parameters::ID::sampleTranspose, pitch);
    setPlainParameterValue(Parameters::ID::sampleGain, gain);
    setPlainParameterValue(Parameters::ID::sampleReverse, reverse ? 1.0f : 0.0f);
    setPlainParameterValue(Parameters::ID::sampleStutterEnabled, stutter ? 1.0f : 0.0f);
    setPlainParameterValue(Parameters::ID::sampleSliceChoke[safeIndex], choke ? 1.0f : 0.0f);
    setPlainParameterValue(Parameters::ID::sampleStutterRepeats, repeats);
    setPlainParameterValue(Parameters::ID::sampleSlicePan[safeIndex], pan);
    setPlainParameterValue(Parameters::ID::sampleSliceProbability[safeIndex], probability);
    captureCurrentSampleSliceSettings(safeIndex, true);

    const auto didAudition = audioProcessor.triggerSampleSliceAudition(static_cast<int>(safeIndex));
    setRandomStatus("Diced slice " + juce::String(static_cast<int>(safeIndex + 1)) + (didAudition ? " auditioned" : ""));
    updateSampleSliceButtons();
    updateSampleSliceEditorStatus();
}

void NateVSTAudioProcessorEditor::toggleSelectedSampleSliceReverse()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto safeIndex = juce::jlimit<size_t>(0, sampleSliceButtons.size() - 1, selectedSampleSliceIndex);
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
}

void NateVSTAudioProcessorEditor::toggleSelectedSampleSliceChoke()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto safeIndex = juce::jlimit<size_t>(0, sampleSliceButtons.size() - 1, selectedSampleSliceIndex);
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
}

void NateVSTAudioProcessorEditor::cycleSelectedSampleSlicePan()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto safeIndex = juce::jlimit<size_t>(0, sampleSliceButtons.size() - 1, selectedSampleSliceIndex);
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
}

void NateVSTAudioProcessorEditor::toggleSelectedSampleSliceGhost()
{
    releaseRandomCandidateAudition(false);
    releasePresetAuditionNote();

    const auto safeIndex = juce::jlimit<size_t>(0, sampleSliceButtons.size() - 1, selectedSampleSliceIndex);
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
}

bool NateVSTAudioProcessorEditor::sampleSliceHasCustomSettings(size_t sliceIndex) const
{
    const auto safeIndex = juce::jlimit<size_t>(0, sampleSliceButtons.size() - 1, sliceIndex);
    return readPlainParameterValue(Parameters::ID::sampleSliceCustom[safeIndex], 0.0f) >= 0.5f;
}

void NateVSTAudioProcessorEditor::updateSampleSliceEditorStatus()
{
    const auto safeIndex = juce::jlimit<size_t>(0, sampleSliceButtons.size() - 1, selectedSampleSliceIndex);
    const auto custom = sampleSliceHasCustomSettings(safeIndex);
    const auto sliceCount = static_cast<float>(sampleSliceButtons.size());
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
    }
    const auto sliceKeysMode = readPlainParameterValue(Parameters::ID::samplePlaybackMode, 1.0f) >= 1.5f;
    const auto pitchText = (preview.pitch >= 0.0f ? "+" : "") + juce::String(preview.pitch, 1) + "st";
    const auto status = "S" + juce::String(static_cast<int>(safeIndex + 1))
        + (custom ? " custom" : " default")
        + " | " + juce::String(juce::roundToInt(regionLower * 100.0f)) + "-"
        + juce::String(juce::roundToInt(regionUpper * 100.0f)) + "%"
        + " | " + pitchText
        + " | " + juce::String(preview.gain, 1) + "dB"
        + " | " + slicePanText(preview.pan)
        + " " + sliceChanceText(preview.probability)
        + (preview.reverse ? " | rev" : " | fwd")
        + (preview.choke ? " | choke" : " | open")
        + (preview.stutter ? " | stut x" + juce::String(preview.repeats) : " | no stut")
        + (sliceKeysMode ? " | C3-G3" : "");

    sampleSliceStatusLabel.setText(status, juce::dontSendNotification);
    sampleSliceStatusLabel.setTooltip("Selected slice memory: " + status);
    sampleSliceReverseEditButton.setToggleState(preview.reverse, juce::dontSendNotification);
    sampleSliceChokeButton.setToggleState(preview.choke, juce::dontSendNotification);
    sampleSlicePanButton.setButtonText("Pan");
    sampleSliceGhostButton.setButtonText("Ghost");
    sampleSliceGhostButton.setToggleState(preview.probability < 0.995f, juce::dontSendNotification);
    sampleSliceRecallButton.setEnabled(custom);
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
    const auto sliceCount = static_cast<float>(sampleSliceButtons.size());

    for (size_t index = 0; index < sampleSliceButtons.size(); ++index)
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
        }
        sampleSliceButtons[index].setToggleState(isSelected, juce::dontSendNotification);
        sampleSliceButtons[index].setButtonText(juce::String(static_cast<int>(index + 1)) + (custom ? "*" : ""));
        sampleSliceButtons[index].setTooltip("Slice " + juce::String(static_cast<int>(index + 1))
                                             + (custom ? " custom" : " default")
                                             + " | Region " + juce::String(juce::roundToInt(orderedSliceStart * 100.0f))
                                             + "-" + juce::String(juce::roundToInt(orderedSliceEnd * 100.0f)) + "%"
                                             + " | Pitch " + (preview.pitch >= 0.0f ? "+" : "") + juce::String(preview.pitch, 1)
                                             + " st | Gain " + juce::String(preview.gain, 1)
                                             + " dB | Pan " + slicePanText(preview.pan)
                                             + " | Chance " + sliceChanceText(preview.probability)
                                             + " | " + (preview.reverse ? "Reverse" : "Forward")
                                             + " | " + (preview.choke ? "Choke" : "Open")
                                             + " | " + (preview.stutter ? "Stutter" : "No stutter"));
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

    sampleWaveformDisplay.setModulationState(juce::jlimit(-1.0f, 1.0f, startModAmount),
                                             juce::jlimit(-1.0f, 1.0f, mixModAmount),
                                             juce::jlimit(-1.0f, 1.0f, pitchModAmount),
                                             juce::jlimit(-1.0f, 1.0f, rampModAmount),
                                             juce::jlimit(-1.0f, 1.0f, stutterModAmount),
                                             sampleRouteCount,
                                             sampleSources.joinIntoString(", "));

    const auto sampleName = audioProcessor.getLoadedSampleName();
    if (sampleName.isEmpty())
    {
        if (sampleWaveformKey.isNotEmpty())
        {
            sampleWaveformDisplay.setOverview({});
            sampleWaveformKey.clear();
        }

        sampleWaveformDisplay.setRange(0.0f, 1.0f);
        return;
    }

    if (sampleWaveformKey != sampleName)
    {
        sampleWaveformDisplay.setOverview(audioProcessor.createSamplePeakOverview(256));
        sampleWaveformKey = sampleName;
    }

    const auto start = readPlainParameterValue(Parameters::ID::sampleStart, 0.0f);
    const auto end = readPlainParameterValue(Parameters::ID::sampleEnd, 1.0f);
    sampleWaveformDisplay.setRange(start, end);
}

int NateVSTAudioProcessorEditor::selectedRandomMutationScope() const
{
    return juce::jmax(0, randomScopeBox.getSelectedId() - 1);
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
    sequencerGrid.repaint();
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
    sequencerGrid.repaint();
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
    sequencerGrid.repaint();
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
    sequencerGrid.repaint();
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
    sequencerGrid.repaint();
    updateRandomCandidateButtons();
    setRandomStatus("Rolled " + sectionName);
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
    sequencerGrid.repaint();
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
    sequencerGrid.repaint();

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
    sequencerGrid.repaint();
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
    if (category.startsWithIgnoreCase("UKG/Organ"))
        return "short organ stab for 2-step or speed-garage hooks";
    if (category.startsWithIgnoreCase("UKG/Stabs"))
        return "minor chord stab or shuffled garage response";
    if (category.startsWithIgnoreCase("UKG/Bells"))
        return "bright garage pluck, fill, or call-response hook";
    if (category.startsWithIgnoreCase("House"))
        return "warm house bass or chord layer around 120-126 BPM";
    if (category.startsWithIgnoreCase("Tech House"))
        return "rolling bass or mid punch around 124-130 BPM";
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
    const auto currentLowestNote = pianoKeyboard.getLowestVisibleKey();
    const auto nextLowestNote = juce::jlimit(keyboardLowestNote,
                                            keyboardMaxLowestVisibleNote,
                                            currentLowestNote + semitones);

    pianoKeyboard.setLowestVisibleKey(nextLowestNote);
    updateKeyboardRangeLabel();
}

void NateVSTAudioProcessorEditor::updateKeyboardRangeLabel()
{
    const auto lowestVisibleNote = pianoKeyboard.getLowestVisibleKey();
    const auto noteName = juce::MidiMessage::getMidiNoteName(lowestVisibleNote, true, true, 3);

    if (keyboardRangeLabel.getText() != noteName)
        keyboardRangeLabel.setText(noteName, juce::dontSendNotification);

    keyboardOctaveDownButton.setEnabled(lowestVisibleNote > keyboardLowestNote);
    keyboardOctaveUpButton.setEnabled(lowestVisibleNote < keyboardMaxLowestVisibleNote);
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
        case FxModule::distortion: return "saturation amount";
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
    updatePanelVisibility();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::setActiveRandomLabPage(RandomLabPage page)
{
    activeRandomLabPage = page;
    updateRandomLabPageButtons();
    resized();
    repaint();
}

void NateVSTAudioProcessorEditor::updatePanelVisibility()
{
    hidePanelComponents();
    updateTabButtons();
    updateRandomLabPageButtons();
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
        &modMatrixSourceHeaderB, &modMatrixDestinationHeaderB, &modMatrixAmountHeaderB, &modMacroAssignLabel, &modMacroAssignStatusLabel,
        &sampleSectionLabel, &sampleSourceLabel, &sampleChopLabel, &sampleShapeLabel, &sampleSliceStatusLabel, &sequencerSectionLabel,
        &hostSyncStatusLabel, &futureSectionLabel, &librarySectionLabel, &libraryFindLabel, &libraryBrowserLabel, &librarySaveLabel, &libraryInspectorLabel, &infoSectionLabel, &infoAboutLabel, &infoWorkflowLabel, &infoDetailsLabel, &infoFocusLabel, &sampleNameLabel, &presetStatusLabel, &presetBrowserHeaderLabel, &randomStatusLabel, &randomRecipeInfoLabel, &performanceStatusLabel,
        &waveformBox, &osc2WaveBox, &noiseTypeBox, &filterModeBox, &filterCharacterBox, &filterSlopeBox, &recipeBox, &randomScopeBox, &sequencerRateBox, &sequencerGrooveBox, &sequencerScaleBox, &sequencerChordBox, &sequencerVoicingBox, &sequencerPatternBox, &sequencerGrooveTransformBox, &sequencerLockDestinationBox, &sampleModeBox, &sampleSliceStyleBox, &sampleStutterRateBox, &presetBox, &presetCategoryBox,
        &presetFilterBox, &presetTagBox, &presetSortBox, &presetRatingBox, &candidateRatingBox, &presetPackBox, &presetKeyBox, &presetBpmBox, &infoTopicBox, &fxAddBox, &fxPresetBox, &fxDelayRateBox, &fxPumpRateBox, &fxPumpCurveBox, &fxTremoloRateBox, &modInspectorDestinationBox, &modInspectorSourceBox, &modMacroAssignSourceBox, &modMacroAssignDestinationBox, &lfo1ShapeBox, &lfo1SyncRateBox, &lfo2ShapeBox, &lfo2SyncRateBox, &lfoCurvePresetBox,
        &monoButton, &sampleEnabledButton, &sampleReverseButton, &sampleStutterEnabledButton, &sequencerEnabledButton, &sequencerChordMemoryButton,
        &fxDistortionEnabledButton, &fxBitcrushEnabledButton, &fxPumpEnabledButton, &fxTremoloEnabledButton, &fxRingEnabledButton, &fxCombEnabledButton, &fxChorusEnabledButton, &fxDelayEnabledButton, &fxDelaySyncButton, &fxReverbEnabledButton, &fxWidthEnabledButton,
        &fxToneEnabledButton, &fxEqEnabledButton, &fxPhaserEnabledButton, &fxGuardEnabledButton,
        &fxFlangerEnabledButton,
        &randomLockPitchButton, &randomLockEnvelopeButton, &randomLockFilterButton, &randomLockSourceButton,
        &randomLockSampleButton, &randomLockFxButton, &randomLockOutputButton, &randomLockSequencerButton,
        &lfo1SyncButton, &lfo1RetriggerButton, &lfo2SyncButton, &lfo2RetriggerButton,
        &lfoCurveInvertButton, &lfoCurveReverseButton, &lfoCurveSmoothButton,
        &lfoCurveQuantizeButton, &lfoCurveRandomButton, &lfoCurveGarageButton,
        &generateButton, &mutateButton, &variationButton, &wildMutateButton, &undoRandomButton, &redoRandomButton,
        &recallSnapshotAButton, &captureSnapshotAButton, &recallSnapshotBButton, &captureSnapshotBButton,
        &loadSampleButton, &clearSampleButton,
        &sampleSliceStoreButton, &sampleSliceRecallButton, &sampleSliceDiceButton, &sampleSliceReverseEditButton, &sampleSliceChokeButton, &sampleSlicePanButton, &sampleSliceGhostButton,
        &randomCutButton, &ukgChopButton, &randomSequencerButton, &mutateSequencerButton, &undoSequencerButton, &clearSequencerButton,
        &bassPatternButton, &stabPatternButton, &ukgPatternButton, &applyPatternButton, &copySequencerButton,
        &rotateSequencerLeftButton, &rotateSequencerRightButton, &exportSequencerMidiButton, &applyGrooveTransformButton,
        &sineWaveButton, &sawWaveButton, &squareWaveButton, &triangleWaveButton, &wavetableWaveButton,
        &osc2SineWaveButton, &osc2SawWaveButton, &osc2SquareWaveButton, &osc2TriangleWaveButton, &osc2WavetableWaveButton,
        &lowpassFilterButton, &bandpassFilterButton, &highpassFilterButton,
        &rateEighthButton, &rateSixteenthButton, &rateThirtySecondButton,
        &previousPresetButton, &nextPresetButton,
        &savePresetButton, &loadPresetButton, &auditionPresetButton, &refreshPresetsButton, &favoritePresetButton, &candidateFavoriteButton,
        &saveCandidateButton,
        &promoteCandidateAButton, &promoteCandidateBButton,
        &fxMoveUpButton, &fxMoveDownButton, &fxResetOrderButton,
        &fxThrowDelayButton, &fxThrowSpaceButton, &fxThrowPumpButton, &fxThrowDryButton,
        &fxHoldDelayButton, &fxHoldSpaceButton, &fxHoldPumpButton, &fxMuteDropButton,
        &fxApplyPresetButton, &modInspectorAddButton, &modInspectorClearButton, &infoOpenLabButton, &infoOpenModButton, &infoOpenFxButton, &infoOpenLibraryButton, &modMacroAssignAddButton, &modMacroAssignReplaceButton, &modMacroAssignClearButton,
        &fxRemoveButton, &fxToneSlotButton, &fxEqSlotButton, &fxDistortionSlotButton, &fxBitcrushSlotButton, &fxPumpSlotButton, &fxTremoloSlotButton, &fxRingSlotButton, &fxCombSlotButton, &fxPhaserSlotButton, &fxFlangerSlotButton, &fxChorusSlotButton,
        &fxDelaySlotButton, &fxReverbSlotButton, &fxWidthSlotButton, &fxGuardSlotButton,
        &presetNameEditor, &presetSearchEditor, &presetAuthorEditor, &presetNotesEditor, &presetNotesTemplateBox, &randomCandidateDetailEditor, &infoAboutEditor, &infoWorkflowEditor, &infoDetailEditor, &presetBrowserList, &fxRackStatusLabel,
        &homeOverviewDisplay, &outputSpectrumDisplay, &presetLibrarySummary, &lowEndAssistant, &performanceXYPad, &sampleWaveformDisplay, &wavetableDisplay, &filterResponseDisplay, &lfoCurveDisplay, &pumpCurveDisplay, &sequencerGrid
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

    for (auto& button : sampleSliceButtons)
        button.setVisible(false);

    for (auto& button : sequencerSceneRecallButtons)
        button.setVisible(false);

    for (auto& button : sequencerSceneCaptureButtons)
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

    const auto osc2WaveformIndex = getChoiceIndex(osc2WaveBox, Parameters::ID::osc2Wave, 1);
    osc2SineWaveButton.setToggleState(osc2WaveformIndex == 0, juce::dontSendNotification);
    osc2SawWaveButton.setToggleState(osc2WaveformIndex == 1, juce::dontSendNotification);
    osc2SquareWaveButton.setToggleState(osc2WaveformIndex == 2, juce::dontSendNotification);
    osc2TriangleWaveButton.setToggleState(osc2WaveformIndex == 3, juce::dontSendNotification);
    osc2WavetableWaveButton.setToggleState(osc2WaveformIndex == 4, juce::dontSendNotification);

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
    const auto osc1Wave = juce::roundToInt(readPlainParameterValue(Parameters::ID::oscWave, 1.0f));
    const auto osc2Wave = juce::roundToInt(readPlainParameterValue(Parameters::ID::osc2Wave, 1.0f));

    auto osc1WtModAmount = 0.0f;
    auto osc2WtModAmount = 0.0f;
    auto wtRouteCount = 0;
    juce::StringArray wtSources;
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

        if (destinationIndex == 18)
            osc1WtModAmount += amount;
        else if (destinationIndex == 19)
            osc2WtModAmount += amount;
        else
            continue;

        ++wtRouteCount;
        if (juce::isPositiveAndBelow(sourceIndex, sourceChoices.size()))
            wtSources.addIfNotAlreadyThere(sourceChoices[sourceIndex]);
    }

    wavetableDisplay.setState(
        readPlainParameterValue(Parameters::ID::oscWavetablePosition, 0.0f),
        readPlainParameterValue(Parameters::ID::osc2WavetablePosition, 0.35f),
        osc1Wave == 4,
        osc2Wave == 4,
        juce::jlimit(-1.0f, 1.0f, osc1WtModAmount),
        juce::jlimit(-1.0f, 1.0f, osc2WtModAmount),
        wtRouteCount,
        wtSources.joinIntoString(", "));
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

        modMatrixRows[index].setState(static_cast<int>(index + 1), sourceText, destinationText, amount, enabled);
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
    const auto summary = routes.isEmpty()
        ? sourceName + ": no assignments | target " + targetText
        : sourceName + ": " + routes.joinIntoString(", ");

    modMacroAssignStatusLabel.setText(summary, juce::dontSendNotification);
    modMacroAssignStatusLabel.setTooltip(summary + " | Add updates a matching route, Replace keeps only the target route for this macro");
    modMacroAssignAddButton.setButtonText(selectedRouteExists ? "Update" : "Add");
    modMacroAssignClearButton.setEnabled(! routes.isEmpty());
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

    recallSnapshotAButton.setEnabled(hasSnapshotA);
    recallSnapshotBButton.setEnabled(hasSnapshotB);
    performanceStatusLabel.setText(juce::String(hasSnapshotA ? "A ready" : "A empty")
                                       + " | "
                                       + juce::String(hasSnapshotB ? "B ready" : "B empty"),
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

    performanceXYPad.setValues(readParameter(Parameters::ID::macroMotion),
                               readParameter(Parameters::ID::macroSpace));
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

void NateVSTAudioProcessorEditor::updateSequencerSceneButtons()
{
    const std::array<juce::String, 4> sceneLabels { "A", "B", "Fill", "Drop" };
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
    sequencerGrid.setRootNote(juce::jlimit(0, 127, root + octaveOffset));
    sequencerGrid.setScaleMode(scaleMode);
}

void NateVSTAudioProcessorEditor::timerCallback()
{
    if (activePresetAuditionNote >= 0
        && juce::Time::getMillisecondCounterHiRes() >= presetAuditionNoteOffMs)
    {
        releasePresetAuditionNote();
    }

    if (activeRandomCandidateAuditionNote >= 0
        && juce::Time::getMillisecondCounterHiRes() >= randomCandidateAuditionNoteOffMs)
    {
        releaseRandomCandidateAudition(true);
    }

    updateSegmentedSelectors();
    updateLfoCurveDisplay();
    updatePumpCurveDisplay();
    updateWavetableDisplay();
    updateFilterResponseDisplay();
    updateHostSyncStatus();
    updateModMatrixRows();
    updateModInspectorStatus();
    updateMacroAssignmentEditorStatus();
    updateModDestinationIndicators();
    updateOutputMeter();
    updateOutputSpectrumDisplay();
    updateLowEndAssistant();
    updatePerformanceSnapshotButtons();
    updatePerformanceXYPad();
    updateHomeOverviewDisplay();
    updateSequencerGridContext();
    updateSampleSliceButtons();
    updateSampleWaveformDisplay();
    updateKeyboardRangeLabel();
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
    const auto macroWidth = juce::jlimit(118, 230, row.getWidth() / 3);
    auto macroArea = row.removeFromRight(macroWidth);
    row.removeFromRight(8);
    const auto infoWidth = juce::jlimit(86, 160, row.getWidth() / 3);
    auto infoArea = row.removeFromRight(infoWidth);
    row.removeFromRight(8);
    auto nameArea = row;

    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    g.setColour(nameColour);
    g.drawFittedText(sourcePrefix + preset.name,
                     nameArea.removeFromTop(15).reduced(2, 0),
                     juce::Justification::centredLeft,
                     1,
                     0.64f);

    g.setFont(juce::FontOptions(9.0f, juce::Font::plain));
    g.setColour(juce::Colour(0xff9dafb2));
    g.drawFittedText(categoryText + " | " + preset.pack,
                     nameArea.reduced(2, 0),
                     juce::Justification::centredLeft,
                     1,
                     0.58f);

    auto pill = infoArea.removeFromTop(16).toFloat().reduced(0.0f, 1.0f);
    g.setColour(preset.isFactory ? juce::Colour(0xff182b3b) : juce::Colour(0xff1d2d28));
    g.fillRoundedRectangle(pill, 4.0f);
    g.setColour(preset.isFactory ? juce::Colour(0xff7bb7ff) : juce::Colour(0xff8ee6c9));
    g.drawRoundedRectangle(pill, 4.0f, 1.0f);
    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.drawFittedText(preset.isFactory ? "FACTORY" : "USER", pill.toNearestInt(), juce::Justification::centred, 1);

    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffc5d1d0));
    g.drawFittedText(preset.key + " | " + formatPresetBpm(preset.bpm) + " | " + ratingText,
                     infoArea.reduced(1, 0),
                     juce::Justification::centred,
                     1,
                     0.56f);

    drawPresetMacroValueStrip(g, macroArea, preset);
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
            case 2: active = tagFilter == "House"; break;
            case 3: active = tagFilter == "Tech House"; break;
            case 4: active = tagFilter == "Minimal"; break;
            case 5: active = tagFilter == "UKG"; break;
            case 6: active = tagFilter == "Techno"; break;
            case 7: active = filter == "Factory"; break;
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

        const auto sourceText = preset.isFactory ? juce::String("Factory") : juce::String("User");
        const auto ratingText = preset.rating > 0 ? juce::String(preset.rating) + " Star" : juce::String("Unrated");
        const auto bpmText = formatPresetBpm(preset.bpm);
        const auto searchable = preset.name + " " + preset.category + " " + preset.source + " " + preset.tags + " "
            + preset.folder + " " + sourceText + " " + ratingText + " " + preset.author + " "
            + preset.pack + " " + preset.key + " " + bpmText + " " + preset.macroSummary + " "
            + presetMacroPreviewText(preset) + " Macro Macros Performance"
            + " " + preset.notes
            + (preset.isFavorite ? " Favorite" : "");

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

    if (filter == "Recent")
    {
        for (const auto& recentName : recentNames)
            if (const auto* preset = findPreset(recentName))
                if (matchesTag(*preset) && matchesSearch(*preset))
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
                || (filter == "120-124 BPM" && preset.bpm >= 120 && preset.bpm <= 124)
                || (filter == "125-128 BPM" && preset.bpm >= 125 && preset.bpm <= 128)
                || (filter == "129-132 BPM" && preset.bpm >= 129 && preset.bpm <= 132)
                || (filter == "133+ BPM" && preset.bpm >= 133)
                || preset.category.equalsIgnoreCase(filter)
                || leafCategory.equalsIgnoreCase(filter)
                || preset.pack.equalsIgnoreCase(filter)
                || preset.key.equalsIgnoreCase(filter)
                || preset.author.equalsIgnoreCase(filter)
                || preset.folder.startsWithIgnoreCase(filter + "/");

            if (matchesFilter && matchesTag(preset) && matchesSearch(preset))
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
    auto statusText = juce::String(presetBox.getNumItems()) + " presets | Filter: " + filter;
    if (tagFilter != "All Tags")
        statusText += " | Tag: " + tagFilter;
    statusText += " | Sort: " + sortMode;
    if (searchText.isNotEmpty())
        statusText += " | Search: " + searchText;
    if (selectedPreset != nullptr)
        statusText += " | " + presetMacroPreviewText(*selectedPreset);
    statusText += " | User: " + audioProcessor.getPresetDirectory().getFullPathName();
    presetStatusLabel.setText(statusText, juce::dontSendNotification);
    presetStatusLabel.setTooltip(selectedPreset != nullptr
                                     ? presetMacroPreviewText(*selectedPreset) + " | " + selectedPreset->pack + " | " + selectedPreset->key + " | " + formatPresetBpm(selectedPreset->bpm)
                                           + (selectedPreset->notes.trim().isNotEmpty() ? " | " + selectedPreset->notes.replaceCharacter('\n', ' ') : juce::String())
                                     : juce::String("Preset browser status"));
    presetBox.setTooltip(selectedPreset != nullptr
                             ? selectedPreset->name + " | " + presetMacroPreviewText(*selectedPreset)
                             : juce::String("Select a preset"));
    updateFavoritePresetButton();
    updatePresetLibrarySummary();
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
        state.macroValues = preset.macroValues;
        break;
    }

    if (! state.hasSelection)
    {
        state.selectedName = state.totalCount > 0 ? "Choose a preset" : "No presets found";
        state.selectedCategory = "All folders";
        state.selectedPack = juce::String(state.totalCount) + " total";
        state.selectedRating = juce::String(state.favoriteCount) + " fav";
    }

    presetLibrarySummary.setState(state);
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
            tag = "House";
            label = "House";
            break;
        case 3:
            tag = "Tech House";
            label = "Tech House";
            break;
        case 4:
            tag = "Minimal";
            label = "Minimal";
            break;
        case 5:
            tag = "UKG";
            label = "UKG";
            break;
        case 6:
            tag = "Techno";
            label = "Techno";
            break;
        case 7:
            filter = "Factory";
            label = "Factory";
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

void NateVSTAudioProcessorEditor::saveCurrentPreset()
{
    releaseRandomCandidateAudition(false);

    auto presetName = presetNameEditor.getText().trim();
    if (presetName.isEmpty())
        presetName = presetBox.getText().trim();

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

    if (audioProcessor.savePreset(presetName, options))
    {
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
        return;
    }

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
    if (presetName.isNotEmpty() && audioProcessor.getPresetNames().contains(presetName))
        captureGlobalEdit("Load preset " + presetName);

    if (audioProcessor.loadPreset(presetName))
    {
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
        presetStatusLabel.setText("Loaded " + presetName, juce::dontSendNotification);
        updateFavoritePresetButton();
        updateSampleNameLabel();
        sequencerGrid.repaint();
        return;
    }

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

    if (! audioProcessor.loadPreset(presetName))
    {
        presetStatusLabel.setText("Select a preset to audition", juce::dontSendNotification);
        return;
    }

    refreshPresetList();
    presetBox.setText(presetName, juce::dontSendNotification);
    presetNameEditor.setText(presetName, juce::dontSendNotification);
    updateFavoritePresetButton();
    updateSampleNameLabel();
    sequencerGrid.repaint();

    auto readParameter = [this] (const juce::String& parameterID, float fallback)
    {
        if (const auto* value = audioProcessor.getValueTreeState().getRawParameterValue(parameterID))
            return value->load();

        return fallback;
    };

    auto note = juce::roundToInt(readParameter(Parameters::ID::sequencerRoot, 60.0f)
                                 + (readParameter(Parameters::ID::sequencerOctave, 0.0f) * 12.0f));
    while (note < 48)
        note += 12;
    while (note > 72)
        note -= 12;

    activePresetAuditionNote = juce::jlimit(0, 127, note);
    presetAuditionNoteOffMs = juce::Time::getMillisecondCounterHiRes() + presetAuditionDurationMs;
    audioProcessor.getMidiKeyboardState().noteOn(1, activePresetAuditionNote, presetAuditionVelocity);

    presetStatusLabel.setText("Auditioning " + presetName + " | "
                                  + juce::MidiMessage::getMidiNoteName(activePresetAuditionNote, true, true, 3),
                              juce::dontSendNotification);
}

void NateVSTAudioProcessorEditor::releasePresetAuditionNote()
{
    if (activePresetAuditionNote < 0)
        return;

    audioProcessor.getMidiKeyboardState().noteOff(1, activePresetAuditionNote, 0.0f);
    activePresetAuditionNote = -1;
    presetAuditionNoteOffMs = 0.0;
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
    favoritePresetButton.setEnabled(presetName.isNotEmpty());
    favoritePresetButton.setToggleState(presetName.isNotEmpty() && audioProcessor.isPresetFavorite(presetName),
                                        juce::dontSendNotification);
    presetRatingBox.setEnabled(presetName.isNotEmpty());
    presetRatingBox.setSelectedId(presetName.isNotEmpty() ? audioProcessor.getPresetRating(presetName) + 1 : 1,
                                  juce::dontSendNotification);

    if (presetName.isEmpty())
        return;

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
        presetBox.setTooltip(preset.name + " | " + preview);
        presetStatusLabel.setTooltip(preview + " | " + preset.pack + " | " + preset.key + " | " + formatPresetBpm(preset.bpm));
        break;
    }

    updatePresetLibrarySummary();
}
