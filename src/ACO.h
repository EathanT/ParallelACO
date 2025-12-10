#ifndef ACO_H
#define ACO_H

#include "Ant.h"

using namespace std;

// Namespace containing constants for the ACO algorithm
namespace constants {
    extern float alpha; // Importance of pheromone
    extern float beta;  // Importance of heuristic information
}

// Random number generator, only for sequential
inline mt19937 rng(static_cast<unsigned>(time(nullptr)));

// A class representing the Ant Colony Optimization algorithm
class ACO {
public:

    // Constructor to initialize ACO with cities, number of ants, and params
    ACO(vector<shared_ptr<city>>& inCitys, int amtAnts, float newQ, float newER)
        : pheromones(inCitys.size(), vector<float>(inCitys.size(), 1.0f)),
        citys(inCitys),
        Q(newQ),
        evaporationRate(newER),
        maxIterations(0) {

        // Create ant instances and assign IDs
        ants.resize(amtAnts);
        for (int i = 0; i < amtAnts; ++i) {
            ants[i] = make_shared<Ant>(i, inCitys.size());
            ants[i]->id = i;
        }

        initializeParameters();
        initializePheromoneTrails();
    }

    void setAlpha(float newVal) { constants::alpha = newVal; }
    void setBeta(float newVal) { constants::beta = newVal; }

    vector<shared_ptr<Ant>>& getAnts() { return ants; }
    vector<vector<float>>& getPheromones() { return pheromones; }
    vector<vector<float>>& getProximity() { return proximitys; }
    vector<vector<float>>& getProbablitys() { return probablitys; }

    // Perform a single step for the specified ant (GUI path)
    void step(shared_ptr<Ant>& ant) {
        int nextId = selectNextCity(ant);
        ant->moveTo(nextId, citys);
    }

    void updatePheromones();

    // Select the next city for the ant to visit based on probabilities
    int selectNextCity(shared_ptr<Ant> ant,
        vector<float>* localProbRow = nullptr,
        float random01 = -1.0f);

    float evaporationRate = 0.5f;
    float Q = 500.0f; // Constant for pheromone deposit

private:
    vector<vector<float>>   pheromones;
    vector<vector<float>>   probablitys;
    vector<vector<float>>   proximitys;
    vector<shared_ptr<Ant>> ants;
    vector<shared_ptr<city>>& citys;

    int maxIterations;

    void initializeParameters();
    void showAllPheromoneTrails();
    void initializePheromoneTrails();
    bool terminationCondition(int iteration);

    void updateProbablity(shared_ptr<Ant> ant,
        const vector<int>& feasibleCityIndexes,
        vector<float>* localProbRow = nullptr);

    void constructAntSolutions(shared_ptr<Ant>& ant);

    int getRandomCityIndex(int numberOfCities) {
        uniform_int_distribution<int> dist(0, numberOfCities - 1);
        return dist(rng);
    }
};

#endif // ACO_H
