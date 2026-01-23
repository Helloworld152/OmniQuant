#pragma once

#include <iostream>
#include <vector>
#include <cstring>
#include <zmq.hpp>
#include "ThostFtdcTraderApi.h"
#include "omni.pb.h"

class TraderHandler : public CThostFtdcTraderSpi {
public:
    TraderHandler(CThostFtdcTraderApi* api, zmq::socket_t* socket) 
        : m_api(api), m_socket(socket), m_reqId(0) {}

    virtual ~TraderHandler() {}

    // --- 核心方法 ---
    void Connect(const std::string& frontAddr, const std::string& brokerId, const std::string& userId, const std::string& password, const std::string& appId, const std::string& authCode);
    bool IsConnected() const { return m_connected; }

    // --- SPI 回调 ---
    void OnFrontConnected() override;
    void OnFrontDisconnected(int nReason) override;
    
    // 认证与登录
    void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    // 委托与成交回报
    void OnRtnOrder(CThostFtdcOrderField *pOrder) override;
    void OnRtnTrade(CThostFtdcTradeField *pTrade) override;

    // 错误回报
    void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo) override;

    // 查询响应
    void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    // --- 业务操作 ---
    void QueryAccount();
    void QueryPosition();

private:
    CThostFtdcTraderApi* m_api;
    zmq::socket_t* m_socket;
    int m_reqId;
    bool m_connected = false;
    bool m_authenticated = false;

    // 用户信息
    std::string m_brokerId;
    std::string m_userId;
    std::string m_password;
    std::string m_appId;
    std::string m_authCode;

    void SendEvent(const omni::EventFrame& event);
    int64_t now_ns();
};
