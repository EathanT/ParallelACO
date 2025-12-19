#include "ACO.h"
#include "Ant.h"
#include "test.h"

#include <chrono>
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    using clock = std::chrono::high_resolution_clock;

    // Basic configuration
    std::size_t width = 1024;
    std::size_t height = 1024;
    int steps = 500;
    float alpha = 0.24f;

    if (argc >= 4) {
        width = static_cast<std::size_t>(std::strtoul(argv[1], nullptr, 10));
        height = static_cast<std::size_t>(std::strtoul(argv[2], nullptr, 10));
        steps = std::atoi(argv[3]);
    }

    std::cout << "Running correctness test first...\n";
    bool testsOk = runAllTests();
    if (!testsOk) {
        std::cerr << "Warning: parallel results differ from sequential.\n";
    }

    HeatSimulation base(width, height, alpha);
    base.initializeHotSpot(width / 2.0f,
        height / 2.0f,
        width / 6.0f,
        100.0f);

    HeatSimulation seq = base;
    HeatSimulation par = base;

    std::cout << "Grid: " << width << " x " << height
        << ", steps: " << steps << '\n';

    // Sequential timing
    auto t0 = clock::now();
    for (int i = 0; i < steps; ++i) {
        seq.stepSequential();
    }
    auto t1 = clock::now();

    // Parallel timing
    auto t2 = clock::now();
    for (int i = 0; i < steps; ++i) {
        par.stepParallel();
    }
    auto t3 = clock::now();

    auto seqMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    auto parMs = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();

    std::cout << "Sequential time: " << seqMs << " ms\n";
    std::cout << "Parallel time:   " << parMs << " ms\n";

#if ENABLE_PARALLEL && defined(_OPENMP)
    std::cout << "OpenMP is enabled (ENABLE_PARALLEL=" << ENABLE_PARALLEL
        << ", HEAT_NUM_THREADS=" << HEAT_NUM_THREADS << ").\n";
#else
    std::cout << "OpenMP is NOT enabled; parallel path falls back to sequential.\n";
#endif

    return testsOk ? 0 : 1;
}
