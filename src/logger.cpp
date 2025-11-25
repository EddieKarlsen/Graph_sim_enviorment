#include "../includes/logger.hpp"
#include "../includes/datatypes.hpp"
#include "../includes/robot.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include "../includes/json.hpp"

using json = nlohmann::json;

EpisodeLogger* globalLogger = nullptr;

EpisodeLogger::EpisodeLogger(const std::string& logDir, double snapshotIntervalSec) 
    : logDirectory(logDir), snapshotInterval(snapshotIntervalSec), 
      isRecording(false), episodeStartTime(0.0), lastSnapshotTime(0.0) {
    
    // Skapa loggmapp om den inte finns
    #ifdef _WIN32
        system(("mkdir " + logDirectory + " 2>nul").c_str());
    #else
        system(("mkdir -p " + logDirectory).c_str());
    #endif
}

void EpisodeLogger::startEpisode(int episodeNumber) {
    clear();
    isRecording = true;
    episodeStartTime = 0.0;
    lastSnapshotTime = 0.0;
    
    metrics.episodeNumber = episodeNumber;
    metrics.totalTime = 0.0;
    metrics.ordersCompleted = 0;
    metrics.ordersFailed = 0;
    metrics.avgCompletionTime = 0.0;
    metrics.totalDistanceTraveled = 0.0;
    metrics.totalBatteryUsed = 0.0;
    metrics.optimalZonePlacements = 0;
    metrics.suboptimalPlacements = 0;
    metrics.robotUtilization = 0.0;
    
    // Initiera heatmap för alla noder
    heatmapData.clear();
    for (size_t i = 0; i < nodes.size(); ++i) {
        HeatmapData hm;
        hm.nodeIndex = i;
        hm.nodeId = nodes[i].id;
        hm.visitCount = 0;
        hm.totalTimeSpent = 0.0;
        hm.robotVisits.resize(robots.size(), 0);
        heatmapData.push_back(hm);
    }
    
    std::cout << "[LOGGER] Started recording episode " << episodeNumber << "\n";
}

void EpisodeLogger::endEpisode() {
    if (!isRecording) return;
    
    metrics.totalTime = lastSnapshotTime;
    
    // Beräkna genomsnittlig completion time
    if (metrics.ordersCompleted > 0) {
        // Detta kan förbättras genom att spåra individuella completion times
        metrics.avgCompletionTime = metrics.totalTime / metrics.ordersCompleted;
    }
    
    // Beräkna robot utilization
    double totalPossibleTime = metrics.totalTime * robots.size();
    double totalActiveTime = 0.0;
    
    for (const auto& event : taskEvents) {
        if (event.eventType == "ORDER_START" || event.eventType == "PICKUP" || 
            event.eventType == "DROPOFF") {
            totalActiveTime += 1.0; // Simplified
        }
    }
    
    if (totalPossibleTime > 0) {
        metrics.robotUtilization = (totalActiveTime / totalPossibleTime) * 100.0;
    }
    
    isRecording = false;
    std::cout << "[LOGGER] Ended recording episode " << metrics.episodeNumber 
              << " (Duration: " << metrics.totalTime << "s)\n";
}

void EpisodeLogger::logRobotSnapshot(double currentTime) {
    if (!isRecording) return;
    
    // Kolla om det är dags för nästa snapshot
    if (currentTime - lastSnapshotTime < snapshotInterval) {
        return;
    }
    
    lastSnapshotTime = currentTime;
    
    // Logga alla robotar
    for (size_t i = 0; i < robots.size(); ++i) {
        const Robot& robot = robots[i];
        
        RobotSnapshot snap;
        snap.timestamp = currentTime;
        snap.robotId = robot.id;
        snap.robotIndex = i;
        snap.posX = robot.positionX;
        snap.posY = robot.positionY;
        snap.currentNode = robot.currentNode;
        snap.nodeId = nodes[robot.currentNode].id;
        snap.battery = robot.battery;
        snap.carrying = robot.carrying;
        snap.carryingProductID = robot.carrying ? robot.currentOrder.productID : -1;
        
        // Konvertera status till string
        switch(robot.status) {
            case RobotStatus::Idle: snap.status = "Idle"; break;
            case RobotStatus::Moving: snap.status = "Moving"; break;
            case RobotStatus::Carrying: snap.status = "Carrying"; break;
            case RobotStatus::Charging: snap.status = "Charging"; break;
            case RobotStatus::Picking: snap.status = "Picking"; break;
            case RobotStatus::Dropping: snap.status = "Dropping"; break;
            default: snap.status = "Unknown"; break;
        }
        
        snapshots.push_back(snap);
        
        // Uppdatera heatmap
        updateHeatmap(robot.currentNode, i, snapshotInterval);
    }
}

void EpisodeLogger::logTaskEvent(double currentTime, int robotIdx, 
                                 const std::string& eventType, int productID, 
                                 int fromNode, int toNode, double distance) {
    if (!isRecording) return;
    
    TaskEvent event;
    event.timestamp = currentTime;
    event.robotIndex = robotIdx;
    event.robotId = robots[robotIdx].id;
    event.eventType = eventType;
    event.productID = productID;
    event.fromNode = fromNode;
    event.toNode = toNode;
    event.distanceTraveled = distance;
    
    taskEvents.push_back(event);
}

void EpisodeLogger::updateHeatmap(int nodeIndex, int robotIndex, double timeSpent) {
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(heatmapData.size())) return;
    
    heatmapData[nodeIndex].visitCount++;
    heatmapData[nodeIndex].totalTimeSpent += timeSpent;
    
    if (robotIndex >= 0 && robotIndex < static_cast<int>(heatmapData[nodeIndex].robotVisits.size())) {
        heatmapData[nodeIndex].robotVisits[robotIndex]++;
    }
}

void EpisodeLogger::updateMetrics(const std::map<std::string, double>& stepResult) {
    if (!isRecording) return;
    
    if (stepResult.count("order_completed") && stepResult.at("order_completed") > 0) {
        metrics.ordersCompleted++;
    }
    
    if (stepResult.count("order_failed") && stepResult.at("order_failed") > 0) {
        metrics.ordersFailed++;
    }
    
    if (stepResult.count("battery_used")) {
        metrics.totalBatteryUsed += stepResult.at("battery_used");
    }
    
    if (stepResult.count("distance_saved")) {
        metrics.totalDistanceTraveled += stepResult.at("distance_saved");
    }
    
    if (stepResult.count("optimal_zone_placement") && stepResult.at("optimal_zone_placement") > 0) {
        metrics.optimalZonePlacements++;
    } else if (stepResult.count("order_completed") && stepResult.at("order_completed") > 0) {
        metrics.suboptimalPlacements++;
    }
}

void EpisodeLogger::saveToJson(const std::string& filename) {
    json j;
    
    // Metadata
    j["episode"] = metrics.episodeNumber;
    j["total_time"] = metrics.totalTime;
    
    // Metrics
    json metricsJson;
    metricsJson["orders_completed"] = metrics.ordersCompleted;
    metricsJson["orders_failed"] = metrics.ordersFailed;
    metricsJson["avg_completion_time"] = metrics.avgCompletionTime;
    metricsJson["total_distance_traveled"] = metrics.totalDistanceTraveled;
    metricsJson["total_battery_used"] = metrics.totalBatteryUsed;
    metricsJson["optimal_zone_placements"] = metrics.optimalZonePlacements;
    metricsJson["suboptimal_placements"] = metrics.suboptimalPlacements;
    metricsJson["robot_utilization"] = metrics.robotUtilization;
    j["metrics"] = metricsJson;
    
    // Robot Snapshots
    json snapshotsJson = json::array();
    for (const auto& snap : snapshots) {
        json snapJson;
        snapJson["timestamp"] = snap.timestamp;
        snapJson["robot_id"] = snap.robotId;
        snapJson["robot_index"] = snap.robotIndex;
        snapJson["pos_x"] = snap.posX;
        snapJson["pos_y"] = snap.posY;
        snapJson["current_node"] = snap.currentNode;
        snapJson["node_id"] = snap.nodeId;
        snapJson["status"] = snap.status;
        snapJson["battery"] = snap.battery;
        snapJson["carrying"] = snap.carrying;
        snapJson["carrying_product_id"] = snap.carryingProductID;
        snapshotsJson.push_back(snapJson);
    }
    j["robot_snapshots"] = snapshotsJson;
    
    // Task Events
    json eventsJson = json::array();
    for (const auto& event : taskEvents) {
        json eventJson;
        eventJson["timestamp"] = event.timestamp;
        eventJson["robot_index"] = event.robotIndex;
        eventJson["robot_id"] = event.robotId;
        eventJson["event_type"] = event.eventType;
        eventJson["product_id"] = event.productID;
        eventJson["from_node"] = event.fromNode;
        eventJson["to_node"] = event.toNode;
        eventJson["distance_traveled"] = event.distanceTraveled;
        eventsJson.push_back(eventJson);
    }
    j["task_events"] = eventsJson;
    
    // Heatmap Data
    json heatmapJson = json::array();
    for (const auto& hm : heatmapData) {
        json hmJson;
        hmJson["node_index"] = hm.nodeIndex;
        hmJson["node_id"] = hm.nodeId;
        hmJson["visit_count"] = hm.visitCount;
        hmJson["total_time_spent"] = hm.totalTimeSpent;
        hmJson["robot_visits"] = hm.robotVisits;
        heatmapJson.push_back(hmJson);
    }
    j["heatmap"] = heatmapJson;
    
    // Spara till fil
    std::string fullPath = logDirectory + "/" + filename;
    std::ofstream file(fullPath);
    if (file.is_open()) {
        file << std::setw(2) << j << std::endl;
        file.close();
        std::cout << "[LOGGER] Saved full episode data to " << fullPath << "\n";
    } else {
        std::cerr << "[LOGGER] Failed to save to " << fullPath << "\n";
    }
}

void EpisodeLogger::saveMetricsOnly(const std::string& filename) {
    json j;
    j["episode"] = metrics.episodeNumber;
    j["total_time"] = metrics.totalTime;
    j["orders_completed"] = metrics.ordersCompleted;
    j["orders_failed"] = metrics.ordersFailed;
    j["avg_completion_time"] = metrics.avgCompletionTime;
    j["total_distance_traveled"] = metrics.totalDistanceTraveled;
    j["total_battery_used"] = metrics.totalBatteryUsed;
    j["optimal_zone_placements"] = metrics.optimalZonePlacements;
    j["suboptimal_placements"] = metrics.suboptimalPlacements;
    j["robot_utilization"] = metrics.robotUtilization;
    
    std::string fullPath = logDirectory + "/" + filename;
    std::ofstream file(fullPath);
    if (file.is_open()) {
        file << std::setw(2) << j << std::endl;
        file.close();
        std::cout << "[LOGGER] Saved metrics to " << fullPath << "\n";
    }
}

void EpisodeLogger::saveHeatmapOnly(const std::string& filename) {
    json j = json::array();
    
    for (const auto& hm : heatmapData) {
        json hmJson;
        hmJson["node_index"] = hm.nodeIndex;
        hmJson["node_id"] = hm.nodeId;
        hmJson["visit_count"] = hm.visitCount;
        hmJson["total_time_spent"] = hm.totalTimeSpent;
        hmJson["robot_visits"] = hm.robotVisits;
        j.push_back(hmJson);
    }
    
    std::string fullPath = logDirectory + "/" + filename;
    std::ofstream file(fullPath);
    if (file.is_open()) {
        file << std::setw(2) << j << std::endl;
        file.close();
        std::cout << "[LOGGER] Saved heatmap to " << fullPath << "\n";
    }
}

void EpisodeLogger::clear() {
    snapshots.clear();
    taskEvents.clear();
    heatmapData.clear();
}

// Helper functions
void initLogger(const std::string& logDir, double snapshotInterval) {
    if (globalLogger != nullptr) {
        delete globalLogger;
    }
    globalLogger = new EpisodeLogger(logDir, snapshotInterval);
}

void startLogging(int episodeNumber) {
    if (globalLogger != nullptr) {
        globalLogger->startEpisode(episodeNumber);
    }
}

void stopLogging() {
    if (globalLogger != nullptr) {
        globalLogger->endEpisode();
    }
}

void logSnapshot(double currentTime) {
    if (globalLogger != nullptr) {
        globalLogger->logRobotSnapshot(currentTime);
    }
}

void logTask(double currentTime, int robotIdx, const std::string& eventType, 
            int productID, int fromNode, int toNode, double distance) {
    if (globalLogger != nullptr) {
        globalLogger->logTaskEvent(currentTime,robotIdx, eventType,  productID, fromNode, toNode, distance);
    }
}

void saveEpisodeData(const std::string& filename) {
    if (globalLogger != nullptr) {
        globalLogger->saveToJson(filename);
    }
}