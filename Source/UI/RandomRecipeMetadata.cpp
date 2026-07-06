#include "RandomRecipeMetadata.h"

#include <array>

namespace UI::RandomRecipeMetadata
{
namespace
{
struct RecipeInfo
{
    const char* name = "";
    const char* genre = "";
    const char* tempo = "";
    const char* goodFor = "";
    const char* bias = "";
};

RecipeInfo infoForName(const juce::String& recipeName)
{
    static const std::array recipes {
        RecipeInfo { "Deep House Bass", "Deep House", "120-126 BPM", "round basslines, warm chords, low drive", "sub weight, warm filter, light pump" },
        RecipeInfo { "Rolling Tech Bass", "Tech House", "124-130 BPM", "rolling bass, tight groove, mid punch", "short envelope, drive, 24 dB filter motion" },
        RecipeInfo { "Acid Line", "Acid / Techno", "128-134 BPM", "rubber riffs, resonant hooks, automation", "acid character, resonance, sync movement" },
        RecipeInfo { "Minimal Blip", "Minimal", "124-130 BPM", "short plucks, sparse riffs, percussive tops", "fast envelope, step motion, restrained space" },
        RecipeInfo { "Dark Stab", "Techno", "128-138 BPM", "warehouse hits, chord stabs, tense movement", "dirty filter, darker WT edge, Guard safety" },
        RecipeInfo { "Noise FX", "Techno / FX", "Any Tempo", "risers, impacts, fills, noisy transitions", "noise source, wide FX, stepped motion" },
        RecipeInfo { "UKG 2-Step Bass", "UKG", "130-134 BPM", "swung bass, skippy garage drops, mono subs", "centered sub, garage pump, safe width" },
        RecipeInfo { "UKG Organ Stab", "UKG", "130-134 BPM", "short organ hits, speed-garage stabs", "warm filter, quick amp, delay/room space" },
        RecipeInfo { "UKG Chord Stab", "UKG", "130-134 BPM", "minor stabs, late chords, shuffled hooks", "chord color, strum-friendly motion, pump" },
        RecipeInfo { "UKG Bell Pluck", "UKG", "130-136 BPM", "bright plucks, metallic hooks, call-response", "clean transient, WT brightness, short tails" },
        RecipeInfo { "UKG Dred Bass", "UKG / Speed Garage", "132-136 BPM", "Dred/Reese bass, darker 2-step pressure", "mono sub, detuned upper bass, slow filter pull" },
        RecipeInfo { "Afro House Pluck", "Afro House", "120-124 BPM", "syncopated plucks, chant hooks, warm percussion", "bright pluck, moderate motion, clean low end" },
        RecipeInfo { "Amapiano Log Bass", "Amapiano", "112-116 BPM", "round log bass, sparse groove, soft transient", "warm low tone, light drive, pitch-safe movement" },
        RecipeInfo { "Bass House Reece", "Bass House", "124-128 BPM", "wide upper bass, mono sub support, dirty hooks", "Reese detune, drive, clipped club safety" },
        RecipeInfo { "Dub Techno Chord", "Dub Techno", "122-128 BPM", "dark chords, long tails, smoky stabs", "darker filter, softer chaos, spacious motion" },
        RecipeInfo { "Detroit Tech Stab", "Detroit Techno", "126-132 BPM", "classic stab riffs, chord memory, machine soul", "mid punch, guarded drive, steady LFO" },
        RecipeInfo { "Hardgroove Rumble", "Hardgroove", "132-138 BPM", "rolling rumble, percussion pressure, hypnotic bass", "low tilt, drive, sequencer and pump motion" },
        RecipeInfo { "Minimal House Pluck", "Minimal House", "122-126 BPM", "small plucks, restrained hooks, dry groove", "low chaos, bright transient, tight envelope" },
        RecipeInfo { "Microhouse Perc Blip", "Microhouse", "122-128 BPM", "clicky blips, micro percussion, short motifs", "bright tone, high motion, small FX throws" },
        RecipeInfo { "Garage Vocal Chop", "Garage", "130-134 BPM", "vocal-like chops, shuffled call-response, hook fills", "bright bell engine, more motion, short tails" },
        RecipeInfo { "Future Garage Pad", "Future Garage", "130-136 BPM", "washed chords, moody beds, atmospheric hooks", "soft drive, darker tone, wider motion" },
        RecipeInfo { "Tech House Chord Tool", "Tech House", "124-128 BPM", "stab chords, groove tools, DJ-friendly hooks", "chord stab engine, mild drive, pump-ready" },
        RecipeInfo { "Peak Techno Acid", "Peak Techno", "132-140 BPM", "acid lead pressure, rave hooks, automation", "bright resonance, more drive, high chaos" },
        RecipeInfo { "Lo-Fi House Keys", "Lo-Fi House", "118-124 BPM", "dusty key stabs, soft chords, mellow riffs", "darker keys, low chaos, gentle motion" },
        RecipeInfo { "Tribal Tech Perc", "Tribal Tech", "124-130 BPM", "percussive blips, tribal loops, call hits", "short envelope, motion bias, drive-safe FX" }
    };

    for (const auto& recipe : recipes)
        if (recipeName.equalsIgnoreCase(recipe.name))
            return recipe;

    return recipes.front();
}
}

juce::String infoText(const juce::String& recipeName)
{
    const auto info = infoForName(recipeName.trim().isNotEmpty() ? recipeName.trim()
                                                                 : juce::String("Deep House Bass"));
    return juce::String(info.genre) + " | " + info.tempo
        + " | Good for: " + info.goodFor
        + " | Bias: " + info.bias;
}
}
