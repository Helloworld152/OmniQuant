#pragma once

#include "IGateway.h"
#include "MdHandler.h"
#include "TraderHandler.h"
#include "rapidjson/document.h"
#include <memory>
#include <mutex>
#include <vector>
#include <string>

namespace omni {

class CtpGateway : public IGateway {
public:
    CtpGateway();
    virtual ~CtpGateway();

    // IGateway interface
    void init(const std::string& config_json, EventCallback cb) override;
    void connect() override;
    void close() override;
    std::string get_name() const override { return "CTP-Gateway"; }

    std::string send_order(const OrderRequest& req) override;
    void cancel_order(const std::string& order_id) override;
    
    void query_account() override;
    void query_position() override;

private:
    EventCallback m_callback;
    
    // CTP APIs
    CThostFtdcMdApi* m_mdApi = nullptr;
    CThostFtdcTraderApi* m_tdApi = nullptr;

    // Handlers
    std::unique_ptr<MdHandler> m_mdHandler;
    std::unique_ptr<TraderHandler> m_tdHandler;

    // Config
    struct Config {
        std::string md_front;
        std::string td_front;
        std::string broker_id;
        std::string user_id;
        std::string password;
        std::string app_id;
        std::string auth_code;
        std::vector<std::string> symbols;
    } m_config;

    void parse_config(const std::string& json_str);
};

} // namespace omni
