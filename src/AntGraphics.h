#pragma once

#include "ACO.h"

struct Color;

// Renders the heat grid using raylib.
class HeatRaylibRenderer {
public:
    HeatRaylibRenderer(int windowWidth,
        int windowHeight,
        float minTemperature,
        float maxTemperature);

    // Draws the current grid into the active raylib render target.
    void draw(const HeatSimulation& simulation) const;

private:
    int windowWidth;
    int windowHeight;
    float minTemp;
    float maxTemp;
};
