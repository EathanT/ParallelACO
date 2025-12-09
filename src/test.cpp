#include "test.h"
#include <iostream>
#include <numeric>
#include <cmath>

// Function to calculate the distance of a given route
float calculateRouteDistance(const vector<shared_ptr<city>>& cities,
    const vector<int>& route) {
    float totalDistance = 0.0f;
    for (size_t i = 0; i + 1 < route.size(); ++i) {
        Vector2 posA = cities[route[i]]->position;
        Vector2 posB = cities[route[i + 1]]->position;
        totalDistance += std::sqrt(std::pow(posA.x - posB.x, 2.0f) +
            std::pow(posA.y - posB.y, 2.0f));
    }
    // Return to the starting city
    if (!route.empty()) {
        Vector2 startPos = cities[route.front()]->position;
        Vector2 endPos = cities[route.back()]->position;
        totalDistance += std::sqrt(std::pow(endPos.x - startPos.x, 2.0f) +
            std::pow(endPos.y - startPos.y, 2.0f));
    }
    return totalDistance;
}

// Brute-force solution to find the shortest path
vector<int> bruteForceTSP(const vector<shared_ptr<city>>& cities) {
    vector<int> bestRoute;
    float shortestDistance = std::numeric_limits<float>::max();

    vector<int> cityIndices(cities.size());
    std::iota(cityIndices.begin(), cityIndices.end(), 0); // 0, 1, ..., n-1

    // Fix city 0 as the starting city to avoid equivalent rotations
    if (cityIndices.size() <= 1) {
        return cityIndices;
    }

    do {
        // Calculate the distance of the current permutation
        float currentDistance = calculateRouteDistance(cities, cityIndices);

        // Update the shortest path if the current one is better
        if (currentDistance < shortestDistance) {
            shortestDistance = currentDistance;
            bestRoute = cityIndices;
        }
    } while (std::next_permutation(cityIndices.begin() + 1, cityIndices.end()));

    return bestRoute;
}

// Function to execute and compare the brute-force and ACO results
void compareACOBestRoute(vector<shared_ptr<city>>& cities,
    vector<vector<float>>& pheromones) {
    bool       haveExact = false;
    vector<int> shortestRoute;
    float       bruteForceDistance = 0.0f;

    if (cities.size() <= 10) {
        // Brute-force baseline on small n
        shortestRoute = bruteForceTSP(cities);
        bruteForceDistance = calculateRouteDistance(cities, shortestRoute);
        std::cout << "Brute-force shortest route distance: "
            << bruteForceDistance << std::endl;
        haveExact = true;
    }
    else {
        std::cout << "Brute-force TSP baseline skipped for n = "
            << cities.size() << " (too large)." << std::endl;
    }

    // Greedy reconstruction from pheromone matrix (visit each city exactly once)
    vector<int>  acoBestRoute;
    vector<bool> visited(cities.size(), false);
    int current = 0;
    if (!cities.empty()) {
        acoBestRoute.push_back(current);
        visited[current] = true;
    }

    for (size_t step = 1; step < cities.size(); ++step) {
        int   next = -1;
        float best = -1.0f;
        for (size_t j = 0; j < cities.size(); ++j) {
            if (!visited[j] && pheromones[current][j] > best) {
                best = pheromones[current][j];
                next = static_cast<int>(j);
            }
        }
        if (next == -1) break;
        acoBestRoute.push_back(next);
        visited[next] = true;
        current = next;
    }

    float acoDistance = calculateRouteDistance(cities, acoBestRoute);
    std::cout << "ACO greedy reconstruction distance: "
        << acoDistance << std::endl;

    if (haveExact) {
        if (acoDistance <= bruteForceDistance) {
            std::cout << "ACO produced a route as good as or better than the "
                "brute-force approach."
                << std::endl;
        }
        else {
            std::cout << "Brute-force found a shorter route." << std::endl;
        }
    }
    else {
        std::cout << "Exact baseline not available; only ACO greedy distance "
            "is reported."
            << std::endl;
    }
}
