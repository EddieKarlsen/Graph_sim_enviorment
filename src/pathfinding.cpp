#include "../includes/pathfinding.hpp"
#include "../includes/datatypes.hpp"
#include <queue>
#include <algorithm>
#include <iostream>
#include <cmath>

const double INF = std::numeric_limits<double>::infinity();

void Path::print() const {
    if (!found) {
        std::cerr << "Path: Not found\n";
        return;
    }
    
    std::cerr << "Path (distance: " << totalDistance << "): ";
    for (size_t i = 0; i < nodes.size(); ++i) {
        std::cerr << nodes[i];
        if (i < nodes.size() - 1) {
            std::cerr << " -> ";
        }
    }
    std::cerr << "\n";
}

// Dijkstra's algorithm - returns distances from source to all nodes
std::vector<double> dijkstraDistances(int sourceNode) {
    int n = static_cast<int>(nodes.size());
    std::vector<double> dist(n, INF);
    std::vector<bool> visited(n, false);
    
    // Priority queue: (distance, node)
    std::priority_queue<std::pair<double, int>, 
                       std::vector<std::pair<double, int>>,
                       std::greater<std::pair<double, int>>> pq;
    
    dist[sourceNode] = 0.0;
    pq.push({0.0, sourceNode});
    
    while (!pq.empty()) {
        auto [currentDist, u] = pq.top();
        pq.pop();
        
        if (visited[u]) continue;
        visited[u] = true;
        
        // Check all adjacent nodes
        for (const Edge& edge : adj[u]) {
            int v = edge.to;
            double weight = edge.distance;
            
            // Relaxation
            if (dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                pq.push({dist[v], v});
            }
        }
    }
    
    return dist;
}

// Dijkstra with predecessor tracking for path reconstruction
std::vector<int> dijkstraPredecessors(int sourceNode) {
    int n = static_cast<int>(nodes.size());
    std::vector<double> dist(n, INF);
    std::vector<int> pred(n, -1);
    std::vector<bool> visited(n, false);
    
    std::priority_queue<std::pair<double, int>, 
                       std::vector<std::pair<double, int>>,
                       std::greater<std::pair<double, int>>> pq;
    
    dist[sourceNode] = 0.0;
    pq.push({0.0, sourceNode});
    
    while (!pq.empty()) {
        auto [currentDist, u] = pq.top();
        pq.pop();
        
        if (visited[u]) continue;
        visited[u] = true;
        
        for (const Edge& edge : adj[u]) {
            int v = edge.to;
            double weight = edge.distance;
            
            if (dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                pred[v] = u;
                pq.push({dist[v], v});
            }
        }
    }
    
    return pred;
}

// Reconstruct path from predecessors
Path reconstructPath(int startNode, int endNode, const std::vector<int>& predecessors,
                    const std::vector<double>& distances) {
    Path path;
    path.found = false;
    path.totalDistance = 0.0;
    
    // Check if path exists
    if (distances[endNode] == INF) {
        return path;
    }
    
    // Reconstruct path backwards
    std::vector<int> reversePath;
    int current = endNode;
    
    while (current != -1) {
        reversePath.push_back(current);
        if (current == startNode) break;
        current = predecessors[current];
    }
    
    // Check if we reached the start
    if (current != startNode) {
        return path;
    }
    
    // Reverse to get forward path
    path.nodes.assign(reversePath.rbegin(), reversePath.rend());
    path.totalDistance = distances[endNode];
    path.found = true;
    
    return path;
}

// Find shortest path between two nodes
Path findShortestPath(int startNode, int endNode) {
    // Validate nodes
    if (startNode < 0 || startNode >= static_cast<int>(nodes.size()) ||
        endNode < 0 || endNode >= static_cast<int>(nodes.size())) {
        Path invalidPath;
        invalidPath.found = false;
        invalidPath.totalDistance = INF;
        return invalidPath;
    }
    
    // If start == end, return trivial path
    if (startNode == endNode) {
        Path trivialPath;
        trivialPath.nodes.push_back(startNode);
        trivialPath.totalDistance = 0.0;
        trivialPath.found = true;
        return trivialPath;
    }
    
    // Run Dijkstra
    int n = static_cast<int>(nodes.size());
    std::vector<double> dist(n, INF);
    std::vector<int> pred(n, -1);
    std::vector<bool> visited(n, false);
    
    std::priority_queue<std::pair<double, int>, 
                       std::vector<std::pair<double, int>>,
                       std::greater<std::pair<double, int>>> pq;
    
    dist[startNode] = 0.0;
    pq.push({0.0, startNode});
    
    while (!pq.empty()) {
        auto [currentDist, u] = pq.top();
        pq.pop();
        
        // Early termination if we reached the target
        if (u == endNode) break;
        
        if (visited[u]) continue;
        visited[u] = true;
        
        for (const Edge& edge : adj[u]) {
            int v = edge.to;
            double weight = edge.distance;
            
            if (dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                pred[v] = u;
                pq.push({dist[v], v});
            }
        }
    }
    
    // Reconstruct path
    return reconstructPath(startNode, endNode, pred, dist);
}

// Find path avoiding certain nodes
Path findShortestPathAvoiding(int startNode, int endNode, const std::vector<int>& avoidNodes) {
    // Check if start or end is in avoid list
    if (std::find(avoidNodes.begin(), avoidNodes.end(), startNode) != avoidNodes.end() ||
        std::find(avoidNodes.begin(), avoidNodes.end(), endNode) != avoidNodes.end()) {
        Path invalidPath;
        invalidPath.found = false;
        return invalidPath;
    }
    
    int n = static_cast<int>(nodes.size());
    std::vector<double> dist(n, INF);
    std::vector<int> pred(n, -1);
    std::vector<bool> visited(n, false);
    
    // Mark avoided nodes as visited
    for (int node : avoidNodes) {
        if (node >= 0 && node < n) {
            visited[node] = true;
        }
    }
    
    std::priority_queue<std::pair<double, int>, 
                       std::vector<std::pair<double, int>>,
                       std::greater<std::pair<double, int>>> pq;
    
    dist[startNode] = 0.0;
    visited[startNode] = false;  // Unmark start
    pq.push({0.0, startNode});
    
    while (!pq.empty()) {
        auto [currentDist, u] = pq.top();
        pq.pop();
        
        if (u == endNode) break;
        
        if (visited[u]) continue;
        visited[u] = true;
        
        for (const Edge& edge : adj[u]) {
            int v = edge.to;
            double weight = edge.distance;
            
            // Skip if node should be avoided
            if (std::find(avoidNodes.begin(), avoidNodes.end(), v) != avoidNodes.end() &&
                v != endNode) {
                continue;
            }
            
            if (dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                pred[v] = u;
                pq.push({dist[v], v});
            }
        }
    }
    
    return reconstructPath(startNode, endNode, pred, dist);
}

// Check if edge exists
bool hasEdge(int fromNode, int toNode) {
    if (fromNode < 0 || fromNode >= static_cast<int>(adj.size())) {
        return false;
    }
    
    for (const Edge& edge : adj[fromNode]) {
        if (edge.to == toNode) {
            return true;
        }
    }
    
    return false;
}

// Get edge distance
double getEdgeDistance(int fromNode, int toNode) {
    if (fromNode < 0 || fromNode >= static_cast<int>(adj.size())) {
        return INF;
    }
    
    for (const Edge& edge : adj[fromNode]) {
        if (edge.to == toNode) {
            return edge.distance;
        }
    }
    
    return INF;
}

// Simple heuristic for A* (Euclidean distance - requires node positions)
// For now, return 0 (makes it equivalent to Dijkstra)
double heuristicDistance(int node1, int node2) {
    // If you have node positions (x, y), implement:
    // double dx = nodes[node2].x - nodes[node1].x;
    // double dy = nodes[node2].y - nodes[node1].y;
    // return sqrt(dx*dx + dy*dy);
    
    // For now, return 0 (Dijkstra behavior)
    return 0.0;
}

// A* pathfinding (future optimization)
Path findPathAStar(int startNode, int endNode) {
    // Validate nodes
    if (startNode < 0 || startNode >= static_cast<int>(nodes.size()) ||
        endNode < 0 || endNode >= static_cast<int>(nodes.size())) {
        Path invalidPath;
        invalidPath.found = false;
        return invalidPath;
    }
    
    if (startNode == endNode) {
        Path trivialPath;
        trivialPath.nodes.push_back(startNode);
        trivialPath.totalDistance = 0.0;
        trivialPath.found = true;
        return trivialPath;
    }
    
    int n = static_cast<int>(nodes.size());
    std::vector<double> gScore(n, INF);  // Actual cost from start
    std::vector<double> fScore(n, INF);  // Estimated total cost
    std::vector<int> pred(n, -1);
    std::vector<bool> visited(n, false);
    
    // Priority queue: (fScore, node)
    std::priority_queue<std::pair<double, int>, 
                       std::vector<std::pair<double, int>>,
                       std::greater<std::pair<double, int>>> pq;
    
    gScore[startNode] = 0.0;
    fScore[startNode] = heuristicDistance(startNode, endNode);
    pq.push({fScore[startNode], startNode});
    
    while (!pq.empty()) {
        auto [currentF, u] = pq.top();
        pq.pop();
        
        if (u == endNode) break;
        
        if (visited[u]) continue;
        visited[u] = true;
        
        for (const Edge& edge : adj[u]) {
            int v = edge.to;
            double weight = edge.distance;
            double tentativeG = gScore[u] + weight;
            
            if (tentativeG < gScore[v]) {
                gScore[v] = tentativeG;
                fScore[v] = gScore[v] + heuristicDistance(v, endNode);
                pred[v] = u;
                pq.push({fScore[v], v});
            }
        }
    }
    
    return reconstructPath(startNode, endNode, pred, gScore);
}