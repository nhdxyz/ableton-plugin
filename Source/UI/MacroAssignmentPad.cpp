#include "MacroAssignmentPad.h"

#include <cmath>
#include <utility>

namespace UI
{
namespace
{
constexpr auto macroCount = 8;
constexpr auto macroRowHeight = 24.0f;
constexpr auto targetRowHeight = 34.0f;
constexpr auto rowGap = 5.0f;
constexpr auto destinationColumns = 11;
}

MacroAssignmentPad::MacroAssignmentPad()
{
    setInterceptsMouseClicks(true, true);
    setRepaintsOnMouseActivity(true);

    state.macroNames = { "Tone", "Dirt", "Motion", "Space", "Weight", "Bounce", "Warp", "Throw" };
    for (size_t index = 0; index < state.macroSourceIndices.size(); ++index)
        state.macroSourceIndices[index] = 4 + static_cast<int>(index);
}

void MacroAssignmentPad::setState(State newState)
{
    newState.targetAmount = clampBipolar(newState.targetAmount);
    for (auto& destination : newState.destinations)
        destination.amount = clampBipolar(destination.amount);

    state = std::move(newState);
    repaint();
}

juce::String MacroAssignmentPad::getTooltip()
{
    const auto detail = "Click a macro or destination. Drag up/down in destination pads or use the wheel to set bipolar amount.";
    return state.summary.isNotEmpty() ? state.summary + "\n" + detail : juce::String(detail);
}

void MacroAssignmentPad::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const auto selectedMacroIndex = static_cast<size_t>(juce::jlimit(0, macroCount - 1, selectedMacroArrayIndex()));
    const auto accent = macroColour(selectedMacroIndex);

    g.setColour(juce::Colour(0xff0c1114));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour(0xff2b373d));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    for (size_t index = 0; index < state.macroNames.size(); ++index)
    {
        auto chip = macroBounds(index);
        const auto selected = state.macroSourceIndices[index] == state.selectedSourceIndex;
        const auto colour = macroColour(index);
        const auto routeCount = state.macroRouteCounts[index];

        g.setColour(selected ? colour.withAlpha(0.28f) : juce::Colour(0xff151e22));
        g.fillRoundedRectangle(chip, 4.0f);
        g.setColour(selected ? colour : juce::Colour(0xff344148));
        g.drawRoundedRectangle(chip, 4.0f, selected ? 1.3f : 0.8f);

        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.setColour(selected ? juce::Colour(0xfff1fffb) : juce::Colour(0xff92a0a5));
        g.drawFittedText(compactName(state.macroNames[index]), chip.toNearestInt().reduced(3, 0), juce::Justification::centred, 1, 0.58f);

        if (routeCount > 0)
        {
            auto badge = chip.removeFromRight(13.0f).removeFromTop(12.0f).reduced(1.0f);
            g.setColour(colour.withAlpha(selected ? 0.92f : 0.72f));
            g.fillEllipse(badge);
            g.setFont(juce::FontOptions(7.0f, juce::Font::bold));
            g.setColour(juce::Colour(0xff07100d));
            g.drawFittedText(juce::String(routeCount), badge.toNearestInt(), juce::Justification::centred, 1);
        }
    }

    auto clear = getClearBounds();
    g.setColour(juce::Colour(0xff151e22));
    g.fillRoundedRectangle(clear, 4.0f);
    g.setColour(state.macroRouteCounts[selectedMacroIndex] > 0 ? juce::Colour(0xffff8f78) : juce::Colour(0xff3b474d));
    g.drawRoundedRectangle(clear, 4.0f, 0.9f);
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.drawFittedText("CLR", clear.toNearestInt(), juce::Justification::centred, 1);

    auto target = getTargetBounds();
    g.setColour(juce::Colour(0xff141d21));
    g.fillRoundedRectangle(target, 5.0f);
    g.setColour(accent.withAlpha(0.72f));
    g.drawRoundedRectangle(target, 5.0f, 1.0f);

    auto targetText = target.reduced(9.0f, 3.0f).removeFromTop(13.0f);
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffdce7e4));
    g.drawFittedText(compactName(state.macroNames[selectedMacroIndex]) + " -> " + compactName(selectedDestinationName()),
                     targetText.removeFromLeft(target.getWidth() - 58.0f).toNearestInt(),
                     juce::Justification::centredLeft,
                     1,
                     0.58f);

    const auto targetPercent = juce::roundToInt(state.targetAmount * 100.0f);
    g.setColour(accent);
    g.drawFittedText((targetPercent >= 0 ? "+" : "") + juce::String(targetPercent) + "%",
                     targetText.toNearestInt(),
                     juce::Justification::centredRight,
                     1);

    auto amountRail = target.reduced(10.0f, 0.0f).removeFromBottom(9.0f).reduced(0.0f, 1.0f);
    const auto centreX = amountRail.getCentreX();
    const auto amountX = juce::jmap(state.targetAmount, -1.0f, 1.0f, amountRail.getX(), amountRail.getRight());

    g.setColour(juce::Colour(0xff263238));
    g.fillRoundedRectangle(amountRail, 3.0f);
    g.setColour(accent.withAlpha(0.82f));
    g.fillRoundedRectangle(juce::Rectangle<float>::leftTopRightBottom(juce::jmin(centreX, amountX),
                                                                      amountRail.getY(),
                                                                      juce::jmax(centreX, amountX),
                                                                      amountRail.getBottom()),
                           3.0f);
    g.setColour(juce::Colour(0xff0a0e10));
    g.drawVerticalLine(static_cast<int>(std::round(centreX)), amountRail.getY(), amountRail.getBottom());
    g.setColour(accent);
    g.fillEllipse(juce::Rectangle<float> { 7.0f, 7.0f }.withCentre({ amountX, amountRail.getCentreY() }));

    auto grid = getDestinationGridBounds();
    g.setFont(juce::FontOptions(7.6f, juce::Font::bold));
    for (size_t index = 0; index < state.destinations.size(); ++index)
    {
        const auto& destination = state.destinations[index];
        auto chip = destinationBounds(index);
        const auto selected = destination.index == state.selectedDestinationIndex;
        const auto hasRoute = destination.assigned && std::abs(destination.amount) > 0.001f;
        const auto colour = hasRoute ? (destination.amount >= 0.0f ? accent : juce::Colour(0xffff9f73))
                                     : juce::Colour(0xff526169);

        g.setColour(selected ? accent.withAlpha(0.22f) : juce::Colour(0xff151e22));
        g.fillRoundedRectangle(chip, 3.0f);
        g.setColour(selected ? accent : juce::Colour(0xff303b42));
        g.drawRoundedRectangle(chip, 3.0f, selected ? 1.0f : 0.7f);

        if (hasRoute)
        {
            auto routeRail = chip.reduced(5.0f, 5.0f);
            if (routeRail.getHeight() > 28.0f)
                routeRail = routeRail.withTrimmedTop(10.0f).withTrimmedBottom(10.0f);

            const auto routeCentre = routeRail.getCentreY();
            const auto routeY = juce::jmap(destination.amount, -1.0f, 1.0f, routeRail.getBottom(), routeRail.getY());
            g.setColour(colour.withAlpha(destination.enabled ? 0.82f : 0.36f));
            g.fillRoundedRectangle(juce::Rectangle<float>::leftTopRightBottom(routeRail.getX(),
                                                                              juce::jmin(routeCentre, routeY),
                                                                              routeRail.getRight(),
                                                                              juce::jmax(routeCentre, routeY)),
                                   2.0f);
            g.setColour(juce::Colour(0xff0a0e10).withAlpha(0.72f));
            g.drawHorizontalLine(static_cast<int>(std::round(routeCentre)), routeRail.getX(), routeRail.getRight());
            g.setColour(colour.withAlpha(destination.enabled ? 0.95f : 0.42f));
            g.fillRoundedRectangle(routeRail.withY(routeY - 1.5f).withHeight(3.0f), 1.5f);
        }

        g.setColour(selected || hasRoute ? juce::Colour(0xffdce7e4) : juce::Colour(0xff87969a));
        g.drawFittedText(compactName(destination.name), chip.toNearestInt().reduced(3, 0), juce::Justification::centred, 1, 0.48f);

        if (selected || hasRoute)
        {
            const auto shownAmount = selected ? state.targetAmount : destination.amount;
            const auto percent = juce::roundToInt(shownAmount * 100.0f);
            auto percentArea = chip.toNearestInt().reduced(4, 3).removeFromBottom(10);
            g.setColour(selected ? accent : colour);
            g.setFont(juce::FontOptions(7.0f, juce::Font::bold));
            g.drawFittedText((percent >= 0 ? "+" : "") + juce::String(percent),
                             percentArea,
                             juce::Justification::centred,
                             1,
                             0.58f);
        }
    }

    if (state.destinations.empty())
    {
        g.setColour(juce::Colour(0xff66757b));
        g.drawFittedText("No destinations", grid.toNearestInt(), juce::Justification::centred, 1);
    }
}

void MacroAssignmentPad::mouseDown(const juce::MouseEvent& event)
{
    if (getClearBounds().contains(event.position))
    {
        if (onClearSource)
            onClearSource(state.selectedSourceIndex);
        return;
    }

    if (const auto sourceIndex = sourceIndexForPosition(event.position); sourceIndex > 0)
    {
        if (onSourceSelected)
            onSourceSelected(sourceIndex);
        return;
    }

    if (const auto destinationIndex = destinationIndexForPosition(event.position); destinationIndex > 0)
    {
        editingDestinationIndex = destinationIndex;
        editingAmount = true;

        if (onDestinationSelected)
            onDestinationSelected(destinationIndex);

        previewAmountAt(event.position);
        return;
    }

    if (getTargetBounds().contains(event.position))
    {
        editingDestinationIndex = state.selectedDestinationIndex;
        editingAmount = true;
        previewAmountAt(event.position);
    }
}

void MacroAssignmentPad::mouseDrag(const juce::MouseEvent& event)
{
    if (editingAmount)
        previewAmountAt(event.position);
}

void MacroAssignmentPad::mouseUp(const juce::MouseEvent&)
{
    if (! editingAmount)
        return;

    commitAmount();
    editingAmount = false;
    editingDestinationIndex = 0;
}

void MacroAssignmentPad::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (const auto sourceIndex = sourceIndexForPosition(event.position); sourceIndex > 0 && onClearSource)
        onClearSource(sourceIndex);
}

void MacroAssignmentPad::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    const auto delta = (wheel.deltaY >= 0.0f ? 0.05f : -0.05f) * (event.mods.isShiftDown() || event.mods.isCommandDown() ? 0.25f : 1.0f);
    nudgeAmountAt(event.position, delta);
}

juce::Rectangle<float> MacroAssignmentPad::getContentBounds() const
{
    return getLocalBounds().toFloat().reduced(8.0f, 6.0f);
}

juce::Rectangle<float> MacroAssignmentPad::getMacroRowBounds() const
{
    return getContentBounds().removeFromTop(macroRowHeight);
}

juce::Rectangle<float> MacroAssignmentPad::getTargetBounds() const
{
    auto content = getContentBounds();
    content.removeFromTop(macroRowHeight + rowGap);
    return content.removeFromTop(targetRowHeight);
}

juce::Rectangle<float> MacroAssignmentPad::getDestinationGridBounds() const
{
    auto content = getContentBounds();
    content.removeFromTop(macroRowHeight + rowGap + targetRowHeight + rowGap);
    return content;
}

juce::Rectangle<float> MacroAssignmentPad::getClearBounds() const
{
    return getMacroRowBounds().removeFromRight(30.0f).reduced(1.0f, 2.0f);
}

juce::Rectangle<float> MacroAssignmentPad::macroBounds(size_t macroIndex) const
{
    auto row = getMacroRowBounds();
    row.removeFromRight(34.0f);

    const auto gap = 3.0f;
    const auto chipWidth = (row.getWidth() - (gap * static_cast<float>(macroCount - 1))) / static_cast<float>(macroCount);
    return row.withX(row.getX() + (static_cast<float>(macroIndex) * (chipWidth + gap)))
              .withWidth(chipWidth)
              .reduced(0.0f, 2.0f);
}

juce::Rectangle<float> MacroAssignmentPad::destinationBounds(size_t destinationIndex) const
{
    auto grid = getDestinationGridBounds();
    const auto rows = juce::jmax(1, static_cast<int>(std::ceil(static_cast<float>(state.destinations.size())
                                                              / static_cast<float>(destinationColumns))));
    const auto gap = 3.0f;
    const auto cellWidth = (grid.getWidth() - (gap * static_cast<float>(destinationColumns - 1))) / static_cast<float>(destinationColumns);
    const auto cellHeight = (grid.getHeight() - (gap * static_cast<float>(rows - 1))) / static_cast<float>(rows);
    const auto column = static_cast<int>(destinationIndex % destinationColumns);
    const auto row = static_cast<int>(destinationIndex / destinationColumns);

    return {
        grid.getX() + (static_cast<float>(column) * (cellWidth + gap)),
        grid.getY() + (static_cast<float>(row) * (cellHeight + gap)),
        cellWidth,
        cellHeight
    };
}

int MacroAssignmentPad::sourceIndexForPosition(juce::Point<float> position) const
{
    for (size_t index = 0; index < state.macroSourceIndices.size(); ++index)
        if (macroBounds(index).contains(position))
            return state.macroSourceIndices[index];

    return 0;
}

int MacroAssignmentPad::destinationIndexForPosition(juce::Point<float> position) const
{
    for (size_t index = 0; index < state.destinations.size(); ++index)
        if (destinationBounds(index).contains(position))
            return state.destinations[index].index;

    return 0;
}

void MacroAssignmentPad::previewAmountAt(juce::Point<float> position)
{
    const auto destinationIndex = editingDestinationIndex > 0 ? editingDestinationIndex : state.selectedDestinationIndex;
    auto amountArea = getTargetBounds();

    for (size_t index = 0; index < state.destinations.size(); ++index)
    {
        if (state.destinations[index].index == destinationIndex)
        {
            amountArea = destinationBounds(index);
            break;
        }
    }

    const auto amount = amountFromPosition(position, amountArea);
    state.targetAmount = amount;
    state.selectedDestinationIndex = destinationIndex;
    repaint();

    if (onTargetAmountPreview)
        onTargetAmountPreview(state.selectedSourceIndex, destinationIndex, amount);
}

void MacroAssignmentPad::commitAmount()
{
    const auto destinationIndex = editingDestinationIndex > 0 ? editingDestinationIndex : state.selectedDestinationIndex;

    if (destinationIndex <= 0)
        return;

    if (onTargetAmountCommit)
        onTargetAmountCommit(state.selectedSourceIndex, destinationIndex, state.targetAmount);
}

void MacroAssignmentPad::nudgeAmountAt(juce::Point<float> position, float delta)
{
    auto destinationIndex = destinationIndexForPosition(position);
    if (destinationIndex <= 0 && getTargetBounds().contains(position))
        destinationIndex = state.selectedDestinationIndex;

    if (destinationIndex <= 0)
        return;

    const auto amount = clampBipolar(currentAmountForDestination(destinationIndex) + delta);
    state.selectedDestinationIndex = destinationIndex;
    state.targetAmount = amount;
    repaint();

    if (onTargetAmountPreview)
        onTargetAmountPreview(state.selectedSourceIndex, destinationIndex, amount);

    if (onTargetAmountCommit)
        onTargetAmountCommit(state.selectedSourceIndex, destinationIndex, amount);
}

float MacroAssignmentPad::currentAmountForDestination(int destinationIndex) const noexcept
{
    for (const auto& destination : state.destinations)
        if (destination.index == destinationIndex && destination.assigned)
            return destination.amount;

    return destinationIndex == state.selectedDestinationIndex ? state.targetAmount : 0.0f;
}

int MacroAssignmentPad::selectedMacroArrayIndex() const noexcept
{
    for (size_t index = 0; index < state.macroSourceIndices.size(); ++index)
        if (state.macroSourceIndices[index] == state.selectedSourceIndex)
            return static_cast<int>(index);

    return 0;
}

float MacroAssignmentPad::amountFromPosition(juce::Point<float> position, juce::Rectangle<float> area) const noexcept
{
    const auto usable = area.reduced(8.0f, 0.0f);
    if (usable.getWidth() > usable.getHeight() * 2.4f)
    {
        const auto normalised = (position.x - usable.getX()) / juce::jmax(1.0f, usable.getWidth());
        return clampBipolar((normalised * 2.0f) - 1.0f);
    }

    const auto normalised = (position.y - usable.getY()) / juce::jmax(1.0f, usable.getHeight());
    return clampBipolar(1.0f - (normalised * 2.0f));
}

juce::String MacroAssignmentPad::selectedDestinationName() const
{
    for (const auto& destination : state.destinations)
        if (destination.index == state.selectedDestinationIndex)
            return destination.name;

    return "Destination";
}

float MacroAssignmentPad::clampBipolar(float value) noexcept
{
    return juce::jlimit(-1.0f, 1.0f, value);
}

juce::Colour MacroAssignmentPad::macroColour(size_t index) noexcept
{
    static constexpr std::array<juce::uint32, macroCount> colours {
        0xff8ee6c9,
        0xffffc36b,
        0xff7bb7ff,
        0xffb7a4ff,
        0xffd7e37b,
        0xff7fd0e6,
        0xfff0a86e,
        0xffff8fb4
    };

    return juce::Colour(colours[index % colours.size()]);
}

juce::String MacroAssignmentPad::compactName(const juce::String& name)
{
    if (name.equalsIgnoreCase("Filter Cutoff"))
        return "Cut";
    if (name.equalsIgnoreCase("Filter Res"))
        return "Res";
    if (name.equalsIgnoreCase("Filter Env"))
        return "Env";
    if (name.equalsIgnoreCase("Osc 2 Tune"))
        return "O2 Tune";
    if (name.equalsIgnoreCase("Osc 2 Level"))
        return "O2 Lev";
    if (name.equalsIgnoreCase("FX Pump Depth"))
        return "Pump";
    if (name.equalsIgnoreCase("FX Delay Mix"))
        return "Dly";
    if (name.equalsIgnoreCase("FX Reverb Mix"))
        return "Rev";
    if (name.equalsIgnoreCase("FX Width"))
        return "Width";
    if (name.equalsIgnoreCase("FX Drive"))
        return "FX Drv";
    if (name.equalsIgnoreCase("Sample Start"))
        return "S Start";
    if (name.equalsIgnoreCase("Sample Mix"))
        return "S Mix";
    if (name.equalsIgnoreCase("Sample Pitch"))
        return "S Pitch";
    if (name.equalsIgnoreCase("Sample Ramp"))
        return "S Ramp";
    if (name.equalsIgnoreCase("Sample Stutter"))
        return "Stut";
    if (name.equalsIgnoreCase("Osc Warp"))
        return "Warp";
    if (name.equalsIgnoreCase("Osc 1 WT Pos"))
        return "WT1";
    if (name.equalsIgnoreCase("Osc 2 WT Pos"))
        return "WT2";
    if (name.equalsIgnoreCase("FX Send Delay"))
        return "Send D";
    if (name.equalsIgnoreCase("FX Send Reverb"))
        return "Send R";
    if (name.equalsIgnoreCase("Motion"))
        return "Move";
    if (name.length() > 7)
        return name.substring(0, 7);

    return name;
}
}
