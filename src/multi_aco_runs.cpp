// multi_aco_runs.cpp
#include <iostream>
#include <random>
#include <memory>
#include <vector>
#include <limits>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "Ant.h"
#include "ACO.h"

int main() {
    const int numRuns = 8;
    const int numAnts = 200;
    const int numberOfCities = 50;
    const int iterations = 200;

    // Generate a base set of cities once
    std::vector<std::shared_ptr<city>> baseCities;
    std::mt19937 cityGen(42);
    std::uniform_real_distribution<float> posDist(0.0f, 1000.0f);

    for (int i = 0; i < numberOfCities; ++i) {
        Vector2 pos{ posDist(cityGen), posDist(cityGen) };
        baseCities.push_back(std::make_shared<city>(i, false, pos));
    }

    // Best route length found in each independent run
    std::vector<float> bestRouteLengths(
        numRuns,
        std::numeric_limits<float>::infinity()
    );

#pragma omp parallel for schedule(dynamic)
    for (int run = 0; run < numRuns; ++run) {
        // Each run gets its own copy of the city list and its own ACO instance
        std::vector<std::shared_ptr<city>> cities = baseCities;

        float Q = 100.0f;
        float evaporationRate = 0.5f;

        // ACO uses the new Ant class which stores city indices
        ACO aco(cities, numAnts, Q, evaporationRate);
        aco.setAlpha(1.0f);
        aco.setBeta(5.0f);

        auto& ants = aco.getAnts();

        std::mt19937 gen(1000 + run);
        std::uniform_int_distribution<int> startDist(0, numberOfCities - 1);

        for (int it = 0; it < iterations; ++it) {
            // Construct routes for all ants (sequential inside each run;
            // OpenMP parallelizes across runs)
            for (auto& ant : ants) {
                int start = startDist(gen);
                ant->startAt(start);

                while ((int)ant->route.size() < numberOfCities + 1) {
                    int nextIdx = aco.selectNextCity(ant);
                    ant->moveTo(nextIdx, cities);
                }
            }

            // Use the optimized, more parallel-friendly pheromone update
            aco.updatePheromones();

            // Track best route length for this run
            for (auto& ant : ants) {
                if (ant->routeLength < bestRouteLengths[run]) {
                    bestRouteLengths[run] = ant->routeLength;
                }
                ant->reset();
            }
        }

        // Print result for this independent run
#pragma omp critical
        {
            std::cout << "Run " << run
                << " best tour length: "
                << bestRouteLengths[run] << std::endl;
        }
    }

    std::cout << "Finished " << numRuns << " independent ACO runs.\n";
    return 0;
}
