// Toggle parallelization:

#include "Ant.h"
#include "ACO.h"
#include "test.h"



int main() {
    int   numAnts = 20;
    int   numberOfCities = 30;
    int   iterations = 50;
    float Q = 100.0f;
    float evaporationRate = 0.5f;
    float alpha = 1.0f;
    float beta = 5.0f;

    // Generate random cities
    std::vector<std::shared_ptr<city>> cities;
    std::mt19937 gen(12345);
    std::uniform_real_distribution<float> dist(0.0f, 1000.0f);

    for (int i = 0; i < numberOfCities; ++i) {
        cities.push_back(std::make_shared<city>(
            i,
            false,
            Vector2{ dist(gen), dist(gen) }
        ));
    }

    // Build ACO object
    ACO aco(cities, numAnts, Q, evaporationRate);
    aco.setAlpha(alpha);
    aco.setBeta(beta);

    auto& ants = aco.getAnts();

    using clock_type = std::chrono::steady_clock;
    auto t_start = clock_type::now();

    for (int it = 0; it < iterations; ++it) {

#if ENABLE_PARALLEL
#pragma omp parallel
        {
            std::mt19937 threadGen(
                12345 + omp_get_thread_num() + it * 9973
            );
            std::uniform_int_distribution<int> startDist(
                0, numberOfCities - 1
            );
            std::uniform_real_distribution<float> uni01(0.0f, 1.0f);

            // Thread-local probability row 
            std::vector<float> localProb(numberOfCities);

#pragma omp for schedule(static)
            for (int antIndex = 0;
                antIndex < static_cast<int>(ants.size());
                ++antIndex) {
                auto& ant = ants[antIndex];

                int start = startDist(threadGen);
                ant->visitCity(cities[start]);

                while (static_cast<int>(ant->route.size()) < numberOfCities + 1) {
                    float u = uni01(threadGen);
                    int nextIdx = aco.selectNextCity(ant, &localProb, u);
                    ant->currCity = cities[nextIdx];
                    ant->visitCity();
                }
            }
        } // end parallel region

#else
        // Sequential path
        for (auto& ant : ants) {
            int start = std::uniform_int_distribution<int>(
                0, numberOfCities - 1
            )(gen);

            ant->visitCity(cities[start]);

            while (static_cast<int>(ant->route.size()) < numberOfCities + 1) {
                int nextIdx = aco.selectNextCity(ant);
                ant->currCity = cities[nextIdx];
                ant->visitCity();
            }
        }
#endif

        // Pheromone update once per iteration (sequential)
        aco.updatePheromones();

        // Reset ants for next iteration
        for (auto& ant : ants) {
            ant->reset();
        }
    }

    auto t_end = clock_type::now();
    std::chrono::duration<double> elapsed = t_end - t_start;

#if ENABLE_PARALLEL
    int threads_used = omp_get_max_threads();
#else
    int threads_used = 1;
#endif

    std::cout << "Total execution time (headless, "
#if ENABLE_PARALLEL
        << "OpenMP, threads=" << threads_used
#else
        << "sequential"
#endif
        << "): " << elapsed.count() << " s\n";

    if (numberOfCities <= 10) {
        compareACOBestRoute(cities, aco.getPheromones());
    }
    else {
        std::cout << "Skipping brute-force TSP check for n = "
            << numberOfCities << " (too large).\n";
    }

    return 0;
}
