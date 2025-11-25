void SimulatorMQTTClient::on_message_received(mqtt::const_message_ptr msg) {
  std::string topic = msg->get_topic();
  std::string payload = msg->to_string();
  std::cout << "Message arrived on topic: " << topic << std::endl;

  if (topic == "sim/action") {
    handle_RL_action(payload);
  } else if (topic == "sim/command") {
         // Hantera globala kommandon (t.ex. RESET)
         handle_command(payload); 
    }
}

void SimulatorMQTTClient::handle_RL_action(const std::string& payload) {
  try {
    auto action_json = nlohmann::json::parse(payload);
        
        std::string action_type = action_json["type"].get<std::string>();
        std::string robot_id = action_json.value("robot_id", "");
        
        // --- Använd den mottagna handlingen för att uppdatera simuleringen ---
        if (action_type == "MOVE") {
            int target_node = action_json["target_node"].get<int>();
            // Starta robotens A*-sökning/rörelse till target_node
        } else if (action_type == "RELOCATE") {
            int product_id = action_json["product_id"].get<int>();
            int target_node = action_json["target_node"].get<int>();
            // Starta process för att flytta produkten (en långsammare process)
        } 
        // ... hantera PICKUP, DROPOFF, CHARGE, TRANSFER
        
    std::cout << "Executed RL Action: " << action_type << " for " << robot_id << std::endl;
  } catch (const nlohmann::json::parse_error& e) {
    std::cerr << "JSON Parse Error in RL Action: " << e.what() << std::endl;
  } catch (const std::exception& e) {
        std::cerr << "Error handling RL Action: " << e.what() << std::endl;
    }
}