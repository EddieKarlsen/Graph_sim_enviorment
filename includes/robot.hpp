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
};

extern std::vector<Robot> robots;

// Robot functions
void initRobots();
std::map<std::string, double> step_simulation(int robotIdx, int actionType, int targetNode, int productID);
int findProductOnShelf(int productID, int& outSlotIndex);
int findBestShelfForProduct(int productID);

#endif