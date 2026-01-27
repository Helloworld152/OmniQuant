#include "../include/framework.h"
#include <dlfcn.h>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

// --- EventBus 实现 ---
class EventBusImpl : public EventBus {
public:
    void subscribe(EventType type, Handler handler) override {
        handlers_[type].push_back(handler);
    }

    void publish(EventType type, void* data) override {
        for (auto& h : handlers_[type]) {
            h(data);
        }
    }

    void clear() override {
        for (auto& vec : handlers_) {
            vec.clear();
        }
    }

private:
    std::array<std::vector<Handler>, MAX_EVENTS> handlers_;
};

// --- Plugin Wrapper ---
struct PluginHandle {
    void* lib_handle;
    std::shared_ptr<IModule> module;
    std::string name;

    ~PluginHandle() {
        // [CRITICAL] 必须先销毁模块对象，因为它的析构函数在动态库里
        module.reset();

        if (lib_handle) {
            std::cout << "[System] Unloading " << name << std::endl;
            // 实际上在复杂系统中，dlclose 可能会导致问题，有些库不建议卸载
            dlclose(lib_handle);
        }
    }
};

int main(int argc, char* argv[]) {
    // 0. 基础环境准备
    std::thread([]{}).join(); // Force pthread init

    std::string config_path = "config.json";
    if (argc > 1) config_path = argv[1];

    std::cout << ">>> HFT Engine Booting using config: " << config_path << std::endl;

    // 1. 读取并解析 JSON
    std::ifstream ifs(config_path);
    if (!ifs.is_open()) {
        std::cerr << "FATAL: Could not open config file!" << std::endl;
        return 1;
    }
    
    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document doc;
    doc.ParseStream(isw);

    if (doc.HasParseError()) {
        std::cerr << "FATAL: JSON Parse Error!" << std::endl;
        return 1;
    }

    EventBusImpl bus;
    std::vector<std::shared_ptr<PluginHandle>> plugins;

    // 2. 遍历插件列表
    if (doc.HasMember("plugins") && doc["plugins"].IsArray()) {
        const auto& plugin_list = doc["plugins"];
        
        for (const auto& p : plugin_list.GetArray()) {
            std::string name = p["name"].GetString();
            std::string lib_path = p["library"].GetString();
            bool enabled = p.HasMember("enabled") ? p["enabled"].GetBool() : true;

            if (!enabled) {
                std::cout << "[Loader] Skipping disabled module: " << name << std::endl;
                continue;
            }

            std::cout << "[Loader] Loading Module: " << name << " (" << lib_path << ")..." << std::endl;

            // A. 加载动态库
            void* handle = dlopen(lib_path.c_str(), RTLD_LAZY);
            if (!handle) {
                std::cerr << "   [ERROR] dlopen failed: " << dlerror() << std::endl;
                continue;
            }

            // B. 获取工厂
            CreateModuleFunc create_fn = (CreateModuleFunc)dlsym(handle, "create_module");
            if (!create_fn) {
                std::cerr << "   [ERROR] create_module symbol not found!" << std::endl;
                dlclose(handle);
                continue;
            }

            // C. 准备配置 map
            ConfigMap config_map;
            if (p.HasMember("config") && p["config"].IsObject()) {
                for (auto& m : p["config"].GetObject()) {
                    if (m.value.IsString()) {
                        config_map[m.name.GetString()] = m.value.GetString();
                    }
                }
            }

            // D. 实例化并初始化
            IModule* raw_ptr = create_fn();
            raw_ptr->init(&bus, config_map);

            auto plugin = std::make_shared<PluginHandle>();
            plugin->lib_handle = handle;
            plugin->module = std::shared_ptr<IModule>(raw_ptr);
            plugin->name = name;
            plugins.push_back(plugin);
        }
    }

    // 3. 启动
    std::cout << ">>> All Modules Loaded. Starting..." << std::endl;
    for (auto& p : plugins) {
        p->module->start();
    }

    // 4. 运行循环
    std::cout << ">>> System Running. (Simulating 5s run...)" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // 5. 退出
    std::cout << ">>> Shutting down..." << std::endl;
    for (auto& p : plugins) {
        p->module->stop();
    }
    
    // [CRITICAL] 清空所有事件回调，防止指向已卸载的内存
    std::cout << ">>> Clearing EventBus..." << std::endl;
    bus.clear();

    // [CRITICAL] 显式释放插件，确保按照预期顺序析构
    plugins.clear();

    std::cout << ">>> Shutdown Complete." << std::endl;
    return 0;
}