#include <iostream>
#include <cmath>
#include "../includes/eventSystem.hpp"
#include "../includes/hotWarmCold.hpp"
#include "../includes/jsonComm.hpp"

std::priority_queue<SimEvent, std::vector<SimEvent>, std::greater<SimEvent>> eventQueue;
std::mt19937 rng;
double currentSimTime = 0.0;

// Static variables för statistik
static int totalDeliveries = 0;
static int totalOrders = 0;
static int totalRestockChecks = 0;
static std::vector<double> deliveryIntervals;
static std::vector<double> orderIntervals;
static double lastDeliveryTime = 0.0;
static double lastOrderTime = 0.0;

// Task ID counter
static int taskIdCounter = 0;

void initEventSystem(unsigned int seed) {
    rng.seed(seed);
    currentSimTime = 0.0;
    taskIdCounter = 0;
    
    // Rensa event queue
    while (!eventQueue.empty()) eventQueue.pop();
    
    // Återställ statistik
    EventSystemAccess::resetEventStats();
    
    // Schemalägg första event av varje typ
    generateIncomingDelivery(0.0);
    generateCustomerOrder(0.0);
    scheduleRestockCheck(0.0);
}

void generateIncomingDelivery(double currentTime, double avgIntervalHours) {
    // Exponentialfördelning för realistiska ankomster
    std::exponential_distribution<> dist(1.0 / (avgIntervalHours * 3600.0));
    double nextTime = currentTime + dist(rng);
    
    // Slumpa lastbilsstorlek
    std::uniform_int_distribution<> lorryDist(0, 2);
    int lorrySize;
    switch(lorryDist(rng)) {
        case 0: lorrySize = Lorry::SMALL_LORRY; break;
        case 1: lorrySize = Lorry::MEDIUM_LORRY; break;
        case 2: lorrySize = Lorry::BIG_LORRY; break;
    }
    
    // Slumpa vilken produkt som levereras (populära produkter beställs oftare)
    std::vector<double> weights;
    for (const auto& p : products) {
        weights.push_back(std::max(1, 10 - p.getPopularity()));
    }
    std::discrete_distribution<> prodDist(weights.begin(), weights.end());
    int productIdx = prodDist(rng);
    
    SimEvent event;
    event.setType(EventType::IncomingDelivery);
    event.setTriggerTime(nextTime);
    event.setNodeIndex(loadingDockNode);
    event.setProductID(products[productIdx].getId());
    event.setQuantity(lorrySize);
    
    eventQueue.push(event);
}

void generateCustomerOrder(double currentTime, double avgIntervalMinutes) {
    std::exponential_distribution<> dist(1.0 / (avgIntervalMinutes * 60.0));
    double nextTime = currentTime + dist(rng);
    
    // Populära produkter beställs oftare (Hot zone bias)
    std::vector<double> weights;
    for (const auto& p : products) {
        weights.push_back(p.getPopularity() + 1);
    }
    std::discrete_distribution<> prodDist(weights.begin(), weights.end());
    int productIdx = prodDist(rng);
    
    // Slumpa antal (1-5 items)
    std::uniform_int_distribution<> qtyDist(1, 5);
    
    SimEvent event;
    event.setType(EventType::CustomerOrder);
    event.setTriggerTime(nextTime);
    event.setNodeIndex(frontDeskNode);
    event.setProductID(products[productIdx].getId());
    event.setQuantity(qtyDist(rng));
    
    eventQueue.push(event);
}

void scheduleRestockCheck(double currentTime) {
    SimEvent event;
    event.setType(EventType::RestockNeeded);
    event.setTriggerTime(currentTime + 1800.0); // 30 min
    event.setNodeIndex(-1);
    event.setProductID(-1);
    event.setQuantity(0);
    
    eventQueue.push(event);
}

void handleIncomingDelivery(const SimEvent& event) {
    // Tracking
    if (lastDeliveryTime > 0.0) {
        deliveryIntervals.push_back(event.getTriggerTime() - lastDeliveryTime);
    }
    lastDeliveryTime = event.getTriggerTime();
    totalDeliveries++;
    
    // Original handler logic
    auto* dockData = nodes[loadingDockNode].getLoadingDock();
    if (!dockData) return;
    
    if (dockData->getIsOccupied()) {
        // Lastbil måste vänta - återschemalägg om 5 minuter
        SimEvent retry = event;
        retry.setTriggerTime(currentSimTime + 300.0);
        eventQueue.push(retry);
        return;
    }
    
    // Markera dock som upptagen
    dockData->setIsOccupied(true);
    dockData->setCurrentLorry(static_cast<Lorry>(event.getQuantity()));
    dockData->setDeliveryCount(dockData->getDeliveryCount() + 1);
    
    std::cout << "[DELIVERY] Product " << event.getProductID() 
              << " x" << event.getQuantity() << " arrived at Loading Dock\n";
    
    // Skapa Task för RL-agenten
    if (globalJsonComm) {
        Task task;
        task.taskId = "delivery_" + std::to_string(taskIdCounter++);
        task.taskType = TaskType::INCOMING_DELIVERY;
        task.productId = event.getProductID();
        task.quantity = event.getQuantity();
        task.sourceNode = loadingDockNode;
        task.targetNode = -1; // RL väljer hylla
        task.priority = "normal";
        task.deadline = currentSimTime + 600.0; // 10 minuter
        
        // Skicka till RL
        globalJsonComm->sendNewTask(task, currentSimTime);
        
        // Vänta på beslut från RL
        Action action = globalJsonComm->receiveAction();
        
        if (action.actionType != ActionType::WAIT) {
            // RL har valt en robot och hylla
            std::cout << "[SIM] RL assigned robot " << action.robotIndex 
                      << " to restock to node " << action.targetNode << "\n";
            
            // Skicka ACK
            globalJsonComm->sendAck(task.taskId, action.robotIndex, 
                                   currentSimTime + 60.0);
            
            // Här skulle du köra din step_simulation för att utföra restock
            // step_simulation(action.robotIndex, RESTOCK_ACTION, action.targetNode, action.productId);
        }
    }
    
    // Schemalägg nästa leverans
    generateIncomingDelivery(currentSimTime);
}

void handleCustomerOrder(const SimEvent& event) {
    // Tracking
    if (lastOrderTime > 0.0) {
        orderIntervals.push_back(event.getTriggerTime() - lastOrderTime);
    }
    lastOrderTime = event.getTriggerTime();
    totalOrders++;
    
    // Original handler logic
    auto* deskData = nodes[frontDeskNode].getFrontDesk();
    if (!deskData) return;
    
    deskData->setPendingOrders(deskData->getPendingOrders() + 1);
    
    std::cout << "[ORDER] Customer ordered Product " << event.getProductID() 
              << " x" << event.getQuantity() << " at Front Desk\n";
    
    // Uppdatera popularity
    updatePopularityAndZone(event.getProductID());
    
    // Skapa Task för RL-agenten
    if (globalJsonComm) {
        Task task;
        task.taskId = "order_" + std::to_string(taskIdCounter++);
        task.taskType = TaskType::CUSTOMER_ORDER;
        task.productId = event.getProductID();
        task.quantity = event.getQuantity();
        task.sourceNode = -1; // RL hittar produkten på hylla
        task.targetNode = frontDeskNode;
        task.priority = "normal";
        task.deadline = currentSimTime + 300.0; // 5 minuter
        
        // Skicka till RL
        globalJsonComm->sendNewTask(task, currentSimTime);
        
        // Vänta på beslut från RL
        Action action = globalJsonComm->receiveAction();
        
        if (action.actionType != ActionType::WAIT) {
            // RL har valt en robot
            std::cout << "[SIM] RL assigned robot " << action.robotIndex 
                      << " to pick from node " << action.sourceNode 
                      << " and deliver to Front Desk\n";
            
            // Skicka ACK
            globalJsonComm->sendAck(task.taskId, action.robotIndex, 
                                   currentSimTime + 45.0);
            
            // Här skulle du köra din step_simulation för att utföra pickup & delivery
            // step_simulation(action.robotIndex, PICKUP_ACTION, action.sourceNode, action.productId);
            // ... sedan DELIVER_ACTION till frontDeskNode
            
            // När robot är färdig, skicka status
            // globalJsonComm->sendRobotStatus(action.robotIndex, StatusType::TASK_COMPLETE,
            //                                task.taskId, currentSimTime, "Order completed");
        }
    }
    
    // Schemalägg nästa order
    generateCustomerOrder(currentSimTime);
}

void handleRestockNeeded(const SimEvent& event) {
    // Tracking
    totalRestockChecks++;
    
    // Original handler logic
    // Kontrollera alla hyllor för låga lagernivåer
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].getType() != NodeType::Shelf) continue;
        
        auto* shelfData = nodes[i].getShelf();
        if (!shelfData) continue;
        
        for (int j = 0; j < shelfData->getSlotCount(); ++j) {
            Slot slot = shelfData->getSlot(j);
            double fillRate = static_cast<double>(slot.getOccupied()) / slot.getCapacity();
            
            if (fillRate < 0.3) { // Under 30% kapacitet
                std::cout << "[RESTOCK] " << shelfData->getName() 
                          << " Slot " << j << " (Product " << slot.getProductID() 
                          << ") needs restocking: " << slot.getOccupied() 
                          << "/" << slot.getCapacity() << "\n";
                
                // Skapa restock task för RL (om vi har varorna på loading dock)
                if (globalJsonComm) {
                    Task task;
                    task.taskId = "restock_" + std::to_string(taskIdCounter++);
                    task.taskType = TaskType::RESTOCK_REQUEST;
                    task.productId = slot.getProductID();
                    task.quantity = slot.getCapacity() - slot.getOccupied();
                    task.sourceNode = loadingDockNode;
                    task.targetNode = i; // Shelf node
                    task.priority = "low";
                    task.deadline = currentSimTime + 1800.0; // 30 minuter
                    
                    // Skicka till RL (asynkront, RL kan prioritera)
                    globalJsonComm->sendNewTask(task, currentSimTime);
                    
                    // För restock kan vi vänta på beslut i nästa iteration
                }
            }
        }
    }
    
    // Schemalägg nästa check
    scheduleRestockCheck(currentSimTime);
}

void processEvents(double deltaTime) {
    currentSimTime += deltaTime;
    
    // Apply popularity decay
    applyPopularityDecay(currentSimTime);
    
    // Bearbeta alla events som ska triggas nu
    while (!eventQueue.empty() && eventQueue.top().getTriggerTime() <= currentSimTime) {
        SimEvent event = eventQueue.top();
        eventQueue.pop();
        
        switch (event.getType()) {
            case EventType::IncomingDelivery:
                handleIncomingDelivery(event);
                break;
            case EventType::CustomerOrder:
                handleCustomerOrder(event);
                break;
            case EventType::RestockNeeded:
                handleRestockNeeded(event);
                break;
            default:
                break;
        }
    }
}

// Implementation av EventSystemAccess namespace
namespace EventSystemAccess {
    double getCurrentSimTime() {
        return currentSimTime;
    }
    
    void setCurrentSimTime(double time) {
        currentSimTime = time;
    }
    
    int getQueueSize() {
        return static_cast<int>(eventQueue.size());
    }
    
    bool hasNextEvent() {
        return !eventQueue.empty();
    }
    
    double getNextEventTime() {
        if (!eventQueue.empty()) {
            return eventQueue.top().getTriggerTime();
        }
        return -1.0;
    }
    
    SimEvent peekNextEvent() {
        if (!eventQueue.empty()) {
            return eventQueue.top();
        }
        SimEvent empty;
        empty.setType(EventType::RestockNeeded);
        empty.setTriggerTime(-1.0);
        empty.setNodeIndex(-1);
        empty.setProductID(-1);
        empty.setQuantity(0);
        return empty;
    }
    
    EventStats getEventStats() {
        EventStats stats;
        stats.totalDeliveries = totalDeliveries;
        stats.totalOrders = totalOrders;
        stats.totalRestockChecks = totalRestockChecks;
        
        // Beräkna genomsnittliga intervall
        if (!deliveryIntervals.empty()) {
            double sum = 0.0;
            for (double interval : deliveryIntervals) {
                sum += interval;
            }
            stats.avgDeliveryInterval = sum / deliveryIntervals.size();
        } else {
            stats.avgDeliveryInterval = 0.0;
        }
        
        if (!orderIntervals.empty()) {
            double sum = 0.0;
            for (double interval : orderIntervals) {
                sum += interval;
            }
            stats.avgOrderInterval = sum / orderIntervals.size();
        } else {
            stats.avgOrderInterval = 0.0;
        }
        
        return stats;
    }
    
    void resetEventStats() {
        totalDeliveries = 0;
        totalOrders = 0;
        totalRestockChecks = 0;
        deliveryIntervals.clear();
        orderIntervals.clear();
        lastDeliveryTime = 0.0;
        lastOrderTime = 0.0;
    }
}