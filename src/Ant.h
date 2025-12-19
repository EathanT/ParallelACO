#pragma once

#include <cstddef>
#include <vector>

#ifndef ENABLE_PARALLEL
#define ENABLE_PARALLEL 1
#endif

// 0 => let OpenMP decide thread count
#ifndef HEAT_NUM_THREADS
#define HEAT_NUM_THREADS 2
#endif

// Simple 2D grid of floats, row-major.
struct HeatGrid {
    std::size_t width{};
    std::size_t height{};
    std::vector<float> data;

    HeatGrid() = default;

    HeatGrid(std::size_t w, std::size_t h)
        : width(w), height(h), data(w* h, 0.0f) {
    }

    void resize(std::size_t w, std::size_t h) {
        width = w;
        height = h;
        data.assign(w * h, 0.0f);
    }

    float& operator()(std::size_t x, std::size_t y) {
        return data[y * width + x];
    }

    const float& operator()(std::size_t x, std::size_t y) const {
        return data[y * width + x];
    }
};
