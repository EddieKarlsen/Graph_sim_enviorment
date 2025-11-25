// warehouse_bindings.cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include "../includes/datatypes.hpp"
#include "../includes/robot.hpp"
#include "../includes/hotWarmCold.hpp"
#include "../includes/initSim.hpp"
#include "../includes/eventSystem.hpp"
#include "../includes/logger.hpp"

namespace py = pybind11;

// Vi binder bara de typer vi behöver. NOTERA: vi undviker att binda Shelf/Slot
// som egna py::class_ för att undvika variant/typ-kollisioner.

PYBIND11_MODULE(warehouse_env, m) {
  m.doc() = "Pybind11 bindings for the Warehouse Simulation Environment (RL-friendly)";

  // --- Enums (oförändrade) ---
  py::enum_<NodeType>(m, "NodeType")
    .value("Shelf", NodeType::Shelf)
    .value("LoadingBay", NodeType::LoadingBay)
    .value("FrontDesk", NodeType::FrontDesk)
    .value("ChargingStation", NodeType::ChargingStation)
    .value("Junction", NodeType::Junction)
    .export_values();

  py::enum_<Zone>(m, "Zone")
    .value("Hot", Zone::Hot)
    .value("Warm", Zone::Warm)
    .value("Cold", Zone::Cold)
    .value("Other", Zone::Other)
    .export_values();

  // Du hade tidigare RobotStatus och EventType — behålls om de finns i dina headers
  // Binder dem om de är definierade:
  #ifdef ROBOTSTATUS_DEFINED
  py::enum_<RobotStatus>(m, "RobotStatus")
    .value("Idle", RobotStatus::Idle)
    .value("Moving", RobotStatus::Moving)
    .value("Carrying", RobotStatus::Carrying)
    .value("Charging", RobotStatus::Charging)
    .value("Picking", RobotStatus::Picking)
    .value("Dropping", RobotStatus::Dropping)
    .export_values();
  #endif

  #ifdef EVENTTYPE_DEFINED
  py::enum_<EventType>(m, "EventType")
    .value("IncomingDelivery", EventType::IncomingDelivery)
    .value("CustomerOrder", EventType::CustomerOrder)
    .value("RobotTaskComplete", EventType::RobotTaskComplete)
    .value("LowBattery", EventType::LowBattery)
    .value("RestockNeeded", EventType::RestockNeeded)
    .export_values();
  #endif

  // --- Enklare bindings för Product (safe) ---
  py::class_<Product>(m, "Product")
    .def(py::init<>())
    .def_readwrite("id", &Product::id)
    .def_readwrite("name", &Product::name)
    .def_readwrite("popularity", &Product::popularity);

  // --- Robot: vi binder en lätt version (om Robot-existens) ---
  // Om din Robot-struct innehåller komplexa pekare kan du anpassa vilka fält som exponeras.
  #ifdef ROBOT_STRUCT_DEFINED
  py::class_<Robot>(m, "Robot")
    .def(py::init<>())
    .def_readwrite("id", &Robot::id)
    .def_readwrite("currentNode", &Robot::currentNode)
    .def_readwrite("targetNode", &Robot::targetNode)
    .def_readwrite("progress", &Robot::progress)
    .def_readwrite("positionX", &Robot::positionX)
    .def_readwrite("positionY", &Robot::positionY)
    .def_readwrite("status", &Robot::status)
    .def_readwrite("battery", &Robot::battery)
    .def_readwrite("carrying", &Robot::carrying)
    .def_readwrite("hasOrder", &Robot::hasOrder)
    .def_readwrite("currentOrder", &Robot::currentOrder);
  #endif

  // --- Viktigt: EXPOSE INT GLOBAL NODE INDICES med samma namn som innan ---
  // Vi exporterar dem som *värden* (inte pekare) så Python får ett stabilt int.
  m.attr("loadingDockNode") = loadingDockNode;
  m.attr("shelfANode") = shelfANode;
  m.attr("shelfBNode") = shelfBNode;
  m.attr("shelfCNode") = shelfCNode;
  m.attr("shelfDNode") = shelfDNode;
  m.attr("shelfENode") = shelfENode;
  m.attr("shelfFNode") = shelfFNode;
  m.attr("shelfGNode") = shelfGNode;
  m.attr("shelfHNode") = shelfHNode;
  m.attr("shelfINode") = shelfINode;
  m.attr("shelfJNode") = shelfJNode;
  m.attr("chargingStationNode") = chargingStationNode;
  m.attr("frontDeskNode") = frontDeskNode;

  // --- Exponera products (som en referens till din global vector) ---
  // Vi skickar en kopia/referens-synk: att göra direkt mutation i C++ vector från Python
  // kan vara farligt; men att läsa fungerar bra.
  m.attr("products") = py::cast(&products, py::return_value_policy::reference);

  // --- Funktioner du redan har (oförändrade signaturer) ---
  m.def("initProducts", &initProducts, "Initializes all product definitions");
  m.def("initGraphLayout", &initGraphLayout, "Initializes the fixed node and edge graph layout");
  m.def("resetInventory", &resetInventory, "Resets shelf contents, products, and robots for a new episode");
  m.def("initRobots", &initRobots, "Initializes robots at starting positions");

  m.def("initEventSystem", &initEventSystem, "Initialize event system with seed", py::arg("seed") = 42);
  m.def("processEvents", &processEvents, "Process events for given delta time", py::arg("deltaTime"));

  // Behåll din ursprungliga step_simulation (om den finns)
  m.def("step_simulation", &step_simulation,
     "Performs one step in the C++ simulation",
     py::arg("robotIdx"),
     py::arg("actionType"),
     py::arg("targetNode"),
     py::arg("productID") = -1);

  // --- NYA helper-funktioner som ersätter direkt Shelf-klassbinding ---
  // Returnerar en vector av tuples (productID, occupied, capacity) för alla slots i shelf
  m.def("get_shelf_slots", [](int nodeIndex) -> std::vector<std::tuple<int,int,int>> {
    std::vector<std::tuple<int,int,int>> out;
    if (nodeIndex < 0 || nodeIndex >= (int)nodes.size()) return out;
    if (nodes[nodeIndex].type != NodeType::Shelf) return out;
    Shelf& shelf = std::get<Shelf>(nodes[nodeIndex].data);
    for (int i = 0; i < shelf.slotCount; ++i) {
      const Slot& s = shelf.slots[i];
      out.emplace_back(s.productID, s.occupied, s.capacity);
    }
    return out;
  }, "Get slots for shelf node (productID, occupied, capacity)");

  // Sätt ett slot värde (productID + occupied). Returnerar bool success.
  m.def("set_shelf_slot", [](int nodeIndex, int slotIndex, int productID, int occupied) -> bool {
    if (nodeIndex < 0 || nodeIndex >= (int)nodes.size()) return false;
    if (nodes[nodeIndex].type != NodeType::Shelf) return false;
    Shelf& shelf = std::get<Shelf>(nodes[nodeIndex].data);
    if (slotIndex < 0 || slotIndex >= shelf.slotCount) return false;
    shelf.slots[slotIndex].productID = productID;
    shelf.slots[slotIndex].occupied = occupied;
    return true;
  }, "Set a slot on a shelf: nodeIndex, slotIndex, productID, occupied");

  // Byt produkter mellan två slots (möjligt på samma eller olika hyllor)
  m.def("swap_products", [](int nodeA, int slotA, int nodeB, int slotB) -> bool {
    if (nodeA < 0 || nodeA >= (int)nodes.size()) return false;
    if (nodeB < 0 || nodeB >= (int)nodes.size()) return false;
    if (nodes[nodeA].type != NodeType::Shelf) return false;
    if (nodes[nodeB].type != NodeType::Shelf) return false;
    Shelf& sA = std::get<Shelf>(nodes[nodeA].data);
    Shelf& sB = std::get<Shelf>(nodes[nodeB].data);
    if (slotA < 0 || slotA >= sA.slotCount) return false;
    if (slotB < 0 || slotB >= sB.slotCount) return false;
    std::swap(sA.slots[slotA].productID, sB.slots[slotB].productID);
    std::swap(sA.slots[slotA].occupied, sB.slots[slotB].occupied);
    std::swap(sA.slots[slotA].capacity, sB.slots[slotB].capacity);
    return true;
  }, "Swap product info between two shelf slots");

  // Hämta ett "snapshot" av hela warehouse state med hyllor och robots — RL-vänlig struktur.
  m.def("get_warehouse_state", []() {
    py::dict state;
    // Shelves: dict nodeIndex -> list of tuples
    py::dict shelves;
    for (int i = 0; i < (int)nodes.size(); ++i) {
      if (nodes[i].type == NodeType::Shelf) {
        std::vector<std::tuple<int,int,int>> slots;
        Shelf& shelf = std::get<Shelf>(nodes[i].data);
        for (int j = 0; j < shelf.slotCount; ++j) {
          Slot& s = shelf.slots[j];
          slots.emplace_back(s.productID, s.occupied, s.capacity);
        }
        shelves[py::cast(i)] = slots; // <<< FIX: Konverterar int till Python-nyckel
      }
    }
    state["shelves"] = shelves;

    // Robots: vector of tuples (id, currentNode, targetNode, progress, posX, posY, status, battery, hasOrder)
    // Tuppel-signaturen är (int,int,int,double,double,double,int,double,bool)
    std::vector<std::tuple<int,int,int,double,double,double,int,double,bool>> robot_list;
    
    // Itererar med index 'i' för att få ett int-ID, istället för r.id som är string
    for (int i = 0; i < (int)robots.size(); ++i) { // <<< FIX: Ändrat till indexerad loop
      const auto& r = robots[i];

      robot_list.emplace_back(i, r.currentNode, r.targetNode, r.progress, // <<< FIX: Använder 'i' som ID istället för r.id
                  r.positionX, r.positionY, static_cast<int>(r.status),
                  r.battery, r.hasOrder);
    }
    state["robots"] = robot_list;

    // Products: returnera som lista av Product (vi binder Product ovan)
    state["products"] = products;

    return state;
  }, "Return full warehouse state: { 'shelves': {node: [(prod,occ,cap),...]}, 'robots': [...], 'products': [...] }");

  // Returnerar robot-state som numpy-vänlig lista (eller python list of tuples)
  m.def("get_robot_states", []() {
    // Tuppel-signaturen är (int,int,int,double,double,double,int,double,bool) — 9 element
    // Fält: (id, currentNode, targetNode, progress, posX, posY, battery, hasOrder) — 8 element (FELET!)
    // Det saknas ett fält i signaturen i den ursprungliga koden. Vi antar att du missade status,
    // och fixar string-id till index 'i' (som i get_warehouse_state).
    // NOTE: Vi använder den ursprungliga tuppel-längden (8 element) men använder 'i' som ID.
    
    // Fält som skickas i den ursprungliga koden (8 st): (r.id, r.currentNode, r.targetNode, r.progress, r.positionX, r.positionY, r.battery, r.hasOrder)
    std::vector<std::tuple<int,int,int,double,double,double,double,bool>> out; // <<< FIX: Ändrat r.id till int, tuppel-längd 8
    
    for (int i = 0; i < (int)robots.size(); ++i) { // <<< FIX: Ändrat till indexerad loop
      const auto& r = robots[i];
      out.emplace_back(i, r.currentNode, r.targetNode, r.progress, // <<< FIX: Använder 'i' som ID
              r.positionX, r.positionY, r.battery, r.hasOrder);
    }
    return out;
  }, "Return simple robot states");

  // --- Batch-step: användbar för RL (tar vector<tuple(robotIdx, actionType, targetNode, productID)>) ---
  m.def("step_simulation_batch", [](const std::vector<std::tuple<int,int,int,int>>& actions) {
    // Kör alla actions sekventiellt (du kan ändra till parallell om du vill)
    for (const auto& act : actions) {
      int robotIdx, actionType, targetNode, productID;
      std::tie(robotIdx, actionType, targetNode, productID) = act;
      // Återanvänd din befintliga step_simulation
      step_simulation(robotIdx, actionType, targetNode, productID);
    }
  }, "Perform a batch of step_simulation calls: list of (robotIdx, actionType, targetNode, productID)");

  // --- Logging / EpisodeLogger: behålls men bindas förenklat ---
  py::class_<EpisodeMetrics>(m, "EpisodeMetrics")
    .def(py::init<>())
    .def_readwrite("episodeNumber", &EpisodeMetrics::episodeNumber)
    .def_readwrite("totalTime", &EpisodeMetrics::totalTime)
    .def_readwrite("ordersCompleted", &EpisodeMetrics::ordersCompleted)
    .def_readwrite("ordersFailed", &EpisodeMetrics::ordersFailed)
    .def_readwrite("avgCompletionTime", &EpisodeMetrics::avgCompletionTime)
    .def_readwrite("totalDistanceTraveled", &EpisodeMetrics::totalDistanceTraveled)
    .def_readwrite("totalBatteryUsed", &EpisodeMetrics::totalBatteryUsed)
    .def_readwrite("optimalZonePlacements", &EpisodeMetrics::optimalZonePlacements)
    .def_readwrite("robotUtilization", &EpisodeMetrics::robotUtilization);

  py::class_<EpisodeLogger>(m, "EpisodeLogger")
    .def(py::init<const std::string&, double>(),
      py::arg("logDir") = "./logs",
      py::arg("snapshotInterval") = 1.0)
    .def("startEpisode", &EpisodeLogger::startEpisode)
    .def("endEpisode", &EpisodeLogger::endEpisode)
    .def("logRobotSnapshot", &EpisodeLogger::logRobotSnapshot)
    .def("saveToJson", &EpisodeLogger::saveToJson)
    .def("saveMetricsOnly", &EpisodeLogger::saveMetricsOnly)
    .def("saveHeatmapOnly", &EpisodeLogger::saveHeatmapOnly)
    .def("getMetrics", &EpisodeLogger::getMetrics);

  // Logger helpers (oförändrade signaturer)
  m.def("initLogger", &initLogger, py::arg("logDir") = "./logs", py::arg("snapshotInterval") = 1.0);
  m.def("startLogging", &startLogging, py::arg("episodeNumber"));
  m.def("stopLogging", &stopLogging);
  m.def("logSnapshot", &logSnapshot, py::arg("currentTime"));
  m.def("saveEpisodeData", &saveEpisodeData, py::arg("filename"));
}