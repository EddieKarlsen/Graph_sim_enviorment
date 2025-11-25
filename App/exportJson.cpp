#include <iostream>
#include <fstream>
#include <filesystem>
#include "../includes/datatypes.hpp"
#include "../includes/json.hpp"


extern std::vector<Node> nodes;
extern std::vector<std::vector<Edge>> adj;
extern std::vector<Product> products;

using json = nlohmann::json;

void exportSimulationJSON(const std::string& filename) {
    json root;

    // --------------------------
    // NODES
    // --------------------------
    root["nodes"] = json::array();

    for (const Node& n : nodes) {
        json nObj;
        nObj["id"] = n.id;

        switch (n.type) {
            case NodeType::Shelf:               nObj["type"] = "Shelf"; break;
            case NodeType::ChargingStation:     nObj["type"] = "ChargingStation"; break;
            case NodeType::LoadingBay:          nObj["type"] = "LoadingBay"; break;
            case NodeType::FrontDesk:           nObj["type"] = "FrontDesk"; break;
            default:                             nObj["type"] = "Unknown"; break;
        }

        nObj["maxRobots"] = n.maxRobots;

        // Shelf data
        if (n.type == NodeType::Shelf) {
            Shelf shelf = std::get<Shelf>(n.data);
            nObj["slots"] = json::array();

            for (int i = 0; i < shelf.slotCount; i++) {
                const Slot& s = shelf.slots[i];
                json slotObj;
                slotObj["productID"] = s.productID;
                slotObj["capacity"] = s.capacity;
                slotObj["occupied"] = s.occupied;
                nObj["slots"].push_back(slotObj);
            }
        }

        root["nodes"].push_back(nObj);
    }

    // --------------------------
    // EDGES
    // --------------------------
    root["edges"] = json::array();

    for (size_t i = 0; i < adj.size(); i++) {
        for (const Edge& e : adj[i]) {
            json eObj;
            eObj["from"] = nodes[i].id;
            eObj["to"] = nodes[e.to].id;
            eObj["distance"] = e.distance;
            eObj["directed"] = e.directed;
            root["edges"].push_back(eObj);
        }
    }

    // --------------------------
    // PRODUCTS
    // --------------------------
    root["products"] = json::array();

    for (const Product& p : products) {
        json pObj;
        pObj["id"] = p.id;
        pObj["name"] = p.name;
        //pObj["popularity"] = p.popularity;
        root["products"].push_back(pObj);
    }

    // --------------------------
    // WRITE TO FILE
    // --------------------------
    std::filesystem::create_directories("exports");
    std::ofstream file(filename);
    file << root.dump(4); // 4 = indentering för snyggt läsbart JSON
    file.close();

    std::cout << "Exported simulation state → " << filename << "\n";
}

// Använd denna funktion i din SimulatorMQTTClient::publish_warehouse_state
json getWarehouseStateJSON() {
    json root;
    json nodes_status = json::array();
    
    // Använd enbart de delar av 'Node' som representerar status, 
    // t.ex. Slot-inventering och antalet robotar.

    for (const Node& n : nodes) {
        json nObj;
        nObj["id"] = n.id;
        
        // Exempel: Inkludera endast status-relevanta fält
        nObj["occupiedRobots"] = n.currentRobots; // Antag att du har denna variabel
        
        // --------------------------
        // Shelf data (Inventariestatus)
        // --------------------------
        if (n.type == NodeType::Shelf) {
            Shelf shelf = std::get<Shelf>(n.data);
            json slots_status = json::array();

            for (int i = 0; i < shelf.slotCount; i++) {
                const Slot& s = shelf.slots[i];
                // Endast de fält som beskriver statusen på hyllan
                json slotObj;
                slotObj["productID"] = s.productID; 
                slotObj["occupied"] = s.occupied;
                slots_status.push_back(slotObj);
            }
            nObj["slots_status"] = slots_status;
        }

        nodes_status.push_back(nObj);
    }
    
    root["nodes_status"] = nodes_status;
    root["timestamp"] = time(nullptr); // Lägg till en tidsstämpel
    // Lägg till globala simulatorvariabler här (t.ex. 'current_orders_pending')
    
    return root;
}
