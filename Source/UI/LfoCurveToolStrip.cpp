#include "LfoCurveToolStrip.h"

namespace UI
{
LfoCurveToolStrip::LfoCurveToolStrip()
{
    setComponentID("LfoCurveToolStrip");

    for (size_t index = 0; index < buttons.size(); ++index)
    {
        auto& button = buttons[index];
        const auto tool = toolForIndex(index);
        button.setButtonText(labelForTool(tool));
        button.setTooltip(tooltipForTool(tool));
        button.setWantsKeyboardFocus(false);
        button.setMouseClickGrabsKeyboardFocus(false);
        button.onClick = [this, tool]
        {
            if (onToolSelected != nullptr)
                onToolSelected(tool);
        };
        addAndMakeVisible(button);
    }
}

void LfoCurveToolStrip::resized()
{
    auto area = getLocalBounds();
    for (size_t index = 0; index < buttons.size(); ++index)
    {
        const auto remaining = static_cast<int>(buttons.size() - index);
        buttons[index].setBounds(area.removeFromLeft(area.getWidth() / remaining).reduced(3, 4));
    }
}

juce::String LfoCurveToolStrip::labelForTool(Tool tool)
{
    switch (tool)
    {
        case Tool::invert: return "Inv";
        case Tool::reverse: return "Rev";
        case Tool::smooth: return "Smooth";
        case Tool::quantize: return "1/8";
        case Tool::randomize: return "Dice";
        case Tool::garage: return "UKG";
        default: return {};
    }
}

juce::String LfoCurveToolStrip::tooltipForTool(Tool tool)
{
    switch (tool)
    {
        case Tool::invert: return "Invert the custom LFO curve around the centre line";
        case Tool::reverse: return "Reverse the MSEG point order so the motion plays backward";
        case Tool::smooth: return "Smooth neighbouring MSEG points for less stepped movement";
        case Tool::quantize: return "Quantize MSEG point values to eighth-depth steps";
        case Tool::randomize: return "Generate a controlled random MSEG curve for new movement ideas";
        case Tool::garage: return "Apply a UK garage swing MSEG shape for shuffled bass, stabs, and chops";
        default: return {};
    }
}

LfoCurveToolStrip::Tool LfoCurveToolStrip::toolForIndex(size_t index) noexcept
{
    switch (index)
    {
        case 0: return Tool::invert;
        case 1: return Tool::reverse;
        case 2: return Tool::smooth;
        case 3: return Tool::quantize;
        case 4: return Tool::randomize;
        case 5: return Tool::garage;
        default: return Tool::invert;
    }
}
}
