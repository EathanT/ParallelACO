#include "ACO.h"
#include "AntGraphics.h"
#include "Ant.h"

#include "raylib.h"

int main() {
    // Window configuration
    const int windowWidth = 1000;
    const int windowHeight = 800;

    InitWindow(windowWidth, windowHeight, "2D Heat Diffusion - raylib");
    SetTargetFPS(60);

    // Simulation configuration
    const std::size_t gridWidth = 200;
    const std::size_t gridHeight = 160;
    const float alpha = 0.24f;

    HeatSimulation simulation(gridWidth, gridHeight, alpha);
    simulation.initializeHotSpot(gridWidth / 2.0f,
        gridHeight / 2.0f,
        gridWidth / 5.0f,
        100.0f);

    HeatRaylibRenderer renderer(windowWidth, windowHeight, 0.0f, 100.0f);

    int stepsPerFrame = 1; // increase for faster diffusion

    while (!WindowShouldClose()) {
        // Update
        for (int i = 0; i < stepsPerFrame; ++i) {
            simulation.stepParallel();
        }

        // Draw
        BeginDrawing();
        ClearBackground(BLACK);

        renderer.draw(simulation);

        DrawText("2D Heat Diffusion (raylib, parallel)", 10, 10, 20, RAYWHITE);
        DrawText("ESC to quit", 10, 35, 16, RAYWHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
