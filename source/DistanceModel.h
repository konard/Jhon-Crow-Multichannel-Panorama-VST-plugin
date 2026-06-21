#pragma once

#include <algorithm>
#include <cmath>

namespace Panorama
{

// Compute gain (linear) from distance using the inverse distance law.
// Reference distance: 1 m gives 0 dB gain (linear 1.0).
// Sources closer than 1 m are held at unity gain to prevent clipping.
// Gain = refDist/dist, giving ~30 dB range across 1–30 m (6 dB per doubling).
inline float distanceToLinearGain (float distanceMetres) noexcept
{
    constexpr float refDist = 1.0f;
    const float dist = std::max (distanceMetres, refDist);
    return refDist / dist;
}

} // namespace Panorama
