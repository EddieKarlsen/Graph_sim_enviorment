#pragma once

#include <string>
#include <iostream>
#include <mqtt/client.h>
#include "datatypes.hpp" // Inkludera dina befintliga datatyper

class SimulatorMQTTClient {
private:
    mqtt::client client;
    const std::string BROKER_ADDRESS = "tcp://localhost:1883";
    const std::string CLIENT_ID = "WarehouseSimulator";

    // Callbacks för MQTT
    void on_message_received(mqtt::const_message_ptr msg);
    void on_connection_lost(const std::string& cause);

public:
    SimulatorMQTTClient();
    void connect();
    void disconnect();
    void loop(); // Håll anslutningen vid liv

    // Funktioner för att skicka data
    void publish_warehouse_state(const nlohmann::json& json_state);
    void publish_order_arrived(const nlohmann::json& order_json);
    
    // Funktion för att hantera kommandon (t.ex. skickade från AWS via Python-gatewayen)
    void handle_command(const std::string& payload);
};