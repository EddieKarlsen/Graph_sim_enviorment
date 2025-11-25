#include <iostream>
#include <cmath>
#include "../includes/eventSystem.hpp"
#include "../includes/hotWarmCold.hpp"

std::priority_queue<SimEvent, std::vector<SimEvent>, std::greater<SimEvent>> eventQueue;
std::mt19937 rng;
double currentSimTime = 0.0;

void initEventSystem(unsigned int seed) {
    rng.seed(seed);
    currentSimTime = 0.0;
    
    // Rensa event queue
    while (!eventQueue.empty()) eventQueue.pop();
    
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
        weights.push_back(std::max(1, 10 - p.popularity)); // Lägre popularity = mer påfyllning behövs
    }
    std::discrete_distribution<> prodDist(weights.begin(), weights.end());
    int productIdx = prodDist(rng);
    
    SimEvent event;
    event.type = EventType::IncomingDelivery;
    event.triggerTime = nextTime;
    event.nodeIndex = loadingDockNode;
    event.productID = products[productIdx].id;
    event.quantity = lorrySize;
    
    eventQueue.push(event);
}

void generateCustomerOrder(double currentTime, double avgIntervalMinutes) {
    std::exponential_distribution<> dist(1.0 / (avgIntervalMinutes * 60.0));
    double nextTime = currentTime + dist(rng);
    
    // Populära produkter beställs oftare (Hot zone bias)
    std::vector<double> weights;
    for (const auto& p : products) {
        weights.push_back(p.popularity + 1); // +1 för att undvika 0
    }
    std::discrete_distribution<> prodDist(weights.begin(), weights.end());
    int productIdx = prodDist(rng);
    
    // Slumpa antal (1-5 items)
    std::uniform_int_distribution<> qtyDist(1, 5);
    
    SimEvent event;
    event.type = EventType::CustomerOrder;
    event.triggerTime = nextTime;
    event.nodeIndex = frontDeskNode;
    event.productID = products[productIdx].id;
    event.quantity = qtyDist(rng);
    
    eventQueue.push(event);
}

void scheduleRestockCheck(double currentTime) {
    // Kör restock check varje 30 minuter
    SimEvent event;
    event.type = EventType::RestockNeeded;
    event.triggerTime = currentTime + 1800.0; // 30 min
    event.nodeIndex = -1;
    event.productID = -1;
    event.quantity = 0;
    
    eventQueue.push(event);
}

void handleIncomingDelivery(const SimEvent& event) {
    auto& dockData = std::get<LoadingDock>(nodes[loadingDockNode].data);
    
    if (dockData.isOccupied) {
        // Lastbil måste vänta - återschemalägg om 5 minuter
        SimEvent retry = event;
        retry.triggerTime = currentSimTime + 300.0;
        eventQueue.push(retry);
        return;
    }
    
    // Markera dock som upptagen
    dockData.isOccupied = true;
    dockData.currentLorry = static_cast<Lorry>(event.quantity);
    dockData.deliveryCount++;
    
    std::cout << "[DELIVERY] Product " << event.productID 
              << " x" << event.quantity << " arrived at Loading Dock\n";
    
    // Skapa unloading order för RL-agenten
    // RL-agenten behöver tilldela robot att flytta varorna till rätt hylla
    
    // Schemalägg nästa leverans
    generateIncomingDelivery(currentSimTime);
}

void handleCustomerOrder(const SimEvent& event) {
    auto& deskData = std::get<FrontDesk>(nodes[frontDeskNode].data);
    deskData.pendingOrders++;
    
    std::cout << "[ORDER] Customer ordered Product " << event.productID 
              << " x" << event.quantity << " at Front Desk\n";
    
    // Uppdatera popularity
    updatePopularityAndZone(event.productID);
    
    // Skapa picking order för RL-agenten
    // RL-agenten behöver hitta produkt på hylla och transportera till FrontDesk
    
    // Schemalägg nästa order
    generateCustomerOrder(currentSimTime);
}

void handleRestockNeeded(const SimEvent& event) {
    // Kontrollera alla hyllor för låga lagernivåer
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].type != NodeType::Shelf) continue;
        
        auto& shelfData = std::get<Shelf>(nodes[i].data);
        for (int j = 0; j < shelfData.slotCount; ++j) {
            auto& slot = shelfData.slots[j];
            double fillRate = static_cast<double>(slot.occupied) / slot.capacity;
            
            if (fillRate < 0.3) { // Under 30% kapacitet
                std::cout << "[RESTOCK] " << shelfData.name 
                          << " Slot " << j << " (Product " << slot.productID 
                          << ") needs restocking: " << slot.occupied 
                          << "/" << slot.capacity << "\n";
                
                // RL-agenten kan prioritera påfyllning av denna slot
            }
        }
    }
    
    // Schemalägg nästa check
    scheduleRestockCheck(currentSimTime);
}

void processEvents(double deltaTime) {
    currentSimTime += deltaTime;
    
    // Bearbeta alla events som ska triggas nu
    while (!eventQueue.empty() && eventQueue.top().triggerTime <= currentSimTime) {
        SimEvent event = eventQueue.top();
        eventQueue.pop();
        
        switch (event.type) {
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