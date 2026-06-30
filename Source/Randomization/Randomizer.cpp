#include "Randomizer.h"

#include <array>
#include <cmath>

namespace Randomization
{
Randomizer::Randomizer(Parameters::APVTS& state)
    : parameters(state),
      rng(std::random_device{}())
{
}

void Randomizer::generate()
{
    const auto profile = recipeProfileForChoice(readChoice(Parameters::ID::randomRecipe));
    const auto amount = juce::jlimit(0.02f, 1.0f, readFloat(Parameters::ID::randomAmount) * profile.amountScale);
    const auto chaos = juce::jlimit(0.0f, 1.0f, readFloat(Parameters::ID::randomChaos) + profile.chaosOffset);
    randomizeForRecipe(profile, amount, chaos, false);
}

void Randomizer::mutate()
{
    const auto profile = recipeProfileForChoice(readChoice(Parameters::ID::randomRecipe));
    const auto amount = juce::jlimit(0.05f, 1.0f, readFloat(Parameters::ID::randomAmount) * 0.65f * profile.amountScale);
    const auto chaos = juce::jlimit(0.0f, 1.0f, readFloat(Parameters::ID::randomChaos) + (profile.chaosOffset * 0.75f));
    randomizeForRecipe(profile, amount, chaos, true);
}

void Randomizer::wildMutate()
{
    const auto profile = recipeProfileForChoice(readChoice(Parameters::ID::randomRecipe));
    const auto amount = juce::jlimit(0.35f, 1.0f, readFloat(Parameters::ID::randomAmount) * 1.15f * profile.amountScale);
    const auto chaos = juce::jlimit(0.2f, 1.0f, (readFloat(Parameters::ID::randomChaos) * 1.25f) + 0.15f + profile.chaosOffset);
    randomizeForRecipe(profile, amount, chaos, false);
}

void Randomizer::variation()
{
    const auto profile = recipeProfileForChoice(readChoice(Parameters::ID::randomRecipe));
    const auto amount = juce::jlimit(0.02f, 0.35f, readFloat(Parameters::ID::randomAmount) * 0.25f * profile.amountScale);
    const auto chaos = juce::jlimit(0.0f, 1.0f, (readFloat(Parameters::ID::randomChaos) * 0.35f) + (profile.chaosOffset * 0.45f));
    randomizeForRecipe(profile, amount, chaos, true);
}

float Randomizer::randomFloat(float min, float max)
{
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(rng);
}

int Randomizer::randomInt(int min, int max)
{
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(rng);
}

float Randomizer::readFloat(const juce::String& parameterID) const
{
    if (auto* value = parameters.getRawParameterValue(parameterID))
        return value->load();

    return 0.0f;
}

int Randomizer::readChoice(const juce::String& parameterID) const
{
    return static_cast<int>(std::round(readFloat(parameterID)));
}

void Randomizer::setParameter(const juce::String& parameterID, float plainValue)
{
    if (auto* parameter = parameters.getParameter(parameterID))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
        parameter->endChangeGesture();
    }
}

void Randomizer::setChoice(const juce::String& parameterID, int choiceIndex)
{
    setParameter(parameterID, static_cast<float>(choiceIndex));
}

Randomizer::RecipeProfile Randomizer::recipeProfileForChoice(int choiceIndex) const
{
    switch (choiceIndex)
    {
        case 0: return { Recipe::deepHouseBass };
        case 1: return { Recipe::rollingTechBass };
        case 2: return { Recipe::acidLine };
        case 3: return { Recipe::minimalBlip };
        case 4: return { Recipe::darkStab };
        case 5: return { Recipe::noiseFx };
        case 6: return { Recipe::ukgTwoStepBass };
        case 7: return { Recipe::ukgOrganStab };
        case 8: return { Recipe::ukgChordStab };
        case 9: return { Recipe::ukgBellPluck };
        case 10: return { Recipe::ukgDredBass };
        case 11: return { Recipe::ukgBellPluck, 1.00f, -0.04f, 0.18f, -0.04f, 0.18f };
        case 12: return { Recipe::deepHouseBass, 1.05f, -0.02f, -0.12f, 0.04f, 0.10f };
        case 13: return { Recipe::ukgDredBass, 1.12f, 0.12f, 0.04f, 0.18f, 0.08f };
        case 14: return { Recipe::darkStab, 0.86f, -0.06f, -0.36f, 0.02f, -0.08f };
        case 15: return { Recipe::darkStab, 0.96f, 0.04f, -0.04f, 0.10f, 0.04f };
        case 16: return { Recipe::rollingTechBass, 1.10f, 0.08f, -0.18f, 0.16f, 0.18f };
        case 17: return { Recipe::minimalBlip, 0.84f, -0.06f, 0.14f, -0.08f, 0.04f };
        case 18: return { Recipe::minimalBlip, 0.96f, 0.06f, 0.34f, -0.02f, 0.22f };
        case 19: return { Recipe::ukgBellPluck, 1.02f, 0.06f, 0.24f, -0.06f, 0.28f };
        case 20: return { Recipe::darkStab, 0.76f, -0.08f, -0.08f, -0.12f, 0.16f };
        case 21: return { Recipe::ukgChordStab, 1.02f, 0.04f, 0.08f, 0.06f, 0.10f };
        case 22: return { Recipe::acidLine, 1.14f, 0.12f, 0.24f, 0.20f, 0.16f };
        case 23: return { Recipe::ukgOrganStab, 0.84f, -0.08f, -0.22f, -0.08f, -0.04f };
        case 24: return { Recipe::minimalBlip, 1.04f, 0.08f, 0.10f, 0.08f, 0.24f };
        default: return { Recipe::deepHouseBass };
    }
}

void Randomizer::randomizeForRecipe(const RecipeProfile& profile, float amount, float chaos, bool subtle)
{
    const auto recipe = profile.recipe;
    const auto brightnessBias = juce::jlimit(-1.0f, 1.0f, readFloat(Parameters::ID::randomBrightnessBias) + profile.brightnessOffset);
    const auto driveBias = juce::jlimit(-1.0f, 1.0f, readFloat(Parameters::ID::randomDriveBias) + profile.driveOffset);
    const auto motionBias = juce::jlimit(-1.0f, 1.0f, readFloat(Parameters::ID::randomMotionBias) + profile.motionOffset);
    const auto wildness = juce::jlimit(0.0f, 1.0f, (amount * 0.75f) + (chaos * 0.25f));

    auto attack = 0.004f;
    auto decay = 0.18f;
    auto sustain = 0.55f;
    auto release = 0.18f;
    auto cutoff = 1200.0f;
    auto resonance = 0.45f;
    auto envAmount = 0.2f;
    auto drive = 0.18f;
    auto wave = 1;
    auto osc2Wave = 1;
    auto filterMode = 0;
    auto filterCharacter = 0;
    auto filterSlope = 0;
    auto octave = 0;
    auto osc2Octave = 0;
    auto tune = 0.0f;
    auto osc2Tune = 0.0f;
    auto osc1Level = 1.0f;
    auto osc2Level = 0.0f;
    auto subLevel = 0.0f;
    auto noiseLevel = 0.0f;
    auto noiseTypeIndex = 0;
    auto noiseDecay = 0.18f;
    auto oscWarpAmount = 0.0f;
    auto oscWavetablePosition = 0.0f;
    auto osc2WavetablePosition = 0.35f;
    auto unisonVoiceCount = 1;
    auto unisonDetune = 0.0f;
    auto unisonBlend = 0.65f;
    auto unisonSpread = 0.0f;
    auto macroTone = 0.0f;
    auto macroDirt = 0.0f;
    auto macroMotion = 0.0f;
    auto macroSpace = 0.0f;
    auto macroWeight = 0.0f;
    auto macroBounce = 0.0f;
    auto macroWarp = 0.0f;
    auto macroThrow = 0.0f;
    auto fxSendDelay = 0.0f;
    auto fxSendReverb = 0.0f;
    auto fxToneEnabled = false;
    auto fxToneTilt = 0.0f;
    auto fxToneLowCut = 30.0f;
    auto fxEqEnabled = false;
    auto fxEqLowGain = 0.0f;
    auto fxEqMidGain = 0.0f;
    auto fxEqHighGain = 0.0f;
    auto fxEqTrim = 0.0f;
    auto fxBitcrushEnabled = false;
    auto fxBitcrushBits = 12.0f;
    auto fxBitcrushDownsample = 1.0f;
    auto fxBitcrushMix = 0.25f;
    auto fxPumpEnabled = false;
    auto fxPumpRate = 0;
    auto fxPumpCurve = 0;
    auto fxPumpDepth = 0.35f;
    auto fxPumpShape = 0.45f;
    auto fxPumpPhase = 0.0f;
    std::array<float, 8> fxPumpCustomCurve { 1.0f, 0.82f, 0.62f, 0.44f, 0.28f, 0.16f, 0.07f, 0.0f };
    auto fxTremoloEnabled = false;
    auto fxTremoloRate = 1;
    auto fxTremoloDepth = 0.28f;
    auto fxTremoloPan = 0.25f;
    auto fxTremoloShape = 0.45f;
    auto fxTremoloPhase = 0.0f;
    auto fxRingEnabled = false;
    auto fxRingFrequency = 72.0f;
    auto fxRingDepth = 0.35f;
    auto fxRingMix = 0.18f;
    auto fxRingBias = 0.45f;
    auto fxCombEnabled = false;
    auto fxCombFrequency = 180.0f;
    auto fxCombFeedback = 0.28f;
    auto fxCombDamping = 0.35f;
    auto fxCombMix = 0.16f;
    auto fxWidthEnabled = false;
    auto fxWidthAmount = 1.15f;
    auto fxWidthMonoCutoff = 120.0f;
    auto fxPhaserEnabled = false;
    auto fxPhaserRate = 0.32f;
    auto fxPhaserDepth = 0.42f;
    auto fxPhaserMix = 0.22f;
    auto fxFlangerEnabled = false;
    auto fxFlangerRate = 0.22f;
    auto fxFlangerDepth = 0.32f;
    auto fxFlangerFeedback = 0.18f;
    auto fxFlangerMix = 0.18f;
    auto fxGuardEnabled = false;
    auto fxGuardPush = 0.0f;
    auto fxGuardGlue = 0.0f;
    auto fxGuardPunch = 0.0f;
    auto fxGuardClipMix = 1.0f;
    auto fxGuardCeiling = 0.92f;
    auto mono = true;
    auto glide = 0.0f;

    switch (recipe)
    {
        case Recipe::deepHouseBass:
            wave = randomInt(0, 2);
            osc2Wave = randomInt(1, 2);
            octave = randomInt(-2, -1);
            osc2Octave = octave;
            osc2Tune = randomFloat(-0.12f, 0.12f);
            osc1Level = randomFloat(0.65f, 1.0f);
            osc2Level = randomFloat(0.0f, 0.35f);
            subLevel = randomFloat(0.35f, 0.85f);
            cutoff = randomFloat(250.0f, 1400.0f);
            resonance = randomFloat(0.18f, 0.55f);
            envAmount = randomFloat(0.05f, 0.35f);
            decay = randomFloat(0.16f, 0.55f);
            sustain = randomFloat(0.35f, 0.8f);
            release = randomFloat(0.08f, 0.28f);
            drive = randomFloat(0.05f, 0.32f);
            filterCharacter = randomFloat(0.0f, 1.0f) < 0.72f ? 1 : 0;
            filterSlope = randomFloat(0.0f, 1.0f) < 0.62f ? 1 : 0;
            oscWarpAmount = randomFloat(0.02f, 0.18f);
            glide = randomFloat(0.0f, 0.08f);
            unisonVoiceCount = 1;
            fxToneEnabled = true;
            fxToneTilt = randomFloat(-0.25f, 0.05f);
            fxToneLowCut = randomFloat(25.0f, 45.0f);
            fxPumpEnabled = randomFloat(0.0f, 1.0f) < 0.28f + (chaos * 0.12f);
            fxPumpRate = randomInt(0, 1);
            fxPumpDepth = randomFloat(0.12f, 0.32f);
            fxPumpShape = randomFloat(0.28f, 0.62f);
            fxPumpPhase = randomFloat(0.0f, 0.12f);
            fxWidthEnabled = randomFloat(0.0f, 1.0f) < 0.18f + (chaos * 0.1f);
            fxWidthAmount = randomFloat(0.95f, 1.18f);
            fxWidthMonoCutoff = randomFloat(85.0f, 135.0f);
            fxGuardEnabled = true;
            fxGuardPush = randomFloat(0.0f, 0.12f);
            fxGuardCeiling = randomFloat(0.9f, 0.96f);
            break;

        case Recipe::rollingTechBass:
            wave = randomInt(1, 2);
            osc2Wave = randomInt(1, 3);
            octave = randomInt(-2, -1);
            osc2Octave = octave;
            osc2Tune = randomFloat(-0.18f, 0.18f);
            osc1Level = randomFloat(0.65f, 1.0f);
            osc2Level = randomFloat(0.18f, 0.55f);
            subLevel = randomFloat(0.18f, 0.55f);
            cutoff = randomFloat(350.0f, 2100.0f);
            resonance = randomFloat(0.25f, 0.8f);
            envAmount = randomFloat(0.1f, 0.55f);
            decay = randomFloat(0.08f, 0.32f);
            sustain = randomFloat(0.15f, 0.58f);
            release = randomFloat(0.035f, 0.2f);
            drive = randomFloat(0.18f, 0.55f);
            filterCharacter = randomFloat(0.0f, 1.0f) < 0.62f ? 2 : 3;
            filterSlope = 1;
            oscWarpAmount = randomFloat(0.12f, 0.34f);
            glide = randomFloat(0.0f, 0.12f);
            unisonVoiceCount = randomInt(1, 2);
            unisonDetune = randomFloat(0.0f, 0.06f);
            unisonBlend = randomFloat(0.25f, 0.5f);
            unisonSpread = randomFloat(0.0f, 0.18f);
            fxToneEnabled = true;
            fxToneTilt = randomFloat(-0.12f, 0.2f);
            fxToneLowCut = randomFloat(32.0f, 70.0f);
            fxBitcrushEnabled = randomFloat(0.0f, 1.0f) < 0.18f + (chaos * 0.12f);
            fxBitcrushBits = randomFloat(9.0f, 14.0f);
            fxBitcrushDownsample = randomFloat(1.0f, 5.0f);
            fxBitcrushMix = randomFloat(0.05f, 0.18f);
            fxPumpEnabled = randomFloat(0.0f, 1.0f) < 0.38f + (chaos * 0.18f);
            fxPumpRate = randomInt(0, 2);
            fxPumpDepth = randomFloat(0.12f, 0.38f);
            fxPumpShape = randomFloat(0.3f, 0.72f);
            fxPumpPhase = randomFloat(0.0f, 0.18f);
            fxWidthEnabled = randomFloat(0.0f, 1.0f) < 0.22f + (chaos * 0.12f);
            fxWidthAmount = randomFloat(0.95f, 1.22f);
            fxWidthMonoCutoff = randomFloat(90.0f, 150.0f);
            fxGuardEnabled = true;
            fxGuardPush = randomFloat(0.04f, 0.2f);
            fxGuardCeiling = randomFloat(0.88f, 0.94f);
            break;

        case Recipe::acidLine:
            wave = randomInt(1, 2);
            osc2Wave = randomInt(1, 2);
            octave = randomInt(-1, 0);
            osc2Octave = octave;
            osc2Tune = randomFloat(-0.08f, 0.08f);
            osc1Level = randomFloat(0.75f, 1.0f);
            osc2Level = randomFloat(0.0f, 0.3f);
            subLevel = randomFloat(0.0f, 0.25f);
            cutoff = randomFloat(600.0f, 4200.0f);
            resonance = randomFloat(0.65f, 1.2f);
            envAmount = randomFloat(0.35f, 0.95f);
            decay = randomFloat(0.06f, 0.22f);
            sustain = randomFloat(0.0f, 0.28f);
            release = randomFloat(0.03f, 0.16f);
            drive = randomFloat(0.28f, 0.72f);
            filterCharacter = 2;
            filterSlope = 1;
            oscWarpAmount = randomFloat(0.18f, 0.44f);
            glide = randomFloat(0.03f, 0.22f);
            unisonVoiceCount = 1;
            fxToneEnabled = true;
            fxToneTilt = randomFloat(0.04f, 0.35f);
            fxToneLowCut = randomFloat(35.0f, 80.0f);
            fxPhaserEnabled = randomFloat(0.0f, 1.0f) < 0.35f + (chaos * 0.25f);
            fxPhaserRate = randomFloat(0.12f, 0.7f);
            fxPhaserDepth = randomFloat(0.25f, 0.7f);
            fxPhaserMix = randomFloat(0.08f, 0.28f);
            fxPumpEnabled = randomFloat(0.0f, 1.0f) < 0.16f + (chaos * 0.12f);
            fxPumpRate = randomInt(0, 1);
            fxPumpDepth = randomFloat(0.08f, 0.24f);
            fxPumpShape = randomFloat(0.35f, 0.75f);
            fxPumpPhase = randomFloat(0.0f, 0.12f);
            fxWidthEnabled = randomFloat(0.0f, 1.0f) < 0.18f + (chaos * 0.1f);
            fxWidthAmount = randomFloat(0.9f, 1.15f);
            fxWidthMonoCutoff = randomFloat(80.0f, 140.0f);
            fxGuardEnabled = true;
            fxGuardPush = randomFloat(0.08f, 0.26f);
            fxGuardCeiling = randomFloat(0.86f, 0.93f);
            break;

        case Recipe::minimalBlip:
            wave = randomFloat(0.0f, 1.0f) < 0.22f + (chaos * 0.12f) ? 4 : randomInt(0, 3);
            osc2Wave = randomFloat(0.0f, 1.0f) < 0.18f + (chaos * 0.1f) ? 4 : randomInt(0, 3);
            filterMode = randomInt(0, 1);
            octave = randomInt(-1, 1);
            osc2Octave = octave + randomInt(-1, 1);
            osc2Tune = randomFloat(-7.0f, 7.0f);
            osc1Level = randomFloat(0.55f, 1.0f);
            osc2Level = randomFloat(0.0f, 0.5f);
            noiseLevel = randomFloat(0.0f, 0.28f);
            noiseTypeIndex = randomInt(3, 6);
            noiseDecay = randomFloat(0.012f, 0.18f);
            cutoff = randomFloat(900.0f, 7000.0f);
            resonance = randomFloat(0.25f, 0.95f);
            envAmount = randomFloat(-0.15f, 0.45f);
            decay = randomFloat(0.025f, 0.16f);
            sustain = randomFloat(0.0f, 0.18f);
            release = randomFloat(0.01f, 0.12f);
            drive = randomFloat(0.02f, 0.38f);
            filterCharacter = randomInt(0, 3);
            filterSlope = randomFloat(0.0f, 1.0f) < 0.38f ? 1 : 0;
            oscWarpAmount = randomFloat(0.05f, 0.52f);
            unisonVoiceCount = randomInt(1, 3);
            unisonDetune = randomFloat(0.0f, 0.12f);
            unisonBlend = randomFloat(0.3f, 0.65f);
            unisonSpread = randomFloat(0.15f, 0.55f);
            mono = false;
            fxToneEnabled = true;
            fxToneTilt = randomFloat(-0.1f, 0.28f);
            fxToneLowCut = randomFloat(35.0f, 95.0f);
            fxBitcrushEnabled = randomFloat(0.0f, 1.0f) < 0.35f + (chaos * 0.2f);
            fxBitcrushBits = randomFloat(7.0f, 13.0f);
            fxBitcrushDownsample = randomFloat(1.0f, 9.0f);
            fxBitcrushMix = randomFloat(0.05f, 0.25f);
            fxPhaserEnabled = randomFloat(0.0f, 1.0f) < 0.5f;
            fxPhaserRate = randomFloat(0.18f, 1.4f);
            fxPhaserDepth = randomFloat(0.25f, 0.82f);
            fxPhaserMix = randomFloat(0.08f, 0.35f);
            fxPumpEnabled = randomFloat(0.0f, 1.0f) < 0.24f + (chaos * 0.16f);
            fxPumpRate = randomInt(0, 3);
            fxPumpDepth = randomFloat(0.08f, 0.36f);
            fxPumpShape = randomFloat(0.25f, 0.82f);
            fxPumpPhase = randomFloat(0.0f, 0.22f);
            fxWidthEnabled = randomFloat(0.0f, 1.0f) < 0.36f + (chaos * 0.16f);
            fxWidthAmount = randomFloat(1.0f, 1.42f);
            fxWidthMonoCutoff = randomFloat(100.0f, 180.0f);
            fxGuardEnabled = true;
            fxGuardPush = randomFloat(0.0f, 0.16f);
            fxGuardCeiling = randomFloat(0.88f, 0.95f);
            break;

        case Recipe::darkStab:
            wave = randomInt(1, 3);
            osc2Wave = randomInt(1, 3);
            octave = randomInt(-1, 0);
            osc2Octave = octave + randomInt(0, 1);
            osc2Tune = randomFloat(-0.25f, 0.25f);
            osc1Level = randomFloat(0.55f, 0.9f);
            osc2Level = randomFloat(0.25f, 0.75f);
            subLevel = randomFloat(0.0f, 0.25f);
            noiseLevel = randomFloat(0.0f, 0.18f);
            noiseTypeIndex = randomInt(3, 5);
            noiseDecay = randomFloat(0.025f, 0.22f);
            cutoff = randomFloat(450.0f, 2800.0f);
            resonance = randomFloat(0.15f, 0.65f);
            envAmount = randomFloat(0.05f, 0.4f);
            decay = randomFloat(0.18f, 0.75f);
            sustain = randomFloat(0.25f, 0.75f);
            release = randomFloat(0.12f, 0.8f);
            drive = randomFloat(0.15f, 0.62f);
            filterCharacter = randomFloat(0.0f, 1.0f) < 0.58f ? 3 : 1;
            filterSlope = randomFloat(0.0f, 1.0f) < 0.7f ? 1 : 0;
            oscWarpAmount = randomFloat(0.12f, 0.38f);
            unisonVoiceCount = randomInt(2, 5);
            unisonDetune = randomFloat(0.04f, 0.18f);
            unisonBlend = randomFloat(0.45f, 0.85f);
            unisonSpread = randomFloat(0.35f, 0.8f);
            mono = false;
            fxToneEnabled = true;
            fxToneTilt = randomFloat(-0.45f, -0.05f);
            fxToneLowCut = randomFloat(45.0f, 110.0f);
            fxPhaserEnabled = randomFloat(0.0f, 1.0f) < 0.35f;
            fxPhaserRate = randomFloat(0.08f, 0.55f);
            fxPhaserDepth = randomFloat(0.35f, 0.85f);
            fxPhaserMix = randomFloat(0.08f, 0.3f);
            fxPumpEnabled = randomFloat(0.0f, 1.0f) < 0.3f + (chaos * 0.16f);
            fxPumpRate = randomInt(0, 1);
            fxPumpDepth = randomFloat(0.1f, 0.34f);
            fxPumpShape = randomFloat(0.28f, 0.68f);
            fxPumpPhase = randomFloat(0.0f, 0.16f);
            fxWidthEnabled = randomFloat(0.0f, 1.0f) < 0.45f + (chaos * 0.18f);
            fxWidthAmount = randomFloat(1.05f, 1.5f);
            fxWidthMonoCutoff = randomFloat(110.0f, 190.0f);
            fxGuardEnabled = true;
            fxGuardPush = randomFloat(0.04f, 0.22f);
            fxGuardCeiling = randomFloat(0.86f, 0.94f);
            break;

        case Recipe::noiseFx:
            wave = randomFloat(0.0f, 1.0f) < 0.38f + (chaos * 0.18f) ? 4 : randomInt(0, 3);
            osc2Wave = randomFloat(0.0f, 1.0f) < 0.32f + (chaos * 0.16f) ? 4 : randomInt(0, 3);
            filterMode = randomInt(0, 2);
            octave = randomInt(-2, 2);
            osc2Octave = randomInt(-2, 2);
            tune = randomFloat(-7.0f, 7.0f) * chaos;
            osc2Tune = randomFloat(-12.0f, 12.0f) * chaos;
            osc1Level = randomFloat(0.0f, 0.65f);
            osc2Level = randomFloat(0.0f, 0.65f);
            subLevel = randomFloat(0.0f, 0.35f);
            noiseLevel = randomFloat(0.35f, 1.0f);
            noiseTypeIndex = randomInt(3, 6);
            noiseDecay = randomFloat(0.12f, 1.4f);
            cutoff = randomFloat(250.0f, 12000.0f);
            resonance = randomFloat(0.15f, 1.15f);
            envAmount = randomFloat(-0.7f, 0.9f);
            attack = randomFloat(0.001f, 0.45f * wildness + 0.02f);
            decay = randomFloat(0.04f, 1.5f);
            sustain = randomFloat(0.0f, 0.85f);
            release = randomFloat(0.03f, 2.0f);
            drive = randomFloat(0.1f, 0.9f);
            filterCharacter = 3;
            filterSlope = randomFloat(0.0f, 1.0f) < 0.68f ? 1 : 0;
            oscWarpAmount = randomFloat(0.18f, 0.68f);
            unisonVoiceCount = randomInt(1, 7);
            unisonDetune = randomFloat(0.05f, 0.4f);
            unisonBlend = randomFloat(0.35f, 1.0f);
            unisonSpread = randomFloat(0.35f, 1.0f);
            mono = false;
            fxToneEnabled = true;
            fxToneTilt = randomFloat(-0.35f, 0.45f);
            fxToneLowCut = randomFloat(50.0f, 160.0f);
            fxBitcrushEnabled = true;
            fxBitcrushBits = randomFloat(4.0f, 10.0f);
            fxBitcrushDownsample = randomFloat(2.0f, 18.0f);
            fxBitcrushMix = randomFloat(0.16f, 0.55f);
            fxPhaserEnabled = true;
            fxPhaserRate = randomFloat(0.05f, 2.4f);
            fxPhaserDepth = randomFloat(0.3f, 1.0f);
            fxPhaserMix = randomFloat(0.12f, 0.45f);
            fxPumpEnabled = randomFloat(0.0f, 1.0f) < 0.4f + (chaos * 0.25f);
            fxPumpRate = randomInt(0, 3);
            fxPumpDepth = randomFloat(0.12f, 0.55f);
            fxPumpShape = randomFloat(0.2f, 0.9f);
            fxPumpPhase = randomFloat(0.0f, 0.35f);
            fxWidthEnabled = randomFloat(0.0f, 1.0f) < 0.5f + (chaos * 0.2f);
            fxWidthAmount = randomFloat(0.85f, 1.55f);
            fxWidthMonoCutoff = randomFloat(120.0f, 230.0f);
            fxGuardEnabled = true;
            fxGuardPush = randomFloat(0.1f, 0.42f);
            fxGuardCeiling = randomFloat(0.82f, 0.91f);
            break;

        case Recipe::ukgTwoStepBass:
            wave = randomInt(1, 2);
            osc2Wave = randomInt(1, 3);
            octave = -1;
            osc2Octave = octave;
            tune = randomFloat(-0.02f, 0.02f);
            osc2Tune = randomFloat(-0.18f, 0.18f);
            osc1Level = randomFloat(0.68f, 0.96f);
            osc2Level = randomFloat(0.18f, 0.46f);
            subLevel = randomFloat(0.46f, 0.72f);
            noiseLevel = randomFloat(0.0f, 0.06f);
            cutoff = randomFloat(520.0f, 2600.0f);
            resonance = randomFloat(0.22f, 0.68f);
            envAmount = randomFloat(0.12f, 0.5f);
            attack = randomFloat(0.001f, 0.018f);
            decay = randomFloat(0.09f, 0.32f);
            sustain = randomFloat(0.2f, 0.55f);
            release = randomFloat(0.035f, 0.18f);
            drive = randomFloat(0.08f, 0.38f);
            filterCharacter = randomFloat(0.0f, 1.0f) < 0.74f ? 1 : 2;
            filterSlope = randomFloat(0.0f, 1.0f) < 0.74f ? 1 : 0;
            oscWarpAmount = randomFloat(0.08f, 0.28f);
            glide = randomFloat(0.025f, 0.11f);
            unisonVoiceCount = randomInt(1, 2);
            unisonDetune = randomFloat(0.0f, 0.055f);
            unisonBlend = randomFloat(0.25f, 0.5f);
            unisonSpread = randomFloat(0.0f, 0.08f);
            macroTone = randomFloat(0.05f, 0.35f);
            macroDirt = randomFloat(0.03f, 0.26f);
            macroMotion = randomFloat(0.08f, 0.32f);
            macroSpace = randomFloat(0.06f, 0.28f);
            mono = true;
            fxToneEnabled = true;
            fxToneTilt = randomFloat(-0.08f, 0.22f);
            fxToneLowCut = randomFloat(28.0f, 45.0f);
            fxBitcrushEnabled = randomFloat(0.0f, 1.0f) < 0.22f + (chaos * 0.16f);
            fxBitcrushBits = randomFloat(9.0f, 14.0f);
            fxBitcrushDownsample = randomFloat(1.0f, 6.0f);
            fxBitcrushMix = randomFloat(0.04f, 0.16f);
            fxPumpEnabled = randomFloat(0.0f, 1.0f) < 0.58f + (chaos * 0.22f);
            fxPumpRate = randomInt(0, 2);
            fxPumpDepth = randomFloat(0.16f, 0.46f);
            fxPumpShape = randomFloat(0.32f, 0.78f);
            fxPumpPhase = randomFloat(0.0f, 0.18f);
            fxWidthEnabled = true;
            fxWidthAmount = randomFloat(0.96f, 1.08f);
            fxWidthMonoCutoff = randomFloat(130.0f, 185.0f);
            fxPhaserEnabled = randomFloat(0.0f, 1.0f) < 0.28f + (chaos * 0.18f);
            fxPhaserRate = randomFloat(0.08f, 0.5f);
            fxPhaserDepth = randomFloat(0.18f, 0.55f);
            fxPhaserMix = randomFloat(0.05f, 0.18f);
            fxGuardEnabled = true;
            fxGuardPush = randomFloat(0.02f, 0.12f);
            fxGuardCeiling = randomFloat(0.88f, 0.93f);
            break;

        case Recipe::ukgDredBass:
            wave = randomInt(1, 2);
            osc2Wave = randomInt(1, 2);
            octave = -1;
            osc2Octave = -1;
            tune = randomFloat(-0.015f, 0.015f);
            osc2Tune = randomFloat(-0.16f, 0.16f);
            osc1Level = randomFloat(0.72f, 0.98f);
            osc2Level = randomFloat(0.28f, 0.58f);
            subLevel = randomFloat(0.48f, 0.76f);
            noiseLevel = randomFloat(0.0f, 0.04f);
            cutoff = randomFloat(260.0f, 980.0f);
            resonance = randomFloat(0.36f, 0.78f);
            envAmount = randomFloat(0.24f, 0.58f);
            attack = randomFloat(0.003f, 0.02f);
            decay = randomFloat(0.18f, 0.48f);
            sustain = randomFloat(0.18f, 0.52f);
            release = randomFloat(0.06f, 0.22f);
            drive = randomFloat(0.16f, 0.44f);
            filterCharacter = randomFloat(0.0f, 1.0f) < 0.55f ? 3 : 1;
            filterSlope = 1;
            oscWarpAmount = randomFloat(0.16f, 0.36f);
            glide = randomFloat(0.035f, 0.13f);
            unisonVoiceCount = randomInt(1, 2);
            unisonDetune = randomFloat(0.02f, 0.08f);
            unisonBlend = randomFloat(0.3f, 0.55f);
            unisonSpread = randomFloat(0.0f, 0.1f);
            macroTone = randomFloat(0.0f, 0.28f);
            macroDirt = randomFloat(0.08f, 0.3f);
            macroMotion = randomFloat(0.2f, 0.48f);
            macroSpace = randomFloat(0.03f, 0.18f);
            mono = true;
            fxToneEnabled = true;
            fxToneTilt = randomFloat(-0.22f, 0.08f);
            fxToneLowCut = randomFloat(28.0f, 48.0f);
            fxBitcrushEnabled = randomFloat(0.0f, 1.0f) < 0.28f + (chaos * 0.18f);
            fxBitcrushBits = randomFloat(10.0f, 14.0f);
            fxBitcrushDownsample = randomFloat(1.0f, 5.0f);
            fxBitcrushMix = randomFloat(0.04f, 0.14f);
            fxPumpEnabled = true;
            fxPumpRate = randomInt(0, 2);
            fxPumpDepth = randomFloat(0.18f, 0.42f);
            fxPumpShape = randomFloat(0.35f, 0.78f);
            fxPumpPhase = randomFloat(0.0f, 0.16f);
            fxWidthEnabled = true;
            fxWidthAmount = randomFloat(0.96f, 1.08f);
            fxWidthMonoCutoff = randomFloat(135.0f, 190.0f);
            fxPhaserEnabled = randomFloat(0.0f, 1.0f) < 0.34f + (chaos * 0.14f);
            fxPhaserRate = randomFloat(0.05f, 0.32f);
            fxPhaserDepth = randomFloat(0.22f, 0.58f);
            fxPhaserMix = randomFloat(0.05f, 0.16f);
            fxGuardEnabled = true;
            fxGuardPush = randomFloat(0.04f, 0.14f);
            fxGuardCeiling = randomFloat(0.88f, 0.93f);
            break;

        case Recipe::ukgOrganStab:
            wave = 5;
            osc2Wave = randomFloat(0.0f, 1.0f) < 0.42f + (chaos * 0.16f) ? 5 : randomInt(0, 3);
            octave = randomInt(-1, 0);
            osc2Octave = octave + randomInt(0, 1);
            tune = randomFloat(-0.03f, 0.03f);
            osc2Tune = randomFloat(-0.12f, 0.12f);
            osc1Level = randomFloat(0.62f, 0.92f);
            osc2Level = randomFloat(0.28f, 0.62f);
            subLevel = randomFloat(0.0f, 0.18f);
            noiseLevel = randomFloat(0.0f, 0.08f);
            noiseTypeIndex = randomInt(3, 4);
            noiseDecay = randomFloat(0.018f, 0.12f);
            cutoff = randomFloat(1100.0f, 5200.0f);
            resonance = randomFloat(0.18f, 0.48f);
            envAmount = randomFloat(0.08f, 0.34f);
            attack = randomFloat(0.001f, 0.012f);
            decay = randomFloat(0.1f, 0.34f);
            sustain = randomFloat(0.22f, 0.58f);
            release = randomFloat(0.045f, 0.2f);
            drive = randomFloat(0.04f, 0.22f);
            filterCharacter = 1;
            filterSlope = randomFloat(0.0f, 1.0f) < 0.36f ? 1 : 0;
            oscWarpAmount = randomFloat(0.06f, 0.22f);
            glide = 0.0f;
            unisonVoiceCount = randomInt(1, 3);
            unisonDetune = randomFloat(0.0f, 0.08f);
            unisonBlend = randomFloat(0.35f, 0.68f);
            unisonSpread = randomFloat(0.18f, 0.5f);
            macroTone = randomFloat(0.12f, 0.45f);
            macroDirt = randomFloat(0.02f, 0.16f);
            macroMotion = randomFloat(0.08f, 0.28f);
            macroSpace = randomFloat(0.06f, 0.24f);
            mono = false;
            fxToneEnabled = true;
            fxToneTilt = randomFloat(0.04f, 0.32f);
            fxToneLowCut = randomFloat(80.0f, 150.0f);
            fxPumpEnabled = randomFloat(0.0f, 1.0f) < 0.28f + (chaos * 0.16f);
            fxPumpRate = randomInt(0, 1);
            fxPumpDepth = randomFloat(0.1f, 0.3f);
            fxPumpShape = randomFloat(0.3f, 0.72f);
            fxPumpPhase = randomFloat(0.0f, 0.14f);
            fxWidthEnabled = true;
            fxWidthAmount = randomFloat(1.08f, 1.42f);
            fxWidthMonoCutoff = randomFloat(105.0f, 175.0f);
            fxPhaserEnabled = randomFloat(0.0f, 1.0f) < 0.2f + (chaos * 0.1f);
            fxPhaserRate = randomFloat(0.08f, 0.38f);
            fxPhaserDepth = randomFloat(0.16f, 0.48f);
            fxPhaserMix = randomFloat(0.04f, 0.16f);
            fxGuardEnabled = true;
            fxGuardPush = randomFloat(0.0f, 0.12f);
            fxGuardCeiling = randomFloat(0.89f, 0.96f);
            break;

        case Recipe::ukgChordStab:
            wave = randomFloat(0.0f, 1.0f) < 0.42f + (chaos * 0.18f) ? 6 : randomInt(1, 2);
            osc2Wave = randomFloat(0.0f, 1.0f) < 0.28f + (chaos * 0.16f) ? randomInt(5, 6) : randomInt(1, 3);
            octave = randomInt(-1, 0);
            osc2Octave = octave;
            tune = randomFloat(-0.08f, 0.08f);
            osc2Tune = randomFloat(3.0f, 7.0f);
            osc1Level = randomFloat(0.58f, 0.88f);
            osc2Level = randomFloat(0.35f, 0.78f);
            subLevel = randomFloat(0.0f, 0.16f);
            noiseLevel = randomFloat(0.0f, 0.1f);
            noiseTypeIndex = randomInt(3, 5);
            noiseDecay = randomFloat(0.018f, 0.16f);
            cutoff = randomFloat(1300.0f, 6500.0f);
            resonance = randomFloat(0.16f, 0.58f);
            envAmount = randomFloat(0.1f, 0.42f);
            attack = randomFloat(0.001f, 0.014f);
            decay = randomFloat(0.07f, 0.26f);
            sustain = randomFloat(0.0f, 0.28f);
            release = randomFloat(0.05f, 0.24f);
            drive = randomFloat(0.08f, 0.32f);
            filterCharacter = randomFloat(0.0f, 1.0f) < 0.78f ? 1 : 2;
            filterSlope = randomFloat(0.0f, 1.0f) < 0.46f ? 1 : 0;
            oscWarpAmount = randomFloat(0.06f, 0.26f);
            glide = 0.0f;
            unisonVoiceCount = randomInt(2, 5);
            unisonDetune = randomFloat(0.04f, 0.16f);
            unisonBlend = randomFloat(0.42f, 0.82f);
            unisonSpread = randomFloat(0.35f, 0.78f);
            macroTone = randomFloat(0.16f, 0.55f);
            macroDirt = randomFloat(0.04f, 0.24f);
            macroMotion = randomFloat(0.1f, 0.35f);
            macroSpace = randomFloat(0.1f, 0.34f);
            mono = false;
            fxToneEnabled = true;
            fxToneTilt = randomFloat(0.02f, 0.36f);
            fxToneLowCut = randomFloat(95.0f, 180.0f);
            fxBitcrushEnabled = randomFloat(0.0f, 1.0f) < 0.16f + (chaos * 0.1f);
            fxBitcrushBits = randomFloat(10.0f, 15.0f);
            fxBitcrushDownsample = randomFloat(1.0f, 4.0f);
            fxBitcrushMix = randomFloat(0.03f, 0.12f);
            fxPumpEnabled = randomFloat(0.0f, 1.0f) < 0.34f + (chaos * 0.16f);
            fxPumpRate = randomInt(0, 1);
            fxPumpDepth = randomFloat(0.12f, 0.34f);
            fxPumpShape = randomFloat(0.32f, 0.78f);
            fxPumpPhase = randomFloat(0.0f, 0.16f);
            fxWidthEnabled = true;
            fxWidthAmount = randomFloat(1.16f, 1.55f);
            fxWidthMonoCutoff = randomFloat(115.0f, 190.0f);
            fxPhaserEnabled = randomFloat(0.0f, 1.0f) < 0.24f + (chaos * 0.12f);
            fxPhaserRate = randomFloat(0.08f, 0.48f);
            fxPhaserDepth = randomFloat(0.18f, 0.55f);
            fxPhaserMix = randomFloat(0.05f, 0.18f);
            fxGuardEnabled = true;
            fxGuardPush = randomFloat(0.02f, 0.16f);
            fxGuardCeiling = randomFloat(0.88f, 0.95f);
            break;

        case Recipe::ukgBellPluck:
            wave = randomInt(0, 3);
            osc2Wave = randomInt(0, 3);
            octave = randomInt(0, 1);
            osc2Octave = octave + randomInt(0, 1);
            tune = randomFloat(-0.04f, 0.04f);
            osc2Tune = randomInt(0, 1) == 0 ? 7.0f : 12.0f;
            osc1Level = randomFloat(0.55f, 0.9f);
            osc2Level = randomFloat(0.22f, 0.58f);
            subLevel = 0.0f;
            noiseLevel = randomFloat(0.0f, 0.12f);
            noiseTypeIndex = randomInt(3, 4);
            noiseDecay = randomFloat(0.008f, 0.07f);
            cutoff = randomFloat(2200.0f, 11000.0f);
            resonance = randomFloat(0.32f, 0.85f);
            envAmount = randomFloat(0.22f, 0.68f);
            attack = randomFloat(0.001f, 0.01f);
            decay = randomFloat(0.045f, 0.18f);
            sustain = randomFloat(0.0f, 0.12f);
            release = randomFloat(0.1f, 0.42f);
            drive = randomFloat(0.02f, 0.18f);
            filterCharacter = randomFloat(0.0f, 1.0f) < 0.68f ? 0 : 1;
            filterSlope = randomFloat(0.0f, 1.0f) < 0.24f ? 1 : 0;
            oscWarpAmount = randomFloat(0.0f, 0.18f);
            glide = 0.0f;
            unisonVoiceCount = randomInt(1, 3);
            unisonDetune = randomFloat(0.0f, 0.08f);
            unisonBlend = randomFloat(0.32f, 0.62f);
            unisonSpread = randomFloat(0.25f, 0.62f);
            macroTone = randomFloat(0.28f, 0.68f);
            macroDirt = randomFloat(0.0f, 0.14f);
            macroMotion = randomFloat(0.12f, 0.38f);
            macroSpace = randomFloat(0.14f, 0.42f);
            mono = false;
            fxToneEnabled = true;
            fxToneTilt = randomFloat(0.1f, 0.42f);
            fxToneLowCut = randomFloat(110.0f, 210.0f);
            fxBitcrushEnabled = randomFloat(0.0f, 1.0f) < 0.18f + (chaos * 0.14f);
            fxBitcrushBits = randomFloat(11.0f, 15.0f);
            fxBitcrushDownsample = randomFloat(1.0f, 5.0f);
            fxBitcrushMix = randomFloat(0.03f, 0.13f);
            fxPumpEnabled = randomFloat(0.0f, 1.0f) < 0.22f + (chaos * 0.12f);
            fxPumpRate = randomInt(0, 2);
            fxPumpDepth = randomFloat(0.08f, 0.24f);
            fxPumpShape = randomFloat(0.28f, 0.72f);
            fxPumpPhase = randomFloat(0.0f, 0.14f);
            fxWidthEnabled = true;
            fxWidthAmount = randomFloat(1.18f, 1.6f);
            fxWidthMonoCutoff = randomFloat(130.0f, 220.0f);
            fxPhaserEnabled = randomFloat(0.0f, 1.0f) < 0.18f + (chaos * 0.1f);
            fxPhaserRate = randomFloat(0.12f, 0.75f);
            fxPhaserDepth = randomFloat(0.12f, 0.45f);
            fxPhaserMix = randomFloat(0.04f, 0.14f);
            fxGuardEnabled = true;
            fxGuardPush = randomFloat(0.0f, 0.1f);
            fxGuardCeiling = randomFloat(0.9f, 0.97f);
            break;
    }

    switch (recipe)
    {
        case Recipe::deepHouseBass:
        case Recipe::rollingTechBass:
        case Recipe::ukgTwoStepBass:
        case Recipe::ukgDredBass:
            fxEqEnabled = true;
            fxEqLowGain = randomFloat(0.4f, 1.8f);
            fxEqMidGain = randomFloat(-2.4f, -0.4f);
            fxEqHighGain = randomFloat(-1.4f, 0.8f);
            fxEqTrim = randomFloat(-1.2f, -0.2f);
            fxFlangerEnabled = randomFloat(0.0f, 1.0f) < 0.12f + (chaos * 0.08f);
            fxFlangerRate = randomFloat(0.05f, 0.32f);
            fxFlangerDepth = randomFloat(0.12f, 0.34f);
            fxFlangerFeedback = randomFloat(0.02f, 0.16f);
            fxFlangerMix = randomFloat(0.02f, 0.09f);
            fxTremoloEnabled = randomFloat(0.0f, 1.0f) < 0.18f + (chaos * 0.1f);
            fxTremoloRate = randomInt(0, 2);
            fxTremoloDepth = randomFloat(0.06f, 0.2f);
            fxTremoloPan = randomFloat(0.02f, 0.18f);
            fxTremoloShape = randomFloat(0.28f, 0.65f);
            fxTremoloPhase = randomFloat(0.0f, 0.16f);
            fxRingEnabled = randomFloat(0.0f, 1.0f) < 0.08f + (chaos * 0.06f);
            fxRingFrequency = randomFloat(18.0f, 110.0f);
            fxRingDepth = randomFloat(0.08f, 0.28f);
            fxRingMix = randomFloat(0.02f, 0.08f);
            fxRingBias = randomFloat(0.45f, 0.85f);
            fxCombEnabled = randomFloat(0.0f, 1.0f) < 0.1f + (chaos * 0.08f);
            fxCombFrequency = randomFloat(55.0f, 180.0f);
            fxCombFeedback = randomFloat(0.08f, 0.28f);
            fxCombDamping = randomFloat(0.35f, 0.75f);
            fxCombMix = randomFloat(0.02f, 0.08f);
            break;

        case Recipe::acidLine:
            fxEqEnabled = randomFloat(0.0f, 1.0f) < 0.65f + (chaos * 0.15f);
            fxEqLowGain = randomFloat(-1.2f, 0.8f);
            fxEqMidGain = randomFloat(-0.8f, 1.4f);
            fxEqHighGain = randomFloat(0.0f, 2.2f);
            fxEqTrim = randomFloat(-1.4f, -0.2f);
            fxFlangerEnabled = randomFloat(0.0f, 1.0f) < 0.2f + (chaos * 0.12f);
            fxFlangerRate = randomFloat(0.05f, 0.48f);
            fxFlangerDepth = randomFloat(0.16f, 0.52f);
            fxFlangerFeedback = randomFloat(0.02f, 0.26f);
            fxFlangerMix = randomFloat(0.03f, 0.16f);
            fxTremoloEnabled = randomFloat(0.0f, 1.0f) < 0.28f + (chaos * 0.14f);
            fxTremoloRate = randomInt(0, 3);
            fxTremoloDepth = randomFloat(0.1f, 0.34f);
            fxTremoloPan = randomFloat(0.04f, 0.28f);
            fxTremoloShape = randomFloat(0.32f, 0.78f);
            fxTremoloPhase = randomFloat(0.0f, 0.2f);
            fxRingEnabled = randomFloat(0.0f, 1.0f) < 0.38f + (chaos * 0.22f);
            fxRingFrequency = randomFloat(45.0f, 620.0f);
            fxRingDepth = randomFloat(0.18f, 0.58f);
            fxRingMix = randomFloat(0.04f, 0.18f);
            fxRingBias = randomFloat(0.0f, 0.55f);
            fxCombEnabled = randomFloat(0.0f, 1.0f) < 0.24f + (chaos * 0.16f);
            fxCombFrequency = randomFloat(90.0f, 760.0f);
            fxCombFeedback = randomFloat(-0.24f, 0.44f);
            fxCombDamping = randomFloat(0.18f, 0.62f);
            fxCombMix = randomFloat(0.03f, 0.14f);
            break;

        case Recipe::minimalBlip:
        case Recipe::noiseFx:
            fxEqEnabled = true;
            fxEqLowGain = randomFloat(-3.8f, 0.4f);
            fxEqMidGain = randomFloat(-1.2f, 1.8f);
            fxEqHighGain = randomFloat(-0.6f, 2.8f);
            fxEqTrim = randomFloat(-2.0f, -0.4f);
            fxFlangerEnabled = true;
            fxFlangerRate = randomFloat(0.08f, 1.2f);
            fxFlangerDepth = randomFloat(0.35f, 0.78f);
            fxFlangerFeedback = randomFloat(-0.32f, 0.5f);
            fxFlangerMix = randomFloat(0.08f, 0.32f);
            fxTremoloEnabled = true;
            fxTremoloRate = randomInt(1, 3);
            fxTremoloDepth = randomFloat(0.18f, 0.58f);
            fxTremoloPan = randomFloat(0.12f, 0.72f);
            fxTremoloShape = randomFloat(0.25f, 0.88f);
            fxTremoloPhase = randomFloat(0.0f, 0.5f);
            fxRingEnabled = true;
            fxRingFrequency = randomFloat(80.0f, 1600.0f);
            fxRingDepth = randomFloat(0.22f, 0.75f);
            fxRingMix = randomFloat(0.06f, 0.28f);
            fxRingBias = randomFloat(0.0f, 0.65f);
            fxCombEnabled = true;
            fxCombFrequency = randomFloat(120.0f, 1800.0f);
            fxCombFeedback = randomFloat(-0.42f, 0.62f);
            fxCombDamping = randomFloat(0.08f, 0.72f);
            fxCombMix = randomFloat(0.05f, 0.24f);
            break;

        case Recipe::darkStab:
        case Recipe::ukgOrganStab:
        case Recipe::ukgChordStab:
        case Recipe::ukgBellPluck:
            fxEqEnabled = true;
            fxEqLowGain = randomFloat(-4.5f, -1.0f);
            fxEqMidGain = randomFloat(-1.0f, 1.2f);
            fxEqHighGain = randomFloat(0.0f, 2.4f);
            fxEqTrim = randomFloat(-1.5f, -0.2f);
            fxFlangerEnabled = randomFloat(0.0f, 1.0f) < 0.2f + (chaos * 0.12f);
            fxFlangerRate = randomFloat(0.05f, 0.48f);
            fxFlangerDepth = randomFloat(0.16f, 0.52f);
            fxFlangerFeedback = randomFloat(0.02f, 0.26f);
            fxFlangerMix = randomFloat(0.03f, 0.16f);
            fxTremoloEnabled = randomFloat(0.0f, 1.0f) < 0.34f + (chaos * 0.18f);
            fxTremoloRate = randomInt(0, 3);
            fxTremoloDepth = randomFloat(0.08f, 0.32f);
            fxTremoloPan = randomFloat(0.12f, 0.55f);
            fxTremoloShape = randomFloat(0.3f, 0.82f);
            fxTremoloPhase = randomFloat(0.0f, 0.32f);
            fxRingEnabled = randomFloat(0.0f, 1.0f) < 0.24f + (chaos * 0.16f);
            fxRingFrequency = randomFloat(35.0f, 420.0f);
            fxRingDepth = randomFloat(0.12f, 0.46f);
            fxRingMix = randomFloat(0.03f, 0.14f);
            fxRingBias = randomFloat(0.12f, 0.72f);
            fxCombEnabled = randomFloat(0.0f, 1.0f) < 0.22f + (chaos * 0.14f);
            fxCombFrequency = randomFloat(110.0f, 920.0f);
            fxCombFeedback = randomFloat(0.08f, 0.46f);
            fxCombDamping = randomFloat(0.2f, 0.72f);
            fxCombMix = randomFloat(0.03f, 0.14f);
            break;
    }

    if (fxPumpEnabled)
    {
        auto makeCustomPumpCurve = [&]
        {
            const auto snap = randomFloat(0.78f, 1.0f);
            const auto hold = randomFloat(0.62f, 0.94f);
            const auto shoulder = randomFloat(0.34f, 0.66f);
            const auto tail = randomFloat(0.04f, 0.20f);

            fxPumpCustomCurve = {
                1.0f,
                snap,
                hold,
                shoulder,
                randomFloat(0.18f, 0.42f),
                randomFloat(tail, 0.28f),
                randomFloat(0.0f, tail),
                0.0f
            };
        };

        switch (recipe)
        {
            case Recipe::deepHouseBass:
                fxPumpCurve = randomInt(0, 1);
                break;

            case Recipe::rollingTechBass:
            case Recipe::acidLine:
                fxPumpCurve = 1;
                break;

            case Recipe::minimalBlip:
            case Recipe::noiseFx:
                fxPumpCurve = randomInt(3, 4);
                break;

            case Recipe::ukgTwoStepBass:
            case Recipe::ukgDredBass:
            case Recipe::ukgOrganStab:
            case Recipe::ukgChordStab:
                fxPumpCurve = 2;
                break;

            case Recipe::darkStab:
                fxPumpCurve = randomInt(1, 2);
                break;

            case Recipe::ukgBellPluck:
                fxPumpCurve = randomInt(2, 3);
                break;
        }

        const auto customChance = subtle ? 0.06f : 0.12f + (chaos * 0.22f);
        if (randomFloat(0.0f, 1.0f) < customChance)
        {
            fxPumpCurve = 5;
            makeCustomPumpCurve();
        }
    }

    const auto cutoffBias = std::pow(2.0f, brightnessBias);
    cutoff = juce::jlimit(35.0f, 18000.0f, cutoff * cutoffBias);
    drive = juce::jlimit(0.0f, 0.95f, drive + (driveBias * 0.24f));
    const auto positiveMotionBias = juce::jmax(0.0f, motionBias);
    unisonDetune = juce::jlimit(0.0f, 1.0f, unisonDetune + (positiveMotionBias * 0.08f));
    unisonSpread = juce::jlimit(0.0f, 1.0f, unisonSpread + (positiveMotionBias * 0.16f));
    macroWeight = juce::jlimit(0.0f, 0.8f, (subLevel * 0.45f) + (mono ? 0.12f : 0.0f));
    macroBounce = fxPumpEnabled ? juce::jlimit(0.0f, 0.7f, fxPumpDepth + 0.08f) : juce::jlimit(0.0f, 0.34f, chaos * 0.18f);
    macroWarp = juce::jlimit(0.0f, 0.7f, (std::abs(osc2Tune) / 12.0f) + (macroMotion * 0.4f) + (unisonDetune * 1.8f));
    macroThrow = juce::jlimit(0.0f, 0.75f, macroSpace * 0.82f);
    fxSendDelay = juce::jlimit(0.0f, 0.55f, macroThrow * 0.72f);
    fxSendReverb = juce::jlimit(0.0f, 0.55f, macroSpace * 0.42f);

    if (fxGuardEnabled)
    {
        const auto isBassRecipe = recipe == Recipe::deepHouseBass
            || recipe == Recipe::rollingTechBass
            || recipe == Recipe::ukgTwoStepBass
            || recipe == Recipe::ukgDredBass;
        const auto isTransientRecipe = recipe == Recipe::minimalBlip
            || recipe == Recipe::darkStab
            || recipe == Recipe::ukgOrganStab
            || recipe == Recipe::ukgChordStab
            || recipe == Recipe::ukgBellPluck;
        const auto driveLoad = juce::jlimit(0.0f, 1.0f, drive + (fxGuardPush * 0.75f) + (chaos * 0.12f));

        fxGuardGlue = juce::jlimit(0.0f,
                                   isBassRecipe ? 0.36f : 0.48f,
                                   randomFloat(isBassRecipe ? 0.10f : 0.04f,
                                               isBassRecipe ? 0.26f : 0.22f)
                                       + (driveLoad * (isBassRecipe ? 0.16f : 0.24f)));
        fxGuardPunch = juce::jlimit(0.0f,
                                    isBassRecipe ? 0.14f : 0.42f,
                                    randomFloat(0.02f, isTransientRecipe ? 0.30f : 0.16f)
                                        + ((isTransientRecipe ? 0.12f : 0.02f) * wildness));
        fxGuardClipMix = juce::jlimit(0.70f,
                                      1.0f,
                                      randomFloat(isBassRecipe ? 0.88f : 0.76f, 1.0f)
                                          + (driveLoad * 0.08f));
    }

    if (subtle)
    {
        auto blend = [this, amount] (const juce::String& id, float target)
        {
            const auto current = readFloat(id);
            return current + ((target - current) * amount);
        };

        attack = blend(Parameters::ID::ampAttack, attack);
        decay = blend(Parameters::ID::ampDecay, decay);
        sustain = blend(Parameters::ID::ampSustain, sustain);
        release = blend(Parameters::ID::ampRelease, release);
        cutoff = blend(Parameters::ID::filterCutoff, cutoff);
        resonance = blend(Parameters::ID::filterResonance, resonance);
        envAmount = blend(Parameters::ID::filterEnvAmount, envAmount);
        drive = blend(Parameters::ID::driveAmount, drive);
        tune = blend(Parameters::ID::oscTune, tune);
        osc2Tune = blend(Parameters::ID::osc2Tune, osc2Tune);
        osc1Level = blend(Parameters::ID::osc1Level, osc1Level);
        osc2Level = blend(Parameters::ID::osc2Level, osc2Level);
        subLevel = blend(Parameters::ID::subLevel, subLevel);
        noiseLevel = blend(Parameters::ID::noiseLevel, noiseLevel);
        noiseDecay = blend(Parameters::ID::noiseDecay, noiseDecay);
        oscWarpAmount = blend(Parameters::ID::oscWarp, oscWarpAmount);
        oscWavetablePosition = blend(Parameters::ID::oscWavetablePosition, oscWavetablePosition);
        osc2WavetablePosition = blend(Parameters::ID::osc2WavetablePosition, osc2WavetablePosition);
        unisonVoiceCount = static_cast<int>(std::round(blend(Parameters::ID::unisonVoices, static_cast<float>(unisonVoiceCount))));
        unisonDetune = blend(Parameters::ID::unisonDetune, unisonDetune);
        unisonBlend = blend(Parameters::ID::unisonBlend, unisonBlend);
        unisonSpread = blend(Parameters::ID::unisonSpread, unisonSpread);
        macroTone = blend(Parameters::ID::macroTone, macroTone);
        macroDirt = blend(Parameters::ID::macroDirt, macroDirt);
        macroMotion = blend(Parameters::ID::macroMotion, macroMotion);
        macroSpace = blend(Parameters::ID::macroSpace, macroSpace);
        macroWeight = blend(Parameters::ID::macroWeight, macroWeight);
        macroBounce = blend(Parameters::ID::macroBounce, macroBounce);
        macroWarp = blend(Parameters::ID::macroWarp, macroWarp);
        macroThrow = blend(Parameters::ID::macroThrow, macroThrow);
        fxSendDelay = blend(Parameters::ID::fxSendDelay, fxSendDelay);
        fxSendReverb = blend(Parameters::ID::fxSendReverb, fxSendReverb);
        fxToneTilt = blend(Parameters::ID::fxToneTilt, fxToneTilt);
        fxToneLowCut = blend(Parameters::ID::fxToneLowCut, fxToneLowCut);
        fxEqLowGain = blend(Parameters::ID::fxEqLowGain, fxEqLowGain);
        fxEqMidGain = blend(Parameters::ID::fxEqMidGain, fxEqMidGain);
        fxEqHighGain = blend(Parameters::ID::fxEqHighGain, fxEqHighGain);
        fxEqTrim = blend(Parameters::ID::fxEqTrim, fxEqTrim);
        fxBitcrushBits = blend(Parameters::ID::fxBitcrushBits, fxBitcrushBits);
        fxBitcrushDownsample = blend(Parameters::ID::fxBitcrushDownsample, fxBitcrushDownsample);
        fxBitcrushMix = blend(Parameters::ID::fxBitcrushMix, fxBitcrushMix);
        fxPumpDepth = blend(Parameters::ID::fxPumpDepth, fxPumpDepth);
        fxPumpShape = blend(Parameters::ID::fxPumpShape, fxPumpShape);
        fxPumpPhase = blend(Parameters::ID::fxPumpPhase, fxPumpPhase);
        for (size_t index = 0; index < fxPumpCustomCurve.size(); ++index)
            fxPumpCustomCurve[index] = blend(Parameters::ID::fxPumpCustomCurve[index], fxPumpCustomCurve[index]);
        fxTremoloDepth = blend(Parameters::ID::fxTremoloDepth, fxTremoloDepth);
        fxTremoloPan = blend(Parameters::ID::fxTremoloPan, fxTremoloPan);
        fxTremoloShape = blend(Parameters::ID::fxTremoloShape, fxTremoloShape);
        fxTremoloPhase = blend(Parameters::ID::fxTremoloPhase, fxTremoloPhase);
        fxRingFrequency = blend(Parameters::ID::fxRingFrequency, fxRingFrequency);
        fxRingDepth = blend(Parameters::ID::fxRingDepth, fxRingDepth);
        fxRingMix = blend(Parameters::ID::fxRingMix, fxRingMix);
        fxRingBias = blend(Parameters::ID::fxRingBias, fxRingBias);
        fxCombFrequency = blend(Parameters::ID::fxCombFrequency, fxCombFrequency);
        fxCombFeedback = blend(Parameters::ID::fxCombFeedback, fxCombFeedback);
        fxCombDamping = blend(Parameters::ID::fxCombDamping, fxCombDamping);
        fxCombMix = blend(Parameters::ID::fxCombMix, fxCombMix);
        fxWidthAmount = blend(Parameters::ID::fxWidthAmount, fxWidthAmount);
        fxWidthMonoCutoff = blend(Parameters::ID::fxWidthMonoCutoff, fxWidthMonoCutoff);
        fxPhaserRate = blend(Parameters::ID::fxPhaserRate, fxPhaserRate);
        fxPhaserDepth = blend(Parameters::ID::fxPhaserDepth, fxPhaserDepth);
        fxPhaserMix = blend(Parameters::ID::fxPhaserMix, fxPhaserMix);
        fxFlangerRate = blend(Parameters::ID::fxFlangerRate, fxFlangerRate);
        fxFlangerDepth = blend(Parameters::ID::fxFlangerDepth, fxFlangerDepth);
        fxFlangerFeedback = blend(Parameters::ID::fxFlangerFeedback, fxFlangerFeedback);
        fxFlangerMix = blend(Parameters::ID::fxFlangerMix, fxFlangerMix);
        fxGuardPush = blend(Parameters::ID::fxGuardPush, fxGuardPush);
        fxGuardGlue = blend(Parameters::ID::fxGuardGlue, fxGuardGlue);
        fxGuardPunch = blend(Parameters::ID::fxGuardPunch, fxGuardPunch);
        fxGuardClipMix = blend(Parameters::ID::fxGuardClipMix, fxGuardClipMix);
        fxGuardCeiling = blend(Parameters::ID::fxGuardCeiling, fxGuardCeiling);
        glide = blend(Parameters::ID::glideTime, glide);

        const auto characterChangeChance = juce::jlimit(0.05f, 0.42f, amount + (chaos * 0.12f));
        if (randomFloat(0.0f, 1.0f) > characterChangeChance)
            filterCharacter = readChoice(Parameters::ID::filterCharacter);

        const auto slopeChangeChance = juce::jlimit(0.04f, 0.32f, (amount * 0.75f) + (chaos * 0.08f));
        if (randomFloat(0.0f, 1.0f) > slopeChangeChance)
            filterSlope = readChoice(Parameters::ID::filterSlope);

        const auto noiseTypeChangeChance = juce::jlimit(0.04f, 0.38f, (amount * 0.75f) + (chaos * 0.1f));
        if (randomFloat(0.0f, 1.0f) > noiseTypeChangeChance)
            noiseTypeIndex = readChoice(Parameters::ID::noiseType);
    }

    if (recipe == Recipe::ukgTwoStepBass || recipe == Recipe::ukgDredBass)
    {
        octave = -1;
        osc2Octave = -1;
        tune = juce::jlimit(-0.03f, 0.03f, tune);
        osc2Tune = juce::jlimit(-0.22f, 0.22f, osc2Tune);
        osc1Level = juce::jlimit(0.62f, 1.0f, osc1Level);
        osc2Level = juce::jlimit(0.12f, 0.48f, osc2Level);
        subLevel = juce::jlimit(0.42f, 0.74f, subLevel);
        noiseLevel = juce::jlimit(0.0f, 0.06f, noiseLevel);
        noiseTypeIndex = 0;
        noiseDecay = juce::jlimit(0.08f, 0.24f, noiseDecay);
        oscWarpAmount = juce::jlimit(0.04f, 0.3f, oscWarpAmount);
        unisonVoiceCount = juce::jlimit(1, 2, unisonVoiceCount);
        unisonDetune = juce::jlimit(0.0f, 0.06f, unisonDetune);
        unisonSpread = juce::jlimit(0.0f, 0.08f, unisonSpread);
        macroMotion = juce::jlimit(0.0f, 0.34f, macroMotion);
        macroWeight = juce::jlimit(0.18f, 0.6f, macroWeight);
        macroBounce = juce::jlimit(0.08f, 0.48f, macroBounce);
        macroWarp = juce::jlimit(0.0f, 0.42f, macroWarp);
        macroThrow = juce::jlimit(0.0f, 0.25f, macroThrow);
        fxSendDelay = juce::jlimit(0.0f, 0.18f, fxSendDelay);
        fxSendReverb = juce::jlimit(0.0f, 0.12f, fxSendReverb);
        fxToneEnabled = true;
        fxToneLowCut = juce::jlimit(24.0f, 48.0f, fxToneLowCut);
        fxWidthEnabled = true;
        fxWidthAmount = juce::jlimit(0.94f, 1.1f, fxWidthAmount);
        fxWidthMonoCutoff = juce::jlimit(125.0f, 190.0f, fxWidthMonoCutoff);
        fxGuardEnabled = true;
        fxGuardPush = juce::jlimit(0.0f, 0.13f, fxGuardPush);
        fxGuardGlue = juce::jlimit(0.08f, 0.34f, fxGuardGlue);
        fxGuardPunch = juce::jlimit(0.0f, 0.16f, fxGuardPunch);
        fxGuardClipMix = juce::jlimit(0.86f, 1.0f, fxGuardClipMix);
        fxGuardCeiling = juce::jlimit(0.87f, 0.93f, fxGuardCeiling);
        mono = true;
        glide = juce::jlimit(0.02f, 0.12f, glide);
    }

    const auto isWavetableFriendlyStab = recipe == Recipe::darkStab
        || recipe == Recipe::ukgChordStab
        || recipe == Recipe::ukgBellPluck;

    if (isWavetableFriendlyStab)
    {
        if (! subtle && randomFloat(0.0f, 1.0f) < 0.18f + (chaos * 0.14f))
            wave = 4;
        if (! subtle && randomFloat(0.0f, 1.0f) < 0.14f + (chaos * 0.12f))
            osc2Wave = 4;
    }

    if (wave == 4)
        oscWavetablePosition = randomFloat(0.08f, recipe == Recipe::noiseFx ? 0.98f : 0.74f);
    if (osc2Wave == 4)
        osc2WavetablePosition = randomFloat(0.18f, 0.92f);

    setChoice(Parameters::ID::oscWave, wave);
    setChoice(Parameters::ID::osc2Wave, osc2Wave);
    setChoice(Parameters::ID::filterMode, filterMode);
    setChoice(Parameters::ID::filterCharacter, filterCharacter);
    setChoice(Parameters::ID::filterSlope, filterSlope);
    setParameter(Parameters::ID::oscOctave, static_cast<float>(octave));
    setParameter(Parameters::ID::oscTune, tune);
    setParameter(Parameters::ID::osc1Level, osc1Level);
    setParameter(Parameters::ID::osc2Octave, static_cast<float>(juce::jlimit(-2, 2, osc2Octave)));
    setParameter(Parameters::ID::osc2Tune, osc2Tune);
    setParameter(Parameters::ID::osc2Level, osc2Level);
    setParameter(Parameters::ID::subLevel, subLevel);
    setParameter(Parameters::ID::noiseLevel, noiseLevel);
    setChoice(Parameters::ID::noiseType, juce::jlimit(0, 6, noiseTypeIndex));
    setParameter(Parameters::ID::noiseDecay, noiseDecay);
    setParameter(Parameters::ID::oscWarp, oscWarpAmount);
    setParameter(Parameters::ID::oscWavetablePosition, oscWavetablePosition);
    setParameter(Parameters::ID::osc2WavetablePosition, osc2WavetablePosition);
    setParameter(Parameters::ID::unisonVoices, static_cast<float>(juce::jlimit(1, 7, unisonVoiceCount)));
    setParameter(Parameters::ID::unisonDetune, unisonDetune);
    setParameter(Parameters::ID::unisonBlend, unisonBlend);
    setParameter(Parameters::ID::unisonSpread, unisonSpread);
    setParameter(Parameters::ID::macroTone, macroTone);
    setParameter(Parameters::ID::macroDirt, macroDirt);
    setParameter(Parameters::ID::macroMotion, macroMotion);
    setParameter(Parameters::ID::macroSpace, macroSpace);
    setParameter(Parameters::ID::macroWeight, macroWeight);
    setParameter(Parameters::ID::macroBounce, macroBounce);
    setParameter(Parameters::ID::macroWarp, macroWarp);
    setParameter(Parameters::ID::macroThrow, macroThrow);
    setParameter(Parameters::ID::fxSendDelay, fxSendDelay);
    setParameter(Parameters::ID::fxSendReverb, fxSendReverb);
    setParameter(Parameters::ID::fxSendTailKill, 0.0f);
    setParameter(Parameters::ID::ampAttack, attack);
    setParameter(Parameters::ID::ampDecay, decay);
    setParameter(Parameters::ID::ampSustain, sustain);
    setParameter(Parameters::ID::ampRelease, release);
    setParameter(Parameters::ID::filterCutoff, cutoff);
    setParameter(Parameters::ID::filterResonance, resonance);
    setParameter(Parameters::ID::filterEnvAmount, envAmount);
    setParameter(Parameters::ID::driveAmount, drive);
    setParameter(Parameters::ID::fxToneEnabled, fxToneEnabled ? 1.0f : 0.0f);
    setParameter(Parameters::ID::fxToneTilt, fxToneTilt);
    setParameter(Parameters::ID::fxToneLowCut, fxToneLowCut);
    setParameter(Parameters::ID::fxEqEnabled, fxEqEnabled ? 1.0f : 0.0f);
    setParameter(Parameters::ID::fxEqLowGain, fxEqLowGain);
    setParameter(Parameters::ID::fxEqMidGain, fxEqMidGain);
    setParameter(Parameters::ID::fxEqHighGain, fxEqHighGain);
    setParameter(Parameters::ID::fxEqTrim, fxEqTrim);
    setParameter(Parameters::ID::fxBitcrushEnabled, fxBitcrushEnabled ? 1.0f : 0.0f);
    setParameter(Parameters::ID::fxBitcrushBits, fxBitcrushBits);
    setParameter(Parameters::ID::fxBitcrushDownsample, fxBitcrushDownsample);
    setParameter(Parameters::ID::fxBitcrushMix, fxBitcrushMix);
    setParameter(Parameters::ID::fxPumpEnabled, fxPumpEnabled ? 1.0f : 0.0f);
    setParameter(Parameters::ID::fxPumpRate, static_cast<float>(juce::jlimit(0, 3, fxPumpRate)));
    setParameter(Parameters::ID::fxPumpCurve, static_cast<float>(juce::jlimit(0, 5, fxPumpCurve)));
    for (size_t index = 0; index < fxPumpCustomCurve.size(); ++index)
        setParameter(Parameters::ID::fxPumpCustomCurve[index], fxPumpCustomCurve[index]);
    setParameter(Parameters::ID::fxPumpDepth, fxPumpDepth);
    setParameter(Parameters::ID::fxPumpShape, fxPumpShape);
    setParameter(Parameters::ID::fxPumpPhase, fxPumpPhase);
    setParameter(Parameters::ID::fxTremoloEnabled, fxTremoloEnabled ? 1.0f : 0.0f);
    setParameter(Parameters::ID::fxTremoloRate, static_cast<float>(juce::jlimit(0, 3, fxTremoloRate)));
    setParameter(Parameters::ID::fxTremoloDepth, fxTremoloDepth);
    setParameter(Parameters::ID::fxTremoloPan, fxTremoloPan);
    setParameter(Parameters::ID::fxTremoloShape, fxTremoloShape);
    setParameter(Parameters::ID::fxTremoloPhase, fxTremoloPhase);
    setParameter(Parameters::ID::fxRingEnabled, fxRingEnabled ? 1.0f : 0.0f);
    setParameter(Parameters::ID::fxRingFrequency, fxRingFrequency);
    setParameter(Parameters::ID::fxRingDepth, fxRingDepth);
    setParameter(Parameters::ID::fxRingMix, fxRingMix);
    setParameter(Parameters::ID::fxRingBias, fxRingBias);
    setParameter(Parameters::ID::fxCombEnabled, fxCombEnabled ? 1.0f : 0.0f);
    setParameter(Parameters::ID::fxCombFrequency, fxCombFrequency);
    setParameter(Parameters::ID::fxCombFeedback, fxCombFeedback);
    setParameter(Parameters::ID::fxCombDamping, fxCombDamping);
    setParameter(Parameters::ID::fxCombMix, fxCombMix);
    setParameter(Parameters::ID::fxWidthEnabled, fxWidthEnabled ? 1.0f : 0.0f);
    setParameter(Parameters::ID::fxWidthAmount, fxWidthAmount);
    setParameter(Parameters::ID::fxWidthMonoCutoff, fxWidthMonoCutoff);
    setParameter(Parameters::ID::fxPhaserEnabled, fxPhaserEnabled ? 1.0f : 0.0f);
    setParameter(Parameters::ID::fxPhaserRate, fxPhaserRate);
    setParameter(Parameters::ID::fxPhaserDepth, fxPhaserDepth);
    setParameter(Parameters::ID::fxPhaserMix, fxPhaserMix);
    setParameter(Parameters::ID::fxFlangerEnabled, fxFlangerEnabled ? 1.0f : 0.0f);
    setParameter(Parameters::ID::fxFlangerRate, fxFlangerRate);
    setParameter(Parameters::ID::fxFlangerDepth, fxFlangerDepth);
    setParameter(Parameters::ID::fxFlangerFeedback, fxFlangerFeedback);
    setParameter(Parameters::ID::fxFlangerMix, fxFlangerMix);
    setParameter(Parameters::ID::fxGuardEnabled, fxGuardEnabled ? 1.0f : 0.0f);
    setParameter(Parameters::ID::fxGuardPush, fxGuardPush);
    setParameter(Parameters::ID::fxGuardGlue, fxGuardGlue);
    setParameter(Parameters::ID::fxGuardPunch, fxGuardPunch);
    setParameter(Parameters::ID::fxGuardClipMix, fxGuardClipMix);
    setParameter(Parameters::ID::fxGuardCeiling, fxGuardCeiling);
    setParameter(Parameters::ID::monoMode, mono ? 1.0f : 0.0f);
    setParameter(Parameters::ID::glideTime, glide);

    for (size_t slotIndex = 0; slotIndex < Parameters::ID::modMatrixSource.size(); ++slotIndex)
    {
        setChoice(Parameters::ID::modMatrixSource[slotIndex], 0);
        setChoice(Parameters::ID::modMatrixDestination[slotIndex], 0);
        setParameter(Parameters::ID::modMatrixAmount[slotIndex], 0.0f);
        setParameter(Parameters::ID::modMatrixEnabled[slotIndex], 1.0f);
    }

    auto setModSlot = [this] (size_t slotIndex, int source, int destination, float slotAmount)
    {
        if (slotIndex >= Parameters::ID::modMatrixSource.size())
            return;

        setChoice(Parameters::ID::modMatrixSource[slotIndex], source);
        setChoice(Parameters::ID::modMatrixDestination[slotIndex], destination);
        setParameter(Parameters::ID::modMatrixAmount[slotIndex], juce::jlimit(-1.0f, 1.0f, slotAmount));
        setParameter(Parameters::ID::modMatrixEnabled[slotIndex], 1.0f);
    };

    auto setLfoCurve = [this] (const std::array<float, 8>& curve)
    {
        for (size_t index = 0; index < Parameters::ID::lfo1Curve.size(); ++index)
            setParameter(Parameters::ID::lfo1Curve[index], juce::jlimit(-1.0f, 1.0f, curve[index]));
    };

    const auto motionScale = juce::jlimit(0.55f, 1.35f, 0.82f + (positiveMotionBias * 0.28f) + (chaos * 0.16f));
    const auto usesCurveLfo = recipe == Recipe::deepHouseBass
        || recipe == Recipe::rollingTechBass
        || recipe == Recipe::ukgTwoStepBass
        || recipe == Recipe::ukgDredBass;
    setParameter(Parameters::ID::lfo1Sync, 1.0f);
    setChoice(Parameters::ID::lfo1SyncRate, recipe == Recipe::minimalBlip || recipe == Recipe::noiseFx ? 3 : 1);
    setChoice(Parameters::ID::lfo1Shape, usesCurveLfo ? 5 : recipe == Recipe::minimalBlip || recipe == Recipe::noiseFx ? 4 : 0);
    setParameter(Parameters::ID::lfo1Rate, randomFloat(0.6f, 2.4f));
    setParameter(Parameters::ID::lfo1Depth, juce::jlimit(0.08f, 0.72f, randomFloat(0.16f, 0.4f) * motionScale));
    setParameter(Parameters::ID::lfo1Phase, randomFloat(0.0f, 0.25f));
    setParameter(Parameters::ID::lfo1Retrigger, recipe == Recipe::minimalBlip || recipe == Recipe::noiseFx ? 0.0f : 1.0f);

    if (recipe == Recipe::ukgDredBass)
    {
        setLfoCurve({
            randomFloat(-0.24f, 0.04f),
            randomFloat(0.08f, 0.34f),
            randomFloat(0.64f, 0.92f),
            randomFloat(0.82f, 1.0f),
            randomFloat(0.32f, 0.62f),
            randomFloat(-0.18f, 0.12f),
            randomFloat(-0.66f, -0.32f),
            randomFloat(-0.42f, -0.12f)
        });
    }
    else if (usesCurveLfo)
    {
        setLfoCurve({
            randomFloat(-0.08f, 0.12f),
            randomFloat(0.48f, 0.82f),
            randomFloat(0.82f, 1.0f),
            randomFloat(0.22f, 0.52f),
            randomFloat(-0.18f, 0.08f),
            randomFloat(-0.56f, -0.24f),
            randomFloat(-0.92f, -0.58f),
            randomFloat(-0.38f, -0.08f)
        });
    }
    else
    {
        setLfoCurve({ 0.0f, 0.58f, 1.0f, 0.42f, -0.18f, -0.72f, -1.0f, -0.36f });
    }

    const auto dredModEnv = recipe == Recipe::ukgDredBass;
    setParameter(Parameters::ID::modEnv1Attack, dredModEnv ? randomFloat(0.045f, 0.14f) : randomFloat(0.002f, 0.03f));
    setParameter(Parameters::ID::modEnv1Decay, dredModEnv ? randomFloat(0.18f, 0.46f) : randomFloat(0.08f, recipe == Recipe::ukgOrganStab || recipe == Recipe::ukgChordStab ? 0.34f : 0.22f));
    setParameter(Parameters::ID::modEnv1Sustain, randomFloat(0.0f, 0.24f));
    setParameter(Parameters::ID::modEnv1Release, randomFloat(0.04f, 0.2f));
    setParameter(Parameters::ID::modEnv1Depth, juce::jlimit(0.08f, 0.68f, randomFloat(0.18f, 0.46f) * motionScale));

    switch (recipe)
    {
        case Recipe::deepHouseBass:
        case Recipe::rollingTechBass:
        case Recipe::ukgTwoStepBass:
            setModSlot(0, 1, 1, randomFloat(0.05f, 0.16f) * motionScale);
            setModSlot(1, 2, 3, randomFloat(0.04f, 0.12f) * motionScale);
            setModSlot(2, 9, 7, randomFloat(0.10f, 0.22f));
            break;

        case Recipe::ukgDredBass:
            setModSlot(0, 2, 1, randomFloat(0.16f, 0.32f) * motionScale);
            setModSlot(1, 1, 1, randomFloat(0.05f, 0.14f) * motionScale);
            setModSlot(2, 2, 3, randomFloat(0.08f, 0.18f) * motionScale);
            setModSlot(3, 9, 7, randomFloat(0.12f, 0.24f));
            break;

        case Recipe::acidLine:
            setModSlot(0, 1, 1, randomFloat(0.08f, 0.22f) * motionScale);
            setModSlot(1, 2, 2, randomFloat(0.04f, 0.14f) * motionScale);
            break;

        case Recipe::minimalBlip:
        case Recipe::noiseFx:
            setModSlot(0, 1, 1, randomFloat(0.14f, 0.3f) * motionScale);
            setModSlot(1, 1, 4, randomFloat(0.03f, 0.1f) * motionScale);
            setModSlot(2, 1, 11, randomFloat(0.05f, 0.13f) * motionScale);
            break;

        case Recipe::darkStab:
        case Recipe::ukgOrganStab:
        case Recipe::ukgChordStab:
        case Recipe::ukgBellPluck:
            setModSlot(0, 2, 1, randomFloat(0.08f, 0.22f) * motionScale);
            setModSlot(1, 3, 1, randomFloat(0.03f, 0.11f) * motionScale);
            setModSlot(2, 1, 5, randomFloat(-0.06f, 0.06f) * motionScale);
            setModSlot(3, 11, 8, randomFloat(0.08f, 0.20f));
            setModSlot(4, 11, 9, randomFloat(0.06f, 0.16f));
            break;
    }

    applyOutputSafety(juce::jlimit(0.0f, 1.0f, drive + (fxGuardPush * 0.35f)));
}

void Randomizer::applyOutputSafety(float drive)
{
    const auto safeGain = juce::jmap(drive, 0.0f, 1.0f, -7.0f, -14.0f);
    setParameter(Parameters::ID::outputGain, safeGain);
}
}
