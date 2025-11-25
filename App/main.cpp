#include <iostream>
#include <chrono>
#include <thread>
#include "../includes/datatypes.hpp"
#include "../includes/exportJson.hpp"
#include "../includes/json.hpp"
#include "../includes/graphMqtt.hpp"

void initSimulation();

void showProducts(){
    for (size_t i = 0; i < nodes.size(); ++i) {

        if (nodes[i].type == NodeType::Shelf) {
            const auto& shelf = std::get<Shelf>(nodes[i].data);
            std::cout << "\n" << nodes[i].id << " (" << shelf.name << "):\n";
            for (int j = 0; j < shelf.slotCount; ++j) {
                if (shelf.slots[j].productID > 0) {
                    std::cout << "  Slot " << j << ": " 
                              << products[shelf.slots[j].productID - 1].name 
                              << " (" << shelf.slots[j].occupied << "/" 
                              << shelf.slots[j].capacity << ")\n";
    
                }
            }
        }
    }
    return;
}

void drawGraph(){
    
    //visualization of the graph layout for easier checking
    std::cout << "\nGraph visualization\n\n";
    std::vector<std::vector<bool>> printed(nodes.size(), std::vector<bool>(nodes.size(), false));

    for (int i = 0; i < nodes.size(); ++i) {
        for (const auto& e : adj[i]) {
            if (e.directed) {
                std::cout << "[" << nodes[i].id << "] --(" << e.distance << ")--> [" 
                          << nodes[e.to].id << "]\n";
            } else {
                if (!printed[i][e.to] && !printed[e.to][i]) {
                    std::cout << "[" << nodes[i].id << "] <--(" << e.distance << ")--> [" 
                              << nodes[e.to].id << "]\n";
                    printed[i][e.to] = true;
                    printed[e.to][i] = true;
                }
            }
        }
    }

    return;
}

int main() {
    initSimulation();

    drawGraph();
    showProducts();
    exportSimulationJSON("exports/warehouse_state.json");
    return 0;
}
