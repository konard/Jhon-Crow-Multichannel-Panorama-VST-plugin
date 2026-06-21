#include "DistanceModel.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void expectGainInRange (float distanceM, float minGain, float maxGain)
{
    const float gain = Panorama::distanceToLinearGain (distanceM);

    if (gain < minGain || gain > maxGain)
    {
        std::cerr << "distanceToLinearGain (" << distanceM << ") = " << gain
                  << ", expected in [" << minGain << ", " << maxGain << "]\n";
        std::exit (1);
    }
}

void expectGainApprox (float distanceM, float expected, float toleranceRatio = 0.01f)
{
    const float gain = Panorama::distanceToLinearGain (distanceM);
    const float delta = std::abs (gain - expected);

    if (delta > toleranceRatio * expected + 1e-6f)
    {
        std::cerr << "distanceToLinearGain (" << distanceM << ") = " << gain
                  << ", expected ~" << expected << " (tolerance " << toleranceRatio * 100 << "%)\n";
        std::exit (1);
    }
}

void expectGainLessOrEqual (float distanceM, float maxGain)
{
    const float gain = Panorama::distanceToLinearGain (distanceM);

    if (gain > maxGain + 1e-6f)
    {
        std::cerr << "distanceToLinearGain (" << distanceM << ") = " << gain
                  << ", expected <= " << maxGain << " (gain must not exceed reference)\n";
        std::exit (1);
    }
}

void expectGainDecreasing (float nearDist, float farDist)
{
    const float nearGain = Panorama::distanceToLinearGain (nearDist);
    const float farGain  = Panorama::distanceToLinearGain (farDist);

    if (farGain >= nearGain)
    {
        std::cerr << "distanceToLinearGain (" << farDist << ") = " << farGain
                  << " is not less than distanceToLinearGain (" << nearDist << ") = " << nearGain
                  << " (farther sources must be quieter)\n";
        std::exit (1);
    }
}
}

int main()
{
    // Reference distance (1 m) must yield unity gain (0 dB) so the mix is not clipped
    expectGainApprox (1.0f, 1.0f);

    // Sources closer than reference must NOT exceed unity gain (this was the pre-fix bug:
    // 1/dist returned >1 for dist<1m, causing clipping and making distance changes inaudible)
    expectGainLessOrEqual (0.5f,  1.0f);
    expectGainLessOrEqual (0.1f,  1.0f);
    expectGainLessOrEqual (0.01f, 1.0f);

    // Distance doubling must give -6 dB (gain halves)
    {
        const float g1  = Panorama::distanceToLinearGain (1.0f);
        const float g2  = Panorama::distanceToLinearGain (2.0f);
        const float g4  = Panorama::distanceToLinearGain (4.0f);
        const float g8  = Panorama::distanceToLinearGain (8.0f);
        const float g16 = Panorama::distanceToLinearGain (16.0f);

        const float tol = 0.02f;
        auto approxEqual = [tol] (float a, float b) { return std::abs (a - b) <= tol * b + 1e-6f; };

        if (!approxEqual (g2,  g1  * 0.5f)) { std::cerr << "2m gain not half of 1m\n"; return 1; }
        if (!approxEqual (g4,  g2  * 0.5f)) { std::cerr << "4m gain not half of 2m\n"; return 1; }
        if (!approxEqual (g8,  g4  * 0.5f)) { std::cerr << "8m gain not half of 4m\n"; return 1; }
        if (!approxEqual (g16, g8  * 0.5f)) { std::cerr << "16m gain not half of 8m\n"; return 1; }
    }

    // Moving farther must always reduce gain (issue #9: distance change must be perceptible)
    expectGainDecreasing (1.0f, 2.0f);
    expectGainDecreasing (2.0f, 5.0f);
    expectGainDecreasing (5.0f, 10.0f);
    expectGainDecreasing (10.0f, 20.0f);
    expectGainDecreasing (20.0f, 30.0f);

    // At maximum scene distance (30 m) gain must be well below unity — distance clearly audible
    // 1/30 ≈ 0.033, roughly -29.5 dB — clearly audible as "far away"
    expectGainInRange (30.0f, 0.01f, 0.05f);

    // The gain at 30 m must be significantly lower than at 2 m (default source Z)
    // This ensures the user can clearly distinguish near from far
    {
        const float gainNear = Panorama::distanceToLinearGain (2.0f);
        const float gainFar  = Panorama::distanceToLinearGain (30.0f);
        const float ratio    = gainNear / gainFar;

        // Ratio must be at least 10 (20 dB difference) to be perceptually clear
        if (ratio < 10.0f)
        {
            std::cerr << "near/far gain ratio = " << ratio
                      << " (gain at 2m=" << gainNear << ", at 30m=" << gainFar
                      << "); expected at least 10x (20 dB) difference for perceptible distance\n";
            return 1;
        }
    }

    return 0;
}
