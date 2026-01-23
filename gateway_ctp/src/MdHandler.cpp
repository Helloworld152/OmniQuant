#include "MdHandler.h"
#include <chrono>
#include <thread>

// 获取当前纳秒时间戳的辅助函数
static int64_t now_ns() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}

void MdHandler::Connect(const std::string& frontAddr) {
    std::cout << "[GATEWAY_CTP] 正在注册前置机地址: " << frontAddr << std::endl;
    m_api->RegisterSpi(this);
    m_api->RegisterFront(const_cast<char*>(frontAddr.c_str()));
    m_api->Init();
}

void MdHandler::Subscribe(const std::vector<std::string>& instruments) {
    m_instruments = instruments;
}

void MdHandler::OnFrontConnected() {
    std::cout << "[GATEWAY_CTP] 已成功连接至 CTP 前置机。" << std::endl;
    
    // 自动登录 (行情接口通常不需要特定的用户名密码即可登录)
    CThostFtdcReqUserLoginField req = {0};
    m_reqId++;
    m_api->ReqUserLogin(&req, m_reqId);
}

void MdHandler::OnFrontDisconnected(int nReason) {
    std::cerr << "[GATEWAY_CTP] 连接断开！原因代码: " << nReason << std::endl;
}

void MdHandler::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        std::cerr << "[GATEWAY_CTP] 登录失败: " << pRspInfo->ErrorMsg << std::endl;
        return;
    }
    std::cout << "[GATEWAY_CTP] 登录成功。当前交易日: " << m_api->GetTradingDay() << std::endl;

    // 登录成功后自动订阅合约
    if (!m_instruments.empty()) {
        std::vector<char*> instruments_ptr;
        for (const auto& inst : m_instruments) {
            instruments_ptr.push_back(const_cast<char*>(inst.c_str()));
        }
        m_api->SubscribeMarketData(instruments_ptr.data(), instruments_ptr.size());
        std::cout << "[GATEWAY_CTP] 已发送订阅请求，合约数量: " << m_instruments.size() << std::endl;
    }
}

void MdHandler::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        std::cerr << "[GATEWAY_CTP] 订阅失败: " << pRspInfo->ErrorMsg << std::endl;
        return;
    }
    if (pSpecificInstrument) {
        std::cout << "[GATEWAY_CTP] 订阅成功: " << pSpecificInstrument->InstrumentID << std::endl;
    }
}

void MdHandler::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pData) {
    if (!pData) return;

    // 1. 构建 Protobuf 事件包
    omni::EventFrame frame;
    frame.set_timestamp_ns(now_ns());
    frame.set_source_id("ctp_real_01");

    omni::MarketData* tick = frame.mutable_tick();
    tick->set_symbol(pData->InstrumentID);
    tick->set_exchange(pData->ExchangeID);
    
    // 处理价格异常值 (CTP 在无效价格时常返回 DBL_MAX)
    if (pData->LastPrice > 1e15) {
        tick->set_last_price(0.0);
    } else {
        tick->set_last_price(pData->LastPrice);
    }
    
    tick->set_volume(pData->Volume);
    tick->set_open_interest(pData->OpenInterest);

    // 2. 序列化并发送至核心路由
    SendEvent(frame);
}

void MdHandler::SendEvent(const omni::EventFrame& event) {
    std::string payload;
    if (event.SerializeToString(&payload)) {
        zmq::message_t msg(payload.size());
        memcpy(msg.data(), payload.data(), payload.size());
        try {
            m_socket->send(msg, zmq::send_flags::dontwait);
        } catch (...) {
            // 忽略发送错误，防止阻塞回调线程
        }
    }
}