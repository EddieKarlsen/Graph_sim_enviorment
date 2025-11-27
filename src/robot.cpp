#include "../includes/robot.hpp"
#include "../includes/eventSystem.hpp"
#include "../includes/hotWarmCold.hpp"
#include "../includes/helpFunctions.hpp"
#include "../includes/logger.hpp"
#include "../includes/pathfinding.hpp"
#include "../includes/datatypes.hpp"
#include <iostream>
#include <cmath>

std::vector<Robot> robots;

void initRobots() {
    robots.clear();
    
    // Initialize 3 robots at charging station
    for (int i = 0; i < 3; ++i) {
        Robot robot;
        robot.setId("robot_" + std::to_string(i));
        robot.setCurrentNode(chargingStationNode);
        robot.setTargetNode(-1);
        robot.setProgress(0.0);
        robot.setPosition(0.0, 0.0);  // Could set based on node position
        robot.setStatus(RobotStatus::Idle);
        robot.setCarrying(false);
        robot.setHasOrder(false);
        robot.setBattery(100.0);
        robot.setSpeed(1.0);  // 1 unit per second
        
        robots.push_back(robot);
    }
    
    std::cerr << "[ROBOTS] Initialized " << robots.size() << " robots\n";
}

// Start robot movement to target
bool startRobotMovement(int robotIdx, int targetNode) {
    if (robotIdx < 0 || robotIdx >= static_cast<int>(robots.size())) {
        std::cerr << "[ROBOT] Invalid robot index: " << robotIdx << "\n";
        return false;
    }
    
    Robot& robot = robots[robotIdx];
    
    // Check if robot is available
    if (robot.getStatus() != RobotStatus::Idle) {
        std::cerr << "[ROBOT] Robot " << robot.getId() << " is not idle (status: " 
                  << robot.getStatusString() << ")\n";
        return false;
    }
    
    // Find path
    Path path = findShortestPath(robot.getCurrentNode(), targetNode);
    
    if (!path.isFound()) {
        std::cerr << "[ROBOT] No path found from node " << robot.getCurrentNode() 
                  << " to node " << targetNode << "\n";
        return false;
    }
    
    std::cerr << "[ROBOT] " << robot.getId() << " starting movement:\n";
    std::cerr << "        ";
    path.print();
    
    // Set first target (next node in path)
    if (path.getNodeCount() > 1) {
        robot.setTargetNode(path.getNode(1));  // Next node after current
        robot.setStatus(RobotStatus::Moving);
        robot.setProgress(0.0);
        robot.currentPath = path;
        // Store full path in robot (you might want to add this to Robot struct)
        // robot.currentPath = path;
        
        return true;
    }
    
    return false;
}

// Update robot movement (call every timestep)
void updateRobotMovement(int robotIdx, double deltaTime, const Path& fullPath) {
    if (robotIdx < 0 || robotIdx >= static_cast<int>(robots.size())) {
        return;
    }
    
    Robot& robot = robots[robotIdx];
    
    if (robot.getStatus() != RobotStatus::Moving) {
        return;
    }
    
    // Get distance to next node
    double edgeDistance = getEdgeDistance(robot.getCurrentNode(), robot.getTargetNode());
    
    if (edgeDistance == std::numeric_limits<double>::infinity()) {
        std::cerr << "[ROBOT] Error: No edge from " << robot.getCurrentNode() 
                  << " to " << robot.getTargetNode() << "\n";
        robot.setStatus(RobotStatus::Idle);
        return;
    }
    
    // Move robot
    double moveDistance = robot.getSpeed() * deltaTime;
    double progressIncrement = moveDistance / edgeDistance;
    
    robot.setProgress(robot.getProgress() + progressIncrement);
    
    // Use battery (proportional to distance)
    double batteryUsage = 0.1 * progressIncrement;
    robot.useBattery(batteryUsage);
    
    // Check if arrived at next node
    if (robot.getProgress() >= 1.0) {
        // Arrived at target node
        robot.setCurrentNode(robot.getTargetNode());
        robot.setProgress(0.0);
        
        std::cerr << "[ROBOT] " << robot.getId() << " arrived at node " 
                  << robot.getCurrentNode() << " (battery: " 
                  << robot.getBattery() << "%)\n";
        
        // Check if this is final destination
        int nextNode = fullPath.getNextNode(robot.getCurrentNode());
        
        if (nextNode == -1) {
            // Reached final destination
            robot.setStatus(RobotStatus::Idle);
            robot.setTargetNode(-1);
            std::cerr << "[ROBOT] " << robot.getId() << " reached final destination\n";
        } else {
            // Continue to next node in path
            robot.setTargetNode(nextNode);
            robot.setProgress(0.0);
        }
    }
}

// Find product on shelf
int findProductOnShelf(int productID, int& outSlotIndex) {
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].getType() != NodeType::Shelf) continue;
        
        const Shelf* shelf = nodes[i].getShelf();
        if (!shelf) continue;
        
        for (int j = 0; j < shelf->getSlotCount(); ++j) {
            Slot slot = shelf->getSlot(j);
            if (slot.getProductID() == productID && slot.getOccupied() > 0) {
                outSlotIndex = j;
                return i;  // Return shelf node index
            }
        }
    }
    
    outSlotIndex = -1;
    return -1;
}

// Find best shelf for product (by zone and capacity)
int findBestShelfForProduct(int productID) {
    // Get recommended zone based on popularity
    Zone recommendedZone = Zone::Cold;
    
    for (const Product& p : products) {
        if (p.getId() == productID) {
            int pop = p.getPopularity();
            if (pop >= 10) recommendedZone = Zone::Hot;
            else if (pop >= 5) recommendedZone = Zone::Warm;
            else recommendedZone = Zone::Cold;
            break;
        }
    }
    
    int bestShelf = -1;
    double lowestFillRate = 1.0;
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].getType() != NodeType::Shelf) continue;
        
        // Prefer shelves in recommended zone
        Zone shelfZone = nodes[i].getZone();
        if (shelfZone != recommendedZone && recommendedZone != Zone::Other) {
            continue;  // Skip wrong zone
        }
        
        const Shelf* shelf = nodes[i].getShelf();
        if (!shelf) continue;
        
        // Find slot with this product or empty slot
        for (int j = 0; j < shelf->getSlotCount(); ++j) {
            Slot slot = shelf->getSlot(j);
            
            if (slot.getProductID() == productID || slot.getProductID() == 0) {
                double fillRate = slot.getCapacity() > 0 ? 
                    (double)slot.getOccupied() / slot.getCapacity() : 1.0;
                
                if (fillRate < lowestFillRate) {
                    lowestFillRate = fillRate;
                    bestShelf = i;
                }
            }
        }
    }
    
    return bestShelf;
}

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
                std::cerr << "Robot " << robotIdx << " out of battery\n";
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
                std::cerr << "Robot already carrying item\n";
                break;
            }
            
            // Find product on shelf
            int slotIndex;
            int shelfNode = findProductOnShelf(productID, slotIndex);
            
            if (shelfNode == -1 || shelfNode != targetNode) {
                result["order_failed"] = 1;
                std::cerr << "Product " << productID << " not found at node " << targetNode << "\n";
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
                
                std::cerr << "Robot " << robotIdx << " completed customer order\n";
                
            } else if (nodes[targetNode].type == NodeType::Shelf) {
                // Restocking
                int bestShelf = findBestShelfForProduct(robot.currentOrder.productID);
                
                if (bestShelf == targetNode) {
                    result["optimal_zone_placement"] = 1;
                    std::cerr << "Optimal zone placement!\n";
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
            
            std::cerr << "Robot " << robotIdx << " charging: " << robot.battery << "%\n";
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
                
                std::cerr << "Task handed over from Robot " << robotIdx 
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