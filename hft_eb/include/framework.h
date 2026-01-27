#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <array>

// ==========================================
// 1. 基础数据结构
// ==========================================

// 事件类型
enum EventType {
    EVENT_MARKET_DATA = 0, // 行情
    EVENT_ORDER_REQ,       // 报单请求
    EVENT_LOG,             // 日志
    MAX_EVENTS
};

// 事件负载 (Payload)
struct MarketData {
    char symbol[32];
    double last_price;
    int volume;
};

struct OrderReq {
    char symbol[32];
    char direction; // 'B'uy or 'S'ell
    double price;
    int volume;
};

// ==========================================
// 2. 事件总线 (Host 提供)
// ==========================================
class EventBus {
public:
    using Handler = std::function<void(void*)>;
    
    virtual ~EventBus() = default;

    virtual void subscribe(EventType type, Handler handler) = 0;
    virtual void publish(EventType type, void* data) = 0;
    
    // 安全退出：清空所有回调
    virtual void clear() = 0;
};

// ==========================================
// 3. 插件接口 (Plugin 实现)
// ==========================================
using ConfigMap = std::unordered_map<std::string, std::string>;

class IModule {
public:
    virtual ~IModule() = default;
    
    // 初始化：Host 会把 bus 指针传进来
    virtual void init(EventBus* bus, const ConfigMap& config) = 0;
    
    // 可选：启动/停止
    virtual void start() {}
    virtual void stop() {}
};

// ==========================================
// 4. 导出符号约定
// ==========================================
// 每个 .so 必须实现这个函数来创建模块实例
typedef IModule* (*CreateModuleFunc)();
#define EXPORT_MODULE(CLASS_NAME) \
    extern "C" { \
        IModule* create_module() { return new CLASS_NAME(); } \
    }
