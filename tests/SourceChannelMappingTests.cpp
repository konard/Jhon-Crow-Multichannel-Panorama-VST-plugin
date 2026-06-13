#include "SourceChannelMapping.h"

#include <cstdlib>
#include <iostream>

namespace
{
void expectMapping (int sourceIndex, int numInputChannels, int expected)
{
    const auto actual = Panorama::sourceInputChannelFor (sourceIndex, numInputChannels);

    if (actual != expected)
    {
        std::cerr << "sourceInputChannelFor (" << sourceIndex << ", "
                  << numInputChannels << ") returned " << actual
                  << ", expected " << expected << '\n';
        std::exit (1);
    }
}
}

int main()
{
    expectMapping (-1, 4, Panorama::noInputChannel);
    expectMapping (0, 0, Panorama::noInputChannel);
    expectMapping (0, 1, 0);
    expectMapping (1, 2, 1);
    expectMapping (3, 4, 3);

    // This reproduces the PR feedback: an active source slot without a
    // matching host input must not fall back to input channel 0.
    expectMapping (4, 4, Panorama::noInputChannel);
    expectMapping (7, 4, Panorama::noInputChannel);

    int routedSources = 0;
    for (int sourceIndex = 0; sourceIndex < 8; ++sourceIndex)
        if (Panorama::sourceInputChannelFor (sourceIndex, 4) != Panorama::noInputChannel)
            ++routedSources;

    if (routedSources != 4)
    {
        std::cerr << "expected exactly 4 routed sources for a 4-channel input, got "
                  << routedSources << '\n';
        return 1;
    }

    return 0;
}
