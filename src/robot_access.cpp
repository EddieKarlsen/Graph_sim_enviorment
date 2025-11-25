#include "../includes/robot.hpp"
#include <algorithm>

// Implementation av RobotAccess namespace
namespace RobotAccess {
    int getRobotCount() {
        return static_cast<int>(robots.size());
    }
    
    Robot* getRobot(int index) {
        if (index >= 0 && index < static_cast<int>(robots.size())) {
            return &robots[index];
        }
        return nullptr;
    }
    
    const Robot* getRobotConst(int index) {
        if (index >= 0 && index < static_cast<int>(robots.size())) {
            return &robots[index];
        }
        return nullptr;
    }
    
    std::vector<double> getAllBatteryLevels() {
        std::vector<double> batteries;
        batteries.reserve(robots.size());
        for (const auto& robot : robots) {
            batteries.push_back(robot.getBattery());
        }
        return batteries;
    }
    
    std::vector<int> getAllCurrentNodes() {
        std::vector<int> nodes;
        nodes.reserve(robots.size());
        for (const auto& robot : robots) {
            nodes.push_back(robot.getCurrentNode());
        }
        return nodes;
    }
    
    std::vector<std::string> getAllStatuses() {
        std::vector<std::string> statuses;
        statuses.reserve(robots.size());
        for (const auto& robot : robots) {
            statuses.push_back(robot.getStatusString());
        }
        return statuses;
    }
}