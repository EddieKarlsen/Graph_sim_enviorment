#ifndef EVENTSYSTEM_HPP
#define EVENTSYSTEM_HPP

#include "datatypes.hpp"
#include "robot.hpp"
#include <queue>
#include <random>

enum class EventType {
    IncomingDelivery,    // Lastbil anländer till LoadingDock
    CustomerOrder,       // Kund beställer från FrontDesk
    RobotTaskComplete,   // Robot slutför uppgift
    LowBattery,         // Robot behöver laddas
    RestockNeeded       // Produkt har låg stock
};

struct SimEvent {
    EventType type;
    double triggerTime;  // När händelsen ska inträffa
    int nodeIndex;       // Relevant nod (LoadingDock/FrontDesk)
    int productID;       // Relevant produkt
    int quantity;        // Antal items
    
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

#endif