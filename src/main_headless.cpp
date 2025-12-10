// Toggle parallelization:

#include "Ant.h"
#include "ACO.h"
#include "test.h"

int main() {
    int   numAnts = 100;
    int   numberOfCities = 75;
    int   iterations = 100;
    float Q = 100.0f;
    float evaporationRate = 0.5f;
    float alpha = 1.0f;
    float beta = 5.0f;

    // Generate random cities
    std::vector<std::shared_ptr<city>> cities;
    std::mt19937 gen(12345);
    std::uniform_real_distribution<float> dist(0.0f, 1000.0f);

    for (int i = 0; i < numberOfCities; ++i) {
        Vector2 pos{ dist(gen), dist(gen) };
        cities.push_back(std::make_shared<city>(i, false, pos));
    }

    // Build ACO object
    ACO aco(cities, numAnts, Q, evaporationRate);
    aco.setAlpha(alpha);
    aco.setBeta(beta);

    auto& ants = aco.getAnts();

    using clock_type = std::chrono::steady_clock;
    auto t_start = clock_type::now();

#if ENABLE_PARALLEL

#if defined(ACO_NUM_THREADS) && (ACO_NUM_THREADS > 0)
    omp_set_num_threads(ACO_NUM_THREADS);
#endif

#pragma omp parallel
    {
        std::mt19937 threadGen(12345 + omp_get_thread_num());
        std::uniform_int_distribution<int> startDist(0, numberOfCities - 1);
        std::uniform_real_distribution<float> uni01(0.0f, 1.0f);

        std::vector<float> localProb(numberOfCities);

        for (int it = 0; it < iterations; ++it) {

#pragma omp for schedule(static)
            for (int antIndex = 0;
                antIndex < static_cast<int>(ants.size());
                ++antIndex) {
                auto& ant = ants[antIndex];

                int start = startDist(threadGen);
                ant->startAt(start);

                while (static_cast<int>(ant->route.size()) < numberOfCities + 1) {
                    float u = uni01(threadGen);
                    int nextIdx = aco.selectNextCity(ant, &localProb, u);
                    ant->moveTo(nextIdx, cities);
                }
            }

#pragma omp single
            {
                aco.updatePheromones();
                for (auto& ant : ants) {
                    ant->reset();
                }
            }

#pragma omp barrier
        }
    }
#else
    for (int it = 0; it < iterations; ++it) {
        for (auto& ant : ants) {
            int start = std::uniform_int_distribution<int>(
                0, numberOfCities - 1
            )(gen);

            ant->startAt(start);

            while (static_cast<int>(ant->route.size()) < numberOfCities + 1) {
                int nextIdx = aco.selectNextCity(ant);
                ant->moveTo(nextIdx, cities);
            }
        }

        aco.updatePheromones();

        for (auto& ant : ants) {
            ant->reset();
        }
    }
#endif

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
