#include "../includes/datatypes.hpp"
#include "../includes/robot.hpp"
#include "../includes/eventSystem.hpp"
#include "../includes/hotWarmCold.hpp"
#include "../includes/logger.hpp"
#include <iostream>
#include <cmath>
#include <map>
#include <algorithm>

// Hjälpfunktioner
double calculateDistance(int nodeA, int nodeB) {
    // Simplified - skulle kunna använda Dijkstra
    // För nu returnerar vi edge-avståndet direkt om det finns
    for (const auto& edge : adj[nodeA]) {
        if (edge.to == nodeB) {
            return edge.distance;
        }
    }
    return 100.0; // Large distance if no direct edge
}

bool isRobotAtNode(int robotIdx, int nodeIdx) {
    return robots[robotIdx].currentNode == nodeIdx && robots[robotIdx].progress >= 1.0;
}