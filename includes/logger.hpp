#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "datatypes.hpp"
#include "robot.hpp"
#include <string>
#include <vector>
#include <fstream>

struct EpisodeMetrics {
    int episodeNumber;
    double totalTime;
    int ordersCompleted;
    int ordersFailed;
    double avgCompletionTime;
    double totalDistanceTraveled;
    double totalBatteryUsed;
    int optimalZonePlacements;
    int suboptimalPlacements;
    double robotUtilization;  // Procent av tiden robotar är aktiva


    //getters
    int getOrdersCompleted() const { return ordersCompleted; }
    int getOrdersFailed() const { return ordersFailed; }
    double getTotalDistanceTraveled() const { return totalDistanceTraveled; }
    double getAvgCompletionTime() const { return avgCompletionTime; }
    double getRobotUtilization() const { return robotUtilization; }
};

struct RobotSnapshot {
    double timestamp;
    std::string robotId;
    int robotIndex;
    double posX;
    double posY;
    int currentNode;
    std::string nodeId;
    std::string status;
    double battery;
    bool carrying;
    int carryingProductID;
};

struct TaskEvent {
    double timestamp;
    int robotIndex;
    std::string robotId;
    std::string eventType;  // "ORDER_START", "PICKUP", "DROPOFF", "FAILED", "HANDOVER"
    int productID;
    int fromNode;
    int toNode;
    double distanceTraveled;
};

struct HeatmapData {
    int nodeIndex;
    std::string nodeId;
    int visitCount;
    double totalTimeSpent;
    std::vector<int> robotVisits;  // Per robot
};

class EpisodeLogger {
private:
    std::vector<RobotSnapshot> snapshots;
    std::vector<TaskEvent> taskEvents;
    std::vector<HeatmapData> heatmapData;
    EpisodeMetrics metrics;
    
    double episodeStartTime;
    double lastSnapshotTime;
    double snapshotInterval;  // Sekunder mellan snapshots
    
    bool isRecording;
    std::string logDirectory;

public:
    EpisodeLogger(const std::string& logDir = "./logs", double snapshotIntervalSec = 1.0);
    
    // Starta/stoppa loggning
    void startEpisode(int episodeNumber);
    void endEpisode();
    
    // Logga events
    void logRobotSnapshot(double currentTime);
    void logTaskEvent(double currentTime, int robotIdx, const std::string& eventType, 
                     int productID, int fromNode, int toNode, double distance);
    void updateHeatmap(int nodeIndex, int robotIndex, double timeSpent);
    
    // Spara till fil
    void saveToJson(const std::string& filename);
    void saveMetricsOnly(const std::string& filename);
    void saveHeatmapOnly(const std::string& filename);
    
    // Utility
    void updateMetrics(const std::map<std::string, double>& stepResult);
    EpisodeMetrics getMetrics() const { return metrics; }
    void clear();
};

// Global logger instans
extern EpisodeLogger* globalLogger;

// Helper functions för enkel användning
void initLogger(const std::string& logDir = "./logs", double snapshotInterval = 1.0);
void startLogging(int episodeNumber);
void stopLogging();
void logSnapshot(double currentTime);
void logTask(double currentTime, int robotIdx, const std::string& eventType, 
            int productID, int fromNode, int toNode, double distance);
void saveEpisodeData(const std::string& filename);

#endif