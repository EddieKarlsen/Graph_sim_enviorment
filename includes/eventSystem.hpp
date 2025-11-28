#ifndef EVENTSYSTEM_HPP
#define EVENTSYSTEM_HPP

#include "datatypes.hpp"
#include "robot.hpp"
#include <queue>
#include <random>

enum class EventType {
    IncomingDelivery,
    CustomerOrder,
    RobotTaskComplete,
    LowBattery,
    RestockNeeded,
    UrgentRestock
};

struct SimEvent {
    EventType type;
    double triggerTime;
    int nodeIndex;
    int productID;
    int quantity;
    
    // Getters
    EventType getType() const { return type; }
    double getTriggerTime() const { return triggerTime; }
    int getNodeIndex() const { return nodeIndex; }
    int getProductID() const { return productID; }
    int getQuantity() const { return quantity; }
    
    // Setters
    void setType(EventType t) { type = t; }
    void setTriggerTime(double time) { triggerTime = time; }
    void setNodeIndex(int idx) { nodeIndex = idx; }
    void setProductID(int pid) { productID = pid; }
    void setQuantity(int qty) { quantity = qty; }
    
    // Utility
    std::string getTypeString() const {
        switch(type) {
            case EventType::IncomingDelivery: return "IncomingDelivery";
            case EventType::CustomerOrder: return "CustomerOrder";
            case EventType::RobotTaskComplete: return "RobotTaskComplete";
            case EventType::LowBattery: return "LowBattery";
            case EventType::RestockNeeded: return "RestockNeeded";
            default: return "Unknown";
        }
    }
    
    bool operator>(const SimEvent& other) const {
        return triggerTime > other.triggerTime;
    }
};

// Global event queue (prioriterad efter tid)
extern std::priority_queue<SimEvent, std::vector<SimEvent>, std::greater<SimEvent>> eventQueue;
extern std::mt19937 rng;
extern double currentSimTime;

// Event generation functions
void generateIncomingDelivery(double currentTime, double avgIntervalHours = 2.0);
void generateCustomerOrder(double currentTime, double avgIntervalMinutes = 5.0);
void scheduleRestockCheck(double currentTime);

// Event handlers
void handleIncomingDelivery(const SimEvent& event);
void handleCustomerOrder(const SimEvent& event);
void handleRestockNeeded(const SimEvent& event);

// Initialize event system
void initEventSystem(unsigned int seed = 42);
void processEvents(double deltaTime);

// Event system accessors for Python
namespace EventSystemAccess {
    double getCurrentSimTime();
    void setCurrentSimTime(double time);
    int getQueueSize();
    bool hasNextEvent();
    double getNextEventTime();
    SimEvent peekNextEvent();  // Get next event without removing it
    
    // Statistics
    struct EventStats {
        int totalDeliveries;
        int totalOrders;
        int totalRestockChecks;
        double avgDeliveryInterval;
        double avgOrderInterval;
        
        // Getters
        int getTotalDeliveries() const { return totalDeliveries; }
        int getTotalOrders() const { return totalOrders; }
        int getTotalRestockChecks() const { return totalRestockChecks; }
        double getAvgDeliveryInterval() const { return avgDeliveryInterval; }
        double getAvgOrderInterval() const { return avgOrderInterval; }
    };
    
    EventStats getEventStats();
    void resetEventStats();
}

#endif