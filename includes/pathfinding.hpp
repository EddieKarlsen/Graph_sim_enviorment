#ifndef PATHFINDING_HPP
#define PATHFINDING_HPP

#include "datatypes.hpp"
#include <vector>
#include <limits>



// Dijkstra's algorithm for shortest path
Path findShortestPath(int startNode, int endNode);

// Find path avoiding certain nodes (useful for avoiding congestion)
Path findShortestPathAvoiding(int startNode, int endNode, const std::vector<int>& avoidNodes);

// Find all shortest paths from a source (useful for pre-computation)
std::vector<double> dijkstraDistances(int sourceNode);
std::vector<int> dijkstraPredecessors(int sourceNode);

// Reconstruct path from predecessors
Path reconstructPath(int startNode, int endNode, const std::vector<int>& predecessors, 
                    const std::vector<double>& distances);

// Helper: Check if edge exists between two nodes
bool hasEdge(int fromNode, int toNode);

// Helper: Get edge distance
double getEdgeDistance(int fromNode, int toNode);

// A* variant (optional, for future optimization)
Path findPathAStar(int startNode, int endNode);

// Utility: Calculate heuristic distance (Euclidean)
double heuristicDistance(int node1, int node2);

#endif