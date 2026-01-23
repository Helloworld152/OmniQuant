#include <iostream>
#include <string>
#include <thread>
#include <zmq.hpp>
#include "MdHandler.h"
#include "TraderHandler.h" // 新增

// 默认 CTP 行情前置地址
const char* DEFAULT_MD_FRONT = "tcp://182.254.243.31:40011"; 
const char* DEFAULT_TD_FRONT = "tcp://182.254.243.31:40001"; // 假设的交易前置

int main() {
    std::cout << "[GATEWAY_CTP] 正在启动 OmniQuant CTP 网关 (全功能)..." << std::endl;

    // 1. 设置 ZMQ PUSH 套接字 (共享)
    zmq::context_t context(1);
    zmq::socket_t pusher(context, zmq::socket_type::push);
    try {
        pusher.connect("tcp://localhost:5555"); 
        std::cout << "[GATEWAY_CTP] ZMQ 已连接至 tcp://localhost:5555" << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "[GATEWAY_CTP] ZMQ 连接失败: " << e.what() << std::endl;
        return 1;
    }

    // 2. 初始化 CTP API
    system("mkdir -p flow_log");
    
    // --- 行情 ---
    CThostFtdcMdApi* pMdApi = CThostFtdcMdApi::CreateFtdcMdApi("flow_log/md_");
    MdHandler mdHandler(pMdApi, &pusher);

    // --- 交易 ---
    CThostFtdcTraderApi* pTdApi = CThostFtdcTraderApi::CreateFtdcTraderApi("flow_log/td_");
    TraderHandler tdHandler(pTdApi, &pusher);

    // 3. 配置并启动
    // 行情部分
    const char* env_md_front = std::getenv("CTP_MD_FRONT");
    std::string md_front = env_md_front ? env_md_front : DEFAULT_MD_FRONT;

    const char* env_inst = std::getenv("CTP_SUB_LIST"); 
    std::vector<std::string> subs;
    if (env_inst) {
        std::string s = env_inst;
        std::string delimiter = ",";
        size_t pos = 0;
        std::string token;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            subs.push_back(token);
            s.erase(0, pos + delimiter.length());
        }
        subs.push_back(s);
    } else {
        subs = {"rb2605", "au2606", "ag2606"}; 
    }

    mdHandler.Subscribe(subs);
    mdHandler.Connect(md_front);

    // 交易部分 (从环境读取账户)
    const char* env_td_front = std::getenv("CTP_TD_FRONT");
    std::string td_front = env_td_front ? env_td_front : DEFAULT_TD_FRONT;
    
    const char* broker = std::getenv("CTP_BROKER");
    const char* user = std::getenv("CTP_USER");
    const char* pass = std::getenv("CTP_PASS");
    const char* appid = std::getenv("CTP_APPID");
    const char* auth = std::getenv("CTP_AUTHCODE");

    if (broker && user && pass && appid && auth) {
        std::cout << "[GATEWAY_CTP] 启动交易接口..." << std::endl;
        tdHandler.Connect(td_front, broker, user, pass, appid, auth);
    } else {
        std::cout << "[GATEWAY_CTP] 警告: 缺少交易账户配置 (CTP_USER/PASS...)，仅启动行情。" << std::endl;
    }

    // 4. 阻塞主线程
    std::cout << "[GATEWAY_CTP] 按 Ctrl+C 停止运行..." << std::endl;
    pMdApi->Join();
    // pTdApi->Join(); // 只需要 Join 一个即可阻塞

    return 0;
}
