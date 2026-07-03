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
float meanAbsoluteDifference(const ControlPoints& first, const ControlPoints& second) noexcept;
}
