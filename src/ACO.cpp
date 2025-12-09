#include "ACO.h"

namespace constants{
  float alpha = 1.0f;
  float beta = 5.0f;
}

/* 
 * Initializes parameters for the ACO algorithm:
 * - Sets up the proximity matrix using Euclidean distances
 */
void ACO::initializeParameters() {
    size_t num = citys.size();

    // Set proximity matrix based on Euclidean distance
    proximitys.resize(num, vector<float>(num, 0.0f));

    // Initialize probability matrix (used by GUI for visualization)
    probablitys.resize(num, vector<float>(num, 0.0f));

    for (size_t i = 0; i < num; ++i) {
        for (size_t j = 0; j < num; ++j) {
            float dx = citys[i]->position.x - citys[j]->position.x;
            float dy = citys[i]->position.y - citys[j]->position.y;
            proximitys[i][j] = std::sqrt(dx * dx + dy * dy); // Euclidean distance
        }
    }
}
/* 
 * Initializes pheromone trails to a starting value
 */
void ACO::initializePheromoneTrails(){
    for (auto& row : pheromones) {
        fill(row.begin(), row.end(), 1.0f);
    }
}

/* 
 * Checks if the termination condition is met
 * - Default: compares current iteration to max iterations
 */
bool ACO::terminationCondition(int iteration){
    return iteration >= maxIterations;
}

/* 
 * Updates the probability of an ant moving to all feasible cities
 */
void ACO::updateProbablity(shared_ptr<Ant> ant,
    const vector<int>& feasibleCityIndexes,
    vector<float>* localProbRow) {
    int i = ant->currCity->id;
    float bottom = 0.0f;

    // Calculate the denominator of the probability equation
    for (int j : feasibleCityIndexes) {
        float heuristic = 1.0f / std::max(proximitys[i][j], 1e-6f);
        float sum = std::pow(pheromones[i][j], constants::alpha) *
            std::pow(heuristic, constants::beta);
        bottom += sum;
    }

    if (bottom <= 0.0f) {
        // Degenerate case: fall back to uniform probabilities.
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

    // Calculate probabilities of paths i to j
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
 * Constructs solutions for the ant
 */
void ACO::constructAntSolutions(shared_ptr<Ant>& ant){
    ant->visitCity();
}

/* 
 * Updates pheromones based on the ant's path
 */
void ACO::updatePheromones() {
    // Evaporate pheromones
    const float keep = 1.0f - evaporationRate;
    const std::size_t n = pheromones.size();

#if ENABLE_PARALLEL && PARALLEL_PHEROMONES
    // Parallel evaporation over the full matrix
#pragma omp parallel for collapse(2)
    for (int i = 0; i < static_cast<int>(n); ++i) {
        for (int j = 0; j < static_cast<int>(pheromones[i].size()); ++j) {
            pheromones[i][j] *= keep;
        }
    }
#else
    // Sequential evaporation (no OpenMP)
    for (auto& row : pheromones) {
        for (auto& p : row) {
            p *= keep;
        }
    }
#endif

    // Deposit pheromones based on each ant's route (kept sequential for now)
    for (auto& ant : ants) {
        if (ant->route.size() < 2 || ant->routeLength <= 0.0f) {
            continue;
        }

        float concentration = Q / ant->routeLength;

        for (std::size_t i = 0; i + 1 < ant->route.size(); ++i) {
            int a = ant->route[i]->id;
            int b = ant->route[i + 1]->id;

            pheromones[a][b] += concentration;
            pheromones[b][a] += concentration;
        }
    }
}

/*
 * Chooses the next city for the ant to visit
 */
int ACO::selectNextCity(shared_ptr<Ant> ant, vector<float>* localProbRow, float random01) {
    vector<int> feasibleCityIndexes;
    feasibleCityIndexes.reserve(citys.size());
    
    for (size_t i = 0; i < citys.size(); ++i){
        if (!ant->hasVisited(citys[i]->id)) {
            feasibleCityIndexes.push_back(static_cast<int>(i));
        }
    }

    if (feasibleCityIndexes.empty()) {
        feasibleCityIndexes.push_back(ant->route[0]->id); // visit starting city
    }

    // Probability function call
    updateProbablity(ant, feasibleCityIndexes,localProbRow);

    struct cityIndexProb {
        int cityId;
        float prob;
    };

    struct
    {
        bool operator()(cityIndexProb a, cityIndexProb b) const {
          return a.prob < b.prob;
        }
    }
    CIPsort;

    // Track probabilities with city id
    vector<cityIndexProb> cityProbabilities;
    cityProbabilities.reserve(feasibleCityIndexes.size());

    for (int i : feasibleCityIndexes) {
        float p = localProbRow
            ? (*localProbRow)[i]
            : probablitys[ant->currCity->id][i];
        cityProbabilities.push_back({i,p});
    }

    std::sort(cityProbabilities.begin(), cityProbabilities.end(), CIPsort);
  
    // Calculate total of all city probabilities
    float total = 0.0f;
    for (const auto& prob : cityProbabilities) {
        total += prob.prob;
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

    // Get random value in 0 to total
    float u = random01;
    if (u < 0.0f) {
        u = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
    }
    float randomValue = u * total;

    float cumulativeProbability = 0.0f;
    for (const auto& prob : cityProbabilities) {
        cumulativeProbability += prob.prob;
        if (cumulativeProbability >= randomValue) {
            return prob.cityId;
        }
    }

    // Default return in case of a rounding error
    return cityProbabilities.back().cityId;
}
