#pragma once

namespace Panorama
{

inline constexpr int noInputChannel = -1;

inline int sourceInputChannelFor (int sourceIndex, int numInputChannels) noexcept
{
    if (sourceIndex < 0 || numInputChannels <= 0 || sourceIndex >= numInputChannels)
        return noInputChannel;

    return sourceIndex;
}

} // namespace Panorama
