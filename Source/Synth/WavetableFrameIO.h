#pragma once

#include "Oscillator.h"

#include <vector>

namespace Synth::WavetableFrameIO
{
using ControlPoints = Oscillator::CustomWavePoints;
using ControlFrameSet = Oscillator::CustomWaveFrames;

ControlFrameSet neutralFrameSet() noexcept;
ControlFrameSet importContiguousSamples(const float* samples, size_t sampleCount) noexcept;
std::vector<float> renderContiguousFrames(const ControlFrameSet& frames, size_t samplesPerFrame);
float maxFrameRange(const ControlFrameSet& frames) noexcept;
}
