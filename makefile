# Makefile for Warehouse Simulation

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iincludes
LDFLAGS = 

# Directories
SRC_DIR = src
INC_DIR = includes
OBJ_DIR = obj
BIN_DIR = .

# Target executables
TARGET = $(BIN_DIR)/warehouse_sim
TEST_PATHFINDING = $(BIN_DIR)/test_pathfinding

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Header files (for dependency tracking)
HEADERS = $(wildcard $(INC_DIR)/*.hpp)

# Default target
all: $(TARGET)

# Create directories if they don't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Link
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)
	@echo "Clean complete"

# Clean logs
clean-logs:
	rm -rf logs/*.json logs/*.log
	@echo "Logs cleaned"

# Full clean
distclean: clean clean-logs
	@echo "Full clean complete"

# Run simulation
run: $(TARGET)
	@chmod +x run_simulation.sh
	@./run_simulation.sh

# Run with debug
debug: $(TARGET)
	@chmod +x run_simulation.sh
	@./run_simulation.sh --debug

# Help
help:
	@echo "Warehouse Simulation Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build the simulation (default)"
	@echo "  clean       - Remove object files and executable"
	@echo "  clean-logs  - Remove log files"
	@echo "  distclean   - Full clean (objects + logs)"
	@echo "  run         - Build and run simulation"
	@echo "  debug       - Build and run with debug output"
	@echo "  help        - Show this help message"
	@echo ""
	@echo "Files will be compiled from:"
	@echo "  Source: $(SRC_DIR)/"
	@echo "  Headers: $(INC_DIR)/"
	@echo "  Objects: $(OBJ_DIR)/"
	@echo ""

.PHONY: all clean clean-logs distclean run debug help