#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <zmq.hpp>
#include "CtpGateway.h"
#include "omni.pb.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

// 默认配置
const char* DEFAULT_MD_FRONT = "tcp://182.254.243.31:40011"; 
const char* DEFAULT_TD_FRONT = "tcp://182.254.243.31:40001";

// 使用 rapidjson 构建 JSON 配置字符串
std::string build_config_json() {
    rapidjson::Document d;
    d.SetObject();
    auto& allocator = d.GetAllocator();

    auto get_env = [](const char* name, const char* def) -> std::string {
        const char* val = std::getenv(name);
        return val ? std::string(val) : std::string(def);
    };

    d.AddMember("md_front", rapidjson::Value(get_env("CTP_MD_FRONT", DEFAULT_MD_FRONT).c_str(), allocator).Move(), allocator);
    d.AddMember("td_front", rapidjson::Value(get_env("CTP_TD_FRONT", DEFAULT_TD_FRONT).c_str(), allocator).Move(), allocator);
    d.AddMember("broker_id", rapidjson::Value(get_env("CTP_BROKER", "").c_str(), allocator).Move(), allocator);
    d.AddMember("user_id", rapidjson::Value(get_env("CTP_USER", "").c_str(), allocator).Move(), allocator);
    d.AddMember("password", rapidjson::Value(get_env("CTP_PASS", "").c_str(), allocator).Move(), allocator);
    d.AddMember("app_id", rapidjson::Value(get_env("CTP_APPID", "").c_str(), allocator).Move(), allocator);
    d.AddMember("auth_code", rapidjson::Value(get_env("CTP_AUTHCODE", "").c_str(), allocator).Move(), allocator);
    
    // 解析订阅列表
    rapidjson::Value symbols(rapidjson::kArrayType);
    std::string subs = get_env("CTP_SUB_LIST", "rb2605,au2606,ag2606");
    size_t pos = 0;
    while ((pos = subs.find(',')) != std::string::npos) {
        symbols.PushBack(rapidjson::Value(subs.substr(0, pos).c_str(), allocator).Move(), allocator);
        subs.erase(0, pos + 1);
    }
    if (!subs.empty()) {
        symbols.PushBack(rapidjson::Value(subs.c_str(), allocator).Move(), allocator);
    }
    d.AddMember("symbols", symbols, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);
    
    return buffer.GetString();
}

std::atomic<bool> g_running(true);

int main() {
    std::cout << "[GATEWAY_MAIN] Starting OmniQuant CTP Gateway..." << std::endl;

    // 1. ZMQ Setup
    zmq::context_t context(1);
    
    // PUSH: 发送行情和交易回报
    zmq::socket_t pusher(context, zmq::socket_type::push);
    pusher.connect("tcp://localhost:5555");
    
    // PULL: 接收交易指令
    zmq::socket_t puller(context, zmq::socket_type::pull);
    puller.bind("tcp://*:6001"); // 监听端口接收指令

    // 2. Gateway Setup
    omni::CtpGateway gateway;
    
    // 回调函数：Gateway 产生的任何数据都推送到 ZMQ
    auto event_callback = [&](const omni::EventFrame& frame) {
        std::string payload;
        if (frame.SerializeToString(&payload)) {
            zmq::message_t msg(payload.size());
            memcpy(msg.data(), payload.data(), payload.size());
            try {
                pusher.send(msg, zmq::send_flags::dontwait);
            } catch (...) {}
        }
    };

    std::string config = build_config_json();
    std::cout << "[GATEWAY_MAIN] Config: " << config << std::endl;

    gateway.init(config, event_callback);
    gateway.connect();

    std::cout << "[GATEWAY_MAIN] Gateway started. Listening for commands on tcp://*:6001" << std::endl;

    // 3. Command Loop
    zmq::pollitem_t items[] = {
        { static_cast<void*>(puller), 0, ZMQ_POLLIN, 0 }
    };

    while (g_running) {
        // Poll with 100ms timeout
        zmq::poll(items, 1, std::chrono::milliseconds(100));

        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t msg;
            if (puller.recv(msg, zmq::recv_flags::none)) {
                // 解析指令
                omni::OrderRequest req;
                if (req.ParseFromArray(msg.data(), msg.size())) {
                    std::cout << "[GATEWAY_MAIN] Received Order: " << req.symbol() << " " << req.direction() << std::endl;
                    gateway.send_order(req);
                } else {
                    std::cerr << "[GATEWAY_MAIN] Received unknown command (parse failed)" << std::endl;
                }
            }
        }
    }

    gateway.close();
    return 0;
}
