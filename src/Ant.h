#define ENABLE_PARALLEL 0
#define PARALLEL_PHEROMONES 0
#define ACO_NUM_THREADS 15

#ifndef ANT_H
#define ANT_H

#include <vector>
#include <memory>
#include <iostream>
#include <cstring>
#include <random>
#include <cmath>
#include <numeric>
#include <limits>
#include <algorithm>
#include <cstddef>
#include <ctime>
#include <filesystem>
#include <cstdint>  // for uint8_t

#include "raylib.h"

#if ENABLE_PARALLEL
#include <omp.h>
#endif

using namespace std;

// Structure to represent a city in the simulation
struct city {
    int    id;       // Unique identifier for the city
    bool   visited;  // NOTE: no longer mutated after construction
    Vector2 position;

    city(int cityId, bool visitedIn, Vector2 positionIn)
        : id(cityId), visited(visitedIn), position(positionIn) {
    }
};

// Class to represent an ant in the simulation
class Ant {
public:

    // Constructor: need to know number of cities to size the visited array
    Ant(int antId, std::size_t numCities)
        : position{ 0.0f, 0.0f },
        routeLength(0.0f),
        currCityId(-1),
        id(antId),
        visited(numCities, 0) {
    }

    // Start a new tour at the given city
    void startAt(int cityId) {
        reset();
        currCityId = cityId;
        route.push_back(cityId);
        if (cityId >= 0 && cityId < (int)visited.size()) {
            visited[cityId] = 1;
        }
    }

    // Move from the current city to cityId and update route length
    void moveTo(int cityId, const std::vector<std::shared_ptr<city>>& cities) {
        int prevId = currCityId;
        currCityId = cityId;
        route.push_back(cityId);

        if (cityId >= 0 && cityId < (int)visited.size()) {
            visited[cityId] = 1;
        }

        if (prevId >= 0) {
            Vector2 a = cities[prevId]->position;
            Vector2 b = cities[cityId]->position;
            float dx = a.x - b.x;
            float dy = a.y - b.y;
            routeLength += std::sqrt(dx * dx + dy * dy);
        }
    }

    // O(1) visited check
    bool hasVisited(int cityId) const {
        return (cityId >= 0 && cityId < (int)visited.size())
            ? (visited[cityId] != 0)
            : false;
    }

    // Reset all state for reuse
    void reset() {
        route.clear();
        routeLength = 0.0f;
        currCityId = -1;
        std::fill(visited.begin(), visited.end(), 0);
    }

    // Public fields used by the rest of the code
    std::vector<int> route;   // indices into global cities[]
    Vector2          position;
    float            routeLength;
    int              currCityId; // current city index
    int              id;

private:
    std::vector<uint8_t> visited; // per-city visited flags
};

#endif