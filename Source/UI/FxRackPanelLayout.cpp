#include "FxRackPanelLayout.h"

namespace UI::FxRackPanelLayout
{
namespace
{
enum ModuleIndex
{
    tone = 0,
    eq,
    distortion,
    bitcrush,
    pump,
    tremolo,
    ring,
    comb,
    phaser,
    flanger,
    chorus,
    delay,
    reverb,
    width,
    guard
};

void setSliderVisible(SliderSlot slot, bool shouldBeVisible)
{
    slot.slider.setVisible(shouldBeVisible);
    slot.label.setVisible(shouldBeVisible);
}

void layoutKnobRow(juce::Rectangle<int> area, std::initializer_list<SliderSlot> slots)
{
    const auto count = static_cast<int>(slots.size());
    if (count == 0)
        return;

    const auto cellWidth = juce::jmax(1, area.getWidth() / count);
    const auto horizontalPadding = cellWidth < 64 ? 2 : 4;

    for (auto slot : slots)
    {
        setSliderVisible(slot, true);
        slot.slider.setBounds(area.removeFromLeft(cellWidth).reduced(horizontalPadding, 0));
    }
}

void layoutRackSlots(juce::Rectangle<int> rackArea, const std::array<RackSlot, moduleCount>& rackSlots)
{
    std::array<FxRackRow*, moduleCount> visibleFxSlots {};
    auto visibleFxSlotCount = 0;

    for (const auto& rackSlot : rackSlots)
    {
        if (rackSlot.row == nullptr)
            continue;

        rackSlot.row->setVisible(rackSlot.visible);
        if (rackSlot.visible)
            visibleFxSlots[static_cast<size_t>(visibleFxSlotCount++)] = rackSlot.row;
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
}
}

void layout(juce::Rectangle<int> content, Components components)
{
    components.sectionLabel.setVisible(true);
    components.sectionLabel.setBounds(content.removeFromTop(28));

    auto actionRow = content.removeFromTop(44);
    components.addBox.setVisible(true);
    components.orderControls.setVisible(true);
    components.performanceControls.setVisible(true);
    components.statusLabel.setVisible(true);
    components.presetBox.setVisible(true);
    components.applyPresetButton.setVisible(true);
    components.hostSyncStatusLabel.setVisible(true);
    components.addBox.setBounds(actionRow.removeFromLeft(160).reduced(4));
    components.orderControls.setBounds(actionRow.removeFromLeft(268));
    components.hostSyncStatusLabel.setBounds(actionRow.removeFromRight(126).reduced(4));
    components.statusLabel.setBounds(actionRow.reduced(8, 4));

    auto performRow = content.removeFromTop(42).withTrimmedTop(2);
    components.performanceControls.setBounds(performRow);

    content.removeFromTop(8);
    auto rackArea = content.removeFromLeft(260).reduced(18, 14);
    rackArea.removeFromTop(26);
    auto detailArea = content.reduced(24, 16);
    detailArea.removeFromTop(30);

    layoutRackSlots(rackArea, components.rackSlots);

    auto detailHeader = detailArea.removeFromTop(38);
    components.applyPresetButton.setBounds(detailHeader.removeFromRight(62).reduced(3, 4));
    components.presetBox.setBounds(detailHeader.removeFromRight(156).reduced(3, 4));
    auto controlsArea = detailArea.withTrimmedTop(16);

    switch (components.selectedModuleIndex)
    {
        case tone:
            components.tone.enabledButton.setVisible(true);
            components.tone.enabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
            layoutKnobRow(controlsArea.removeFromTop(150), { components.tone.tilt, components.tone.lowCut });
            break;

        case eq:
            components.eq.enabledButton.setVisible(true);
            components.eq.enabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
            layoutKnobRow(controlsArea.removeFromTop(150), {
                components.eq.lowGain,
                components.eq.midGain,
                components.eq.highGain,
                components.eq.trim
            });
            break;

        case distortion:
            components.distortion.enabledButton.setVisible(true);
            components.distortion.modeBox.setVisible(true);
            components.distortion.enabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
            components.distortion.modeBox.setBounds(detailHeader.removeFromLeft(132).reduced(3, 4));
            layoutKnobRow(controlsArea.removeFromTop(74), {
                components.distortion.amount,
                components.distortion.bassSafe,
                components.distortion.mix
            });
            layoutKnobRow(controlsArea.removeFromTop(74), {
                components.distortion.lowBand,
                components.distortion.midBand,
                components.distortion.highBand
            });
            break;

        case bitcrush:
            components.bitcrush.enabledButton.setVisible(true);
            components.bitcrush.enabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
            layoutKnobRow(controlsArea.removeFromTop(150), {
                components.bitcrush.bits,
                components.bitcrush.downsample,
                components.bitcrush.mix
            });
            break;

        case pump:
            components.pump.enabledButton.setVisible(true);
            components.pump.rateBox.setVisible(true);
            components.pump.curveBox.setVisible(true);
            components.pump.curveDisplay.setVisible(true);
            components.pump.enabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
            components.pump.rateBox.setBounds(detailHeader.removeFromLeft(96).reduced(3, 4));
            components.pump.curveBox.setBounds(detailHeader.removeFromLeft(118).reduced(3, 4));
            components.pump.curveDisplay.setBounds(controlsArea.removeFromTop(86).reduced(4, 2));
            layoutKnobRow(controlsArea.removeFromTop(128).withTrimmedTop(8), {
                components.pump.depth,
                components.pump.shape,
                components.pump.phase
            });
            break;

        case tremolo:
            components.tremolo.enabledButton.setVisible(true);
            components.tremolo.rateBox.setVisible(true);
            components.tremolo.enabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
            components.tremolo.rateBox.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
            layoutKnobRow(controlsArea.removeFromTop(150), {
                components.tremolo.depth,
                components.tremolo.pan,
                components.tremolo.shape,
                components.tremolo.phase
            });
            break;

        case ring:
            components.ring.enabledButton.setVisible(true);
            components.ring.enabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
            layoutKnobRow(controlsArea.removeFromTop(150), {
                components.ring.frequency,
                components.ring.depth,
                components.ring.mix,
                components.ring.bias
            });
            break;

        case comb:
            components.comb.enabledButton.setVisible(true);
            components.comb.enabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
            layoutKnobRow(controlsArea.removeFromTop(150), {
                components.comb.frequency,
                components.comb.feedback,
                components.comb.damping,
                components.comb.mix
            });
            break;

        case phaser:
            components.phaser.enabledButton.setVisible(true);
            components.phaser.enabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
            layoutKnobRow(controlsArea.removeFromTop(150), {
                components.phaser.rate,
                components.phaser.depth,
                components.phaser.mix
            });
            break;

        case flanger:
            components.flanger.enabledButton.setVisible(true);
            components.flanger.enabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
            layoutKnobRow(controlsArea.removeFromTop(150), {
                components.flanger.rate,
                components.flanger.depth,
                components.flanger.feedback,
                components.flanger.mix
            });
            break;

        case chorus:
            components.chorus.enabledButton.setVisible(true);
            components.chorus.enabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
            layoutKnobRow(controlsArea.removeFromTop(150), {
                components.chorus.rate,
                components.chorus.depth,
                components.chorus.mix
            });
            break;

        case delay:
            components.delay.enabledButton.setVisible(true);
            components.delay.syncButton.setVisible(true);
            components.delay.rateBox.setVisible(true);
            components.delay.enabledButton.setBounds(detailHeader.removeFromLeft(96).reduced(3, 4));
            components.delay.syncButton.setBounds(detailHeader.removeFromLeft(72).reduced(3, 4));
            components.delay.rateBox.setBounds(detailHeader.removeFromLeft(104).reduced(3, 4));
            layoutKnobRow(controlsArea.removeFromTop(150), {
                components.delay.time,
                components.delay.feedback,
                components.delay.mix,
                components.delay.send
            });
            break;

        case reverb:
            components.reverb.enabledButton.setVisible(true);
            components.reverb.enabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
            layoutKnobRow(controlsArea.removeFromTop(150), {
                components.reverb.size,
                components.reverb.damping,
                components.reverb.mix,
                components.reverb.send
            });
            break;

        case width:
            components.width.enabledButton.setVisible(true);
            components.width.enabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
            layoutKnobRow(controlsArea.removeFromTop(150), {
                components.width.amount,
                components.width.monoCutoff
            });
            break;

        case guard:
        default:
            components.guard.enabledButton.setVisible(true);
            components.guard.enabledButton.setBounds(detailHeader.removeFromLeft(112).reduced(3, 4));
            layoutKnobRow(controlsArea.removeFromTop(150), {
                components.guard.push,
                components.guard.glue,
                components.guard.punch,
                components.guard.clipMix,
                components.guard.ceiling
            });
            break;
    }
}
}
