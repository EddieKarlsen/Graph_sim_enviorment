#include <iostream>
#include "../includes/datatypes.hpp"

int addNode(const Node& n) {
    nodes.push_back(n);
    adj.emplace_back();  
    return nodes.size() - 1;
}

void addEdge(int from, int to, double distance, bool directed = false) {
    adj[from].push_back({to, directed, distance});
    if (!directed) {
        adj[to].push_back({from, directed, distance});
    }
}

void initProducts() {
    // Clothing (IDs 1-5)
    products.push_back({1, "T-shirts"});
    products.push_back({2, "Jeans"});
    products.push_back({3, "Jackets"});
    products.push_back({4, "Shoes"});
    products.push_back({5, "Accessories"});
    
    // Beverages (IDs 6-8)
    products.push_back({6, "Soda"});
    products.push_back({7, "Juice"});
    products.push_back({8, "Energy Drinks"});
    
    // Cosmetics (IDs 9-12)
    products.push_back({9, "Skin Care"});
    products.push_back({10, "Makeup"});
    products.push_back({11, "Perfume"});
    products.push_back({12, "Hair Care"});
    
    // Electronics (IDs 13-17)
    products.push_back({13, "Mobile Phones"});
    products.push_back({14, "Laptops"});
    products.push_back({15, "Headphones"});
    products.push_back({16, "Game Consoles"});
    products.push_back({17, "Cameras"});
    
    // Books & Media (IDs 18-20)
    products.push_back({18, "Books"});
    products.push_back({19, "Magazines"});
    products.push_back({20, "Games"});
    
    // Home & Household (IDs 21-25)
    products.push_back({21, "Kitchen Utensils"});
    products.push_back({22, "Textiles"});
    products.push_back({23, "Furniture"});
    products.push_back({24, "Lighting"});
    products.push_back({25, "Decoration"});
    
    // Sports & Recreation (IDs 26-28)
    products.push_back({26, "Training Equipment"});
    products.push_back({27, "Sports Clothing"});
    products.push_back({28, "Outdoor Equipment"});
    
    // Toys (IDs 29-30)
    products.push_back({29, "Children's Toys"});
    products.push_back({30, "Board Games"});
}

void assignProductToSlot(Shelf& shelf, int slotIndex, int productID, int capacity, int occupied) {
    if (slotIndex < shelf.slotCount && slotIndex < MAX_SLOTS) {
        shelf.slots[slotIndex].productID = productID;
        shelf.slots[slotIndex].capacity = capacity;
        shelf.slots[slotIndex].occupied = occupied;
    }
}

void initSimulation() {
    
    // Initialize products first
    initProducts();
    
    // Loading dock
    LoadingDock loadingDock;
    loadingDock.isOccupied = false;
    loadingDock.deliveryCount = 0;
    loadingDock.currentLorry = Lorry::MEDIUM_LORRY;
    
    int loadingDockNode = addNode(Node{
        .id = "loading_dock",
        .type = NodeType::LoadingBay,
        .maxRobots = 2,
        .data = loadingDock
    });

    // Shelf A - Clothing
    Shelf shelfA;
    shelfA.name = "Shelf A";
    shelfA.slotCount = 5;
    int shelfANode = addNode(Node{
        .id = "shelf_A",
        .type = NodeType::Shelf,
        .maxRobots = 1,
        .data = shelfA
    });
    // Assign products to Shelf A
    auto& shelfAData = std::get<Shelf>(nodes[shelfANode].data);
    assignProductToSlot(shelfAData, 0, 1, 50, 35);  // T-shirts
    assignProductToSlot(shelfAData, 1, 2, 40, 28);  // Jeans
    assignProductToSlot(shelfAData, 2, 3, 30, 15);  // Jackets
    assignProductToSlot(shelfAData, 3, 4, 45, 30);  // Shoes
    assignProductToSlot(shelfAData, 4, 5, 60, 45);  // Accessories

    // Shelf B - Electronics
    Shelf shelfB;
    shelfB.name = "Shelf B";
    shelfB.slotCount = 5;
    int shelfBNode = addNode(Node{
        .id = "shelf_B",
        .type = NodeType::Shelf,
        .maxRobots = 1,
        .data = shelfB
    });
    auto& shelfBData = std::get<Shelf>(nodes[shelfBNode].data);
    assignProductToSlot(shelfBData, 0, 13, 25, 12);  // Mobile Phones
    assignProductToSlot(shelfBData, 1, 14, 20, 8);   // Laptops
    assignProductToSlot(shelfBData, 2, 15, 50, 35);  // Headphones
    assignProductToSlot(shelfBData, 3, 16, 15, 7);   // Game Consoles
    assignProductToSlot(shelfBData, 4, 17, 30, 18);  // Cameras

    // Shelf C - Cosmetics
    Shelf shelfC;
    shelfC.name = "Shelf C";
    shelfC.slotCount = 4;
    int shelfCNode = addNode(Node{
        .id = "shelf_C",
        .type = NodeType::Shelf,
        .maxRobots = 1,
        .data = shelfC
    });
    auto& shelfCData = std::get<Shelf>(nodes[shelfCNode].data);
    assignProductToSlot(shelfCData, 0, 9, 40, 25);   // Skin Care
    assignProductToSlot(shelfCData, 1, 10, 45, 30);  // Makeup
    assignProductToSlot(shelfCData, 2, 11, 35, 20);  // Perfume
    assignProductToSlot(shelfCData, 3, 12, 40, 28);  // Hair Care

    // Shelf D - Beverages
    Shelf shelfD;
    shelfD.name = "Shelf D";
    shelfD.slotCount = 3;
    int shelfDNode = addNode(Node{
        .id = "shelf_D",
        .type = NodeType::Shelf,
        .maxRobots = 1,
        .data = shelfD
    });
    auto& shelfDData = std::get<Shelf>(nodes[shelfDNode].data);
    assignProductToSlot(shelfDData, 0, 6, 100, 75);  // Soda
    assignProductToSlot(shelfDData, 1, 7, 80, 60);   // Juice
    assignProductToSlot(shelfDData, 2, 8, 70, 45);   // Energy Drinks

    // Shelf E - Books & Media
    Shelf shelfE;
    shelfE.name = "Shelf E";
    shelfE.slotCount = 3;
    int shelfENode = addNode(Node{
        .id = "shelf_E",
        .type = NodeType::Shelf,
        .maxRobots = 1,
        .data = shelfE
    });
    auto& shelfEData = std::get<Shelf>(nodes[shelfENode].data);
    assignProductToSlot(shelfEData, 0, 18, 60, 45);  // Books
    assignProductToSlot(shelfEData, 1, 19, 50, 30);  // Magazines
    assignProductToSlot(shelfEData, 2, 20, 40, 25);  // Games

    // Shelf F - Home & Household (part 1)
    Shelf shelfF;
    shelfF.name = "Shelf F";
    shelfF.slotCount = 3;
    int shelfFNode = addNode(Node{
        .id = "shelf_F",
        .type = NodeType::Shelf,
        .maxRobots = 1,
        .data = shelfF
    });
    auto& shelfFData = std::get<Shelf>(nodes[shelfFNode].data);
    assignProductToSlot(shelfFData, 0, 21, 35, 20);  // Kitchen Utensils
    assignProductToSlot(shelfFData, 1, 22, 45, 30);  // Textiles
    assignProductToSlot(shelfFData, 2, 23, 15, 8);   // Furniture

    // Shelf G - Home & Household (part 2)
    Shelf shelfG;
    shelfG.name = "Shelf G";
    shelfG.slotCount = 2;
    int shelfGNode = addNode(Node{
        .id = "shelf_G",
        .type = NodeType::Shelf,
        .maxRobots = 1,
        .data = shelfG
    });
    auto& shelfGData = std::get<Shelf>(nodes[shelfGNode].data);
    assignProductToSlot(shelfGData, 0, 24, 40, 25);  // Lighting
    assignProductToSlot(shelfGData, 1, 25, 50, 35);  // Decoration

    // Shelf H - Sports & Recreation
    Shelf shelfH;
    shelfH.name = "Shelf H";
    shelfH.slotCount = 3;
    int shelfHNode = addNode(Node{
        .id = "shelf_H",
        .type = NodeType::Shelf,
        .maxRobots = 1,
        .data = shelfH
    });
    auto& shelfHData = std::get<Shelf>(nodes[shelfHNode].data);
    assignProductToSlot(shelfHData, 0, 26, 30, 18);  // Training Equipment
    assignProductToSlot(shelfHData, 1, 27, 40, 25);  // Sports Clothing
    assignProductToSlot(shelfHData, 2, 28, 25, 15);  // Outdoor Equipment

    // Shelf I - Toys
    Shelf shelfI;
    shelfI.name = "Shelf I";
    shelfI.slotCount = 2;
    int shelfINode = addNode(Node{
        .id = "shelf_I",
        .type = NodeType::Shelf,
        .maxRobots = 1,
        .data = shelfI
    });
    auto& shelfIData = std::get<Shelf>(nodes[shelfINode].data);
    assignProductToSlot(shelfIData, 0, 29, 55, 40);  // Children's Toys
    assignProductToSlot(shelfIData, 1, 30, 35, 20);  // Board Games

    // Shelf J - Mixed products (overflow/popular products)
    Shelf shelfJ;
    shelfJ.name = "Shelf J";
    shelfJ.slotCount = 4;
    int shelfJNode = addNode(Node{
        .id = "shelf_J",
        .type = NodeType::Shelf,
        .maxRobots = 1,
        .data = shelfJ
    });
    auto& shelfJData = std::get<Shelf>(nodes[shelfJNode].data);
    assignProductToSlot(shelfJData, 0, 1, 50, 40);   // T-shirts (extra stock)
    assignProductToSlot(shelfJData, 1, 15, 50, 35);  // Headphones (extra stock)
    assignProductToSlot(shelfJData, 2, 6, 100, 80);  // Soda (extra stock)
    assignProductToSlot(shelfJData, 3, 18, 60, 45);  // Books (extra stock)

    // Charging station
    ChargingStation chargingStation;
    chargingStation.isOccupied = 0;
    chargingStation.chargingPorts = 3;
    int chargingStationNode = addNode(Node{
        .id = "charging_station",
        .type = NodeType::ChargingStation,
        .maxRobots = 3,
        .data = chargingStation
    });

    // Front Desk
    FrontDesk frontDesk;
    frontDesk.pendingOrders = 0;
    int frontDeskNode = addNode(Node{
        .id = "front_desk",
        .type = NodeType::FrontDesk,
        .maxRobots = 2,
        .data = frontDesk
    });

    
    // Loading dock <-> Shelf A
    addEdge(loadingDockNode, shelfANode, 5.0, false);
    
    // Shelf A -> Charging Station
    addEdge(shelfANode, chargingStationNode, 3.0, true);
    
    // Shelf A <-> Shelf B
    addEdge(shelfANode, shelfBNode, 4.0, false);
    
    // Shelf A <-> Front Desk 
    addEdge(shelfANode, frontDeskNode, 6.0, false);
    
    // Charging Station -> Shelf B (directed)
    addEdge(chargingStationNode, shelfBNode, 4.0, true);
    
    // Shelf B <-> Shelf C
    addEdge(shelfBNode, shelfCNode, 3.0, false);
    
    // Shelf B <-> Shelf D
    addEdge(shelfBNode, shelfDNode, 4.0, false);
    
    // Shelf B <-> Shelf E
    addEdge(shelfBNode, shelfENode, 5.0, false);
    
    // Shelf C -> Shelf G (directed)
    addEdge(shelfCNode, shelfGNode, 4.0, true);
    
    // Shelf C -> Shelf F
    addEdge(shelfCNode, shelfFNode, 5.0, true);
    
    // Shelf D -> Shelf C
    addEdge(shelfDNode, shelfCNode, 3.0, true);
    
    // Shelf D -> Shelf H
    addEdge(shelfDNode, shelfHNode, 4.0, true);
    
    // Shelf E -> Shelf D
    addEdge(shelfENode, shelfDNode, 7.0, true);
    
    // Shelf F <-> Shelf J
    addEdge(shelfFNode, shelfJNode, 6.0, false);

    // Shelf F -> Shelf G
    addEdge(shelfFNode, shelfGNode, 3.0, true);
    
    // Shelf G -> Shelf D (directed)
    addEdge(shelfGNode, shelfDNode, 3.0, true);
    
    // Shelf H <-> Shelf I
    addEdge(shelfHNode, shelfINode, 4.0, false);
    
    // Shelf H -> Shelf J (directed)
    addEdge(shelfHNode, shelfJNode, 5.0, true);
    
    // Shelf I <-> Front Desk
    addEdge(shelfINode, frontDeskNode, 8.0, false);

    // Shelf F -> Charging Station
    addEdge(shelfFNode, chargingStationNode, 10.0, true);

    std::cout << "Simulation graph initialized\n";
    std::cout << "Total nodes: " << nodes.size() << "\n";
    std::cout << "Total products: " << products.size() << "\n";
}