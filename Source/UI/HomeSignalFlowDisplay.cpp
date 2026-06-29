#include "HomeSignalFlowDisplay.h"

namespace UI
{
float HomeSignalFlowDisplay::clamp01(float value) noexcept
{
    return juce::jlimit(0.0f, 1.0f, value);
}

juce::Colour HomeSignalFlowDisplay::nodeColour(size_t index, bool active)
{
    static constexpr std::array<juce::uint32, 6> colours {
        0xff8ee6c9,
        0xffffc36b,
        0xff7bb7ff,
        0xffff8f78,
        0xffd7e37b,
        0xffc4a7ff
    };

    const auto colour = juce::Colour(colours[index % colours.size()]);
    return active ? colour : juce::Colour(0xff5e6b70);
}

void HomeSignalFlowDisplay::setState(const State& newState)
{
    state = newState;
    for (auto& node : state.nodes)
        node.amount = clamp01(node.amount);

    repaint();
}

void HomeSignalFlowDisplay::drawNode(juce::Graphics& g,
                                     juce::Rectangle<float> area,
                                     const Node& node,
                                     juce::Colour colour)
{
    const auto fill = node.active ? juce::Colour(0xff162126) : juce::Colour(0xff11181b);
    g.setColour(fill);
    g.fillRoundedRectangle(area, 4.0f);
    g.setColour(colour.withAlpha(node.active ? 0.74f : 0.38f));
    g.drawRoundedRectangle(area, 4.0f, node.active ? 1.2f : 0.8f);

    auto content = area.reduced(6.0f, 3.0f);
    auto title = content.removeFromTop(10.0f);
    g.setFont(juce::FontOptions(7.8f, juce::Font::bold));
    g.setColour(colour.withAlpha(node.active ? 1.0f : 0.72f));
    g.drawFittedText(node.label, title.toNearestInt(), juce::Justification::centred, 1, 0.54f);

    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.setColour(node.active ? juce::Colour(0xffedf7f4) : juce::Colour(0xff879499));
    g.drawFittedText(node.detail, content.toNearestInt(), juce::Justification::centred, 1, 0.5f);

    auto meter = area.reduced(5.0f, 2.0f).removeFromBottom(2.0f);
    g.setColour(juce::Colour(0xff263137));
    g.fillRoundedRectangle(meter, 1.0f);
    if (node.amount > 0.002f)
    {
        g.setColour(colour.withAlpha(node.active ? 0.9f : 0.34f));
        g.fillRoundedRectangle(meter.withWidth(meter.getWidth() * clamp01(node.amount)), 1.0f);
    }
}

void HomeSignalFlowDisplay::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);

    g.setColour(juce::Colour(0xff0b1113));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour(0xff29363c));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    auto content = bounds.reduced(8.0f, 5.0f);
    const auto arrowWidth = 9.0f;
    const auto gapWidth = 3.0f;
    const auto totalArrowWidth = arrowWidth * static_cast<float>(state.nodes.size() - 1);
    const auto totalGapWidth = gapWidth * static_cast<float>((state.nodes.size() - 1) * 2);
    const auto nodeWidth = juce::jmax(32.0f,
                                      (content.getWidth() - totalArrowWidth - totalGapWidth)
                                          / static_cast<float>(state.nodes.size()));

    for (size_t index = 0; index < state.nodes.size(); ++index)
    {
        auto nodeArea = content.removeFromLeft(nodeWidth);
        drawNode(g, nodeArea, state.nodes[index], nodeColour(index, state.nodes[index].active));

        if (index == state.nodes.size() - 1)
            break;

        content.removeFromLeft(gapWidth);
        auto arrowArea = content.removeFromLeft(arrowWidth);
        content.removeFromLeft(gapWidth);
        const auto start = juce::Point<float> { arrowArea.getX(), arrowArea.getCentreY() };
        const auto end = juce::Point<float> { arrowArea.getRight(), arrowArea.getCentreY() };
        const auto leftActive = state.nodes[index].active;
        const auto rightActive = state.nodes[index + 1].active;
        const auto arrowColour = (leftActive || rightActive)
            ? nodeColour(index + 1, true).withAlpha(0.74f)
            : juce::Colour(0xff38444a);

        g.setColour(arrowColour);
        g.drawLine(start.x, start.y, end.x - 3.0f, end.y, 1.2f);

        juce::Path head;
        head.startNewSubPath(end.x - 3.0f, end.y - 3.0f);
        head.lineTo(end.x, end.y);
        head.lineTo(end.x - 3.0f, end.y + 3.0f);
        g.strokePath(head, juce::PathStrokeType(1.2f));
    }
}
}
