// #include <pybind11/pybind11.h>
// #include <pybind11/stl.h>
// #include <pybind11/complex.h>
// #include "../includes/datatypes.hpp" 
// #include "../includes/robot.hpp"
// #include "../includes/hotWarmCold.hpp"
// #include "../includes/initSim.hpp"
// #include "../includes/eventSystem.hpp"
// #include "../includes/logger.hpp"

// namespace py = pybind11;

// PYBIND11_MODULE(warehouse_env, m) {
//     m.doc() = "Pybind11 bindings for the Warehouse Simulation Environment";

//     // Enums
//     py::enum_<NodeType>(m, "NodeType")
//         .value("Shelf", NodeType::Shelf)
//         .value("LoadingBay", NodeType::LoadingBay)
//         .value("FrontDesk", NodeType::FrontDesk)
//         .value("ChargingStation", NodeType::ChargingStation)
//         .value("Junction", NodeType::Junction)
//         .export_values();
        
//     py::enum_<Zone>(m, "Zone")
//         .value("Hot", Zone::Hot)
//         .value("Warm", Zone::Warm)
//         .value("Cold", Zone::Cold)
//         .value("Other", Zone::Other)
//         .export_values();
        
//     py::enum_<RobotStatus>(m, "RobotStatus")
//         .value("Idle", RobotStatus::Idle)
//         .value("Moving", RobotStatus::Moving)
//         .value("Carrying", RobotStatus::Carrying)
//         .value("Charging", RobotStatus::Charging)
//         .value("Picking", RobotStatus::Picking)
//         .value("Dropping", RobotStatus::Dropping)
//         .export_values();
        
//     py::enum_<EventType>(m, "EventType")
//         .value("IncomingDelivery", EventType::IncomingDelivery)
//         .value("CustomerOrder", EventType::CustomerOrder)
//         .value("RobotTaskComplete", EventType::RobotTaskComplete)
//         .value("LowBattery", EventType::LowBattery)
//         .value("RestockNeeded", EventType::RestockNeeded)
//         .export_values();

//     // Structures
//     py::class_<Product>(m, "Product")
//         .def(py::init<>())
//         .def_readwrite("id", &Product::id)
//         .def_readwrite("name", &Product::name)
//         .def_readwrite("popularity", &Product::popularity);

//     py::class_<Slot>(m, "Slot")
//         .def(py::init<>())
//         .def_readwrite("occupied", &Slot::occupied)
//         .def_readwrite("productID", &Slot::productID)
//         .def_readwrite("capacity", &Slot::capacity);

//     py::class_<Shelf>(m, "Shelf")
//         .def(py::init<>())
//         .def_readwrite("name", &Shelf::name)
//         .def_readwrite("slotCount", &Shelf::slotCount)
//         .def_property_readonly("slots", [](const Shelf& s) {
//             std::vector<Slot> slots_vec;
//             for (int i = 0; i < s.slotCount; ++i) {
//                 slots_vec.push_back(s.slots[i]);
//             }
//             return slots_vec;
//         });
        
//     py::class_<LoadingDock>(m, "LoadingDock")
//         .def(py::init<>())
//         .def_readwrite("isOccupied", &LoadingDock::isOccupied)
//         .def_readwrite("deliveryCount", &LoadingDock::deliveryCount);
        
//     py::class_<ChargingStation>(m, "ChargingStation")
//         .def(py::init<>())
//         .def_readwrite("isOccupied", &ChargingStation::isOccupied)
//         .def_readwrite("chargingPorts", &ChargingStation::chargingPorts);
        
//     py::class_<FrontDesk>(m, "FrontDesk")
//         .def(py::init<>())
//         .def_readwrite("pendingOrders", &FrontDesk::pendingOrders);

//     py::class_<Node>(m, "Node")
//         .def(py::init<>())
//         .def_readwrite("id", &Node::id)
//         .def_readwrite("type", &Node::type)
//         .def_readwrite("maxRobots", &Node::maxRobots)
//         .def_readwrite("currentRobots", &Node::currentRobots)
//         .def_readwrite("zone", &Node::zone);
        
//     py::class_<Order>(m, "Order")
//         .def(py::init<>())
//         .def_readwrite("productID", &Order::productID)
//         .def_readwrite("slotIndex", &Order::slotIndex)
//         .def_readwrite("quantity", &Order::quantity);

//     py::class_<Robot>(m, "Robot")
//         .def(py::init<>())
//         .def_readwrite("id", &Robot::id)
//         .def_readwrite("currentNode", &Robot::currentNode)
//         .def_readwrite("targetNode", &Robot::targetNode)
//         .def_readwrite("progress", &Robot::progress)
//         .def_readwrite("positionX", &Robot::positionX)
//         .def_readwrite("positionY", &Robot::positionY)
//         .def_readwrite("status", &Robot::status)
//         .def_readwrite("battery", &Robot::battery)
//         .def_readwrite("carrying", &Robot::carrying)
//         .def_readwrite("hasOrder", &Robot::hasOrder)
//         .def_readwrite("currentOrder", &Robot::currentOrder);
        
//     py::class_<TimestepLog>(m, "TimestepLog")
//         .def(py::init<>())
//         .def_readwrite("time", &TimestepLog::time)
//         .def_readwrite("robotPositions", &TimestepLog::robotPositions)
//         .def_readwrite("taskUpdates", &TimestepLog::taskUpdates);

//     // Global variables
//     m.attr("nodes") = py::cast(&nodes, py::return_value_policy::reference);
//     m.attr("products") = py::cast(&products, py::return_value_policy::reference);
//     m.attr("robots") = py::cast(&robots, py::return_value_policy::reference);
    
//     // Global node indices
//     m.attr("loadingDockNode") = py::cast(&loadingDockNode, py::return_value_policy::reference);
//     m.attr("shelfANode") = py::cast(&shelfANode, py::return_value_policy::reference);
//     m.attr("shelfBNode") = py::cast(&shelfBNode, py::return_value_policy::reference);
//     m.attr("chargingStationNode") = py::cast(&chargingStationNode, py::return_value_policy::reference);
//     m.attr("frontDeskNode") = py::cast(&frontDeskNode, py::return_value_policy::reference);

//     // Initialization functions
//     m.def("initProducts", &initProducts, "Initializes all product definitions");
//     m.def("initGraphLayout", &initGraphLayout, "Initializes the fixed node and edge graph layout");
//     m.def("resetInventory", &resetInventory, "Resets shelf contents, products, and robots for a new episode");
//     m.def("initRobots", &initRobots, "Initializes robots at starting positions");
    
//     // Simulation functions
//     m.def("step_simulation", &step_simulation, 
//           "Performs one step in the C++ simulation",
//           py::arg("robotIdx"), 
//           py::arg("actionType"), 
//           py::arg("targetNode"), 
//           py::arg("productID") = -1);
    
//     // Popularity system
//     m.def("updatePopularityAndZone", &updatePopularityAndZone, 
//           "Increments popularity for a product",
//           py::arg("productID"));
          
//     // Event system
//     m.def("initEventSystem", &initEventSystem, 
//           "Initialize event system with seed",
//           py::arg("seed") = 42);
//     m.def("processEvents", &processEvents,
//           "Process events for given delta time",
//           py::arg("deltaTime"));
    
//     // Helper functions
//     m.def("findProductOnShelf", &findProductOnShelf,
//           "Find product location on shelves",
//           py::arg("productID"),
//           py::arg("outSlotIndex"));
//     m.def("findBestShelfForProduct", &findBestShelfForProduct,
//           "Find optimal shelf based on popularity zones",
//           py::arg("productID"));

//     py::class_<EpisodeMetrics>(m, "EpisodeMetrics")
//     .def_readwrite("episodeNumber", &EpisodeMetrics::episodeNumber)
//     .def_readwrite("totalTime", &EpisodeMetrics::totalTime)
//     .def_readwrite("ordersCompleted", &EpisodeMetrics::ordersCompleted)
//     .def_readwrite("ordersFailed", &EpisodeMetrics::ordersFailed)
//     .def_readwrite("avgCompletionTime", &EpisodeMetrics::avgCompletionTime)
//     .def_readwrite("totalDistanceTraveled", &EpisodeMetrics::totalDistanceTraveled)
//     .def_readwrite("totalBatteryUsed", &EpisodeMetrics::totalBatteryUsed)
//     .def_readwrite("optimalZonePlacements", &EpisodeMetrics::optimalZonePlacements)
//     .def_readwrite("robotUtilization", &EpisodeMetrics::robotUtilization);

//     py::class_<EpisodeLogger>(m, "EpisodeLogger")
//         .def(py::init<const std::string&, double>(),
//             py::arg("logDir") = "./logs",
//             py::arg("snapshotInterval") = 1.0)
//         .def("startEpisode", &EpisodeLogger::startEpisode)
//         .def("endEpisode", &EpisodeLogger::endEpisode)
//         .def("logRobotSnapshot", &EpisodeLogger::logRobotSnapshot)
//         .def("saveToJson", &EpisodeLogger::saveToJson)
//         .def("saveMetricsOnly", &EpisodeLogger::saveMetricsOnly)
//         .def("saveHeatmapOnly", &EpisodeLogger::saveHeatmapOnly)
//         .def("getMetrics", &EpisodeLogger::getMetrics);

//     // Helper functions
//     m.def("initLogger", &initLogger, 
//         py::arg("logDir") = "./logs", 
//         py::arg("snapshotInterval") = 1.0);
//     m.def("startLogging", &startLogging, py::arg("episodeNumber"));
//     m.def("stopLogging", &stopLogging);
//     m.def("logSnapshot", &logSnapshot, py::arg("currentTime"));
//     m.def("saveEpisodeData", &saveEpisodeData, py::arg("filename"));
// }