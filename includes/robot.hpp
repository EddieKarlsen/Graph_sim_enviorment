#ifndef ROBOT_HPP
#define ROBOT_HPP

#include "datatypes.hpp"
#include <string>
#include <map>

enum class RobotStatus {
    Idle,
    Moving,
    Carrying,
    Charging,
    Picking,
    Dropping
};

struct Order {
    int productID = -1;
    int slotIndex = -1;
    int quantity = 1;
    
    // Getters
    int getProductID() const { return productID; }
    int getSlotIndex() const { return slotIndex; }
    int getQuantity() const { return quantity; }
    
    // Setters
    void setProductID(int pid) { productID = pid; }
    void setSlotIndex(int idx) { slotIndex = idx; }
    void setQuantity(int qty) { quantity = qty; }
    
    // Utility
    bool isValid() const { return productID >= 0; }
    void reset() { productID = -1; slotIndex = -1; quantity = 1; }
};

struct Robot {
    std::string id;
    int currentNode;
    int targetNode;
    double progress;
    double positionX;
    double positionY;
    RobotStatus status;
    bool carrying;
    bool hasOrder;
    double battery;
    double speed;
    Order currentOrder;
    
    // Getters
    std::string getId() const { return id; }
    int getCurrentNode() const { return currentNode; }
    int getTargetNode() const { return targetNode; }
    double getProgress() const { return progress; }
    double getPositionX() const { return positionX; }
    double getPositionY() const { return positionY; }
    RobotStatus getStatus() const { return status; }
    bool isCarrying() const { return carrying; }
    bool getHasOrder() const { return hasOrder; }
    double getBattery() const { return battery; }
    double getSpeed() const { return speed; }
    const Order& getCurrentOrder() const { return currentOrder; }
    Order& getCurrentOrderMutable() { return currentOrder; }
    
    // Setters
    void setId(const std::string& newId) { id = newId; }
    void setCurrentNode(int node) { currentNode = node; }
    void setTargetNode(int node) { targetNode = node; }
    void setProgress(double prog) { progress = prog; }
    void setPosition(double x, double y) { positionX = x; positionY = y; }
    void setPositionX(double x) { positionX = x; }
    void setPositionY(double y) { positionY = y; }
    void setStatus(RobotStatus newStatus) { status = newStatus; }
    void setCarrying(bool carry) { carrying = carry; }
    void setHasOrder(bool order) { hasOrder = order; }
    void setBattery(double batt) { battery = batt; }
    void setSpeed(double spd) { speed = spd; }
    void setCurrentOrder(const Order& order) { currentOrder = order; }
    
    // Utility methods
    std::string getStatusString() const {
        switch(status) {
            case RobotStatus::Idle: return "Idle";
            case RobotStatus::Moving: return "Moving";
            case RobotStatus::Carrying: return "Carrying";
            case RobotStatus::Charging: return "Charging";
            case RobotStatus::Picking: return "Picking";
            case RobotStatus::Dropping: return "Dropping";
            default: return "Unknown";
        }
    }
    
    bool needsCharging(double threshold = 20.0) const { 
        return battery < threshold; 
    }
    
    bool isIdle() const { 
        return status == RobotStatus::Idle; 
    }
    
    bool isMoving() const { 
        return status == RobotStatus::Moving; 
    }
    
    void useBattery(double amount) { 
        battery = std::max(0.0, battery - amount); 
    }
    
    void charge(double amount) { 
        battery = std::min(100.0, battery + amount); 
    }
};

extern std::vector<Robot> robots;

// Robot functions
void initRobots();
std::map<std::string, double> step_simulation(int robotIdx, int actionType, int targetNode, int productID);
int findProductOnShelf(int productID, int& outSlotIndex);
int findBestShelfForProduct(int productID);

// Robot access functions for Python binding
namespace RobotAccess {
    int getRobotCount();
    Robot* getRobot(int index);
    const Robot* getRobotConst(int index);
    
    // Batch getters for efficiency
    std::vector<double> getAllBatteryLevels();
    std::vector<int> getAllCurrentNodes();
    std::vector<std::string> getAllStatuses();
}

#endif