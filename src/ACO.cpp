#include "ACO.h"

namespace constants {
    float alpha = 1.0f;
    float beta = 5.0f;
}

/*
 * Initializes parameters for the ACO algorithm:
 * - Sets up the proximity matrix using Euclidean distances
 */
void ACO::initializeParameters() {
    size_t num = citys.size();

    proximitys.resize(num, vector<float>(num, 0.0f));
    probablitys.resize(num, vector<float>(num, 0.0f));

    for (size_t i = 0; i < num; ++i) {
        for (size_t j = 0; j < num; ++j) {
            float dx = citys[i]->position.x - citys[j]->position.x;
            float dy = citys[i]->position.y - citys[j]->position.y;
            proximitys[i][j] = std::sqrt(dx * dx + dy * dy);
        }
    }
}

/*
 * Initializes pheromone trails to a starting value
 */
void ACO::initializePheromoneTrails() {
    for (auto& row : pheromones) {
        std::fill(row.begin(), row.end(), 1.0f);
    }
}

bool ACO::terminationCondition(int iteration) {
    return iteration >= maxIterations;
}

/*
 * Updates the probability of an ant moving to all feasible cities
 */
void ACO::updateProbablity(shared_ptr<Ant> ant,
    const vector<int>& feasibleCityIndexes,
    vector<float>* localProbRow) {
    int i = ant->currCityId;
    float bottom = 0.0f;

    for (int j : feasibleCityIndexes) {
        float heuristic = 1.0f / std::max(proximitys[i][j], 1e-6f);
        float sum = std::pow(pheromones[i][j], constants::alpha) *
            std::pow(heuristic, constants::beta);
        bottom += sum;
    }

    if (feasibleCityIndexes.empty()) return;

    if (bottom <= 0.0f) {
        float uniform = 1.0f / static_cast<float>(feasibleCityIndexes.size());
        if (localProbRow) {
            if (localProbRow->size() < citys.size()) {
                localProbRow->assign(citys.size(), 0.0f);
            }
            for (int j : feasibleCityIndexes) {
                (*localProbRow)[j] = uniform;
            }
        }
        else {
            for (int j : feasibleCityIndexes) {
                probablitys[i][j] = uniform;
                probablitys[j][i] = uniform;
            }
        }
        return;
    }

    if (localProbRow && localProbRow->size() < citys.size()) {
        localProbRow->assign(citys.size(), 0.0f);
    }

    for (int j : feasibleCityIndexes) {
        float heuristic = 1.0f / std::max(proximitys[i][j], 1e-6f);
        float top = std::pow(pheromones[i][j], constants::alpha) *
            std::pow(heuristic, constants::beta);
        float p = top / bottom;

        if (localProbRow) {
            (*localProbRow)[j] = p;
        }
        else {
            probablitys[i][j] = p;
            probablitys[j][i] = p;
        }
    }
}

/*
 * Constructs solutions for the ant (no-op now; distance is in Ant::moveTo)
 */
void ACO::constructAntSolutions(shared_ptr<Ant>& /*ant*/) {
    // kept for compatibility; logic moved into Ant::moveTo
}

/*
 * Updates pheromones based on the ants' paths
 * - evaporation + (optionally parallel) deposit
 */
void ACO::updatePheromones() {
    const float keep = 1.0f - evaporationRate;
    const std::size_t n = pheromones.size();

#if ENABLE_PARALLEL && PARALLEL_PHEROMONES
    // Evaporation
#pragma omp parallel for schedule(static)
    for (int i = 0; i < static_cast<int>(n); ++i) {
        for (int j = 0; j < static_cast<int>(pheromones[i].size()); ++j) {
            pheromones[i][j] *= keep;
        }
    }

    // Thread-local delta matrices for deposit
    int nThreads = omp_get_max_threads();
    std::vector<std::vector<std::vector<float>>> localDelta(
        nThreads,
        std::vector<std::vector<float>>(n, std::vector<float>(n, 0.0f))
    );

#pragma omp parallel
    {
        int tid = omp_get_thread_num();
        auto& delta = localDelta[tid];

#pragma omp for schedule(static)
        for (int k = 0; k < static_cast<int>(ants.size()); ++k) {
            auto& ant = ants[k];
            if (ant->route.size() < 2 || ant->routeLength <= 0.0f) {
                continue;
            }

            float concentration = Q / ant->routeLength;

            for (std::size_t i = 0; i + 1 < ant->route.size(); ++i) {
                int a = ant->route[i];
                int b = ant->route[i + 1];
                delta[a][b] += concentration;
                delta[b][a] += concentration;
            }
        }
    }

    // Reduce deltas into global pheromone matrix
#pragma omp parallel for schedule(static)
    for (int i = 0; i < static_cast<int>(n); ++i) {
        for (int j = 0; j < static_cast<int>(n); ++j) {
            float sum = 0.0f;
            for (int t = 0; t < nThreads; ++t) {
                sum += localDelta[t][i][j];
            }
            pheromones[i][j] *= 1.0f; // already evaporated above
            pheromones[i][j] += sum;
        }
    }

#else
    // Sequential evaporation
    for (auto& row : pheromones) {
        for (auto& p : row) {
            p *= keep;
        }
    }

    // Sequential deposit
    for (auto& ant : ants) {
        if (ant->route.size() < 2 || ant->routeLength <= 0.0f) {
            continue;
        }

        float concentration = Q / ant->routeLength;
        for (std::size_t i = 0; i + 1 < ant->route.size(); ++i) {
            int a = ant->route[i];
            int b = ant->route[i + 1];

            pheromones[a][b] += concentration;
            pheromones[b][a] += concentration;
        }
    }
#endif
}

/*
 * Chooses the next city for the ant to visit
 */
int ACO::selectNextCity(shared_ptr<Ant> ant,
    vector<float>* localProbRow,
    float random01) {
    vector<int> feasibleCityIndexes;
    feasibleCityIndexes.reserve(citys.size());

    // Cities are indexed 0..citys.size()-1
    for (std::size_t i = 0; i < citys.size(); ++i) {
        int idx = static_cast<int>(i);
        if (!ant->hasVisited(idx)) {
            feasibleCityIndexes.push_back(idx);
        }
    }

    // If everything is visited, force return to starting city
    if (feasibleCityIndexes.empty()) {
        if (!ant->route.empty()) {
            feasibleCityIndexes.push_back(ant->route[0]);
        }
        else {
            // degenerate: no route yet, choose city 0
            feasibleCityIndexes.push_back(0);
        }
    }

    // Probability function call
    updateProbablity(ant, feasibleCityIndexes, localProbRow);

    struct cityIndexProb {
        int   cityId;
        float prob;
    };

    struct {
        bool operator()(cityIndexProb a, cityIndexProb b) const {
            return a.prob < b.prob;
        }
    } CIPsort;

    vector<cityIndexProb> cityProbabilities;
    cityProbabilities.reserve(feasibleCityIndexes.size());

    for (int idx : feasibleCityIndexes) {
        float p = localProbRow
            ? (*localProbRow)[idx]
            : probablitys[ant->currCityId][idx];
        cityProbabilities.push_back({ idx, p });
    }

    std::sort(cityProbabilities.begin(), cityProbabilities.end(), CIPsort);

    float total = 0.0f;
    for (const auto& cp : cityProbabilities) {
        total += cp.prob;
    }

    if (total <= 0.0f) {
        // choose uniformly among feasible cities
        int n = static_cast<int>(feasibleCityIndexes.size());
        if (n == 1) {
            return cityProbabilities[0].cityId;
        }
        if (random01 < 0.0f) {
            std::uniform_int_distribution<int> dist(0, n - 1);
            return feasibleCityIndexes[dist(rng)];
        }
        else {
            int k = static_cast<int>(random01 * n);
            if (k >= n) k = n - 1;
            return feasibleCityIndexes[k];
        }
    }

    float u = random01;
    if (u < 0.0f) {
        u = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
    }
    float randomValue = u * total;

    float cumulativeProbability = 0.0f;
    for (const auto& cp : cityProbabilities) {
        cumulativeProbability += cp.prob;
        if (cumulativeProbability >= randomValue) {
            return cp.cityId;
        }
    }

    // Default: highest probability
    return cityProbabilities.back().cityId;
}
