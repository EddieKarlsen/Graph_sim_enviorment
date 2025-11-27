#include <iostream>
#include <cstdlib>
#include "../includes/datatypes.hpp"
#include "../includes/robot.hpp"
#include "../includes/initSim.hpp"
#include "../includes/eventSystem.hpp"
#include "../includes/jsonComm.hpp"
#include "../includes/logger.hpp"

// Configuration
const double EPISODE_DURATION = 3600.0;  // 1 hour
const double TIMESTEP = 1.0;             // 1 second
const bool ENABLE_LOGGING = true;
const bool ENABLE_JSON_LOGGING = false;  // Set to true for debug

int main(int argc, char* argv[]) {
    std::cout << "=== Warehouse Simulation Starting ===\n\n";
    
    // 1. Initialize simulation
    std::cout << "[INIT] Initializing products...\n";
    initProducts();
    
    std::cout << "[INIT] Initializing graph layout...\n";
    initGraphLayout();
    
    std::cout << "[INIT] Initializing robots...\n";
    initRobots();
    
    std::cout << "[INIT] Initializing event system...\n";
    initEventSystem(42);  // Fixed seed for reproducibility
    
    std::cout << "[INIT] Initializing logger...\n";
    if (ENABLE_LOGGING) {
        initLogger("./logs", 1.0);
    }
    
    std::cout << "[INIT] Initializing JSON communication...\n";
    initJsonComm(ENABLE_JSON_LOGGING);
    
    // 2. Send INIT message to Python RL agent
    std::cout << "[INIT] Sending INIT to RL agent...\n";
    sendInitMessage();
    
    // Wait for Python to be ready (it will send back any message)
    std::cout << "[INIT] Waiting for RL agent to be ready...\n";
    json ackMsg = globalJsonComm->receiveMessage();
    if (ackMsg.empty() || ackMsg.value("type", "") != "READY") {
        std::cerr << "[ERROR] Did not receive READY from RL agent. Exiting.\n";
        return 1;
    }
    std::cout << "[INIT] RL agent is ready!\n\n";
    
    // 3. Main simulation loop
    int episodeNumber = 1;
    bool running = true;
    
    while (running) {
        std::cout << "=== Episode " << episodeNumber << " Starting ===\n";
        
        // Start episode logging
        if (ENABLE_LOGGING) {
            startLogging(episodeNumber);
        }
        
        double simTime = 0.0;
        
        // Episode loop
        while (simTime < EPISODE_DURATION) {
            // Process events (this will send tasks to RL and wait for decisions)
            processEvents(TIMESTEP);
            
            // Update robots (move them, update battery, etc.)
            for (size_t i = 0; i < robots.size(); ++i) {
                Robot& robot = robots[i];
                
                // Simple robot update (you should expand this)
                if (robot.getStatus() == RobotStatus::Moving) {
                    // Move robot
                    robot.setProgress(robot.getProgress() + TIMESTEP * robot.getSpeed());
                    
                    // Use battery
                    robot.useBattery(0.1 * TIMESTEP);
                    
                    // Check if arrived
                    if (robot.getProgress() >= 1.0) {
                        robot.setCurrentNode(robot.getTargetNode());
                        robot.setStatus(RobotStatus::Idle);
                        robot.setProgress(0.0);
                        
                        std::cout << "[ROBOT] " << robot.getId() 
                                  << " arrived at node " << robot.getCurrentNode() << "\n";
                    }
                }
                
                // Check battery
                if (robot.needsCharging(20.0) && robot.isIdle()) {
                    std::cout << "[ROBOT] " << robot.getId() 
                              << " needs charging (battery: " << robot.getBattery() << "%)\n";
                    
                    // Send status to RL
                    if (globalJsonComm) {
                        globalJsonComm->sendRobotStatus(
                            i, 
                            StatusType::LOW_BATTERY, 
                            "", 
                            simTime,
                            "Battery low, requesting charge"
                        );
                    }
                }
            }
            
            // Log snapshot
            if (ENABLE_LOGGING) {
                logSnapshot(simTime);
            }
            
            // Increment time
            simTime += TIMESTEP;
            
            // Progress indicator every 10 seconds
            if (static_cast<int>(simTime) % 10 == 0) {
                std::cout << "[TIME] " << simTime << "s / " << EPISODE_DURATION << "s\n";
            }
        }
        
        std::cout << "\n=== Episode " << episodeNumber << " Ended ===\n";
        
        // End episode logging
        if (ENABLE_LOGGING) {
            stopLogging();
            std::string filename = "episode_" + std::to_string(episodeNumber) + ".json";
            saveEpisodeData(filename);
        }
        
        // Send episode end to RL
        if (globalJsonComm) {
            globalJsonComm->sendEpisodeEnd(simTime);
        }
        
        // Wait for reset command from RL
        std::cout << "[SIM] Waiting for RESET command from RL...\n";
        int nextEpisodeNumber = 0;
        bool shouldReset = globalJsonComm->receiveReset(nextEpisodeNumber);
        
        if (!shouldReset) {
            std::cout << "[SIM] No RESET received, exiting.\n";
            running = false;
        } else {
            // Reset simulation
            std::cout << "[RESET] Resetting for episode " << nextEpisodeNumber << "...\n";
            episodeNumber = nextEpisodeNumber;
            
            resetInventory();
            initRobots();
            initEventSystem(42 + episodeNumber);  // Different seed per episode
            
            // Send new INIT
            sendInitMessage();
            
            // Wait for ready
            ackMsg = globalJsonComm->receiveMessage();
        }
    }
    
    // Cleanup
    std::cout << "\n=== Simulation Shutting Down ===\n";
    shutdownJsonComm();
    
    return 0;
}