#!/bin/bash
set -e

# Resolve absolute paths
# $(dirname "$0") gives relative path to script. 
# We go there, then pwd to get absolute.
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
CPP_DIR="$SCRIPT_DIR/cpp_push"
RUST_DIR="$SCRIPT_DIR/rust_pull"

# Function to clean up background processes on exit
cleanup() {
    echo ""
    echo "--- Stopping Demo ---"
    if [ -n "$RUST_PID" ]; then
        kill $RUST_PID 2>/dev/null || true
    fi
    exit
}

# Trap signals
trap cleanup SIGINT SIGTERM

echo "=== 1. Building C++ Client ==="
mkdir -p "$CPP_DIR/build"
cd "$CPP_DIR/build"
cmake ..
make -j$(nproc)

echo ""
echo "=== 2. Building Rust Server ==="
cd "$RUST_DIR"
cargo build --quiet

echo ""
echo "=== 3. Running Demo ==="
echo "Starting Rust Server (Background)..."
cargo run --quiet &
RUST_PID=$!

# Give the server a moment to bind the port
sleep 1

echo "Starting C++ Client (Foreground)..."
echo "Press Ctrl+C to stop."
echo "-----------------------------------"

# Run C++ client
"$CPP_DIR/build/cpp_push"

# Wait for background process (this part won't be reached if C++ loops forever until Ctrl+C)
wait $RUST_PID
