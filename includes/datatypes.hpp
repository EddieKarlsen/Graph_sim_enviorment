#ifndef DATATYPES_HPP
#define DATATYPES_HPP

#include <string>
#include <vector>
#include <variant>

#define MAX_SLOTS 10

enum class Zone {
    Hot,
    Warm,
    Cold,
    Other
};

enum Lorry {
    BIG_LORRY = 30,      
    MEDIUM_LORRY = 20,   
    SMALL_LORRY = 10     
};

enum class NodeType {
    Shelf,
    LoadingBay,
    FrontDesk,
    ChargingStation,
    Junction
};

struct Edge {
    int to; 
    bool directed;
    double distance;
};

struct Product {
    int id;
    std::string name;
    int popularity = 0;
    
    // Getters
    int getId() const { return id; }
    std::string getName() const { return name; }
    int getPopularity() const { return popularity; }
    
    // Setters
    void setPopularity(int pop) { popularity = pop; }
};

struct Slot {
    int occupied;       
    int productID; 
    int capacity;
    
    // Getters
    int getOccupied() const { return occupied; }
    int getProductID() const { return productID; }
    int getCapacity() const { return capacity; }
    
    // Setters
    void setOccupied(int occ) { occupied = occ; }
    void setProductID(int pid) { productID = pid; }
    void setCapacity(int cap) { capacity = cap; }
};

struct Shelf {
    std::string name;
    struct Slot slots[MAX_SLOTS];
    int slotCount;
    
    // Getters
    std::string getName() const { return name; }
    int getSlotCount() const { return slotCount; }
    Slot getSlot(int index) const { 
        if (index >= 0 && index < slotCount && index < MAX_SLOTS) {
            return slots[index];
        }
        return Slot{0, 0, 0};
    }
    
    // Setters
    void setName(const std::string& n) { name = n; }
    void setSlot(int index, const Slot& slot) {
        if (index >= 0 && index < slotCount && index < MAX_SLOTS) {
            slots[index] = slot;
        }
    }
    void setSlotOccupied(int index, int occupied) {
        if (index >= 0 && index < slotCount && index < MAX_SLOTS) {
            slots[index].occupied = occupied;
        }
    }
};

struct LoadingDock {
    bool isOccupied;
    int deliveryCount;
    Lorry currentLorry;
    
    // Getters
    bool getIsOccupied() const { return isOccupied; }
    int getDeliveryCount() const { return deliveryCount; }
    int getCurrentLorry() const { return static_cast<int>(currentLorry); }
    
    // Setters
    void setIsOccupied(bool occupied) { isOccupied = occupied; }
    void setDeliveryCount(int count) { deliveryCount = count; }
    void setCurrentLorry(Lorry lorry) { currentLorry = lorry; }
};

struct ChargingStation {
    int isOccupied;
    int chargingPorts;
    
    // Getters
    int getIsOccupied() const { return isOccupied; }
    int getChargingPorts() const { return chargingPorts; }
    
    // Setters
    void setIsOccupied(int occupied) { isOccupied = occupied; }
    void setChargingPorts(int ports) { chargingPorts = ports; }
};

struct FrontDesk {
    int pendingOrders;
    
    // Getters
    int getPendingOrders() const { return pendingOrders; }
    
    // Setters
    void setPendingOrders(int orders) { pendingOrders = orders; }
};

struct Node {
    std::string id;
    NodeType type;
    int maxRobots;
    int currentRobots = 0;
    std::variant<Shelf, LoadingDock, ChargingStation, FrontDesk> data;
    Zone zone = Zone::Other;
    
    // Getters
    std::string getId() const { return id; }
    NodeType getType() const { return type; }
    int getMaxRobots() const { return maxRobots; }
    int getCurrentRobots() const { return currentRobots; }
    Zone getZone() const { return zone; }
    
    // Setters
    void setCurrentRobots(int robots) { currentRobots = robots; }
    void setZone(Zone z) { zone = z; }
    
    // Variant helpers
    bool isShelf() const { return std::holds_alternative<Shelf>(data); }
    bool isLoadingDock() const { return std::holds_alternative<LoadingDock>(data); }
    bool isChargingStation() const { return std::holds_alternative<ChargingStation>(data); }
    bool isFrontDesk() const { return std::holds_alternative<FrontDesk>(data); }
    
    Shelf* getShelf() { 
        return std::get_if<Shelf>(&data); 
    }
    LoadingDock* getLoadingDock() { 
        return std::get_if<LoadingDock>(&data); 
    }
    ChargingStation* getChargingStation() { 
        return std::get_if<ChargingStation>(&data); 
    }
    FrontDesk* getFrontDesk() { 
        return std::get_if<FrontDesk>(&data); 
    }
    
    const Shelf* getShelf() const { 
        return std::get_if<Shelf>(&data); 
    }
    const LoadingDock* getLoadingDock() const { 
        return std::get_if<LoadingDock>(&data); 
    }
    const ChargingStation* getChargingStation() const { 
        return std::get_if<ChargingStation>(&data); 
    }
    const FrontDesk* getFrontDesk() const { 
        return std::get_if<FrontDesk>(&data); 
    }
};

struct TimestepLog {
    double time;
    std::vector<std::tuple<std::string, double, double, int>> robotPositions; 
    std::vector<std::tuple<int, int, std::string>> taskUpdates; 
    
    // Getters
    double getTime() const { return time; }
    const std::vector<std::tuple<std::string, double, double, int>>& getRobotPositions() const { 
        return robotPositions; 
    }
    const std::vector<std::tuple<int, int, std::string>>& getTaskUpdates() const { 
        return taskUpdates; 
    }
};

// Global accessors (wrapper functions för Python)
namespace DataAccess {
    // Node access
    int getNodeCount();
    Node* getNode(int index);
    const Node* getNodeConst(int index);
    
    // Product access
    int getProductCount();
    Product* getProduct(int index);
    const Product* getProductConst(int index);
    
    // Adjacency list access
    int getAdjListSize(int nodeIndex);
    Edge getEdge(int nodeIndex, int edgeIndex);
    
    // Node index getters
    int getLoadingDockNode();
    int getShelfNode(char shelfLetter); // 'A' to 'J'
    int getChargingStationNode();
    int getFrontDeskNode();
}

// External declarations
extern std::vector<Node> nodes;
extern std::vector<std::vector<Edge>> adj;
extern std::vector<Product> products;

extern int loadingDockNode;
extern int shelfANode;
extern int shelfBNode;
extern int shelfCNode;
extern int shelfDNode;
extern int shelfENode;
extern int shelfFNode;
extern int shelfGNode;
extern int shelfHNode;
extern int shelfINode;
extern int shelfJNode;
extern int chargingStationNode;
extern int frontDeskNode;

#endif