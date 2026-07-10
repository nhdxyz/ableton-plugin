#include "RandomCandidateExplorer.h"

#include <cmath>

namespace UI
{
RandomCandidateExplorer::RandomCandidateExplorer()
{
    setComponentID("RandomCandidateExplorer");
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void RandomCandidateExplorer::setTheme(const Theme& newTheme)
{
    if (theme.id == newTheme.id)
        return;

    theme = newTheme;
    repaint();
}

void RandomCandidateExplorer::setState(State newState)
{
    state = std::move(newState);
    repaint();
}

RandomCandidateExplorer::LayoutMetrics RandomCandidateExplorer::getLayoutMetricsForAudit() const
{
    LayoutMetrics metrics;
    const auto cards = cardBounds();
    metrics.minCardWidth = cards.front().getWidth();
    metrics.minCardHeight = cards.front().getHeight();
    for (const auto& card : cards)
    {
        if (! card.isEmpty())
            ++metrics.visibleCards;
        metrics.minCardWidth = juce::jmin(metrics.minCardWidth, card.getWidth());
        metrics.minCardHeight = juce::jmin(metrics.minCardHeight, card.getHeight());
    }
    metrics.readable = metrics.visibleCards == static_cast<int>(candidateCount)
        && metrics.minCardWidth >= 58.0f
        && metrics.minCardHeight >= 38.0f;
    return metrics;
}

juce::String RandomCandidateExplorer::getTooltip()
{
    if (hoveredCandidate < 0)
        return "Candidate branches: click to recall; click Cue for non-destructive audition";

    const auto& candidate = state.candidates[static_cast<size_t>(hoveredCandidate)];
    return candidate.ready
        ? candidate.title + " | " + candidate.traits + " | click to recall or Cue to audition"
        : juce::String("Generate or mutate to fill candidate ") + juce::String(hoveredCandidate + 1);
}

void RandomCandidateExplorer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(theme.background.darker(0.10f));
    g.fillRoundedRectangle(bounds, 5.0f);
    g.setColour(theme.outline);
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

    auto root = bounds.reduced(7.0f, 7.0f).removeFromLeft(42.0f);
    const auto rootPoint = root.getCentre();
    g.setColour(theme.accent.withAlpha(0.16f));
    g.fillEllipse(juce::Rectangle<float>(26.0f, 26.0f).withCentre(rootPoint));
    g.setColour(theme.accent);
    g.drawEllipse(juce::Rectangle<float>(26.0f, 26.0f).withCentre(rootPoint), 1.2f);
    g.setFont(juce::FontOptions(7.0f, juce::Font::bold));
    g.setColour(theme.text);
    g.drawFittedText("SEED", juce::Rectangle<float>(30.0f, 12.0f).withCentre(rootPoint).toNearestInt(), juce::Justification::centred, 1);
    g.setFont(juce::FontOptions(6.4f));
    g.setColour(theme.textDim);
    g.drawFittedText(state.rootName,
                     root.withTrimmedTop(root.getHeight() * 0.72f).toNearestInt(),
                     juce::Justification::centred,
                     1,
                     0.48f);

    const auto cards = cardBounds();
    for (size_t index = 0; index < cards.size(); ++index)
    {
        const auto& candidate = state.candidates[index];
        const auto card = cards[index];
        const auto accent = index % 2 == 0 ? theme.accent : theme.accentSecondary;
        const auto hovered = hoveredCandidate == static_cast<int>(index);

        g.setColour((candidate.ready ? accent : theme.textDim).withAlpha(candidate.active ? 0.56f : 0.22f));
        g.drawLine(rootPoint.x + 13.0f, rootPoint.y,
                   card.getX(), card.getCentreY(),
                   candidate.active ? 1.8f : 1.0f);

        g.setColour(candidate.active ? accent.withAlpha(0.22f)
                                     : (hovered ? theme.panelRaised : theme.panelAlt));
        g.fillRoundedRectangle(card, 4.0f);
        g.setColour(candidate.active || hovered ? accent : theme.outline);
        g.drawRoundedRectangle(card, 4.0f, candidate.active ? 1.7f : 1.0f);

        auto content = card.reduced(5.0f, 4.0f);
        auto title = content.removeFromTop(12.0f);
        g.setFont(juce::FontOptions(7.6f, juce::Font::bold));
        g.setColour(candidate.ready ? theme.text : theme.textDim);
        g.drawFittedText(juce::String(static_cast<int>(index + 1)) + " " + candidate.title,
                         title.toNearestInt(), juce::Justification::centredLeft, 1, 0.58f);

        auto fingerprint = content.removeFromTop(juce::jmax(14.0f, content.getHeight() - 13.0f));
        juce::Path wave;
        constexpr auto points = 30;
        for (auto point = 0; point < points; ++point)
        {
            const auto xNorm = static_cast<float>(point) / static_cast<float>(points - 1);
            const auto value = fingerprintValue(candidate.fingerprint, point);
            const auto x = fingerprint.getX() + fingerprint.getWidth() * xNorm;
            const auto y = fingerprint.getCentreY() - value * fingerprint.getHeight() * 0.38f;
            if (point == 0)
                wave.startNewSubPath(x, y);
            else
                wave.lineTo(x, y);
        }
        g.setColour(theme.outline.withAlpha(0.55f));
        g.drawHorizontalLine(juce::roundToInt(fingerprint.getCentreY()), fingerprint.getX(), fingerprint.getRight());
        g.setColour((candidate.ready ? accent : theme.textDim).withAlpha(candidate.ready ? 0.82f : 0.30f));
        g.strokePath(wave, juce::PathStrokeType(1.15f, juce::PathStrokeType::curved));

        auto footer = content;
        const auto cue = cueBounds(index);
        if (candidate.changedSections > 0)
        {
            g.setFont(juce::FontOptions(7.0f, juce::Font::bold));
            g.setColour(theme.warning);
            g.drawFittedText(juce::String(candidate.changedSections) + " CHG",
                             footer.withTrimmedRight(30.0f).toNearestInt(), juce::Justification::centredLeft, 1);
        }

        g.setColour(candidate.cueing ? accent.withAlpha(0.28f) : theme.background.withAlpha(0.72f));
        g.fillRoundedRectangle(cue, 2.5f);
        g.setColour(candidate.ready ? accent : theme.outline);
        g.drawRoundedRectangle(cue, 2.5f, 0.8f);
        g.setFont(juce::FontOptions(6.8f, juce::Font::bold));
        g.setColour(candidate.ready ? theme.text : theme.textDim);
        g.drawFittedText("CUE", cue.toNearestInt(), juce::Justification::centred, 1);
    }
}

void RandomCandidateExplorer::mouseMove(const juce::MouseEvent& event)
{
    const auto next = candidateAt(event.position);
    if (next == hoveredCandidate)
        return;

    hoveredCandidate = next;
    repaint();
}

void RandomCandidateExplorer::mouseExit(const juce::MouseEvent&)
{
    hoveredCandidate = -1;
    repaint();
}

void RandomCandidateExplorer::mouseDown(const juce::MouseEvent& event)
{
    const auto index = candidateAt(event.position);
    if (index < 0 || ! state.candidates[static_cast<size_t>(index)].ready)
        return;

    if (cueBounds(static_cast<size_t>(index)).contains(event.position))
    {
        if (onCue)
            onCue(static_cast<size_t>(index));
    }
    else if (onRecall)
    {
        onRecall(static_cast<size_t>(index));
    }
}

std::array<juce::Rectangle<float>, RandomCandidateExplorer::candidateCount>
RandomCandidateExplorer::cardBounds() const
{
    auto area = getLocalBounds().toFloat().reduced(7.0f, 6.0f);
    area.removeFromLeft(48.0f);
    const auto gap = 5.0f;
    const auto width = (area.getWidth() - gap * 3.0f) / 4.0f;
    std::array<juce::Rectangle<float>, candidateCount> cards;
    for (size_t index = 0; index < cards.size(); ++index)
    {
        cards[index] = area.removeFromLeft(width);
        if (index + 1 < cards.size())
            area.removeFromLeft(gap);
    }
    return cards;
}

juce::Rectangle<float> RandomCandidateExplorer::cueBounds(size_t index) const
{
    if (index >= candidateCount)
        return {};
    return cardBounds()[index].reduced(4.0f, 3.0f).removeFromBottom(11.0f).removeFromRight(27.0f);
}

int RandomCandidateExplorer::candidateAt(juce::Point<float> point) const
{
    const auto cards = cardBounds();
    for (size_t index = 0; index < cards.size(); ++index)
        if (cards[index].contains(point))
            return static_cast<int>(index);
    return -1;
}

float RandomCandidateExplorer::fingerprintValue(juce::int64 seed, int index) noexcept
{
    const auto phaseA = static_cast<float>((seed & 0xffff) + 31) * 0.00019f;
    const auto phaseB = static_cast<float>(((seed >> 16) & 0xffff) + 17) * 0.00013f;
    const auto x = static_cast<float>(index) / 29.0f;
    return juce::jlimit(-1.0f, 1.0f,
                        std::sin((x * 2.0f + phaseA) * juce::MathConstants<float>::twoPi) * 0.62f
                            + std::sin((x * 5.0f + phaseB) * juce::MathConstants<float>::twoPi) * 0.26f);
}
}
