#include <iostream>
#include "../includes/datatypes.hpp"
#include "../includes/robot.hpp"
#include "../includes/hotWarmCold.hpp"
#include "../includes/initSim.hpp"

int addNode(const Node& n) {
    nodes.push_back(n);
    adj.emplace_back();  
    return nodes.size() - 1;
}

void addEdge(int from, int to, double distance, bool directed) {
    adj[from].push_back({to, directed, distance});
    if (!directed) {
        adj[to].push_back({from, directed, distance});
    }
}

void assignProductToSlot(Shelf& shelf, int slotIndex, int productID, int capacity, int occupied) {
    if (slotIndex < shelf.getSlotCount() && slotIndex < MAX_SLOTS) {
        Slot slot;
        slot.setProductID(productID);
        slot.setCapacity(capacity);
        slot.setOccupied(occupied);
        shelf.setSlot(slotIndex, slot);
    }
}

// INITIALIZE PRODUCTS
void initProducts() {
    products.clear();
    
    // Clothing (IDs 1-5)
    products.push_back({1, "T-shirts", 0});
    products.push_back({2, "Jeans", 0});
    products.push_back({3, "Jackets", 0});
    products.push_back({4, "Shoes", 0});
    products.push_back({5, "Accessories", 0});
    
    // Beverages (IDs 6-8)
    products.push_back({6, "Soda", 0});
    products.push_back({7, "Juice", 0});
    products.push_back({8, "Energy Drinks", 0});
    
    // Cosmetics (IDs 9-12)
    products.push_back({9, "Skin Care", 0});
    products.push_back({10, "Makeup", 0});
    products.push_back({11, "Perfume", 0});
    products.push_back({12, "Hair Care", 0});
    
    // Electronics (IDs 13-17)
    products.push_back({13, "Mobile Phones", 0});
    products.push_back({14, "Laptops", 0});
    products.push_back({15, "Headphones", 0});
    products.push_back({16, "Game Consoles", 0});
    products.push_back({17, "Cameras", 0});
    
    // Books & Media (IDs 18-20)
    products.push_back({18, "Books", 0});
    products.push_back({19, "Magazines", 0});
    products.push_back({20, "Games", 0});
    
    // Home & Household (IDs 21-25)
    products.push_back({21, "Kitchen Utensils", 0});
    products.push_back({22, "Textiles", 0});
    products.push_back({23, "Furniture", 0});
    products.push_back({24, "Lighting", 0});
    products.push_back({25, "Decoration", 0});
    
    // Sports & Recreation (IDs 26-28)
    products.push_back({26, "Training Equipment", 0});
    products.push_back({27, "Sports Clothing", 0});
    products.push_back({28, "Outdoor Equipment", 0});
    
    // Toys (IDs 29-30)
    products.push_back({29, "Children's Toys", 0});
    products.push_back({30, "Board Games", 0});
    
    std::cerr << "Initialized " << products.size() << " products\n";
}

void initGraphLayout() {
    // 1. Loading Dock
    LoadingDock loadingDock;
    loadingDock.setIsOccupied(false);
    loadingDock.setDeliveryCount(0);
    loadingDock.setCurrentLorry(Lorry::MEDIUM_LORRY);
    
    loadingDockNode = addNode(Node{ .id = "loading_dock", .type = NodeType::LoadingBay, .maxRobots = 2, .data = loadingDock });
    nodes[loadingDockNode].setZone(Zone::Other);

    // 2. Skapa alla hyllnoder
    Shelf shelfA; 
    shelfA.setName("Shelf A"); 
    shelfA.slotCount = 5;
    shelfANode = addNode(Node{ .id = "shelf_A", .type = NodeType::Shelf, .maxRobots = 1, .data = shelfA });
    nodes[shelfANode].setZone(Zone::Hot);

    Shelf shelfB; 
    shelfB.setName("Shelf B"); 
    shelfB.slotCount = 5;
    shelfBNode = addNode(Node{ .id = "shelf_B", .type = NodeType::Shelf, .maxRobots = 1, .data = shelfB });
    nodes[shelfBNode].setZone(Zone::Warm);

    Shelf shelfC; 
    shelfC.setName("Shelf C"); 
    shelfC.slotCount = 4;
    shelfCNode = addNode(Node{ .id = "shelf_C", .type = NodeType::Shelf, .maxRobots = 1, .data = shelfC });
    nodes[shelfCNode].setZone(Zone::Cold);

    Shelf shelfD; 
    shelfD.setName("Shelf D"); 
    shelfD.slotCount = 3;
    shelfDNode = addNode(Node{ .id = "shelf_D", .type = NodeType::Shelf, .maxRobots = 1, .data = shelfD });
    nodes[shelfDNode].setZone(Zone::Cold);

    Shelf shelfE; 
    shelfE.setName("Shelf E"); 
    shelfE.slotCount = 3;
    shelfENode = addNode(Node{ .id = "shelf_E", .type = NodeType::Shelf, .maxRobots = 1, .data = shelfE });
    nodes[shelfENode].setZone(Zone::Cold);

    Shelf shelfF; 
    shelfF.setName("Shelf F"); 
    shelfF.slotCount = 3;
    shelfFNode = addNode(Node{ .id = "shelf_F", .type = NodeType::Shelf, .maxRobots = 1, .data = shelfF });
    nodes[shelfFNode].setZone(Zone::Cold);

    Shelf shelfG; 
    shelfG.setName("Shelf G"); 
    shelfG.slotCount = 2;
    shelfGNode = addNode(Node{ .id = "shelf_G", .type = NodeType::Shelf, .maxRobots = 1, .data = shelfG });
    nodes[shelfGNode].setZone(Zone::Cold);

    Shelf shelfH; 
    shelfH.setName("Shelf H"); 
    shelfH.slotCount = 3;
    shelfHNode = addNode(Node{ .id = "shelf_H", .type = NodeType::Shelf, .maxRobots = 1, .data = shelfH });
    nodes[shelfHNode].setZone(Zone::Cold);

    Shelf shelfI; 
    shelfI.setName("Shelf I"); 
    shelfI.slotCount = 2;
    shelfINode = addNode(Node{ .id = "shelf_I", .type = NodeType::Shelf, .maxRobots = 1, .data = shelfI });
    nodes[shelfINode].setZone(Zone::Hot);

    Shelf shelfJ; 
    shelfJ.setName("Shelf J"); 
    shelfJ.slotCount = 4;
    shelfJNode = addNode(Node{ .id = "shelf_J", .type = NodeType::Shelf, .maxRobots = 1, .data = shelfJ });
    nodes[shelfJNode].setZone(Zone::Warm);

    ChargingStation chargingStation; 
    chargingStation.setIsOccupied(0); 
    chargingStation.setChargingPorts(3);
    chargingStationNode = addNode(Node{ .id = "charging_station", .type = NodeType::ChargingStation, .maxRobots = 3, .data = chargingStation });
    nodes[chargingStationNode].setZone(Zone::Other);

    FrontDesk frontDesk; 
    frontDesk.setPendingOrders(0);
    frontDeskNode = addNode(Node{ .id = "front_desk", .type = NodeType::FrontDesk, .maxRobots = 2, .data = frontDesk });
    nodes[frontDeskNode].setZone(Zone::Other);

    // 3. Lägger till kanter
    addEdge(loadingDockNode, shelfANode, 5.0, false);
    addEdge(shelfANode, chargingStationNode, 3.0, true);
    addEdge(shelfANode, shelfBNode, 4.0, false);
    addEdge(shelfANode, frontDeskNode, 6.0, false);
    addEdge(chargingStationNode, shelfBNode, 4.0, true);
    addEdge(shelfBNode, shelfCNode, 3.0, false);
    addEdge(shelfBNode, shelfDNode, 4.0, false);
    addEdge(shelfBNode, shelfENode, 5.0, false);
    addEdge(shelfCNode, shelfGNode, 4.0, true);
    addEdge(shelfCNode, shelfFNode, 5.0, true);
    addEdge(shelfDNode, shelfCNode, 3.0, true);
    addEdge(shelfDNode, shelfHNode, 4.0, true);
    addEdge(shelfENode, shelfDNode, 7.0, true);
    addEdge(shelfFNode, shelfJNode, 6.0, false);
    addEdge(shelfFNode, shelfGNode, 3.0, true);
    addEdge(shelfGNode, shelfDNode, 3.0, true);
    addEdge(shelfHNode, shelfINode, 4.0, false);
    addEdge(shelfHNode, shelfJNode, 5.0, true);
    addEdge(shelfINode, frontDeskNode, 8.0, false);
    addEdge(shelfFNode, chargingStationNode, 10.0, true);

    std::cerr << "Simulation graph layout initialized with " << nodes.size() << " nodes\n";
}

void resetInventory() {
    // 1. Återställ popularitet
    for (auto& p : products) {
        p.setPopularity(0);
    }
    
    // 2. Fyll hyllor med nya getters/setters
    auto* shelfAData = nodes[shelfANode].getShelf();
    if (shelfAData) {
        assignProductToSlot(*shelfAData, 0, 1, 50, 35);
        assignProductToSlot(*shelfAData, 1, 2, 40, 28);
        assignProductToSlot(*shelfAData, 2, 3, 30, 15);
        assignProductToSlot(*shelfAData, 3, 4, 45, 30);
        assignProductToSlot(*shelfAData, 4, 5, 60, 45);
    }

    auto* shelfBData = nodes[shelfBNode].getShelf();
    if (shelfBData) {
        assignProductToSlot(*shelfBData, 0, 13, 25, 12);
        assignProductToSlot(*shelfBData, 1, 14, 20, 8);
        assignProductToSlot(*shelfBData, 2, 15, 50, 35);
        assignProductToSlot(*shelfBData, 3, 16, 15, 7);
        assignProductToSlot(*shelfBData, 4, 17, 30, 18);
    }

    auto* shelfCData = nodes[shelfCNode].getShelf();
    if (shelfCData) {
        assignProductToSlot(*shelfCData, 0, 9, 40, 25);
        assignProductToSlot(*shelfCData, 1, 10, 45, 30);
        assignProductToSlot(*shelfCData, 2, 11, 35, 20);
        assignProductToSlot(*shelfCData, 3, 12, 40, 28);
    }

    auto* shelfDData = nodes[shelfDNode].getShelf();
    if (shelfDData) {
        assignProductToSlot(*shelfDData, 0, 6, 100, 75);
        assignProductToSlot(*shelfDData, 1, 7, 80, 60);
        assignProductToSlot(*shelfDData, 2, 8, 70, 45);
    }

    auto* shelfEData = nodes[shelfENode].getShelf();
    if (shelfEData) {
        assignProductToSlot(*shelfEData, 0, 18, 60, 45);
        assignProductToSlot(*shelfEData, 1, 19, 50, 30);
        assignProductToSlot(*shelfEData, 2, 20, 40, 25);
    }

    auto* shelfFData = nodes[shelfFNode].getShelf();
    if (shelfFData) {
        assignProductToSlot(*shelfFData, 0, 21, 35, 20);
        assignProductToSlot(*shelfFData, 1, 22, 45, 30);
        assignProductToSlot(*shelfFData, 2, 23, 15, 8);
    }

    auto* shelfGData = nodes[shelfGNode].getShelf();
    if (shelfGData) {
        assignProductToSlot(*shelfGData, 0, 24, 40, 25);
        assignProductToSlot(*shelfGData, 1, 25, 50, 35);
    }

    auto* shelfHData = nodes[shelfHNode].getShelf();
    if (shelfHData) {
        assignProductToSlot(*shelfHData, 0, 26, 30, 18);
        assignProductToSlot(*shelfHData, 1, 27, 40, 25);
        assignProductToSlot(*shelfHData, 2, 28, 25, 15);
    }

    auto* shelfIData = nodes[shelfINode].getShelf();
    if (shelfIData) {
        assignProductToSlot(*shelfIData, 0, 29, 55, 40);
        assignProductToSlot(*shelfIData, 1, 30, 35, 20);
    }

    auto* shelfJData = nodes[shelfJNode].getShelf();
    if (shelfJData) {
        assignProductToSlot(*shelfJData, 0, 1, 50, 40);
        assignProductToSlot(*shelfJData, 1, 15, 50, 35);
        assignProductToSlot(*shelfJData, 2, 6, 100, 80);
        assignProductToSlot(*shelfJData, 3, 18, 60, 45);
    }

    // 3. Återställ nodes med getters/setters
    auto* dockData = nodes[loadingDockNode].getLoadingDock();
    if (dockData) {
        dockData->setIsOccupied(false);
        dockData->setDeliveryCount(0);
    }

    auto* chargeData = nodes[chargingStationNode].getChargingStation();
    if (chargeData) {
        chargeData->setIsOccupied(0);
    }
    
    auto* deskData = nodes[frontDeskNode].getFrontDesk();
    if (deskData) {
        deskData->setPendingOrders(0);
    }
    
    // 4. Återställ robot counters
    for (auto& node : nodes) {
        node.setCurrentRobots(0);
    }

    std::cerr << "Inventory reset for new episode\n";
}