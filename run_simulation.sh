#!/bin/bash

# Warehouse Simulation Runner
# Starts C++ simulation and Python RL agent

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

# Create logs directory
mkdir -p logs

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

echo "Logs will be saved to:"
echo "  Simulation: $SIM_LOG"
echo "  RL Agent:   $RL_LOG"
echo "  Combined:   $COMBINED_LOG"
echo ""

# Function to cleanup on exit
cleanup() {
    echo ""
    echo "=== Simulation Stopped ==="
    
    # Kill any remaining processes
    pkill -P $$ 2>/dev/null || true
    
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
}

trap cleanup EXIT INT TERM

# Start simulation
echo "=== Starting Simulation ==="
echo ""

if [ "$DEBUG" = true ]; then
    # Debug mode: show output in terminal
    ./warehouse_sim 2> >(tee "$SIM_LOG" >&2) | python3 ./rl_agent.py 2> >(tee "$RL_LOG" >&2)
else
    # Normal mode: redirect to log files
    ./warehouse_sim 2>"$SIM_LOG" | python3 ./rl_agent.py 2>"$RL_LOG"
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