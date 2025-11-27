#include "../includes/jsonComm.hpp"
#include "../includes/datatypes.hpp"
#include "../includes/robot.hpp"
#include "../includes/logger.hpp"
#include <fstream>

JsonComm* globalJsonComm = nullptr;

// Task JSON conversion
json Task::toJson() const {
    json j;
    j["task_id"] = taskId;
    j["product_id"] = productId;
    j["quantity"] = quantity;
    j["source_node"] = sourceNode;
    j["target_node"] = targetNode;
    j["priority"] = priority;
    j["deadline"] = deadline;
    
    switch(taskType) {
        case TaskType::CUSTOMER_ORDER: j["task_type"] = "CUSTOMER_ORDER"; break;
        case TaskType::INCOMING_DELIVERY: j["task_type"] = "INCOMING_DELIVERY"; break;
        case TaskType::RESTOCK_REQUEST: j["task_type"] = "RESTOCK_REQUEST"; break;
    }
    
    return j;
}

Task Task::fromJson(const json& j) {
    Task t;
    t.taskId = j.value("task_id", "");
    t.productId = j.value("product_id", -1);
    t.quantity = j.value("quantity", 1);
    t.sourceNode = j.value("source_node", -1);
    t.targetNode = j.value("target_node", -1);
    t.priority = j.value("priority", "normal");
    t.deadline = j.value("deadline", 0.0);
    
    std::string typeStr = j.value("task_type", "CUSTOMER_ORDER");
    if (typeStr == "CUSTOMER_ORDER") t.taskType = TaskType::CUSTOMER_ORDER;
    else if (typeStr == "INCOMING_DELIVERY") t.taskType = TaskType::INCOMING_DELIVERY;
    else if (typeStr == "RESTOCK_REQUEST") t.taskType = TaskType::RESTOCK_REQUEST;
    
    return t;
}

// Action JSON conversion
Action Action::fromJson(const json& j) {
    Action a;
    a.robotIndex = j.value("robot_index", -1);
    a.productId = j.value("product_id", -1);
    a.sourceNode = j.value("source_node", -1);
    a.targetNode = j.value("target_node", -1);
    a.strategy = j.value("strategy", "direct");
    
    std::string typeStr = j.value("action_type", "WAIT");
    if (typeStr == "PICKUP_AND_DELIVER") a.actionType = ActionType::PICKUP_AND_DELIVER;
    else if (typeStr == "RESTOCK") a.actionType = ActionType::RESTOCK;
    else if (typeStr == "CHARGE") a.actionType = ActionType::CHARGE;
    else if (typeStr == "HANDOVER") a.actionType = ActionType::HANDOVER;
    else a.actionType = ActionType::WAIT;
    
    // Handover specific
    if (j.contains("secondary_robot")) {
        a.secondaryRobot = j["secondary_robot"];
        a.handoverNode = j.value("handover_node", -1);
        a.reason = j.value("reason", "");
    }
    
    return a;
}

// JsonComm implementation
JsonComm::JsonComm(std::istream* in, std::ostream* out, bool log)
    : input(in), output(out), messageCount(0), logMessages(log) {}

void JsonComm::sendInit(double timestamp) {
    json msg;
    msg["type"] = "INIT";
    msg["timestamp"] = timestamp;
    msg["warehouse_layout"] = buildWarehouseLayout();
    msg["products"] = serializeProducts();
    msg["robots"] = serializeRobots(timestamp);

    //debug for checking init json structure
    // std::string debug_json = msg.dump(4);
    // std::cerr << "\n[JSON DEBUG] Skickar INIT-meddelande till RL-agent:\n";
    // std::cerr << "------------------------------------------\n";
    // std::cerr << debug_json << "\n";
    // std::cerr << "------------------------------------------\n";
    // std::cerr.flush();

    *output << msg.dump() << std::endl;
    flush();
    
    if (logMessages) logMessage("SEND", msg);
}

void JsonComm::sendNewTask(const Task& task, double timestamp) {
    json msg;
    msg["type"] = "NEW_TASK";
    msg["timestamp"] = timestamp;
    msg["task"] = task.toJson();
    msg["state"] = buildStateJson(timestamp);
    
    *output << msg.dump() << std::endl;
    flush();
    
    if (logMessages) logMessage("SEND", msg);
}

void JsonComm::sendRobotStatus(int robotIndex, StatusType status, 
                               const std::string& taskId, double timestamp,
                               const std::string& message) {
    json msg;
    msg["type"] = "ROBOT_STATUS";
    msg["timestamp"] = timestamp;
    msg["robot_index"] = robotIndex;
    msg["task_id"] = taskId;
    msg["status_type"] = statusTypeToString(status);
    msg["message"] = message;
    
    if (robotIndex >= 0 && robotIndex < static_cast<int>(robots.size())) {
        const Robot& robot = robots[robotIndex];
        msg["robot_id"] = robot.getId();
        msg["current_node"] = robot.getCurrentNode();
        msg["battery"] = robot.getBattery();
    }
    
    msg["state"] = buildStateJson(timestamp);
    
    *output << msg.dump() << std::endl;
    flush();
    
    if (logMessages) logMessage("SEND", msg);
}

void JsonComm::sendAck(const std::string& taskId, int robotIndex, double estimatedCompletionTime) {
    json msg;
    msg["type"] = "ACK";
    msg["task_id"] = taskId;
    msg["robot_index"] = robotIndex;
    msg["status"] = "accepted";
    msg["estimated_completion_time"] = estimatedCompletionTime;
    
    if (robotIndex >= 0 && robotIndex < static_cast<int>(robots.size())) {
        msg["robot_id"] = robots[robotIndex].getId();
    }
    
    *output << msg.dump() << std::endl;
    flush();
    
    if (logMessages) logMessage("SEND", msg);
}

void JsonComm::sendError(const std::string& taskId, const std::string& errorCode,
                        const std::string& message, int robotIndex) {
    json msg;
    msg["type"] = "ERROR";
    msg["task_id"] = taskId;
    msg["error_code"] = errorCode;
    msg["message"] = message;
    msg["robot_index"] = robotIndex;
    
    *output << msg.dump() << std::endl;
    flush();
    
    if (logMessages) logMessage("SEND", msg);
}

void JsonComm::sendEpisodeEnd(double timestamp) {
    json msg;
    msg["type"] = "EPISODE_END";
    msg["timestamp"] = timestamp;
    
    // Add metrics from logger if available
    if (globalLogger) {
        EpisodeMetrics metrics = globalLogger->getMetrics();
        json metricsJson;
        metricsJson["orders_completed"] = metrics.getOrdersCompleted();
        metricsJson["orders_failed"] = metrics.getOrdersFailed();
        metricsJson["total_distance"] = metrics.getTotalDistanceTraveled();
        metricsJson["avg_completion_time"] = metrics.getAvgCompletionTime();
        metricsJson["robot_utilization"] = metrics.getRobotUtilization();
        msg["metrics"] = metricsJson;
    }
    
    msg["final_state"] = buildStateJson(timestamp);
    
    *output << msg.dump() << std::endl;
    flush();
    
    if (logMessages) logMessage("SEND", msg);
}

json JsonComm::receiveMessage() {
    std::string line;
    if (std::getline(*input, line)) {
        try {
            json msg = json::parse(line);
            if (logMessages) logMessage("RECV", msg);
            return msg;
        } catch (const json::exception& e) {
            std::cerr << "[JSON] Parse error: " << e.what() << std::endl;
            return json::object();
        }
    }
    return json::object();
}

Action JsonComm::receiveAction() {
    json msg = receiveMessage();
    
    if (msg.contains("action")) {
        return Action::fromJson(msg["action"]);
    }
    
    // Return wait action if invalid
    Action waitAction;
    waitAction.actionType = ActionType::WAIT;
    waitAction.robotIndex = -1;
    return waitAction;
}

bool JsonComm::receiveReset(int& outEpisodeNumber) {
    json msg = receiveMessage();
    
    if (msg.value("type", "") == "RESET") {
        outEpisodeNumber = msg.value("episode_number", 0);
        return true;
    }
    
    return false;
}

json JsonComm::buildStateJson(double timestamp) {
    json state;
    state["sim_time"] = timestamp;
    state["robots"] = serializeRobots(timestamp);
    state["inventory"] = serializeInventory();
    state["loading_dock"] = serializeLoadingDock();
    state["front_desk"] = serializeFrontDesk();
    state["charging_station"] = serializeChargingStation();
    return state;
}

json JsonComm::buildWarehouseLayout() {
    json layout;
    layout["nodes"] = serializeNodes();
    layout["edges"] = serializeEdges();
    return layout;
}

json JsonComm::serializeNodes() {
    json nodesArray = json::array();
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        const Node& node = nodes[i];
        json nodeJson;
        nodeJson["index"] = i;
        nodeJson["id"] = node.getId();
        nodeJson["max_robots"] = node.getMaxRobots();
        
        // Type
        switch(node.getType()) {
            case NodeType::Shelf: nodeJson["type"] = "Shelf"; break;
            case NodeType::LoadingBay: nodeJson["type"] = "LoadingBay"; break;
            case NodeType::FrontDesk: nodeJson["type"] = "FrontDesk"; break;
            case NodeType::ChargingStation: nodeJson["type"] = "ChargingStation"; break;
            case NodeType::Junction: nodeJson["type"] = "Junction"; break;
        }
        
        // Zone
        switch(node.getZone()) {
            case Zone::Hot: nodeJson["zone"] = "Hot"; break;
            case Zone::Warm: nodeJson["zone"] = "Warm"; break;
            case Zone::Cold: nodeJson["zone"] = "Cold"; break;
            case Zone::Other: nodeJson["zone"] = "Other"; break;
        }
        
        nodesArray.push_back(nodeJson);
    }
    
    return nodesArray;
}

json JsonComm::serializeEdges() {
    json edgesArray = json::array();
    
    for (size_t from = 0; from < adj.size(); ++from) {
        for (const Edge& edge : adj[from]) {
            json edgeJson;
            edgeJson["from"] = from;
            edgeJson["to"] = edge.to;
            edgeJson["distance"] = edge.distance;
            edgeJson["directed"] = edge.directed;
            edgesArray.push_back(edgeJson);
        }
    }
    
    return edgesArray;
}

json JsonComm::serializeProducts() {
    json productsArray = json::array();
    
    for (const Product& p : products) {
        json prodJson;
        prodJson["id"] = p.getId();
        prodJson["name"] = p.getName();
        prodJson["popularity"] = p.getPopularity();
        productsArray.push_back(prodJson);
    }
    
    return productsArray;
}

json JsonComm::serializeRobots(double timestamp) {
    json robotsArray = json::array();
    
    for (size_t i = 0; i < robots.size(); ++i) {
        const Robot& robot = robots[i];
        json robotJson;
        robotJson["id"] = robot.getId();
        robotJson["index"] = i;
        robotJson["current_node"] = robot.getCurrentNode();
        robotJson["target_node"] = robot.getTargetNode();
        robotJson["battery"] = robot.getBattery();
        robotJson["status"] = robot.getStatusString();
        robotJson["carrying"] = robot.isCarrying();
        robotJson["has_order"] = robot.getHasOrder();
        robotJson["speed"] = robot.getSpeed();
        
        if (robot.getHasOrder()) {
            const Order& order = robot.getCurrentOrder();
            json orderJson;
            orderJson["product_id"] = order.getProductID();
            orderJson["quantity"] = order.getQuantity();
            orderJson["slot_index"] = order.getSlotIndex();
            robotJson["current_order"] = orderJson;
        }
        
        robotsArray.push_back(robotJson);
    }
    
    return robotsArray;
}

json JsonComm::serializeInventory() {
    json inventoryArray = json::array();
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        const Node& node = nodes[i];
        if (node.getType() != NodeType::Shelf) continue;
        
        const Shelf* shelf = node.getShelf();
        if (!shelf) continue;
        
        json shelfJson;
        shelfJson["node_index"] = i;
        shelfJson["shelf_name"] = shelf->getName();
        
        // Zone
        switch(node.getZone()) {
            case Zone::Hot: shelfJson["zone"] = "Hot"; break;
            case Zone::Warm: shelfJson["zone"] = "Warm"; break;
            case Zone::Cold: shelfJson["zone"] = "Cold"; break;
            default: shelfJson["zone"] = "Other"; break;
        }
        
        json slotsArray = json::array();
        for (int j = 0; j < shelf->getSlotCount(); ++j) {
            Slot slot = shelf->getSlot(j);
            json slotJson;
            slotJson["slot_index"] = j;
            slotJson["product_id"] = slot.getProductID();
            slotJson["occupied"] = slot.getOccupied();
            slotJson["capacity"] = slot.getCapacity();
            slotJson["fill_rate"] = slot.getCapacity() > 0 ? 
                (double)slot.getOccupied() / slot.getCapacity() : 0.0;
            slotsArray.push_back(slotJson);
        }
        
        shelfJson["slots"] = slotsArray;
        inventoryArray.push_back(shelfJson);
    }
    
    return inventoryArray;
}

json JsonComm::serializeLoadingDock() {
    json dockJson;
    
    if (loadingDockNode >= 0 && loadingDockNode < static_cast<int>(nodes.size())) {
        const LoadingDock* dock = nodes[loadingDockNode].getLoadingDock();
        if (dock) {
            dockJson["occupied"] = dock->getIsOccupied();
            dockJson["delivery_count"] = dock->getDeliveryCount();
        }
    }
    
    return dockJson;
}

json JsonComm::serializeFrontDesk() {
    json deskJson;
    
    if (frontDeskNode >= 0 && frontDeskNode < static_cast<int>(nodes.size())) {
        const FrontDesk* desk = nodes[frontDeskNode].getFrontDesk();
        if (desk) {
            deskJson["pending_orders"] = desk->getPendingOrders();
        }
    }
    
    return deskJson;
}

json JsonComm::serializeChargingStation() {
    json chargeJson;
    
    if (chargingStationNode >= 0 && chargingStationNode < static_cast<int>(nodes.size())) {
        const ChargingStation* station = nodes[chargingStationNode].getChargingStation();
        if (station) {
            chargeJson["occupied"] = station->getIsOccupied();
            chargeJson["available_ports"] = station->getChargingPorts() - station->getIsOccupied();
        }
    }
    
    return chargeJson;
}

void JsonComm::flush() {
    output->flush();
}

void JsonComm::logMessage(const std::string& direction, const json& msg) {
    std::cerr << "[JSON " << direction << " #" << messageCount++ << "] " 
              << msg.dump(2) << std::endl;
}

std::string JsonComm::messageTypeToString(MessageType type) {
    switch(type) {
        case MessageType::INIT: return "INIT";
        case MessageType::NEW_TASK: return "NEW_TASK";
        case MessageType::ROBOT_STATUS: return "ROBOT_STATUS";
        case MessageType::ACTION_DECISION: return "ACTION_DECISION";
        case MessageType::ACK: return "ACK";
        case MessageType::ERROR_MSG: return "ERROR";
        case MessageType::EPISODE_END: return "EPISODE_END";
        case MessageType::RESET: return "RESET";
        default: return "UNKNOWN";
    }
}

std::string JsonComm::taskTypeToString(TaskType type) {
    switch(type) {
        case TaskType::CUSTOMER_ORDER: return "CUSTOMER_ORDER";
        case TaskType::INCOMING_DELIVERY: return "INCOMING_DELIVERY";
        case TaskType::RESTOCK_REQUEST: return "RESTOCK_REQUEST";
        default: return "UNKNOWN";
    }
}

std::string JsonComm::statusTypeToString(StatusType type) {
    switch(type) {
        case StatusType::TASK_COMPLETE: return "TASK_COMPLETE";
        case StatusType::TASK_FAILED: return "TASK_FAILED";
        case StatusType::LOW_BATTERY: return "LOW_BATTERY";
        case StatusType::STUCK: return "STUCK";
        case StatusType::HANDOVER_READY: return "HANDOVER_READY";
        case StatusType::CHARGING: return "CHARGING";
        default: return "UNKNOWN";
    }
}

std::string JsonComm::actionTypeToString(ActionType type) {
    switch(type) {
        case ActionType::PICKUP_AND_DELIVER: return "PICKUP_AND_DELIVER";
        case ActionType::RESTOCK: return "RESTOCK";
        case ActionType::CHARGE: return "CHARGE";
        case ActionType::HANDOVER: return "HANDOVER";
        case ActionType::WAIT: return "WAIT";
        default: return "UNKNOWN";
    }
}

// Global helpers
void initJsonComm(bool logging) {
    if (globalJsonComm) delete globalJsonComm;
    globalJsonComm = new JsonComm(&std::cin, &std::cout, logging);
}

void shutdownJsonComm() {
    if (globalJsonComm) {
        delete globalJsonComm;
        globalJsonComm = nullptr;
    }
}

void sendInitMessage() {
    if (globalJsonComm) {
        globalJsonComm->sendInit(0.0);
    }
}

void sendNewTaskMessage(const Task& task, double currentTime) {
    if (globalJsonComm) {
        globalJsonComm->sendNewTask(task, currentTime);
    }
}

void sendRobotStatusMessage(int robotIdx, StatusType status, const std::string& taskId,
                           double currentTime, const std::string& msg) {
    if (globalJsonComm) {
        globalJsonComm->sendRobotStatus(robotIdx, status, taskId, currentTime, msg);
    }
}