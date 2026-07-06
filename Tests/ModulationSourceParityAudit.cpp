#include "../Source/PluginProcessor.h"

#include <cmath>
#include <iostream>
#include <memory>

namespace
{
bool setPlainParameter(NateVSTAudioProcessor& processor, const juce::String& parameterID, float value)
{
    if (auto* parameter = processor.getValueTreeState().getParameter(parameterID))
    {
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        return true;
    }

    return false;
}

bool writeTestSample(const juce::File& file)
{
    juce::AudioBuffer<float> buffer(2, 44100);
    buffer.clear();

    for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto sliceIndex = juce::jlimit(0, 7, (sample * 8) / buffer.getNumSamples());
        const auto frequency = 140.0 + (static_cast<double>(sliceIndex) * 55.0);
        const auto phase = (static_cast<double>(sample) / 44100.0) * frequency * juce::MathConstants<double>::twoPi;
        const auto value = static_cast<float>(std::sin(phase) * 0.62);
        buffer.setSample(0, sample, value);
        buffer.setSample(1, sample, value * 0.92f);
    }

    file.deleteFile();
    if (! file.getParentDirectory().createDirectory())
        return false;

    juce::WavAudioFormat format;
    std::unique_ptr<juce::FileOutputStream> fileStream(file.createOutputStream());
    if (fileStream == nullptr || ! fileStream->openedOk())
        return false;

    std::unique_ptr<juce::OutputStream> stream(std::move(fileStream));
    const auto options = juce::AudioFormatWriterOptions {}
        .withSampleRate(44100.0)
        .withNumChannels(2)
        .withBitsPerSample(16);
    auto writer = format.createWriterFor(stream, options);
    return writer != nullptr && writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}

void clearModMatrix(NateVSTAudioProcessor& processor)
{
    for (size_t index = 0; index < Parameters::ID::modMatrixSource.size(); ++index)
    {
        setPlainParameter(processor, Parameters::ID::modMatrixSource[index], 0.0f);
        setPlainParameter(processor, Parameters::ID::modMatrixDestination[index], 0.0f);
        setPlainParameter(processor, Parameters::ID::modMatrixAmount[index], 0.0f);
        setPlainParameter(processor, Parameters::ID::modMatrixEnabled[index], 1.0f);
        setPlainParameter(processor, Parameters::ID::modMatrixPolarity[index], 0.0f);
        setPlainParameter(processor, Parameters::ID::modMatrixCurve[index], 0.0f);
        setPlainParameter(processor, Parameters::ID::modMatrixRangeMin[index], -1.0f);
        setPlainParameter(processor, Parameters::ID::modMatrixRangeMax[index], 1.0f);
        setPlainParameter(processor, Parameters::ID::modMatrixSlew[index], 0.0f);
    }
}

void setFirstRoute(NateVSTAudioProcessor& processor, int sourceIndex, int destinationIndex, float amount, int polarityIndex = 0)
{
    clearModMatrix(processor);
    setPlainParameter(processor, Parameters::ID::modMatrixSource[0], static_cast<float>(sourceIndex));
    setPlainParameter(processor, Parameters::ID::modMatrixDestination[0], static_cast<float>(destinationIndex));
    setPlainParameter(processor, Parameters::ID::modMatrixAmount[0], amount);
    setPlainParameter(processor, Parameters::ID::modMatrixEnabled[0], 1.0f);
    setPlainParameter(processor, Parameters::ID::modMatrixPolarity[0], static_cast<float>(polarityIndex));
}

struct RenderStats
{
    float peak = 0.0f;
    float rms = 0.0f;
    bool finite = true;
};

struct RenderComparison
{
    float baselineRms = 0.0f;
    float routedRms = 0.0f;
    float diffRms = 0.0f;
    bool finite = true;
};

const char* modulationSourceName(int sourceIndex)
{
    switch (sourceIndex)
    {
        case 1: return "LFO 1";
        case 2: return "Mod Env 1";
        case 3: return "Velocity";
        case 4: return "Tone";
        case 5: return "Dirt";
        case 6: return "Motion";
        case 7: return "Space";
        case 8: return "Weight";
        case 9: return "Bounce";
        case 10: return "Warp";
        case 11: return "Throw";
        case 12: return "S&H";
        case 13: return "Smooth";
        case 14: return "Chaos";
        case 15: return "LFO 2";
        case 16: return "Mod Wheel";
        case 17: return "Aftertouch";
        case 18: return "Pitch Bend";
        case 19: return "Note";
        case 20: return "Step LFO";
        default: return "Source";
    }
}

int noteForSource(int sourceIndex)
{
    return sourceIndex == 19 ? 84 : 60;
}

int routePolarityForSource(int sourceIndex)
{
    switch (sourceIndex)
    {
        case 1:
        case 12:
        case 13:
        case 14:
        case 15:
            return 1;

        default:
            return 0;
    }
}

bool sourceCanStartNearZero(int sourceIndex)
{
    return sourceIndex == 12 || sourceIndex == 13 || sourceIndex == 14;
}

bool configureSourceForAudit(NateVSTAudioProcessor& processor, int sourceIndex)
{
    if (! setPlainParameter(processor, Parameters::ID::lfo1Depth, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::lfo1Sync, sourceCanStartNearZero(sourceIndex) ? 0.0f : 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::lfo1Rate, sourceCanStartNearZero(sourceIndex) ? 20.0f : 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::lfo1Shape, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::lfo2Depth, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::lfo2Shape, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::stepLfoDepth, 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::stepLfoValue[0], 1.0f))
    {
        return false;
    }

    switch (sourceIndex)
    {
        case 4: return setPlainParameter(processor, Parameters::ID::macroTone, 1.0f);
        case 5: return setPlainParameter(processor, Parameters::ID::macroDirt, 1.0f);
        case 6: return setPlainParameter(processor, Parameters::ID::macroMotion, 1.0f);
        case 7: return setPlainParameter(processor, Parameters::ID::macroSpace, 1.0f);
        case 8: return setPlainParameter(processor, Parameters::ID::macroWeight, 1.0f);
        case 9: return setPlainParameter(processor, Parameters::ID::macroBounce, 1.0f);
        case 10: return setPlainParameter(processor, Parameters::ID::macroWarp, 1.0f);
        case 11: return setPlainParameter(processor, Parameters::ID::macroThrow, 1.0f);
        default: return true;
    }
}

bool auditPerformanceModulationStatus()
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);
    setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::controllerEvent(1, 1, 127), 0);
    midi.addEvent(juce::MidiMessage::channelPressureChange(1, 96), 1);
    midi.addEvent(juce::MidiMessage::pitchWheel(1, 16383), 2);
    midi.addEvent(juce::MidiMessage::noteOn(1, 84, 0.64f), 3);
    processor.processBlock(buffer, midi);

    auto status = processor.getPerformanceModulationStatus();
    if (std::abs(status.velocity - 0.64f) > 0.01f)
    {
        std::cerr << "Performance status did not report velocity: " << status.velocity << '\n';
        return false;
    }

    if (status.modWheel < 0.99f)
    {
        std::cerr << "Performance status did not report mod wheel: " << status.modWheel << '\n';
        return false;
    }

    if (std::abs(status.aftertouch - (96.0f / 127.0f)) > 0.01f)
    {
        std::cerr << "Performance status did not report aftertouch: " << status.aftertouch << '\n';
        return false;
    }

    if (status.pitchBend < 0.99f)
    {
        std::cerr << "Performance status did not report pitch bend: " << status.pitchBend << '\n';
        return false;
    }

    if (std::abs(status.note - ((84.0f - 60.0f) / 36.0f)) > 0.01f)
    {
        std::cerr << "Performance status did not report note position: " << status.note << '\n';
        return false;
    }

    midi.clear();
    midi.addEvent(juce::MidiMessage::noteOff(1, 84), 0);
    processor.processBlock(buffer, midi);
    status = processor.getPerformanceModulationStatus();
    if (std::abs(status.velocity) > 0.001f || std::abs(status.note) > 0.001f)
    {
        std::cerr << "Performance status stayed active after note off: velocity "
                  << status.velocity << " note " << status.note << '\n';
        return false;
    }

    midi.clear();
    midi.addEvent(juce::MidiMessage::noteOn(1, 72, 0.5f), 0);
    processor.processBlock(buffer, midi);

    midi.clear();
    midi.addEvent(juce::MidiMessage::allNotesOff(1), 0);
    processor.processBlock(buffer, midi);
    status = processor.getPerformanceModulationStatus();
    if (std::abs(status.velocity) > 0.001f || std::abs(status.note) > 0.001f)
    {
        std::cerr << "Performance status stayed active after all notes off: velocity "
                  << status.velocity << " note " << status.note << '\n';
        return false;
    }

    midi.clear();
    midi.addEvent(juce::MidiMessage::controllerEvent(1, 1, 100), 0);
    midi.addEvent(juce::MidiMessage::channelPressureChange(1, 100), 1);
    midi.addEvent(juce::MidiMessage::pitchWheel(1, 12000), 2);
    processor.processBlock(buffer, midi);

    midi.clear();
    midi.addEvent(juce::MidiMessage::controllerEvent(1, 121, 0), 0);
    processor.processBlock(buffer, midi);
    status = processor.getPerformanceModulationStatus();
    if (std::abs(status.modWheel) > 0.001f
        || std::abs(status.aftertouch) > 0.001f
        || std::abs(status.pitchBend) > 0.001f)
    {
        std::cerr << "Performance status did not reset controllers: mod wheel "
                  << status.modWheel << " aftertouch " << status.aftertouch
                  << " pitch bend " << status.pitchBend << '\n';
        return false;
    }

    midi.clear();
    midi.addEvent(juce::MidiMessage::noteOn(1, 48, 1.0f), 0);
    processor.processBlock(buffer, midi);
    processor.panicAllNotesOff();
    status = processor.getPerformanceModulationStatus();
    if (std::abs(status.velocity) > 0.001f || std::abs(status.note) > 0.001f)
    {
        std::cerr << "Performance status stayed active after panic: velocity "
                  << status.velocity << " note " << status.note << '\n';
        return false;
    }

    return true;
}

void addMidiSourceStimulus(juce::MidiBuffer& midi, int sourceIndex, int note, int samplePosition)
{
    switch (sourceIndex)
    {
        case 16:
            midi.addEvent(juce::MidiMessage::controllerEvent(1, 1, 127), samplePosition);
            break;

        case 17:
            midi.addEvent(juce::MidiMessage::channelPressureChange(1, 127), samplePosition);
            break;

        case 18:
            midi.addEvent(juce::MidiMessage::pitchWheel(1, 16383), samplePosition);
            break;

        case 19:
            juce::ignoreUnused(note);
            break;

        default:
            juce::ignoreUnused(note);
            break;
    }
}

RenderStats renderNote(NateVSTAudioProcessor& processor, int note, float velocity, int blocks = 8, int stimulusSourceIndex = 0)
{
    juce::AudioBuffer<float> buffer(2, 512);
    RenderStats stats;
    double sumSquares = 0.0;
    auto sampleCount = 0;

    for (auto block = 0; block < blocks; ++block)
    {
        juce::MidiBuffer midi;
        if (block == 0)
        {
            midi.addEvent(juce::MidiMessage::noteOn(1, note, velocity), 0);
            addMidiSourceStimulus(midi, stimulusSourceIndex, note, 1);
        }
        if (block == 3)
            midi.addEvent(juce::MidiMessage::noteOff(1, note), 0);

        processor.processBlock(buffer, midi);
        for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const auto* samples = buffer.getReadPointer(channel);
            for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                const auto value = samples[sample];
                if (! std::isfinite(value))
                    stats.finite = false;

                stats.peak = juce::jmax(stats.peak, std::abs(value));
                sumSquares += static_cast<double>(value) * static_cast<double>(value);
                ++sampleCount;
            }
        }
    }

    stats.rms = sampleCount > 0 ? static_cast<float>(std::sqrt(sumSquares / static_cast<double>(sampleCount))) : 0.0f;
    return stats;
}

RenderComparison compareRenderedNotes(NateVSTAudioProcessor& baseline,
                                      NateVSTAudioProcessor& routed,
                                      int note,
                                      float velocity,
                                      int blocks,
                                      int stimulusSourceIndex = 3,
                                      int noteOffBlock = 3)
{
    juce::AudioBuffer<float> baselineBuffer(2, 512);
    juce::AudioBuffer<float> routedBuffer(2, 512);
    RenderComparison comparison;
    double baselineSquares = 0.0;
    double routedSquares = 0.0;
    double diffSquares = 0.0;
    auto sampleCount = 0;

    for (auto block = 0; block < blocks; ++block)
    {
        juce::MidiBuffer midi;
        if (block == 0)
        {
            midi.addEvent(juce::MidiMessage::noteOn(1, note, velocity), 0);
            addMidiSourceStimulus(midi, stimulusSourceIndex, note, 1);
        }
        if (block == noteOffBlock)
            midi.addEvent(juce::MidiMessage::noteOff(1, note), 0);

        baseline.processBlock(baselineBuffer, midi);
        routed.processBlock(routedBuffer, midi);

        for (auto channel = 0; channel < baselineBuffer.getNumChannels(); ++channel)
        {
            const auto* baselineSamples = baselineBuffer.getReadPointer(channel);
            const auto* routedSamples = routedBuffer.getReadPointer(channel);
            for (auto sample = 0; sample < baselineBuffer.getNumSamples(); ++sample)
            {
                const auto baselineValue = baselineSamples[sample];
                const auto routedValue = routedSamples[sample];
                if (! std::isfinite(baselineValue) || ! std::isfinite(routedValue))
                    comparison.finite = false;

                baselineSquares += static_cast<double>(baselineValue) * static_cast<double>(baselineValue);
                routedSquares += static_cast<double>(routedValue) * static_cast<double>(routedValue);
                const auto diff = routedValue - baselineValue;
                diffSquares += static_cast<double>(diff) * static_cast<double>(diff);
                ++sampleCount;
            }
        }
    }

    if (sampleCount > 0)
    {
        comparison.baselineRms = static_cast<float>(std::sqrt(baselineSquares / static_cast<double>(sampleCount)));
        comparison.routedRms = static_cast<float>(std::sqrt(routedSquares / static_cast<double>(sampleCount)));
        comparison.diffRms = static_cast<float>(std::sqrt(diffSquares / static_cast<double>(sampleCount)));
    }

    return comparison;
}

bool configureSampleOnly(NateVSTAudioProcessor& processor, const juce::File& file)
{
    processor.prepareToPlay(44100.0, 512);
    if (! processor.loadSampleFile(file))
        return false;

    return setPlainParameter(processor, Parameters::ID::osc1Level, 0.0f)
        && setPlainParameter(processor, Parameters::ID::osc2Level, 0.0f)
        && setPlainParameter(processor, Parameters::ID::subLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sampleEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::samplePlaybackMode, 1.0f)
        && setPlainParameter(processor, Parameters::ID::sampleGain, -1.0f)
        && setPlainParameter(processor, Parameters::ID::sampleMix, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxReverbEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxPumpEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::outputGain, 0.0f);
}

float renderSampleMixRoute(const juce::File& sampleFile, int sourceIndex)
{
    NateVSTAudioProcessor processor;
    if (! configureSampleOnly(processor, sampleFile))
        return -1.0f;

    setPlainParameter(processor, Parameters::ID::modEnv1Attack, 0.001f);
    setPlainParameter(processor, Parameters::ID::modEnv1Decay, 0.03f);
    setPlainParameter(processor, Parameters::ID::modEnv1Sustain, 1.0f);
    setPlainParameter(processor, Parameters::ID::modEnv1Release, 0.02f);
    setPlainParameter(processor, Parameters::ID::modEnv1Depth, 1.0f);
    if (! configureSourceForAudit(processor, sourceIndex))
        return -1.0f;

    setFirstRoute(processor, sourceIndex, 13, 1.0f, routePolarityForSource(sourceIndex));

    const auto note = noteForSource(sourceIndex);
    const auto stats = renderNote(processor, note, 1.0f, sourceCanStartNearZero(sourceIndex) ? 64 : 8, sourceIndex);
    return stats.finite ? stats.rms : -1.0f;
}

float renderFxPumpReduction(int sourceIndex)
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);
    clearModMatrix(processor);

    setPlainParameter(processor, Parameters::ID::osc1Level, 0.0f);
    setPlainParameter(processor, Parameters::ID::osc2Level, 0.0f);
    setPlainParameter(processor, Parameters::ID::subLevel, 0.0f);
    setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f);
    setPlainParameter(processor, Parameters::ID::sampleEnabled, 0.0f);
    setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f);
    setPlainParameter(processor, Parameters::ID::macroBounce, 0.0f);
    setPlainParameter(processor, Parameters::ID::fxPumpEnabled, 1.0f);
    setPlainParameter(processor, Parameters::ID::fxPumpDepth, 0.0f);
    setPlainParameter(processor, Parameters::ID::fxPumpRate, 0.0f);
    setPlainParameter(processor, Parameters::ID::modEnv1Attack, 0.001f);
    setPlainParameter(processor, Parameters::ID::modEnv1Decay, 0.03f);
    setPlainParameter(processor, Parameters::ID::modEnv1Sustain, 1.0f);
    setPlainParameter(processor, Parameters::ID::modEnv1Release, 0.02f);
    setPlainParameter(processor, Parameters::ID::modEnv1Depth, 1.0f);
    if (! configureSourceForAudit(processor, sourceIndex))
        return -1.0f;

    setFirstRoute(processor, sourceIndex, 7, 1.0f, routePolarityForSource(sourceIndex));

    juce::AudioBuffer<float> buffer(2, 512);
    auto maxReduction = 0.0f;
    const auto note = noteForSource(sourceIndex);
    const auto blocks = sourceCanStartNearZero(sourceIndex) ? 64 : 4;
    for (auto block = 0; block < blocks; ++block)
    {
        juce::MidiBuffer midi;
        if (block == 0)
        {
            midi.addEvent(juce::MidiMessage::noteOn(1, note, 1.0f), 0);
            addMidiSourceStimulus(midi, sourceIndex, note, 1);
        }

        processor.processBlock(buffer, midi);
        float phase = 0.0f;
        float gain = 1.0f;
        float reduction = 0.0f;
        bool active = false;
        processor.getPumpMeterLevels(phase, gain, reduction, active);
        maxReduction = juce::jmax(maxReduction, reduction);
    }

    return maxReduction;
}

bool auditFxPerformanceRouteRelease(int sourceIndex, bool releaseWithAllNotesOff)
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);
    clearModMatrix(processor);

    setPlainParameter(processor, Parameters::ID::osc1Level, 0.0f);
    setPlainParameter(processor, Parameters::ID::osc2Level, 0.0f);
    setPlainParameter(processor, Parameters::ID::subLevel, 0.0f);
    setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f);
    setPlainParameter(processor, Parameters::ID::sampleEnabled, 0.0f);
    setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f);
    setPlainParameter(processor, Parameters::ID::macroBounce, 0.0f);
    setPlainParameter(processor, Parameters::ID::fxPumpEnabled, 1.0f);
    setPlainParameter(processor, Parameters::ID::fxPumpDepth, 0.0f);
    setPlainParameter(processor, Parameters::ID::fxPumpRate, 0.0f);
    setPlainParameter(processor, Parameters::ID::outputGain, 0.0f);

    setFirstRoute(processor, sourceIndex, 7, 1.0f, routePolarityForSource(sourceIndex));

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 84, 1.0f), 0);
    processor.processBlock(buffer, midi);

    float phase = 0.0f;
    float gain = 1.0f;
    float reduction = 0.0f;
    bool active = false;
    processor.getPumpMeterLevels(phase, gain, reduction, active);
    if (! active)
    {
        std::cerr << "FX " << modulationSourceName(sourceIndex)
                  << " route did not activate pump before release\n";
        return false;
    }

    midi.clear();
    if (releaseWithAllNotesOff)
        midi.addEvent(juce::MidiMessage::allNotesOff(1), 0);
    else
        midi.addEvent(juce::MidiMessage::noteOff(1, 84), 0);

    processor.processBlock(buffer, midi);
    processor.getPumpMeterLevels(phase, gain, reduction, active);
    if (active)
    {
        std::cerr << "FX " << modulationSourceName(sourceIndex)
                  << " route stayed active after "
                  << (releaseWithAllNotesOff ? "all notes off" : "note off") << '\n';
        return false;
    }

    return true;
}

float renderSynthLevelRoute(int destinationIndex)
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);
    clearModMatrix(processor);

    setPlainParameter(processor, Parameters::ID::osc1Level, 0.0f);
    setPlainParameter(processor, Parameters::ID::osc2Level, 0.0f);
    setPlainParameter(processor, Parameters::ID::subLevel, 0.0f);
    setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f);
    setPlainParameter(processor, Parameters::ID::sampleEnabled, 0.0f);
    setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f);
    setPlainParameter(processor, Parameters::ID::fxDelayEnabled, 0.0f);
    setPlainParameter(processor, Parameters::ID::fxReverbEnabled, 0.0f);
    setPlainParameter(processor, Parameters::ID::fxPumpEnabled, 0.0f);
    setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 0.0f);
    setPlainParameter(processor, Parameters::ID::outputGain, 0.0f);

    if (destinationIndex > 0)
        setFirstRoute(processor, 3, destinationIndex, 1.0f);

    const auto stats = renderNote(processor, 60, 1.0f);
    return stats.finite ? stats.rms : -1.0f;
}

float renderSynthOsc1Route(int sourceIndex)
{
    NateVSTAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);
    clearModMatrix(processor);

    if (! setPlainParameter(processor, Parameters::ID::osc1Level, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::osc2Level, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::subLevel, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::sampleEnabled, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxDelayEnabled, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxReverbEnabled, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxPumpEnabled, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::outputGain, 0.0f)
        || ! configureSourceForAudit(processor, sourceIndex))
    {
        return -1.0f;
    }

    setFirstRoute(processor, sourceIndex, 22, 1.0f, routePolarityForSource(sourceIndex));

    const auto stats = renderNote(processor,
                                  noteForSource(sourceIndex),
                                  1.0f,
                                  sourceCanStartNearZero(sourceIndex) ? 64 : 8,
                                  sourceIndex);
    return stats.finite ? stats.rms : -1.0f;
}

bool configureSynthDestinationAudit(NateVSTAudioProcessor& processor, int destinationIndex)
{
    processor.prepareToPlay(44100.0, 512);
    clearModMatrix(processor);

    if (! setPlainParameter(processor, Parameters::ID::oscWave, destinationIndex == 18 ? 4.0f : 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::osc2Wave, destinationIndex == 19 ? 4.0f : 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::osc1Level, destinationIndex == 5 || destinationIndex == 6 || destinationIndex == 19 ? 0.0f : 0.75f)
        || ! setPlainParameter(processor, Parameters::ID::osc2Level, destinationIndex == 5 || destinationIndex == 19 ? 0.85f : 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::subLevel, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::ampAttack, 0.001f)
        || ! setPlainParameter(processor, Parameters::ID::ampDecay, 0.08f)
        || ! setPlainParameter(processor, Parameters::ID::ampSustain, 0.78f)
        || ! setPlainParameter(processor, Parameters::ID::ampRelease, 0.02f)
        || ! setPlainParameter(processor, Parameters::ID::filterCutoff, destinationIndex == 1 ? 260.0f : 1200.0f)
        || ! setPlainParameter(processor, Parameters::ID::filterResonance, 0.18f)
        || ! setPlainParameter(processor, Parameters::ID::filterEnvAmount, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::driveAmount, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::oscWarp, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::oscWarpB, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::osc2Warp, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::osc2WarpB, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::oscWarpMode, 2.0f)
        || ! setPlainParameter(processor, Parameters::ID::osc2WarpMode, 2.0f)
        || ! setPlainParameter(processor, Parameters::ID::oscWavetablePosition, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::osc2WavetablePosition, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::sampleEnabled, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxDelayEnabled, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxReverbEnabled, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxPumpEnabled, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 0.0f)
        || ! setPlainParameter(processor, Parameters::ID::outputGain, 0.0f))
    {
        return false;
    }

    if (destinationIndex == 3)
    {
        setPlainParameter(processor, Parameters::ID::ampDecay, 0.2f);
        setPlainParameter(processor, Parameters::ID::filterCutoff, 420.0f);
    }

    return true;
}

RenderComparison compareSynthDestination(int destinationIndex)
{
    NateVSTAudioProcessor baseline;
    NateVSTAudioProcessor routed;
    if (! configureSynthDestinationAudit(baseline, destinationIndex)
        || ! configureSynthDestinationAudit(routed, destinationIndex))
    {
        return { 0.0f, 0.0f, -1.0f, false };
    }

    setFirstRoute(routed, 3, destinationIndex, 1.0f);
    return compareRenderedNotes(baseline, routed, 60, 1.0f, 10);
}

bool configureSampleDestinationAudit(NateVSTAudioProcessor& processor, const juce::File& sampleFile)
{
    processor.prepareToPlay(44100.0, 512);
    clearModMatrix(processor);
    if (! processor.loadSampleFile(sampleFile))
        return false;

    return setPlainParameter(processor, Parameters::ID::osc1Level, 0.0f)
        && setPlainParameter(processor, Parameters::ID::osc2Level, 0.0f)
        && setPlainParameter(processor, Parameters::ID::subLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sampleEnabled, 1.0f)
        && setPlainParameter(processor, Parameters::ID::samplePlaybackMode, 1.0f)
        && setPlainParameter(processor, Parameters::ID::sampleStart, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sampleEnd, 1.0f)
        && setPlainParameter(processor, Parameters::ID::sampleGain, -1.0f)
        && setPlainParameter(processor, Parameters::ID::sampleMix, 0.78f)
        && setPlainParameter(processor, Parameters::ID::sampleTranspose, 0.0f)
        && setPlainParameter(processor, Parameters::ID::samplePitchRamp, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sampleStutterEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sampleStutterRepeats, 2.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxReverbEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxPumpEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::outputGain, 0.0f);
}

RenderComparison compareSampleDestination(const juce::File& sampleFile, int destinationIndex)
{
    NateVSTAudioProcessor baseline;
    NateVSTAudioProcessor routed;
    if (! configureSampleDestinationAudit(baseline, sampleFile)
        || ! configureSampleDestinationAudit(routed, sampleFile))
    {
        return { 0.0f, 0.0f, -1.0f, false };
    }

    setFirstRoute(routed, 3, destinationIndex, 1.0f);
    return compareRenderedNotes(baseline, routed, 60, 1.0f, 12);
}

bool configureFxDestinationAudit(NateVSTAudioProcessor& processor, int destinationIndex)
{
    processor.prepareToPlay(44100.0, 512);
    clearModMatrix(processor);

    return setPlainParameter(processor, Parameters::ID::oscWave, 1.0f)
        && setPlainParameter(processor, Parameters::ID::osc1Level, 0.78f)
        && setPlainParameter(processor, Parameters::ID::osc2Level, 0.0f)
        && setPlainParameter(processor, Parameters::ID::subLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::noiseLevel, 0.0f)
        && setPlainParameter(processor, Parameters::ID::ampAttack, 0.001f)
        && setPlainParameter(processor, Parameters::ID::ampDecay, 0.05f)
        && setPlainParameter(processor, Parameters::ID::ampSustain, 0.72f)
        && setPlainParameter(processor, Parameters::ID::ampRelease, 0.01f)
        && setPlainParameter(processor, Parameters::ID::filterCutoff, 16000.0f)
        && setPlainParameter(processor, Parameters::ID::filterResonance, 0.12f)
        && setPlainParameter(processor, Parameters::ID::driveAmount, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sampleEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::sequencerEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::unisonVoices, destinationIndex == 10 ? 4.0f : 1.0f)
        && setPlainParameter(processor, Parameters::ID::unisonDetune, destinationIndex == 10 ? 0.38f : 0.0f)
        && setPlainParameter(processor, Parameters::ID::unisonSpread, destinationIndex == 10 ? 1.0f : 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxPumpEnabled, destinationIndex == 7 ? 1.0f : 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxPumpDepth, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDistortionEnabled, destinationIndex == 11 ? 1.0f : 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDistortionAmount, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayEnabled, destinationIndex == 8 ? 1.0f : 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelaySync, 1.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayRate, 4.0f)
        && setPlainParameter(processor, Parameters::ID::fxDelayFeedback, 0.54f)
        && setPlainParameter(processor, Parameters::ID::fxDelayMix, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxSendDelay, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxReverbEnabled, destinationIndex == 9 ? 1.0f : 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxReverbSize, 0.72f)
        && setPlainParameter(processor, Parameters::ID::fxReverbMix, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxSendReverb, 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxWidthEnabled, destinationIndex == 10 ? 1.0f : 0.0f)
        && setPlainParameter(processor, Parameters::ID::fxWidthAmount, 0.55f)
        && setPlainParameter(processor, Parameters::ID::fxGuardEnabled, 0.0f)
        && setPlainParameter(processor, Parameters::ID::outputGain, 0.0f);
}

RenderComparison compareFxDestination(int destinationIndex)
{
    NateVSTAudioProcessor baseline;
    NateVSTAudioProcessor routed;
    if (! configureFxDestinationAudit(baseline, destinationIndex)
        || ! configureFxDestinationAudit(routed, destinationIndex))
    {
        return { 0.0f, 0.0f, -1.0f, false };
    }

    setFirstRoute(routed, 3, destinationIndex, 1.0f);
    const auto blocks = destinationIndex == 8 || destinationIndex == 20 || destinationIndex == 21 ? 38 : 12;
    const auto noteOffBlock = destinationIndex == 8 || destinationIndex == 20 || destinationIndex == 21 ? 24 : 3;
    return compareRenderedNotes(baseline, routed, 60, 1.0f, blocks, 3, noteOffBlock);
}

bool destinationChangedEnough(const RenderComparison& comparison, float minimumDiff)
{
    return comparison.finite
        && comparison.diffRms >= minimumDiff
        && comparison.routedRms >= 0.0001f;
}
}

int main()
{
    if (! auditPerformanceModulationStatus())
        return 1;

    if (! auditFxPerformanceRouteRelease(3, false)
        || ! auditFxPerformanceRouteRelease(19, false)
        || ! auditFxPerformanceRouteRelease(3, true)
        || ! auditFxPerformanceRouteRelease(19, true))
    {
        return 1;
    }

    const auto sampleFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("NateVST_ModulationParityAudit.wav");
    if (! writeTestSample(sampleFile))
    {
        std::cerr << "Could not write temporary modulation parity sample\n";
        return 1;
    }

    const auto noSampleRouteRms = renderSampleMixRoute(sampleFile, 0);
    const auto sampleEnvRms = renderSampleMixRoute(sampleFile, 2);
    const auto sampleVelocityRms = renderSampleMixRoute(sampleFile, 3);

    if (noSampleRouteRms < 0.0f || noSampleRouteRms > 0.0001f)
    {
        std::cerr << "Sample mix baseline was not silent with mix at zero: " << noSampleRouteRms << '\n';
        return 1;
    }

    if (sampleEnvRms <= 0.01f || sampleVelocityRms <= 0.01f)
    {
        std::cerr << "Sample Mod Env/Velocity sources did not open sample mix: env "
                  << sampleEnvRms << " velocity " << sampleVelocityRms << '\n';
        return 1;
    }

    const auto noFxRouteReduction = renderFxPumpReduction(0);
    const auto fxEnvReduction = renderFxPumpReduction(2);
    const auto fxVelocityReduction = renderFxPumpReduction(3);

    if (noFxRouteReduction > 0.001f)
    {
        std::cerr << "FX pump baseline reduced gain with no modulation route: "
                  << noFxRouteReduction << '\n';
        return 1;
    }

    if (fxEnvReduction <= 0.02f || fxVelocityReduction <= 0.02f)
    {
        std::cerr << "FX Mod Env/Velocity sources did not drive pump depth: env "
                  << fxEnvReduction << " velocity " << fxVelocityReduction << '\n';
        return 1;
    }

    const int routedSources[] { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
    for (const auto sourceIndex : routedSources)
    {
        const auto sampleMidiRms = renderSampleMixRoute(sampleFile, sourceIndex);
        if (sampleMidiRms <= 0.01f)
        {
            std::cerr << "Sample " << modulationSourceName(sourceIndex)
                      << " source did not open sample mix: " << sampleMidiRms << '\n';
            return 1;
        }

        const auto fxMidiReduction = renderFxPumpReduction(sourceIndex);
        if (fxMidiReduction <= 0.02f)
        {
            std::cerr << "FX " << modulationSourceName(sourceIndex)
                      << " source did not drive pump depth: " << fxMidiReduction << '\n';
            return 1;
        }

        const auto synthSourceRms = renderSynthOsc1Route(sourceIndex);
        if (synthSourceRms <= 0.005f)
        {
            std::cerr << "Synth " << modulationSourceName(sourceIndex)
                      << " source did not open Osc 1 Level: " << synthSourceRms << '\n';
            return 1;
        }
    }

    const int synthDestinations[] { 1, 2, 3, 4, 5, 6, 17, 18, 19 };
    for (const auto destinationIndex : synthDestinations)
    {
        const auto comparison = compareSynthDestination(destinationIndex);
        if (! destinationChangedEnough(comparison, 0.00045f))
        {
            std::cerr << "Synth destination " << destinationIndex
                      << " did not produce a meaningful routed change: baseline "
                      << comparison.baselineRms
                      << " routed " << comparison.routedRms
                      << " diff " << comparison.diffRms
                      << " finite " << comparison.finite << '\n';
            return 1;
        }
    }

    const int sampleDestinations[] { 12, 14, 15, 16 };
    for (const auto destinationIndex : sampleDestinations)
    {
        const auto comparison = compareSampleDestination(sampleFile, destinationIndex);
        if (! destinationChangedEnough(comparison, 0.0008f))
        {
            std::cerr << "Sample destination " << destinationIndex
                      << " did not produce a meaningful routed change: baseline "
                      << comparison.baselineRms
                      << " routed " << comparison.routedRms
                      << " diff " << comparison.diffRms
                      << " finite " << comparison.finite << '\n';
            return 1;
        }
    }

    const int fxDestinations[] { 8, 9, 10, 11, 20, 21 };
    for (const auto destinationIndex : fxDestinations)
    {
        const auto comparison = compareFxDestination(destinationIndex);
        if (! destinationChangedEnough(comparison, 0.00045f))
        {
            std::cerr << "FX destination " << destinationIndex
                      << " did not produce a meaningful routed change: baseline "
                      << comparison.baselineRms
                      << " routed " << comparison.routedRms
                      << " diff " << comparison.diffRms
                      << " finite " << comparison.finite << '\n';
            return 1;
        }
    }

    const auto noSynthLevelRouteRms = renderSynthLevelRoute(0);
    const auto osc1LevelRouteRms = renderSynthLevelRoute(22);
    const auto subLevelRouteRms = renderSynthLevelRoute(23);
    const auto noiseLevelRouteRms = renderSynthLevelRoute(24);

    if (noSynthLevelRouteRms < 0.0f || noSynthLevelRouteRms > 0.0001f)
    {
        std::cerr << "Synth level baseline was not silent with all source levels at zero: "
                  << noSynthLevelRouteRms << '\n';
        return 1;
    }

    if (osc1LevelRouteRms <= 0.005f || subLevelRouteRms <= 0.005f || noiseLevelRouteRms <= 0.001f)
    {
        std::cerr << "Velocity routes did not open synth source levels: osc1 "
                  << osc1LevelRouteRms
                  << " sub " << subLevelRouteRms
                  << " noise " << noiseLevelRouteRms << '\n';
        return 1;
    }

    sampleFile.deleteFile();
    std::cout << "Modulation source parity audit passed for sample, FX, and synth level routes.\n";
    return 0;
}
