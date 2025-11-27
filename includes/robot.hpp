#ifndef ROBOT_HPP
#define ROBOT_HPP

#include "datatypes.hpp"
#include "pathfinding.hpp"
#include <string>
#include <map>







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