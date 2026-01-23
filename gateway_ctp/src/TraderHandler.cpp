#include "TraderHandler.h"
#include <chrono>
#include <thread>
#include <iomanip>

int64_t TraderHandler::now_ns() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}

void TraderHandler::Connect(const std::string& frontAddr, const std::string& brokerId, const std::string& userId, const std::string& password, const std::string& appId, const std::string& authCode) {
    m_brokerId = brokerId;
    m_userId = userId;
    m_password = password;
    m_appId = appId;
    m_authCode = authCode;

    std::cout << "[TRADER_CTP] 注册交易前置: " << frontAddr << std::endl;
    m_api->RegisterSpi(this);
    m_api->RegisterFront(const_cast<char*>(frontAddr.c_str()));
    m_api->SubscribePrivateTopic(THOST_TERT_QUICK);
    m_api->SubscribePublicTopic(THOST_TERT_QUICK);
    m_api->Init();
}

void TraderHandler::OnFrontConnected() {
    std::cout << "[TRADER_CTP] 交易前置已连接，直接发起登录..." << std::endl;
    
    // 跳过认证，直接登录
    CThostFtdcReqUserLoginField req = {0};
    strcpy(req.BrokerID, m_brokerId.c_str());
    strcpy(req.UserID, m_userId.c_str());
    strcpy(req.Password, m_password.c_str());
    
    m_reqId++;
    m_api->ReqUserLogin(&req, m_reqId);
}

void TraderHandler::OnFrontDisconnected(int nReason) {
    std::cerr << "[TRADER_CTP] 交易连接断开! Reason: " << nReason << std::endl;
    m_connected = false;
    m_authenticated = false;
}

void TraderHandler::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        std::cerr << "[TRADER_CTP] 认证失败: " << pRspInfo->ErrorMsg << std::endl;
        return;
    }
    std::cout << "[TRADER_CTP] 认证成功，开始登录..." << std::endl;
    m_authenticated = true;

    CThostFtdcReqUserLoginField req = {0};
    strcpy(req.BrokerID, m_brokerId.c_str());
    strcpy(req.UserID, m_userId.c_str());
    strcpy(req.Password, m_password.c_str());
    
    m_reqId++;
    m_api->ReqUserLogin(&req, m_reqId);
}

void TraderHandler::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        std::cerr << "[TRADER_CTP] 登录失败: " << pRspInfo->ErrorMsg << std::endl;
        return;
    }
    std::cout << "[TRADER_CTP] 交易账户登录成功. SessionID: " << pRspUserLogin->SessionID << std::endl;
    m_connected = true;

    // 启动定时查询线程 (每 30 秒查询一次资金和持仓)
    std::thread([this]() {
        while (this->m_connected) {
            std::cout << "[TRADER_CTP] 正在执行定时查询 (资金/持仓)..." << std::endl;
            this->QueryAccount();
            std::this_thread::sleep_for(std::chrono::seconds(1)); // 避开流量控制
            this->QueryPosition();
            
            // 等待 10 秒
            for (int i = 0; i < 10 && this->m_connected; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }).detach();
}

// --- 回报处理 ---

void TraderHandler::OnRtnOrder(CThostFtdcOrderField *pOrder) {
    if (!pOrder) return;
    
    omni::EventFrame frame;
    frame.set_timestamp_ns(now_ns());
    frame.set_source_id(m_userId);

    // 智能 ID 生成策略:
    // 1. 如果有交易所编号 (OrderSysID)，优先使用 (它是全市场唯一的)
    // 2. 如果没有 (刚发单未排队)，使用 FrontID_SessionID_OrderRef 组合
    std::string unique_id;
    std::string sys_id = pOrder->OrderSysID;
    if (!sys_id.empty()) {
        unique_id = sys_id;
    } else {
        // 临时 ID，格式: ref:FrontID-SessionID-OrderRef
        unique_id = "ref:" + std::to_string(pOrder->FrontID) + "-" + 
                    std::to_string(pOrder->SessionID) + "-" + pOrder->OrderRef;
    }

    omni::OrderUpdate* order = frame.mutable_order();
    order->set_order_id(unique_id);
    order->set_symbol(pOrder->InstrumentID);
    order->set_direction(pOrder->Direction == THOST_FTDC_D_Buy ? "Buy" : "Sell");
    order->set_offset(pOrder->CombOffsetFlag); 
    order->set_price(pOrder->LimitPrice);
    order->set_volume(pOrder->VolumeTotalOriginal);
    order->set_status(std::string(1, pOrder->OrderStatus)); 

    std::cout << "[TRADER_CTP] 收到委托回报: [" << unique_id << "] " 
              << pOrder->InstrumentID << " " << pOrder->OrderStatus 
              << " (" << pOrder->StatusMsg << ")" << std::endl;
              
    SendEvent(frame);
}

void TraderHandler::OnRtnTrade(CThostFtdcTradeField *pTrade) {
    if (!pTrade) return;

    omni::EventFrame frame;
    frame.set_timestamp_ns(now_ns());
    frame.set_source_id(m_userId);

    omni::TradeUpdate* trade = frame.mutable_trade();
    trade->set_trade_id(pTrade->TradeID);
    trade->set_symbol(pTrade->InstrumentID);
    trade->set_direction(pTrade->Direction == THOST_FTDC_D_Buy ? "Buy" : "Sell");
    trade->set_offset(pTrade->OffsetFlag == THOST_FTDC_OF_Open ? "Open" : "Close");
    trade->set_price(pTrade->Price);
    trade->set_volume(pTrade->Volume);

    std::cout << "[TRADER_CTP] 收到成交回报: " << pTrade->InstrumentID << " @ " << pTrade->Price << std::endl;
    SendEvent(frame);
}

void TraderHandler::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        std::cerr << "[TRADER_CTP] 报单被拒: " << pRspInfo->ErrorMsg << std::endl;
    }
}

void TraderHandler::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        std::cerr << "[TRADER_CTP] 报单错误: " << pRspInfo->ErrorMsg << std::endl;
    }
}

// --- 查询处理 ---

void TraderHandler::QueryAccount() {
    if (!m_connected) return;
    CThostFtdcQryTradingAccountField req = {0};
    strcpy(req.BrokerID, m_brokerId.c_str());
    strcpy(req.InvestorID, m_userId.c_str());
    m_reqId++;
    m_api->ReqQryTradingAccount(&req, m_reqId);
}

void TraderHandler::QueryPosition() {
    if (!m_connected) return;
    CThostFtdcQryInvestorPositionField req = {0};
    strcpy(req.BrokerID, m_brokerId.c_str());
    strcpy(req.InvestorID, m_userId.c_str());
    m_reqId++;
    m_api->ReqQryInvestorPosition(&req, m_reqId);
}

void TraderHandler::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (!pTradingAccount) return;

    omni::EventFrame frame;
    frame.set_timestamp_ns(now_ns());
    frame.set_source_id(m_userId);

    omni::AccountUpdate* acc = frame.mutable_account();
    acc->set_account_id(pTradingAccount->AccountID);
    acc->set_balance(pTradingAccount->Balance);
    acc->set_available(pTradingAccount->Available);
    acc->set_frozen(pTradingAccount->FrozenMargin + pTradingAccount->FrozenCash);

    std::cout << "[TRADER_CTP] 资金更新: Balance=" << pTradingAccount->Balance << std::endl;
    SendEvent(frame);
}

void TraderHandler::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (!pInvestorPosition) return;

    omni::EventFrame frame;
    frame.set_timestamp_ns(now_ns());
    frame.set_source_id(m_userId);

    omni::PositionUpdate* pos = frame.mutable_position();
    pos->set_symbol(pInvestorPosition->InstrumentID);
    // CTP PosDirection: '2'=Long, '3'=Short. But Net mode uses different logic.
    // For simplicity:
    pos->set_direction(pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long ? "Long" : "Short");
    pos->set_volume(pInvestorPosition->Position);
    pos->set_open_price(pInvestorPosition->OpenCost); // Needs calculation usually
    pos->set_last_price(0.0); // CTP doesn't give last price here
    pos->set_pnl(pInvestorPosition->PositionProfit);

    std::cout << "[TRADER_CTP] 持仓更新: " << pInvestorPosition->InstrumentID << " Vol=" << pInvestorPosition->Position << std::endl;
    SendEvent(frame);
}

void TraderHandler::SendEvent(const omni::EventFrame& event) {
    std::string payload;
    if (event.SerializeToString(&payload)) {
        zmq::message_t msg(payload.size());
        memcpy(msg.data(), payload.data(), payload.size());
        try {
            m_socket->send(msg, zmq::send_flags::dontwait);
        } catch (...) {}
    }
}
