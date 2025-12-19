#include "test.h"
#include "ACO.h"

#include <cmath>
#include <cstddef>
#include <iostream>

bool runAllTests() {
    const std::size_t width = 128;
    const std::size_t height = 128;
    const float alpha = 0.2f;
    const int steps = 200;

    HeatSimulation base(width, height, alpha);
    base.initializeHotSpot(width / 2.0f,
        height / 2.0f,
        width / 6.0f,
        100.0f);

    HeatSimulation seq = base;
    HeatSimulation par = base;

    for (int i = 0; i < steps; ++i) {
        seq.stepSequential();
    }
    for (int i = 0; i < steps; ++i) {
        par.stepParallel();
    }

    const HeatGrid& g1 = seq.grid();
    const HeatGrid& g2 = par.grid();

    if (g1.width != g2.width || g1.height != g2.height) {
        std::cerr << "Test failed: grid sizes differ.\n";
        return false;
    }

    float maxDiff = 0.0f;
    for (std::size_t i = 0; i < g1.data.size(); ++i) {
        float diff = std::fabs(g1.data[i] - g2.data[i]);
        if (diff > maxDiff) {
            maxDiff = diff;
        }
    }

    const float tolerance = 1e-4f;
    bool ok = maxDiff <= tolerance;

    if (!ok) {
        std::cerr << "Test failed: max difference = " << maxDiff
            << " (tolerance " << tolerance << ")\n";
    }
    else {
        std::cout << "Test passed: max difference = " << maxDiff << '\n';
    }

    return ok;
}
