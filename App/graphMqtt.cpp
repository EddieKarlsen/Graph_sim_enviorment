#include "../includes/graphMqtt.hpp"
#include "../includes/json.hpp" // Används för JSON-hantering (behöver inkluderas i ditt projekt)

extern nlohmann::json getWarehouseStateJSON(); // Antag att du exporterar denna funktion från exportJson.hpp/cpp
extern void applySimulationChanges(const nlohmann::json& changes); // Ny funktion för att ändra simulatorns tillstånd

SimulatorMQTTClient::SimulatorMQTTClient() : client(BROKER_ADDRESS, CLIENT_ID) {
    client.set_callback([this](mqtt::const_message_ptr msg) { 
        this->on_message_received(msg); 
    });
}

void SimulatorMQTTClient::connect() {
    try {
        std::cout << "Attempting to connect to local MQTT broker at " << BROKER_ADDRESS << "..." << std::endl;
        mqtt::connect_options conn_opts;
        conn_opts.set_keep_alive_interval(20);
        conn_opts.set_clean_session(true);

        client.connect(conn_opts);
        std::cout << "Connected to local MQTT broker." << std::endl;

        // Simulatorn prenumererar på kommandon från gatewayen (skickas från AWS)
        client.subscribe("simulator/command", 1); 
        std::cout << "Subscribed to 'simulator/command'." << std::endl;

    } catch (const mqtt::exception& exc) {
        std::cerr << "ERROR: " << exc.what() << std::endl;
        throw;
    }
}

void SimulatorMQTTClient::disconnect() {
    try {
        client.disconnect();
        std::cout << "Disconnected from local MQTT broker." << std::endl;
    } catch (const mqtt::exception& exc) {
        std::cerr << "ERROR during disconnect: " << exc.what() << std::endl;
    }
}

void SimulatorMQTTClient::on_message_received(mqtt::const_message_ptr msg) {
    std::string topic = msg->get_topic();
    std::string payload = msg->to_string();
    std::cout << "Message arrived on topic: " << topic << std::endl;

    if (topic == "simulator/command") {
        handle_command(payload);
    }
}

void SimulatorMQTTClient::handle_command(const std::string& payload) {
    try {
        // Exempel: AWS kan skicka ett kommando för att uppdatera inventering/hyllor
        auto command_json = nlohmann::json::parse(payload);
        std::cout << "Applying simulation command..." << std::endl;
        applySimulationChanges(command_json); // <-- Denna funktion måste du implementera
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON Parse Error in command: " << e.what() << std::endl;
    }
}

void SimulatorMQTTClient::publish_warehouse_state(const nlohmann::json& json_state) {
    // Skickar lagerstatus till Python Gateway (topic: simulator/warehouse_update)
    try {
        std::string payload = json_state.dump();
        mqtt::message_ptr pubmsg = mqtt::make_message("simulator/warehouse_update", payload);
        pubmsg->set_qos(1);
        client.publish(pubmsg)->wait_for(std::chrono::seconds(5));
    } catch (const mqtt::exception& exc) {
        std::cerr << "ERROR publishing warehouse state: " << exc.what() << std::endl;
    }
}

void SimulatorMQTTClient::publish_order_arrived(const nlohmann::json& order_json) {
    // Skickar nya ordrar (topic: simulator/order_arrived)
    try {
        std::string payload = order_json.dump();
        mqtt::message_ptr pubmsg = mqtt::make_message("simulator/order_arrived", payload);
        pubmsg->set_qos(1);
        client.publish(pubmsg);
    } catch (const mqtt::exception& exc) {
        std::cerr << "ERROR publishing order arrived: " << exc.what() << std::endl;
    }
}