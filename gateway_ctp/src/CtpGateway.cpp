#include "CtpGateway.h"
#include <filesystem>
#include <iostream>

namespace omni {

CtpGateway::CtpGateway() {}

CtpGateway::~CtpGateway() {
    close();
}

void CtpGateway::init(const std::string& config_json, EventCallback cb) {
    m_callback = cb;
    parse_config(config_json);

    // Ensure log directory exists
    std::string log_dir = "flow_log";
    std::filesystem::create_directory(log_dir);

    // Initialize MdApi
    m_mdApi = CThostFtdcMdApi::CreateFtdcMdApi((log_dir + "/md_").c_str());
    m_mdHandler = std::make_unique<MdHandler>(m_mdApi, m_callback);

    // Initialize TraderApi
    m_tdApi = CThostFtdcTraderApi::CreateFtdcTraderApi((log_dir + "/td_").c_str());
    m_tdHandler = std::make_unique<TraderHandler>(m_tdApi, m_callback);
}

void CtpGateway::connect() {
    // Connect MD
    if (m_mdHandler) {
        m_mdHandler->Subscribe(m_config.symbols);
        m_mdHandler->Connect(m_config.md_front);
    }

    // Connect Trader
    if (m_tdHandler && !m_config.user_id.empty()) {
        m_tdHandler->Connect(m_config.td_front, m_config.broker_id, 
                             m_config.user_id, m_config.password, 
                             m_config.app_id, m_config.auth_code);
    } else {
        std::cout << "[CtpGateway] Warning: Trader config missing, skipping Trader login." << std::endl;
    }
}

void CtpGateway::close() {
    if (m_mdApi) {
        m_mdApi->RegisterSpi(nullptr);
        m_mdApi->Release();
        m_mdApi = nullptr;
    }
    if (m_tdApi) {
        m_tdApi->RegisterSpi(nullptr);
        m_tdApi->Release();
        m_tdApi = nullptr;
    }
}

std::string CtpGateway::send_order(const OrderRequest& req) {
    if (m_tdHandler) {
        return m_tdHandler->SendOrder(req);
    }
    return "";
}

void CtpGateway::cancel_order(const std::string& order_id) {
    if (m_tdHandler) {
        m_tdHandler->CancelOrder(order_id);
    }
}

void CtpGateway::query_account() {
    if (m_tdHandler) {
        m_tdHandler->QueryAccount();
    }
}

void CtpGateway::query_position() {
    if (m_tdHandler) {
        m_tdHandler->QueryPosition();
    }
}

void CtpGateway::parse_config(const std::string& json_str) {
    rapidjson::Document d;
    if (d.Parse(json_str.c_str()).HasParseError()) {
        std::cerr << "[CtpGateway] Config Parse Error!" << std::endl;
        return;
    }

    auto get_str = [&](const char* key, std::string& target) {
        if (d.HasMember(key) && d[key].IsString()) {
            target = d[key].GetString();
        }
    };

    get_str("md_front", m_config.md_front);
    get_str("td_front", m_config.td_front);
    get_str("broker_id", m_config.broker_id);
    get_str("user_id", m_config.user_id);
    get_str("password", m_config.password);
    get_str("app_id", m_config.app_id);
    get_str("auth_code", m_config.auth_code);

    if (d.HasMember("symbols") && d["symbols"].IsArray()) {
        for (auto& v : d["symbols"].GetArray()) {
            if (v.IsString()) {
                m_config.symbols.push_back(v.GetString());
            }
        }
    }
}

} // namespace omni
