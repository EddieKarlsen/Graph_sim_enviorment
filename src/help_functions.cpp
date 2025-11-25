#include "../includes/datatypes.hpp"
#include "../includes/robot.hpp"
#include "../includes/eventSystem.hpp"
#include "../includes/hotWarmCold.hpp"
#include "../includes/logger.hpp"
#include <iostream>
#include <cmath>
#include <map>

// Global robot vector
std::vector<Robot> robots;

// Hjälpfunktioner
double calculateDistance(int nodeA, int nodeB) {
    // Simplified - skulle kunna använda Dijkstra
    // För nu returnerar vi edge-avståndet direkt om det finns
    for (const auto& edge : adj[nodeA]) {
        if (edge.to == nodeB) {
            return edge.distance;
        }
    }
    return 100.0; // Large distance if no direct edge
}

bool isRobotAtNode(int robotIdx, int nodeIdx) {
    return robots[robotIdx].currentNode == nodeIdx && robots[robotIdx].progress >= 1.0;
}

// Hitta närmaste produkt på hyllor
int findProductOnShelf(int productID, int& outSlotIndex) {
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].type != NodeType::Shelf) continue;
        
        auto& shelfData = std::get<Shelf>(nodes[i].data);
        for (int j = 0; j < shelfData.slotCount; ++j) {
            if (shelfData.slots[j].productID == productID && 
                shelfData.slots[j].occupied > 0) {
                outSlotIndex = j;
                return i; // Return shelf node index
            }
        }
    }
    return -1; // Not found
}

// Hitta bästa platsen att placera produkt (Hot/Warm/Cold optimization)
int findBestShelfForProduct(int productID) {
    // Hämta produktpopularitet
    auto it = std::find_if(products.begin(), products.end(),
                           [productID](const Product& p) { return p.id == productID; });
    
    if (it == products.end()) return -1;
    
    int popularity = it->popularity;
    Zone targetZone;
    
    // Klassificera baserat på popularity
    if (popularity >= 10) {
        targetZone = Zone::Hot;
    } else if (popularity >= 5) {
        targetZone = Zone::Warm;
    } else {
        targetZone = Zone::Cold;
    }
    
    // Hitta hylla i rätt zon med ledigt utrymme
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].type != NodeType::Shelf) continue;
        if (nodes[i].zone != targetZone) continue;
        
        auto& shelfData = std::get<Shelf>(nodes[i].data);
        for (int j = 0; j < shelfData.slotCount; ++j) {
            if (shelfData.slots[j].productID == productID &&
                shelfData.slots[j].occupied < shelfData.slots[j].capacity) {
                return i; // Found suitable shelf
            }
        }
    }
    
    // Om ingen perfekt match, returnera första lediga
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].type != NodeType::Shelf) continue;
        
        auto& shelfData = std::get<Shelf>(nodes[i].data);
        for (int j = 0; j < shelfData.slotCount; ++j) {
            if (shelfData.slots[j].occupied < shelfData.slots[j].capacity) {
                return i;
            }
        }
    }
    
    return -1;
}

// Huvudfunktion för step simulation
std::map<std::string, double> step_simulation(
    int robotIdx, 
    int actionType, 
    int targetNode, 
    int productID
) {
    std::map<std::string, double> result;
    result["order_completed"] = 0;
    result["order_failed"] = 0;
    result["battery_used"] = 0;
    result["charging_optimal"] = 0;
    result["handover_success"] = 0;
    result["distance_saved"] = 0;
    result["optimal_zone_placement"] = 0;
    result["robot_idle"] = 0;
    result["blocked"] = 0;
    result["completion_time"] = 0;
    
    if (robotIdx < 0 || robotIdx >= static_cast<int>(robots.size())) {
        std::cerr << "Invalid robot index\n";
        result["order_failed"] = 1;
        return result;
    }
    
    Robot& robot = robots[robotIdx];
    
    // Handle olika action types
    switch (actionType) {
        case 0: { // MOVE
            if (targetNode < 0 || targetNode >= static_cast<int>(nodes.size())) {
                result["order_failed"] = 1;
                break;
            }
            
            // Check if node is full
            if (nodes[targetNode].currentRobots >= nodes[targetNode].maxRobots) {
                result["blocked"] = 1;
                break;
            }
            
            // Calculate distance and battery cost
            double distance = calculateDistance(robot.currentNode, targetNode);
            double batteryUsed = distance * 0.5; // 0.5% per meter
            
            if (robot.battery < batteryUsed) {
                result["order_failed"] = 1;
                std::cout << "Robot " << robotIdx << " out of battery\n";
                break;
            }
            
            // Update robot state
            nodes[robot.currentNode].currentRobots--;
            robot.currentNode = targetNode;
            nodes[robot.currentNode].currentRobots++;
            robot.battery -= batteryUsed;
            robot.status = RobotStatus::Idle;
            
            result["battery_used"] = batteryUsed;
            
            if (globalLogger != nullptr) {
                logTask(currentSimTime, robotIdx, "MOVE", -1,robot.currentNode, targetNode, distance);
            }
            break;
        }
        
        case 1: { // PICKUP
            if (!isRobotAtNode(robotIdx, targetNode)) {
                result["order_failed"] = 1;
                break;
            }
            
            if (robot.carrying) {
                result["order_failed"] = 1;
                std::cout << "Robot already carrying item\n";
                break;
            }
            
            // Find product on shelf
            int slotIndex;
            int shelfNode = findProductOnShelf(productID, slotIndex);
            
            if (shelfNode == -1 || shelfNode != targetNode) {
                result["order_failed"] = 1;
                std::cout << "Product " << productID << " not found at node " << targetNode << "\n";
                break;
            }
            
            // Pick up item
            auto& shelfData = std::get<Shelf>(nodes[shelfNode].data);
            shelfData.slots[slotIndex].occupied--;
            
            robot.carrying = true;
            robot.currentOrder.productID = productID;
            robot.currentOrder.slotIndex = slotIndex;
            robot.status = RobotStatus::Carrying;
            
            if (globalLogger != nullptr) {
                logTask(currentSimTime, robotIdx, "PICKUP", productID, shelfNode, shelfNode, 0.0);
            }
            break;
        }
        
        case 2: { // DROPOFF
            if (!robot.carrying) {
                result["order_failed"] = 1;
                break;
            }
            
            if (!isRobotAtNode(robotIdx, targetNode)) {
                result["order_failed"] = 1;
                break;
            }
            
            // Check if dropping at FrontDesk (customer order) or Shelf (restocking)
            if (nodes[targetNode].type == NodeType::FrontDesk) {
                // Customer order completed
                auto& deskData = std::get<FrontDesk>(nodes[targetNode].data);
                if (deskData.pendingOrders > 0) {
                    deskData.pendingOrders--;
                }
                
                result["order_completed"] = 1;
                updatePopularityAndZone(robot.currentOrder.productID);
                
                std::cout << "Robot " << robotIdx << " completed customer order\n";
                
            } else if (nodes[targetNode].type == NodeType::Shelf) {
                // Restocking
                int bestShelf = findBestShelfForProduct(robot.currentOrder.productID);
                
                if (bestShelf == targetNode) {
                    result["optimal_zone_placement"] = 1;
                    std::cout << "Optimal zone placement!\n";
                }
                
                auto& shelfData = std::get<Shelf>(nodes[targetNode].data);
                for (int i = 0; i < shelfData.slotCount; ++i) {
                    if (shelfData.slots[i].productID == robot.currentOrder.productID) {
                        shelfData.slots[i].occupied++;
                        break;
                    }
                }
                
                result["order_completed"] = 1;
            if (globalLogger != nullptr) {
                    logTask(currentSimTime, robotIdx, "DROPOFF", robot.currentOrder.productID, targetNode, targetNode, 0.0);
                }
            }
            
            // Reset robot
            robot.carrying = false;
            robot.currentOrder = Order();
            robot.status = RobotStatus::Idle;
            
            break;
        }
        
        case 3: { // CHARGE
            if (robot.currentNode != chargingStationNode) {
                // Robot not at charging station - need to move there first
                result["order_failed"] = 1;
                break;
            }
            
            auto& chargeData = std::get<ChargingStation>(nodes[chargingStationNode].data);
            
            if (chargeData.isOccupied >= chargeData.chargingPorts) {
                result["blocked"] = 1;
                break;
            }
            
            // Charge robot (10% per step, simplified)
            double chargeAmount = std::min(10.0, 100.0 - robot.battery);
            robot.battery += chargeAmount;
            robot.status = RobotStatus::Charging;
            
            // Check if charging was optimal (battery < 30%)
            if (robot.battery - chargeAmount < 30.0) {
                result["charging_optimal"] = 1;
            }
            
            std::cout << "Robot " << robotIdx << " charging: " << robot.battery << "%\n";
            break;
        }
        
        case 4: { // TRANSFER TASK
            // Handover task to nearest available robot
            int nearestRobot = -1;
            double minDistance = 1000.0;
            
            for (size_t i = 0; i < robots.size(); ++i) {
                if (i == static_cast<size_t>(robotIdx)) continue;
                if (robots[i].hasOrder) continue;
                if (robots[i].battery < 20.0) continue;
                
                double dist = calculateDistance(robot.currentNode, robots[i].currentNode);
                if (dist < minDistance) {
                    minDistance = dist;
                    nearestRobot = i;
                }
            }
            
            if (nearestRobot != -1) {
                // Transfer order
                robots[nearestRobot].currentOrder = robot.currentOrder;
                robots[nearestRobot].hasOrder = true;
                
                robot.currentOrder = Order();
                robot.hasOrder = false;
                
                // Calculate distance saved
                double originalDistance = calculateDistance(robot.currentNode, targetNode);
                double newDistance = calculateDistance(robots[nearestRobot].currentNode, targetNode);
                result["distance_saved"] = std::max(0.0, originalDistance - newDistance);
                result["handover_success"] = 1;
                
                std::cout << "Task handed over from Robot " << robotIdx 
                          << " to Robot " << nearestRobot << "\n";
            } else {
                result["order_failed"] = 1;
            }
            
            break;
        }
        
        default:
            result["order_failed"] = 1;
            break;
    }
    
    // Check if robot is idle
    if (robot.status == RobotStatus::Idle && !robot.hasOrder) {
        result["robot_idle"] = 1;
    }

    if (globalLogger != nullptr) {
        globalLogger->updateMetrics(result);
    }
    
    return result;
}

// Initialize robots
void initRobots() {
    robots.clear();
    
    for (int i = 0; i < 5; ++i) {
        Robot r;
        r.id = "Robot_" + std::to_string(i);
        r.currentNode = chargingStationNode; // Start at charging station
        r.targetNode = -1;
        r.progress = 1.0;
        r.positionX = 0.0;
        r.positionY = 0.0;
        r.status = RobotStatus::Idle;
        r.carrying = false;
        r.hasOrder = false;
        r.battery = 100.0;
        r.speed = 1.0;
        
        robots.push_back(r);
    }
    
    std::cout << "Initialized " << robots.size() << " robots\n";
}