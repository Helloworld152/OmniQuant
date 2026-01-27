#pragma once

#include <iostream>
#include <vector>
#include <cstring>
#include "ThostFtdcMdApi.h"
#include "omni.pb.h"
#include "IGateway.h"

class MdHandler : public CThostFtdcMdSpi {
public:
    MdHandler(CThostFtdcMdApi* api, omni::EventCallback cb) 
        : m_api(api), m_callback(cb), m_reqId(0) {}

    virtual ~MdHandler() {}

    // --- SPI 回调函数 ---

    // 1. 当客户端与后台命令行服务建立连接时，该方法被调用
    void OnFrontConnected() override;

    // 2. 当客户端与后台命令行服务断开连接时，该方法被调用
    void OnFrontDisconnected(int nReason) override;

    // 3. 登录请求响应
    void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    // 4. 订阅行情响应
    void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    // 5. 深度行情通知 (核心行情推送)
    void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) override;

    // --- 辅助方法 ---
    void Connect(const std::string& frontAddr);
    void Subscribe(const std::vector<std::string>& instruments);

private:
    CThostFtdcMdApi* m_api;
    omni::EventCallback m_callback;
    int m_reqId;
    std::vector<std::string> m_instruments;
    
    // [Optimization] Memory Reuse
    omni::EventFrame m_event_buffer;
};