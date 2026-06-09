#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>

namespace Panorama
{

static constexpr int maxSources = 8;

struct Vec3
{
    float x = 0.0f, y = 0.0f, z = 0.0f;

    float distanceTo (const Vec3& other) const noexcept
    {
        const float dx = x - other.x;
        const float dy = y - other.y;
        const float dz = z - other.z;
        return std::sqrt (dx * dx + dy * dy + dz * dz);
    }

    Vec3 relativeTo (const Vec3& origin) const noexcept
    {
        return { x - origin.x, y - origin.y, z - origin.z };
    }
};

struct SourcePose
{
    Vec3 position;
    float gainDb = 0.0f;
    bool active = false;
    juce::String label;
};

struct ListenerPose
{
    Vec3 position;
};

// Thread-safe atomic transfer of a source position for the audio thread
struct AtomicSourceData
{
    std::atomic<float> x { 0.0f };
    std::atomic<float> y { 0.0f };
    std::atomic<float> z { 0.0f };
    std::atomic<float> gainDb { 0.0f };
    std::atomic<bool> active { false };
};

struct AtomicListenerData
{
    std::atomic<float> x { 0.0f };
    std::atomic<float> y { 0.0f };
    std::atomic<float> z { 0.0f };
};

struct SceneAtomics
{
    AtomicListenerData listener;
    std::array<AtomicSourceData, maxSources> sources;
};

// Compute gain (linear) from distance with an inverse-square roll-off.
// Reference distance: 1 m gives 0 dB gain (linear 1.0).
inline float distanceToLinearGain (float distanceMetres) noexcept
{
    const float dist = std::max (distanceMetres, 0.01f);
    return 1.0f / dist;
}

// Compute left/right gain from a relative XZ angle.
// relX: source X relative to listener (positive = right)
// relZ: source Z relative to listener (positive = front)
// Returns { leftGain, rightGain } linear amplitudes.
inline std::pair<float, float> panFromRelative (float relX, float relZ) noexcept
{
    const float angle = std::atan2 (relX, std::abs (relZ) + 0.001f);
    const float panNorm = juce::jlimit (-1.0f, 1.0f, angle / juce::MathConstants<float>::halfPi);

    // Constant-power panning
    const float panAngle = (panNorm + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
    const float leftGain  = std::cos (panAngle);
    const float rightGain = std::sin (panAngle);
    return { leftGain, rightGain };
}

} // namespace Panorama
