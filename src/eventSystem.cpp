#include <iostream>
#include <cmath>
#include "../includes/eventSystem.hpp"
#include "../includes/hotWarmCold.hpp"
#include "../includes/jsonComm.hpp"

static std::map<int, int> postponeCount;
static std::map<int, double> lastPostponeTime;
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

void handleUrgentRestock(const SimEvent& event) {
    std::cerr << "[URGENT-RESTOCK] Handling urgent restock for Product " 
              << event.getProductID() << "\n";
    
    auto* dockData = nodes[loadingDockNode].getLoadingDock();
    if (!dockData) return;
    
    // Om dock är upptagen, schemalägg om direkt
    if (dockData->getIsOccupied()) {
        std::cerr << "[URGENT-RESTOCK] Loading dock busy - Rescheduling in 30s\n";
        SimEvent retry = event;
        retry.setTriggerTime(currentSimTime + 30.0);
        eventQueue.push(retry);
        return;
    }
    
    // Hitta vilken hylla som har denna produkt (för att restock till samma plats)
    int targetShelfNode = -1;
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].getType() != NodeType::Shelf) continue;
        
        auto* shelfData = nodes[i].getShelf();
        if (!shelfData) continue;
        
        for (int j = 0; j < shelfData->getSlotCount(); ++j) {
            Slot slot = shelfData->getSlot(j);
            if (slot.getProductID() == event.getProductID()) {
                targetShelfNode = i;
                break;
            }
        }
        
        if (targetShelfNode != -1) break;
    }
    
    if (targetShelfNode == -1) {
        std::cerr << "[ERROR] No shelf found for Product " 
                  << event.getProductID() << " - Cannot restock\n";
        return;
    }
    
    // Markera dock som upptagen
    dockData->setIsOccupied(true);
    
    std::cerr << "[URGENT-RESTOCK] Creating high-priority restock task for Product " 
              << event.getProductID() << "\n";
    
    if (globalJsonComm) {
        Task task;
        task.taskId = "urgent_restock_" + std::to_string(taskIdCounter++);
        task.taskType = TaskType::RESTOCK_REQUEST;
        task.productId = event.getProductID();
        task.quantity = event.getQuantity();
        task.sourceNode = loadingDockNode;
        task.targetNode = targetShelfNode;
        task.priority = "urgent";
        task.deadline = currentSimTime + 180.0;  // 3 minuter
        
        globalJsonComm->sendNewTask(task, currentSimTime);
        Action action = globalJsonComm->receiveAction();
        
        if (action.actionType != ActionType::WAIT) {
            std::cerr << "[URGENT-RESTOCK] RL assigned robot " 
                      << action.robotIndex << " for urgent restock\n";
            
            // Uppdatera lagret
            auto* shelfData = nodes[targetShelfNode].getShelf();
            if (shelfData) {
                int slotIndex = -1;
                for (int j = 0; j < shelfData->getSlotCount(); ++j) {
                    Slot slot = shelfData->getSlot(j);
                    if (slot.getProductID() == event.getProductID()) {
                        slotIndex = j;
                        break;
                    }
                }
                
                if (slotIndex != -1) {
                    Slot slot = shelfData->getSlot(slotIndex);
                    int newOccupied = std::min(
                        slot.getOccupied() + event.getQuantity(),
                        slot.getCapacity()
                    );
                    shelfData->setSlotOccupied(slotIndex, newOccupied);
                    
                    std::cerr << "[URGENT-RESTOCK] Restocked " 
                              << event.getQuantity() << " units - "
                              << slot.getOccupied() << " -> " << newOccupied << "\n";
                }
            }
            
            dockData->setIsOccupied(false);
            globalJsonComm->sendAck(task.taskId, action.robotIndex, 
                                   currentSimTime + 60.0);
        } else {
            std::cerr << "[URGENT-RESTOCK] RL rejected - Rescheduling in 60s\n";
            dockData->setIsOccupied(false);
            
            SimEvent retry = event;
            retry.setTriggerTime(currentSimTime + 60.0);
            eventQueue.push(retry);
        }
    }
}

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
    event.setTriggerTime(currentTime + 1800); // 30 min
    event.setNodeIndex(-1);
    event.setProductID(-1);
    event.setQuantity(0);
    
    eventQueue.push(event);
}

void handleIncomingDelivery(const SimEvent& event) {
    // Tracking
    if (lastDeliveryTime > 0.0) 
    {
        deliveryIntervals.push_back(event.getTriggerTime() - lastDeliveryTime);
    }
    lastDeliveryTime = event.getTriggerTime();
    totalDeliveries++;
    
    // Original handler logic
    auto* dockData = nodes[loadingDockNode].getLoadingDock();
    if (!dockData) return;
    
    if (dockData->getIsOccupied()) 
    {
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
    
    std::cerr << "[DELIVERY] Product " << event.getProductID() 
              << " x" << event.getQuantity() << " arrived at Loading Dock\n";
    
    // Skapa Task för RL-agenten
    if (globalJsonComm) 
    {
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
    
    if (action.actionType != ActionType::WAIT) 
    {
        std::cerr << "[SIM] RL assigned robot " << action.robotIndex 
                << " to restock to node " << action.targetNode << "\n";
        
        // Uppdatera hyllans lager
        auto* shelfData = nodes[action.targetNode].getShelf();
        if (shelfData) {
            int slotIndex = -1;
            for (int j = 0; j < shelfData->getSlotCount(); ++j) {
                Slot slot = shelfData->getSlot(j);
                if (slot.getProductID() == event.getProductID()) {
                    slotIndex = j;
                    break;
                }
            }
            
            if (slotIndex != -1) {
                Slot slot = shelfData->getSlot(slotIndex);
                int newOccupied = std::min(
                    slot.getOccupied() + event.getQuantity(),
                    slot.getCapacity()
                );
                shelfData->setSlotOccupied(slotIndex, newOccupied);
                
                std::cerr << "[INVENTORY] Restocked Shelf " 
                        << nodes[action.targetNode].getId() 
                        << " Slot " << slotIndex 
                        << " Product " << event.getProductID()
                        << ": " << slot.getOccupied() << " -> " << newOccupied << "\n";
            } else {
                std::cerr << "[ERROR] Product " << event.getProductID() 
                        << " not found on target shelf!\n";
            }
        }
        // Frigör loading dock
        dockData->setIsOccupied(false);
        
        globalJsonComm->sendAck(task.taskId, action.robotIndex, 
                            currentSimTime + 60.0);
    }else{
            std::cerr << "[DELIVERY] RL cannot handle delivery - Postponing\n";
            
            // Frigör dock
            dockData->setIsOccupied(false);
            
            // Återschemalägg om 2 minuter
            SimEvent retry = event;
            retry.setTriggerTime(currentSimTime + 120.0);
            eventQueue.push(retry);
        }
    }

    generateIncomingDelivery(currentSimTime);
}

void handleCustomerOrder(const SimEvent& event) {
    // Tracking
    if (lastOrderTime > 0.0) {
        orderIntervals.push_back(event.getTriggerTime() - lastOrderTime);
    }
    lastOrderTime = event.getTriggerTime();
    totalOrders++;
    
    auto* deskData = nodes[frontDeskNode].getFrontDesk();
    if (!deskData) return;
    
    deskData->setPendingOrders(deskData->getPendingOrders() + 1);
    
    std::cerr << "[ORDER] Customer ordered Product " << event.getProductID() 
              << " x" << event.getQuantity() << " at Front Desk\n";
    
    // Kolla om ordern har försökt för många gånger
    int productId = event.getProductID();
    if (postponeCount[productId] >= 10) {
        std::cerr << "[ORDER] CANCELLED - Product " << productId 
                  << " unavailable after " << postponeCount[productId] 
                  << " attempts\n";
        
        postponeCount[productId] = 0;
        deskData->setPendingOrders(deskData->getPendingOrders() - 1);
        generateCustomerOrder(currentSimTime);
        return;
    }
    
    // Hitta produkten på hyllan
    int availableQuantity = 0;
    int sourceShelfNode = -1;
    int sourceSlotIndex = -1;

    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].getType() != NodeType::Shelf) continue;
        
        auto* shelfData = nodes[i].getShelf();
        if (!shelfData) continue;
        
        for (int j = 0; j < shelfData->getSlotCount(); ++j) {
            Slot slot = shelfData->getSlot(j);
            
            if (slot.getProductID() == event.getProductID() && 
                slot.getOccupied() >= event.getQuantity()) {
                availableQuantity = slot.getOccupied();
                sourceShelfNode = i;
                sourceSlotIndex = j;
                break;
            }
        }
        
        if (sourceShelfNode != -1) break;
    }
    
    // Om produkten INTE finns
    if (sourceShelfNode == -1) {
        postponeCount[productId]++;
        lastPostponeTime[productId] = currentSimTime;
        
        int attempts = postponeCount[productId];
        double delay = 30.0 * std::pow(2.0, std::min(attempts - 1, 4));
        
        std::cerr << "[ORDER] Product " << productId 
                  << " x" << event.getQuantity() 
                  << " NOT AVAILABLE (attempt " << attempts 
                  << ") - Postponing for " << delay << "s\n";
        
        if (attempts == 3) {
            std::cerr << "[URGENT] Product " << productId 
                      << " postponed " << attempts 
                      << " times - Scheduling URGENT restock event\n";
            
            SimEvent urgentRestock;
            urgentRestock.setType(EventType::UrgentRestock);
            urgentRestock.setTriggerTime(currentSimTime + 1.0);
            urgentRestock.setNodeIndex(-1);
            urgentRestock.setProductID(productId);
            urgentRestock.setQuantity(30);
            
            eventQueue.push(urgentRestock);
        }
        
        // Återschemalägg order med exponentiell backoff
        SimEvent retry = event;
        retry.setTriggerTime(currentSimTime + delay);
        eventQueue.push(retry);
        
        deskData->setPendingOrders(deskData->getPendingOrders() - 1);
        generateCustomerOrder(currentSimTime);
        return;
    }
    
    // Produkten finns! Nollställ postpone counter
    postponeCount[productId] = 0;
    
    // RESERVERA produkten INNAN RL-call
    auto* shelfData = nodes[sourceShelfNode].getShelf();
    if (shelfData) {
        Slot slot = shelfData->getSlot(sourceSlotIndex);
        int newOccupied = slot.getOccupied() - event.getQuantity();
        
        if (newOccupied < 0) {
            std::cerr << "[ERROR] Race condition detected! Postponing.\n";
            SimEvent retry = event;
            retry.setTriggerTime(currentSimTime + 10.0);
            eventQueue.push(retry);
            deskData->setPendingOrders(deskData->getPendingOrders() - 1);
            generateCustomerOrder(currentSimTime);
            return;
        }
        
        shelfData->setSlotOccupied(sourceSlotIndex, newOccupied);
        
        std::cerr << "[INVENTORY] Reserved from Shelf " 
                  << nodes[sourceShelfNode].getId() 
                  << " Slot " << sourceSlotIndex 
                  << " Product " << event.getProductID()
                  << ": " << slot.getOccupied() << " -> " << newOccupied << "\n";
    }
    
    updatePopularityAndZone(event.getProductID());
    
    // Skicka task till RL-agenten
    if (globalJsonComm) {
        Task task;
        task.taskId = "order_" + std::to_string(taskIdCounter++);
        task.taskType = TaskType::CUSTOMER_ORDER;
        task.productId = event.getProductID();
        task.quantity = event.getQuantity();
        task.sourceNode = sourceShelfNode;
        task.targetNode = frontDeskNode;
        task.priority = "normal";
        task.deadline = currentSimTime + 300.0;
        
        globalJsonComm->sendNewTask(task, currentSimTime);
        Action action = globalJsonComm->receiveAction();
        
        if (action.actionType != ActionType::WAIT) {
            std::cerr << "[SIM] RL assigned robot " << action.robotIndex 
                      << " to deliver Product " << event.getProductID()
                      << " from Shelf " << nodes[sourceShelfNode].getId()
                      << " to Front Desk\n";
            
            globalJsonComm->sendAck(task.taskId, action.robotIndex, 
                                   currentSimTime + 45.0);
        } else {
            std::cerr << "[ORDER] RL rejected task - Unreserving products\n";
            
            if (shelfData) {
                Slot slot = shelfData->getSlot(sourceSlotIndex);
                shelfData->setSlotOccupied(sourceSlotIndex, 
                    slot.getOccupied() + event.getQuantity());
                
                std::cerr << "[INVENTORY] Unreserved " << event.getQuantity() 
                          << " units back to " 
                          << (slot.getOccupied() + event.getQuantity()) << "\n";
            }
            
            SimEvent retry = event;
            retry.setTriggerTime(currentSimTime + 30.0);
            eventQueue.push(retry);
            deskData->setPendingOrders(deskData->getPendingOrders() - 1);
        }
    }
    
    generateCustomerOrder(currentSimTime);
}

void handleRestockNeeded(const SimEvent& event) {
    totalRestockChecks++;
    
    std::cerr << "[RESTOCK-CHECK] Checking all shelves for low stock...\n";
    
    int restockTasksCreated = 0;
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].getType() != NodeType::Shelf) continue;
        
        auto* shelfData = nodes[i].getShelf();
        if (!shelfData) continue;
        
        for (int j = 0; j < shelfData->getSlotCount(); ++j) {
            Slot slot = shelfData->getSlot(j);

            if (slot.getProductID() < 0 || slot.getCapacity() <= 0) {
                continue;
            }

            double fillRate = static_cast<double>(slot.getOccupied()) / slot.getCapacity();
            
            // Olika tröskelvärden baserat på popularity
            double threshold = 0.3;  // Default
            
            // Hitta produktens popularity
            for (const auto& p : products) {
                if (p.getId() == slot.getProductID()) {
                    if (p.getPopularity() >= 5) {
                        threshold = 0.5;  // Högt threshold för populära produkter
                    } else if (p.getPopularity() >= 3) {
                        threshold = 0.4;
                    }
                    break;
                }
            }
            
            if (fillRate < threshold && slot.getOccupied() < slot.getCapacity()) {
                int qtyToRestock = slot.getCapacity() - slot.getOccupied();
                
                // Beställ mer för populära produkter
                if (fillRate < 0.1) {  // Nästan slut!
                    qtyToRestock = slot.getCapacity();  // Fyll helt
                }
                
                if (qtyToRestock > 0) {
                    std::cerr << "[RESTOCK] Shelf " << nodes[i].getId() 
                              << " Slot " << j << " Product " << slot.getProductID()
                              << " needs " << qtyToRestock 
                              << " units (fill rate: " << (fillRate * 100) << "%)\n";
                    
                    Task task;
                    task.taskId = "restock_" + std::to_string(taskIdCounter++);
                    task.taskType = TaskType::RESTOCK_REQUEST;
                    task.productId = slot.getProductID();
                    task.quantity = qtyToRestock;
                    task.sourceNode = loadingDockNode;
                    task.targetNode = i;
                    task.priority = (fillRate < 0.1) ? "high" : "low";
                    task.deadline = currentSimTime + 900.0;  // 15 minuter
                    
                    globalJsonComm->sendNewTask(task, currentSimTime);
                    restockTasksCreated++;

                }
            }
        }
    }
    
    std::cerr << "[RESTOCK-CHECK] Created " << restockTasksCreated << " restock tasks\n";
    
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
            case EventType::UrgentRestock:
                handleUrgentRestock(event);
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