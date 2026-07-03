#include "HouseLayerRackDisplay.h"

#include <algorithm>
#include <cmath>

namespace UI
{
namespace
{
constexpr auto activeThreshold = 0.04f;

juce::String levelText(float level)
{
    return juce::String(juce::roundToInt(juce::jlimit(0.0f, 1.0f, level) * 100.0f)) + "%";
}

bool useCompactLayerRack(juce::Rectangle<float> bounds)
{
    return bounds.getHeight() < 58.0f;
}

bool useVerticalLayerRack(juce::Rectangle<float> bounds)
{
    return bounds.getWidth() < 520.0f && bounds.getHeight() >= 220.0f;
}
}

HouseLayerRackDisplay::HouseLayerRackDisplay()
{
    setComponentID("HouseLayerRackDisplay");
    tooltipText = "House layer rack: sub, body, character, transient, and chop/source layers";
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void HouseLayerRackDisplay::setTheme(const Theme& newTheme)
{
    if (theme.id == newTheme.id)
        return;

    theme = newTheme;
    repaint();
}

void HouseLayerRackDisplay::setState(State newState)
{
    auto changed = state.summary != newState.summary;
    for (size_t index = 0; index < layerCount; ++index)
        changed = changed || ! layersEqual(state.layers[index], newState.layers[index]);

    if (! changed)
        return;

    state = std::move(newState);

    juce::StringArray tooltipLines;
    tooltipLines.add(state.summary.isNotEmpty() ? state.summary : juce::String("House layer rack"));
    for (const auto& layer : state.layers)
    {
        auto line = layer.role + ": " + layer.source + " " + levelText(layer.level);
        if (layer.hint.isNotEmpty())
            line << " - " << layer.hint;
        tooltipLines.add(line);
    }
    tooltipText = tooltipLines.joinIntoString("\n");

    repaint();
}

HouseLayerRackDisplay::LayoutMetrics HouseLayerRackDisplay::getLayoutMetricsForAudit() const
{
    LayoutMetrics metrics;
    const auto cards = layerBoundsForArea(getLocalBounds().toFloat().reduced(1.0f));
    metrics.visibleCards = static_cast<int>(std::count_if(cards.begin(),
                                                          cards.end(),
                                                          [] (const auto& card)
                                                          {
                                                              return card.getWidth() > 0.0f && card.getHeight() > 0.0f;
                                                          }));
    if (metrics.visibleCards <= 0)
        return metrics;

    metrics.minCardWidth = cards.front().getWidth();
    metrics.minCardHeight = cards.front().getHeight();
    for (const auto& card : cards)
    {
        metrics.minCardWidth = std::min(metrics.minCardWidth, card.getWidth());
        metrics.minCardHeight = std::min(metrics.minCardHeight, card.getHeight());
    }
    metrics.readable = metrics.visibleCards == static_cast<int>(layerCount)
        && metrics.minCardWidth >= 30.0f
        && metrics.minCardHeight >= 22.0f;
    return metrics;
}

juce::String HouseLayerRackDisplay::getTooltip()
{
    return tooltipText;
}

void HouseLayerRackDisplay::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    auto inner = bounds.reduced(7.0f, 6.0f);
    const auto compact = useCompactLayerRack(bounds);
    const auto activeCount = std::count_if(state.layers.begin(),
                                           state.layers.end(),
                                           [] (const Layer& layer) { return layer.active && layer.level > activeThreshold; });

    g.setColour(theme.panelRaised);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(theme.outline);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    if (! compact)
    {
        auto header = inner.removeFromTop(14.0f);
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.setColour(theme.accent);
        g.drawFittedText("HOUSE LAYERS",
                         header.toNearestInt(),
                         juce::Justification::centredLeft,
                         1,
                         0.72f);

        g.setColour(theme.textDim);
        g.drawFittedText(juce::String(activeCount) + "/" + juce::String(static_cast<int>(layerCount)) + " ON",
                         header.toNearestInt(),
                         juce::Justification::centredRight,
                         1,
                         0.72f);
    }

    const auto cards = layerBoundsForArea(bounds);

    for (size_t index = 0; index < layerCount; ++index)
    {
        auto card = cards[index];
        const auto& layer = state.layers[index];
        const auto level = juce::jlimit(0.0f, 1.0f, layer.level);
        const auto active = layer.active && level > activeThreshold;
        const auto accent = colourForLayer(theme, layer, index);
        const auto hovered = hoveredLayerIndex == static_cast<int>(index);

        g.setColour(active ? accent.withAlpha(hovered ? 0.28f : 0.18f) : theme.panelAlt);
        g.fillRoundedRectangle(card, 4.0f);
        g.setColour(hovered ? accent : (active ? accent.withAlpha(0.72f) : theme.outline));
        g.drawRoundedRectangle(card, 4.0f, hovered ? 1.5f : (active ? 1.2f : 1.0f));

        const auto meterHeight = juce::jmax(2.0f, card.getHeight() * level);
        auto meter = card.withY(card.getBottom() - meterHeight).withHeight(meterHeight).reduced(2.0f, 1.5f);
        g.setColour(active ? accent.withAlpha(0.50f) : theme.outline.withAlpha(0.35f));
        g.fillRoundedRectangle(meter, 2.5f);

        auto text = card.reduced(3.0f, 3.0f);
        g.setFont(juce::FontOptions(8.6f, juce::Font::bold));
        g.setColour(active ? theme.text : theme.textDim);
        g.drawFittedText(layer.role,
                         text.removeFromTop(11.0f).toNearestInt(),
                         juce::Justification::centred,
                         1,
                         0.58f);

        if (! compact || card.getHeight() >= 30.0f)
        {
            g.setFont(juce::FontOptions(compact ? 7.2f : 7.8f));
            g.setColour(active ? theme.textMuted : theme.textDim);
            g.drawFittedText(layer.source,
                             text.removeFromTop(compact ? 8.0f : 10.0f).toNearestInt(),
                             juce::Justification::centred,
                             1,
                             0.50f);
        }

        g.setFont(juce::FontOptions(compact ? 7.0f : 7.6f, juce::Font::bold));
        g.setColour(active ? accent : theme.textDim);
        g.drawFittedText(levelText(level),
                         text.removeFromBottom(9.0f).toNearestInt(),
                         juce::Justification::centred,
                         1,
                         0.58f);

        if (layer.lowSafe || layer.wide)
        {
            auto badge = card.reduced(3.0f, 3.0f).removeFromBottom(8.0f);
            badge.setHeight(7.0f);
            g.setColour((layer.lowSafe ? theme.accentBright : theme.accentSecondary).withAlpha(active ? 0.70f : 0.32f));
            g.drawHorizontalLine(juce::roundToInt(badge.getCentreY()), badge.getX(), badge.getRight());
        }
    }
}

void HouseLayerRackDisplay::mouseMove(const juce::MouseEvent& event)
{
    const auto newHoveredLayer = layerIndexAt(event.position);
    if (newHoveredLayer == hoveredLayerIndex)
        return;

    hoveredLayerIndex = newHoveredLayer;
    repaint();
}

void HouseLayerRackDisplay::mouseExit(const juce::MouseEvent&)
{
    if (hoveredLayerIndex < 0)
        return;

    hoveredLayerIndex = -1;
    repaint();
}

void HouseLayerRackDisplay::mouseDown(const juce::MouseEvent& event)
{
    const auto layerIndex = layerIndexAt(event.position);
    if (layerIndex < 0)
        return;

    editingLayerIndex = layerIndex;
    hoveredLayerIndex = layerIndex;

    if (onLayerSelected)
        onLayerSelected(static_cast<size_t>(layerIndex));

    if (onLayerEditStarted)
        onLayerEditStarted(static_cast<size_t>(layerIndex));

    updateLayerLevelAt(event.position);
}

void HouseLayerRackDisplay::mouseDrag(const juce::MouseEvent& event)
{
    updateLayerLevelAt(event.position);
}

void HouseLayerRackDisplay::mouseUp(const juce::MouseEvent&)
{
    editingLayerIndex = -1;
}

bool HouseLayerRackDisplay::layersEqual(const Layer& left, const Layer& right) noexcept
{
    return left.role == right.role
        && left.source == right.source
        && left.hint == right.hint
        && std::abs(left.level - right.level) < 0.001f
        && left.active == right.active
        && left.lowSafe == right.lowSafe
        && left.wide == right.wide;
}

juce::Colour HouseLayerRackDisplay::colourForLayer(const Theme& theme, const Layer& layer, size_t index)
{
    if (layer.lowSafe)
        return theme.accentBright;

    if (layer.wide)
        return theme.accentSecondary;

    switch (index)
    {
        case 0: return theme.accentBright;
        case 1: return theme.accent;
        case 2: return theme.warning;
        case 3: return theme.accentSecondary;
        case 4: return theme.accentSecondary.brighter(0.18f);
        default: return theme.accent;
    }
}

std::array<juce::Rectangle<float>, HouseLayerRackDisplay::layerCount> HouseLayerRackDisplay::layerBoundsForArea(juce::Rectangle<float> bounds) const
{
    std::array<juce::Rectangle<float>, layerCount> cards {};
    auto inner = bounds.reduced(7.0f, 6.0f);
    if (! useCompactLayerRack(bounds))
    {
        inner.removeFromTop(14.0f);
        inner.removeFromTop(4.0f);
    }

    if (useVerticalLayerRack(bounds))
    {
        const auto gap = 4.0f;
        const auto cardHeight = juce::jmax(22.0f, (inner.getHeight() - (gap * static_cast<float>(layerCount - 1))) / static_cast<float>(layerCount));
        auto cardArea = inner;

        for (size_t index = 0; index < layerCount; ++index)
        {
            cards[index] = cardArea.removeFromTop(cardHeight);
            if (index + 1 < layerCount)
                cardArea.removeFromTop(gap);
        }

        return cards;
    }

    const auto gap = 4.0f;
    const auto cardWidth = juce::jmax(30.0f, (inner.getWidth() - (gap * static_cast<float>(layerCount - 1))) / static_cast<float>(layerCount));
    auto cardArea = inner;

    for (size_t index = 0; index < layerCount; ++index)
    {
        cards[index] = cardArea.removeFromLeft(cardWidth);
        if (index + 1 < layerCount)
            cardArea.removeFromLeft(gap);
    }

    return cards;
}

int HouseLayerRackDisplay::layerIndexAt(juce::Point<float> position) const
{
    const auto cards = layerBoundsForArea(getLocalBounds().toFloat().reduced(1.0f));
    for (size_t index = 0; index < cards.size(); ++index)
        if (cards[index].contains(position))
            return static_cast<int>(index);

    return -1;
}

void HouseLayerRackDisplay::updateLayerLevelAt(juce::Point<float> position)
{
    if (editingLayerIndex < 0 || editingLayerIndex >= static_cast<int>(layerCount))
        return;

    const auto cards = layerBoundsForArea(getLocalBounds().toFloat().reduced(1.0f));
    const auto card = cards[static_cast<size_t>(editingLayerIndex)];
    if (card.isEmpty())
        return;

    const auto level = juce::jlimit(0.0f, 1.0f, 1.0f - ((position.y - card.getY()) / juce::jmax(1.0f, card.getHeight())));
    auto& layer = state.layers[static_cast<size_t>(editingLayerIndex)];
    if (std::abs(layer.level - level) < 0.002f)
        return;

    layer.level = level;
    layer.active = level > activeThreshold;

    if (onLayerLevelChanged)
        onLayerLevelChanged(static_cast<size_t>(editingLayerIndex), level);

    repaint();
}
}
