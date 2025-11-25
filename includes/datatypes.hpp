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

struct Product
{
    int id;
    std::string name;
    int popularity = 0;
};

struct Slot {
    int occupied;       
    int productID; 
    int capacity;     
};

struct Shelf {
    std::string name;
    struct Slot slots[MAX_SLOTS];
    int slotCount;
};

struct LoadingDock {
    bool isOccupied;
    int deliveryCount;
    Lorry currentLorry;
};

struct ChargingStation {
    int isOccupied;
    int chargingPorts;
};;

struct FrontDesk {
    int pendingOrders;
};

struct Node
{
    std::string id;
        NodeType type;
        int maxRobots;
        int currentRobots = 0;
        std::variant<Shelf, LoadingDock, ChargingStation, FrontDesk> data;
        Zone zone = Zone::Other;
};

// I datatypes.hpp (eller en ny loggfil)
struct TimestepLog {
    double time; // Tid sedan episodstart
    
    // För Heatmap/Rörelse
    std::vector<std::tuple<std::string, double, double, int>> robotPositions; 
    // std::tuple<Robot ID, X-pos, Y-pos, current Node Index>

    // För Uppgiftsspårning
    std::vector<std::tuple<int, int, std::string>> taskUpdates; 
    // std::tuple<Robot Index, Order ID, Status (Start, Picked, Drop-off)>
};





//external deklarations

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