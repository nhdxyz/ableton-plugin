#pragma once

#include "Oscillator.h"

namespace Synth::WavetableFrameRecipes
{
using ControlPoints = Oscillator::CustomWavePoints;
using ControlFrameSet = Oscillator::CustomWaveFrames;

ControlPoints normalise(ControlPoints points) noexcept;
ControlFrameSet currentSweep(const ControlPoints& seed) noexcept;
ControlFrameSet classicHouseStack() noexcept;
ControlFrameSet raveSweep() noexcept;
ControlFrameSet reverseFrameOrder(ControlFrameSet frames) noexcept;
ControlFrameSet rotateFrameOrder(ControlFrameSet frames, int offset) noexcept;
ControlFrameSet smoothFrameMotion(const ControlFrameSet& frames) noexcept;
ControlFrameSet emphasiseFrameMotion(const ControlFrameSet& frames) noexcept;
ControlFrameSet blendFrameStacks(const ControlFrameSet& first, const ControlFrameSet& second, float amount) noexcept;
ControlFrameSet morphBetweenFrameStacks(const ControlFrameSet& first, const ControlFrameSet& second) noexcept;
ControlFrameSet spliceFrameStacks(const ControlFrameSet& first, const ControlFrameSet& second) noexcept;
float meanAbsoluteDifference(const ControlPoints& first, const ControlPoints& second) noexcept;
}
