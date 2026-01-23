# C++ to Rust ZeroMQ Demo

This demo replicates the core communication channel of OmniQuant:
- **Sender (C++)**: Simulates the Gateway.
- **Receiver (Rust)**: Simulates the Core Router.

## Prerequisites
- CMake
- `libzmq3-dev` (Debian/Ubuntu) or `zeromq` (Homebrew)
- Rust & Cargo

## 1. Rust PULL Server (Receiver)
Start the receiver first so it binds the port.

```bash
cd rust_pull
cargo run
```
*Output: Listening on tcp://127.0.0.1:5556*

## 2. C++ PUSH Client (Sender)
In a new terminal:

```bash
cd cpp_push
mkdir build && cd build
cmake ..
make
./cpp_push
```
*Output: Sent: Hello from C++ #0 ...*

## Result
You should see the Rust terminal printing the messages sent by the C++ process.