#pragma once

#include "Ant.h"

// 2D heat diffusion simulation using a simple 5-point stencil.
class HeatSimulation {
public:
    // diffusionCoefficient is the "alpha" in the stencil update.
    HeatSimulation(std::size_t width, std::size_t height, float diffusionCoefficient);

    // Initialize with a circular hot spot.
    void initializeHotSpot(float cx, float cy, float radius, float temperature);

    // Initialize with random temperatures in [minTemp, maxTemp].
    void initializeRandom(float minTemp, float maxTemp, unsigned int seed);

    // One explicit time step (sequential).
    void stepSequential();

    // One explicit time step (OpenMP parallel).
    void stepParallel();

    const HeatGrid& grid() const { return current; }

    std::size_t width() const { return current.width; }
    std::size_t height() const { return current.height; }

private:
    HeatGrid current;
    HeatGrid next;
    float alpha; // diffusion coefficient
};
