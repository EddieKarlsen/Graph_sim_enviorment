#ifndef DATATYPES_HPP
#define DATATYPES_HPP


#include <string>
#include <vector>
#include <variant>

#define MAX_SLOTS 10

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
};


extern std::vector<Node> nodes;
extern std::vector<std::vector<Edge>> adj;
extern std::vector<Product> products;
#endif