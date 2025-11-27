#!/bin/bash

# Warehouse Simulation Runner
# Starts C++ simulation and Python RL agent with bidirectional communication

set -e  # Exit on error

echo "=== Warehouse Simulation Runner ==="
echo ""

# Check if compiled
if [ ! -f "./warehouse_sim" ]; then
    echo "ERROR: warehouse_sim executable not found"
    echo "Please compile first with: make"
    exit 1
fi

# Check if Python script exists
if [ ! -f "./rl_agent.py" ]; then
    echo "ERROR: rl_agent.py not found"
    exit 1
fi

# Make Python script executable
chmod +x ./rl_agent.py

# Create logs and pipes directory
mkdir -p logs
mkdir -p /tmp/warehouse_pipes

# Parse arguments
EPISODES=1
DEBUG=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -e|--episodes)
            EPISODES="$2"
            shift 2
            ;;
        -d|--debug)
            DEBUG=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -e, --episodes NUM    Number of episodes to run (default: 1)"
            echo "  -d, --debug           Enable debug logging"
            echo "  -h, --help            Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h for help"
            exit 1
            ;;
    esac
done

echo "Configuration:"
echo "  Episodes: $EPISODES"
echo "  Debug: $DEBUG"
echo ""

# Prepare log files
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SIM_LOG="logs/sim_${TIMESTAMP}.log"
RL_LOG="logs/rl_${TIMESTAMP}.log"
COMBINED_LOG="logs/combined_${TIMESTAMP}.log"

# Create named pipes for bidirectional communication
CPP_TO_PY_PIPE="/tmp/warehouse_pipes/cpp_to_py_$$"
PY_TO_CPP_PIPE="/tmp/warehouse_pipes/py_to_cpp_$$"

echo "Setting up bidirectional communication..."
echo "  C++ → Python pipe: $CPP_TO_PY_PIPE"
echo "  Python → C++ pipe: $PY_TO_CPP_PIPE"
echo ""

# Remove old pipes if they exist
rm -f "$CPP_TO_PY_PIPE" "$PY_TO_CPP_PIPE"

# Create named pipes
mkfifo "$CPP_TO_PY_PIPE"
mkfifo "$PY_TO_CPP_PIPE"

echo "Logs will be saved to:"
echo "  Simulation: $SIM_LOG"
echo "  RL Agent:   $RL_LOG"
echo "  Combined:   $COMBINED_LOG"
echo ""

# Function to cleanup on exit
cleanup() {
    echo ""
    echo "=== Cleaning up ==="
    
    # Kill any remaining processes
    jobs -p | xargs -r kill 2>/dev/null || true
    wait 2>/dev/null || true
    
    # Remove named pipes
    rm -f "$CPP_TO_PY_PIPE" "$PY_TO_CPP_PIPE"
    
    # Merge logs
    if [ -f "$SIM_LOG" ] && [ -f "$RL_LOG" ]; then
        echo "Merging logs..."
        {
            echo "=== SIMULATION LOG ==="
            cat "$SIM_LOG"
            echo ""
            echo "=== RL AGENT LOG ==="
            cat "$RL_LOG"
        } > "$COMBINED_LOG"
        echo "Combined log saved to: $COMBINED_LOG"
    fi
    
    echo "=== Simulation Stopped ==="
}

trap cleanup EXIT INT TERM

# Start simulation
echo "=== Starting Simulation with Duplex Communication ==="
echo ""

if [ "$DEBUG" = true ]; then
    # Debug mode: show output in terminal
    
    # Start both processes simultaneously in background to avoid pipe deadlock
    echo "Starting C++ simulation in background..."
    ./warehouse_sim "$CPP_TO_PY_PIPE" "$PY_TO_CPP_PIPE" 2> >(tee "$SIM_LOG" >&2) &
    CPP_PID=$!
    
    echo "Starting Python RL agent in background..."
    python3 ./rl_agent.py "$CPP_TO_PY_PIPE" "$PY_TO_CPP_PIPE" 2> >(tee "$RL_LOG" >&2) &
    PY_PID=$!
    
    # Wait for both processes
    echo "Waiting for processes to complete..."
    wait $CPP_PID
    CPP_EXIT=$?
    wait $PY_PID
    PY_EXIT=$?
    
    if [ $CPP_EXIT -ne 0 ]; then
        echo "WARNING: C++ simulation exited with code $CPP_EXIT"
    fi
    if [ $PY_EXIT -ne 0 ]; then
        echo "WARNING: Python agent exited with code $PY_EXIT"
    fi
else
    # Normal mode: redirect to log files
    
    # Start both processes simultaneously in background
    echo "Starting C++ simulation in background..."
    ./warehouse_sim "$CPP_TO_PY_PIPE" "$PY_TO_CPP_PIPE" 2>"$SIM_LOG" &
    CPP_PID=$!
    
    echo "Starting Python RL agent in background..."
    python3 ./rl_agent.py "$CPP_TO_PY_PIPE" "$PY_TO_CPP_PIPE" 2>"$RL_LOG" &
    PY_PID=$!
    
    # Wait for both processes
    echo "Waiting for processes to complete..."
    wait $CPP_PID
    CPP_EXIT=$?
    wait $PY_PID
    PY_EXIT=$?
    
    if [ $CPP_EXIT -ne 0 ]; then
        echo "WARNING: C++ simulation exited with code $CPP_EXIT"
    fi
    if [ $PY_EXIT -ne 0 ]; then
        echo "WARNING: Python agent exited with code $PY_EXIT"
    fi
fi

echo ""
echo "=== Simulation Completed ==="
echo ""

# Show summary
if [ -f "$RL_LOG" ]; then
    echo "Summary:"
    echo "--------"
    grep -E "(Episode.*ended|Orders completed|Decisions made)" "$RL_LOG" || echo "No statistics found"
fi

echo ""
echo "Done!"