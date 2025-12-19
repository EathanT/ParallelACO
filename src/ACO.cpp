#include "ACO.h"

#include <algorithm>
#include <cmath>
#include <random>

#if ENABLE_PARALLEL && defined(_OPENMP)
#include <omp.h>
#endif

HeatSimulation::HeatSimulation(std::size_t width,
    std::size_t height,
    float diffusionCoefficient)
    : current(width, height),
    next(width, height),
    alpha(diffusionCoefficient) {
}

void HeatSimulation::initializeHotSpot(float cx,
    float cy,
    float radius,
    float temperature) {
    const float r2 = radius * radius;

    for (std::size_t y = 0; y < current.height; ++y) {
        for (std::size_t x = 0; x < current.width; ++x) {
            float dx = static_cast<float>(x) - cx;
            float dy = static_cast<float>(y) - cy;
            float dist2 = dx * dx + dy * dy;
            current(x, y) = (dist2 <= r2) ? temperature : 0.0f;
        }
    }

    next = current;
}

void HeatSimulation::initializeRandom(float minTemp,
    float maxTemp,
    unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(minTemp, maxTemp);

    for (std::size_t y = 0; y < current.height; ++y) {
        for (std::size_t x = 0; x < current.width; ++x) {
            current(x, y) = dist(rng);
        }
    }

    next = current;
}

// Sequential 5-point stencil.
void HeatSimulation::stepSequential() {
    if (current.width < 2 || current.height < 2) {
        return;
    }

    // Interior points.
    for (std::size_t y = 1; y + 1 < current.height; ++y) {
        for (std::size_t x = 1; x + 1 < current.width; ++x) {
            float center = current(x, y);
            float up = current(x, y - 1);
            float down = current(x, y + 1);
            float left = current(x - 1, y);
            float right = current(x + 1, y);

            next(x, y) = center + alpha * (up + down + left + right - 4.0f * center);
        }
    }

    // Boundaries: copy unchanged.
    for (std::size_t x = 0; x < current.width; ++x) {
        next(x, 0) = current(x, 0);
        next(x, current.height - 1) = current(x, current.height - 1);
    }
    for (std::size_t y = 0; y < current.height; ++y) {
        next(0, y) = current(0, y);
        next(current.width - 1, y) = current(current.width - 1, y);
    }

    std::swap(current.data, next.data);
}

// Parallel version: same math, just parallel over rows.
void HeatSimulation::stepParallel() {
#if ENABLE_PARALLEL && defined(_OPENMP)
    if (current.width < 2 || current.height < 2) {
        return;
    }

#  if HEAT_NUM_THREADS > 0
    omp_set_num_threads(HEAT_NUM_THREADS);
#  endif

#pragma omp parallel for schedule(static)
    for (int y = 1; y < static_cast<int>(current.height) - 1; ++y) {
        for (int x = 1; x < static_cast<int>(current.width) - 1; ++x) {
            std::size_t sx = static_cast<std::size_t>(x);
            std::size_t sy = static_cast<std::size_t>(y);

            float center = current(sx, sy);
            float up = current(sx, sy - 1);
            float down = current(sx, sy + 1);
            float left = current(sx - 1, sy);
            float right = current(sx + 1, sy);

            next(sx, sy) = center + alpha * (up + down + left + right - 4.0f * center);
        }
    }

    // Boundaries sequential; cost is negligible.
    for (std::size_t x = 0; x < current.width; ++x) {
        next(x, 0) = current(x, 0);
        next(x, current.height - 1) = current(x, current.height - 1);
    }
    for (std::size_t y = 0; y < current.height; ++y) {
        next(0, y) = current(0, y);
        next(current.width - 1, y) = current(current.width - 1, y);
    }

    std::swap(current.data, next.data);
#else
    // If OpenMP is not available, fall back to sequential so the code still works.
    stepSequential();
#endif
}
