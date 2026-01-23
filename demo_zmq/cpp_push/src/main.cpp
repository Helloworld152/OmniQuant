#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <zmq.hpp>    // Use cppzmq instead of zmq.h
#include "omni.pb.h"

int main() {
    std::cout << "Starting C++ PUSH Client (cppzmq + Protobuf)..." << std::endl;

    // 1. Create Context and Socket using cppzmq classes
    zmq::context_t context(1);
    zmq::socket_t pusher(context, zmq::socket_type::push);

    // 2. Connect to Rust Server
    try {
        pusher.connect("tcp://localhost:5556");
        std::cout << "Connected to tcp://localhost:5556" << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "Connection failed: " << e.what() << std::endl;
        return 1;
    }

    // 3. Send Loop
    int seq = 0;
    while (true) {
        omni::EventFrame frame;
        
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        frame.set_timestamp_ns(timestamp);
        frame.set_source_id("cpp_demo_gateway");

        omni::MarketData* tick = frame.mutable_tick();
        tick->set_symbol("BTC-USDT");
        tick->set_exchange("BINANCE");
        tick->set_last_price(42000.0 + (seq % 100));
        tick->set_volume(seq * 10);

        // Serialize
        std::string payload;
        frame.SerializeToString(&payload);
        
        // Send using zmq::message_t
        zmq::message_t message(payload.size());
        memcpy(message.data(), payload.data(), payload.size());
        
        auto res = pusher.send(message, zmq::send_flags::none);
        
        std::cout << "Sent Tick #" << seq << ": " << tick->symbol() << " @ " << tick->last_price() << std::endl;

        seq++;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}