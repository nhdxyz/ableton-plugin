#include "../Source/PluginProcessor.h"

#include <cmath>
#include <iostream>

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

float readPlainParameter(NateVSTAudioProcessor& processor, const juce::String& parameterID, float fallback)
{
    if (auto* value = processor.getValueTreeState().getRawParameterValue(parameterID))
        return value->load();

    return fallback;
}

bool near(float actual, float expected, float tolerance = 0.001f)
{
    return std::abs(actual - expected) <= tolerance;
}
}

int main()
{
    NateVSTAudioProcessor processor;

    Sequencer::Step initialStep;
    initialStep.enabled = true;
    initialStep.noteOffset = 2;
    initialStep.velocity = 0.45f;
    initialStep.probability = 0.84f;
    initialStep.timing = 0.08f;
    initialStep.length = 0.42f;
    initialStep.lock = 0.18f;
    initialStep.ratchet = 2;
    initialStep.condition = 1;

    Sequencer::Step initialSlideStep = initialStep;
    initialSlideStep.noteOffset = -5;
    initialSlideStep.ratchet = 1;
    initialSlideStep.condition = 0;
    initialSlideStep.slide = true;

    if (! setPlainParameter(processor, Parameters::ID::filterCutoff, 760.0f)
        || ! setPlainParameter(processor, Parameters::ID::driveAmount, 0.12f)
        || ! setPlainParameter(processor, Parameters::ID::modMatrixSource[0], 4.0f)
        || ! setPlainParameter(processor, Parameters::ID::modMatrixDestination[0], 1.0f)
        || ! setPlainParameter(processor, Parameters::ID::modMatrixAmount[0], 0.18f)
        || ! setPlainParameter(processor, Parameters::ID::modMatrixEnabled[0], 1.0f))
    {
        std::cerr << "Could not seed initial global edit parameters\n";
        return 1;
    }

    processor.setSequencerStep(0, initialStep);
    processor.setSequencerStep(1, initialSlideStep);
    processor.capturePerformanceSnapshot(0);
    processor.captureGlobalEditState("Initial audit state");

    Sequencer::Step editedStep = initialStep;
    editedStep.noteOffset = 7;
    editedStep.velocity = 0.93f;
    editedStep.probability = 0.38f;
    editedStep.timing = 0.11f;
    editedStep.length = 0.88f;
    editedStep.lock = 0.72f;
    editedStep.ratchet = 4;
    editedStep.condition = 3;

    Sequencer::Step editedSlideStep = initialSlideStep;
    editedSlideStep.noteOffset = 5;
    editedSlideStep.velocity = 0.62f;
    editedSlideStep.slide = false;

    if (! setPlainParameter(processor, Parameters::ID::filterCutoff, 2460.0f)
        || ! setPlainParameter(processor, Parameters::ID::driveAmount, 0.58f)
        || ! setPlainParameter(processor, Parameters::ID::modMatrixSource[0], 12.0f)
        || ! setPlainParameter(processor, Parameters::ID::modMatrixDestination[0], 8.0f)
        || ! setPlainParameter(processor, Parameters::ID::modMatrixAmount[0], -0.44f)
        || ! setPlainParameter(processor, Parameters::ID::modMatrixEnabled[0], 0.0f))
    {
        std::cerr << "Could not seed edited global edit parameters\n";
        return 1;
    }

    processor.setSequencerStep(0, editedStep);
    processor.setSequencerStep(1, editedSlideStep);
    processor.capturePerformanceSnapshot(0);

    if (! processor.canUndoGlobalEdit() || processor.canRedoGlobalEdit())
    {
        std::cerr << "Initial global edit stack state is wrong: "
                  << processor.getGlobalEditHistorySummary() << '\n';
        return 1;
    }

    if (! processor.undoGlobalEdit())
    {
        std::cerr << "undoGlobalEdit returned false\n";
        return 1;
    }

    const auto undoStep = processor.getSequencerStep(0);
    const auto undoSlideStep = processor.getSequencerStep(1);
    if (! near(readPlainParameter(processor, Parameters::ID::filterCutoff, 0.0f), 760.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::driveAmount, 0.0f), 0.12f)
        || ! near(readPlainParameter(processor, Parameters::ID::modMatrixSource[0], 0.0f), 4.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::modMatrixDestination[0], 0.0f), 1.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::modMatrixAmount[0], 0.0f), 0.18f)
        || ! near(readPlainParameter(processor, Parameters::ID::modMatrixEnabled[0], 0.0f), 1.0f)
        || undoStep.noteOffset != initialStep.noteOffset
        || ! near(undoStep.velocity, initialStep.velocity)
        || ! near(undoStep.probability, initialStep.probability)
        || ! near(undoStep.timing, initialStep.timing)
        || ! near(undoStep.length, initialStep.length)
        || ! near(undoStep.lock, initialStep.lock)
        || undoStep.ratchet != initialStep.ratchet
        || undoStep.condition != initialStep.condition
        || undoSlideStep.noteOffset != initialSlideStep.noteOffset
        || ! undoSlideStep.slide
        || processor.canUndoGlobalEdit()
        || ! processor.canRedoGlobalEdit())
    {
        std::cerr << "Global undo did not restore the initial synth/mod/sequencer state\n"
                  << " cutoff=" << readPlainParameter(processor, Parameters::ID::filterCutoff, 0.0f)
                  << " drive=" << readPlainParameter(processor, Parameters::ID::driveAmount, 0.0f)
                  << " modSrc=" << readPlainParameter(processor, Parameters::ID::modMatrixSource[0], 0.0f)
                  << " modDest=" << readPlainParameter(processor, Parameters::ID::modMatrixDestination[0], 0.0f)
                  << " modAmt=" << readPlainParameter(processor, Parameters::ID::modMatrixAmount[0], 0.0f)
                  << " modEnabled=" << readPlainParameter(processor, Parameters::ID::modMatrixEnabled[0], 0.0f)
                  << " stepNote=" << undoStep.noteOffset
                  << " stepVel=" << undoStep.velocity
                  << " stepProb=" << undoStep.probability
                  << " stepTiming=" << undoStep.timing
                  << " stepLen=" << undoStep.length
                  << " stepLock=" << undoStep.lock
                  << " stepRatchet=" << undoStep.ratchet
                  << " stepCondition=" << undoStep.condition
                  << " slideStepNote=" << undoSlideStep.noteOffset
                  << " slideStepSlide=" << undoSlideStep.slide
                  << " canUndo=" << processor.canUndoGlobalEdit()
                  << " canRedo=" << processor.canRedoGlobalEdit()
                  << '\n';
        return 1;
    }

    if (! setPlainParameter(processor, Parameters::ID::filterCutoff, 111.0f)
        || ! processor.recallPerformanceSnapshot(0)
        || ! near(readPlainParameter(processor, Parameters::ID::filterCutoff, 0.0f), 760.0f))
    {
        std::cerr << "Global undo did not restore performance snapshot A\n";
        return 1;
    }

    if (! processor.redoGlobalEdit())
    {
        std::cerr << "redoGlobalEdit returned false\n";
        return 1;
    }

    const auto redoStep = processor.getSequencerStep(0);
    const auto redoSlideStep = processor.getSequencerStep(1);
    if (! near(readPlainParameter(processor, Parameters::ID::filterCutoff, 0.0f), 2460.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::driveAmount, 0.0f), 0.58f)
        || ! near(readPlainParameter(processor, Parameters::ID::modMatrixSource[0], 0.0f), 12.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::modMatrixDestination[0], 0.0f), 8.0f)
        || ! near(readPlainParameter(processor, Parameters::ID::modMatrixAmount[0], 0.0f), -0.44f)
        || ! near(readPlainParameter(processor, Parameters::ID::modMatrixEnabled[0], 0.0f), 0.0f)
        || redoStep.noteOffset != editedStep.noteOffset
        || ! near(redoStep.velocity, editedStep.velocity)
        || ! near(redoStep.probability, editedStep.probability)
        || ! near(redoStep.timing, editedStep.timing)
        || ! near(redoStep.length, editedStep.length)
        || ! near(redoStep.lock, editedStep.lock)
        || redoStep.ratchet != editedStep.ratchet
        || redoStep.condition != editedStep.condition
        || redoSlideStep.noteOffset != editedSlideStep.noteOffset
        || redoSlideStep.slide
        || ! processor.canUndoGlobalEdit()
        || processor.canRedoGlobalEdit())
    {
        std::cerr << "Global redo did not restore the edited synth/mod/sequencer state\n";
        return 1;
    }

    if (! setPlainParameter(processor, Parameters::ID::filterCutoff, 333.0f)
        || ! processor.recallPerformanceSnapshot(0)
        || ! near(readPlainParameter(processor, Parameters::ID::filterCutoff, 0.0f), 2460.0f))
    {
        std::cerr << "Global redo did not restore edited performance snapshot A\n";
        return 1;
    }

    std::cout << "Global edit history audit passed.\n";
    return 0;
}
