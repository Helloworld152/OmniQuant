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
    auto t1 = std::chrono::high_resolution_clock::now(); // [PERF] Start

    if (!pData) return;

    // [Optimization] Object Reuse (No Malloc/Free)
    m_event_buffer.Clear();
    
    m_event_buffer.set_timestamp_ns(now_ns());
    m_event_buffer.set_source_id("ctp_real_01");

    omni::MarketData* tick = m_event_buffer.mutable_tick();
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
    m_callback(m_event_buffer);

    // [PERF] End & Report
    auto t2 = std::chrono::high_resolution_clock::now();
    
    static long count = 0;
    static long total_us = 0;
    
    long us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    total_us += us;
    count++;

    if (count >= 50 ) {
        std::cout << "[PERF] Last 50 ticks avg processing time: " 
                  << (total_us / 50.0) << " us" << std::endl;
        count = 0;
        total_us = 0;
    }
}