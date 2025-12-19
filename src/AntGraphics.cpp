// AntGraphics.cpp
#include "AntGraphics.h"

#include "ACO.h"
#include "Ant.h"

#include <algorithm>
#include <cstddef>
#include <cmath>

#include "raylib.h"

HeatRaylibRenderer::HeatRaylibRenderer(int windowWidth_,
    int windowHeight_,
    float minTemperature,
    float maxTemperature)
    : windowWidth(windowWidth_),
    windowHeight(windowHeight_),
    minTemp(minTemperature),
    maxTemp(maxTemperature) {
    if (maxTemp <= minTemp) {
        maxTemp = minTemp + 1.0f;
    }
}

void HeatRaylibRenderer::draw(const HeatSimulation& simulation) const {
    const HeatGrid& grid = simulation.grid();

    if (grid.width == 0 || grid.height == 0) {
        return;
    }

    float cellWf = static_cast<float>(windowWidth) / static_cast<float>(grid.width);
    float cellHf = static_cast<float>(windowHeight) / static_cast<float>(grid.height);

    for (std::size_t y = 0; y < grid.height; ++y) {
        for (std::size_t x = 0; x < grid.width; ++x) {
            float t = grid(x, y);
            float clamped = std::max(minTemp, std::min(maxTemp, t));
            float normalized = (clamped - minTemp) / (maxTemp - minTemp);

            // Simple blue -> red gradient
            float n = std::clamp(normalized, 0.0f, 1.0f);
            unsigned char r = static_cast<unsigned char>(255.0f * n);
            unsigned char g = 0;
            unsigned char b = static_cast<unsigned char>(255.0f * (1.0f - n));

            Color color{ r, g, b, 255 };

            int px = static_cast<int>(x * cellWf);
            int py = static_cast<int>(y * cellHf);
            int pw = static_cast<int>(cellWf + 1.0f);
            int ph = static_cast<int>(cellHf + 1.0f);

            DrawRectangle(px, py, pw, ph, color);
        }
    }
}
