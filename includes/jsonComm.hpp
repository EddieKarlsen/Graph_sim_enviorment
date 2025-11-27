#ifndef JSON_COMM_HPP
#define JSON_COMM_HPP

#include "datatypes.hpp"
#include "robot.hpp"
#include "json.hpp"
#include <string>
#include <iostream>
#include <sstream>

using json = nlohmann::json;

// Message types
enum class MessageType {
    INIT,
    NEW_TASK,
    ROBOT_STATUS,
    ACTION_DECISION,
    HANDOVER_DECISION,
    WAIT_DECISION,
    ACK,
    ERROR_MSG,
    EPISODE_END,
    RESET
};

// Task types
enum class TaskType {
    CUSTOMER_ORDER,
    INCOMING_DELIVERY,
    RESTOCK_REQUEST
};

// Status types
enum class StatusType {
    TASK_COMPLETE,
    TASK_FAILED,
    LOW_BATTERY,
    STUCK,
    HANDOVER_READY,
    CHARGING
};

// Action types for RL decisions
enum class ActionType {
    PICKUP_AND_DELIVER,
    RESTOCK,
    CHARGE,
    HANDOVER,
    WAIT
};

// Task structure
struct Task {
    std::string taskId;
    TaskType taskType;
    int productId;
    int quantity;
    int sourceNode;
    int targetNode;
    std::string priority;
    double deadline;
    
    json toJson() const;
    static Task fromJson(const json& j);
};

// Action structure (from RL)
struct Action {
    int robotIndex;
    ActionType actionType;
    int productId;
    int sourceNode;
    int targetNode;
    std::string strategy;
    
    // For handover
    int secondaryRobot = -1;
    int handoverNode = -1;
    std::string reason;
    
    static Action fromJson(const json& j);
};

// JSON Communication Manager
class JsonComm {
private:
    std::istream* input;
    std::ostream* output;
    int messageCount;
    bool logMessages;
    
public:
    JsonComm(std::istream* in = &std::cin, std::ostream* out = &std::cout, bool log = false);
    
    // Send messages to RL
    void sendInit(double timestamp);
    void sendNewTask(const Task& task, double timestamp);
    void sendRobotStatus(int robotIndex, StatusType status, const std::string& taskId, 
                        double timestamp, const std::string& message = "");
    void sendAck(const std::string& taskId, int robotIndex, double estimatedCompletionTime);
    void sendError(const std::string& taskId, const std::string& errorCode, 
                  const std::string& message, int robotIndex = -1);
    void sendEpisodeEnd(double timestamp);
    
    // Receive messages from RL
    json receiveMessage();
    Action receiveAction();
    bool receiveReset(int& outEpisodeNumber);
    
    // Helper: Build full state
    json buildStateJson(double timestamp);
    
    // Helper: Build warehouse layout
    json buildWarehouseLayout();
    
    // Helper: Serialize nodes
    json serializeNodes();
    json serializeEdges();
    json serializeProducts();
    json serializeRobots(double timestamp);
    json serializeInventory();
    json serializeLoadingDock();
    json serializeFrontDesk();
    json serializeChargingStation();
    
    // Utility
    void flush();
    void setLogging(bool enabled) { logMessages = enabled; }
    
private:
    void logMessage(const std::string& direction, const json& msg);
    std::string messageTypeToString(MessageType type);
    std::string taskTypeToString(TaskType type);
    std::string statusTypeToString(StatusType type);
    std::string actionTypeToString(ActionType type);
};

// Global instance
extern JsonComm* globalJsonComm;

// Helper functions
void initJsonComm(bool logging = false);
void shutdownJsonComm();

// Convenience wrappers
void sendInitMessage();
void sendNewTaskMessage(const Task& task, double currentTime);
void sendRobotStatusMessage(int robotIdx, StatusType status, const std::string& taskId, 
                           double currentTime, const std::string& msg = "");

#endif