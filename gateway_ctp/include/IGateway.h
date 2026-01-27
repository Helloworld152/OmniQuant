#pragma once

#include <string>
#include <vector>
#include <functional>
#include "omni.pb.h" // Protobuf 生成的头文件

namespace omni {

// =========================================================
// 0. 核心定义
// =========================================================

/**
 * @brief 通用事件回调
 * 这是网关向外“吐”数据的唯一喉舌。
 * 无论是行情(Tick)、成交(Trade)、还是错误日志(Log)，全装进 EventFrame 里吐出来。
 */
using EventCallback = std::function<void(const EventFrame& frame)>;


// =========================================================
// 1. 基类接口 (IBaseGateway)
// 定义所有网关都必须具备的“基础生命周期”
// =========================================================
class IBaseGateway {
public:
    virtual ~IBaseGateway() = default;

    /**
     * @brief 初始化
     * @param config_json 包含账号、密码、服务器地址的 JSON 字符串
     * @param cb 数据回调函数
     */
    virtual void init(const std::string& config_json, EventCallback cb) = 0;

    /**
     * @brief 发起连接
     * 注意：这里不包含“登录”参数，登录逻辑封装在实现类内部
     * CTP: Init() -> OnFrontConnected -> ReqUserLogin
     */
    virtual void connect() = 0;

    /**
     * @brief 断开连接并清理资源
     */
    virtual void close() = 0;

    /**
     * @brief 获取网关名称 (e.g. "CTP-Md-SimNow")
     */
    virtual std::string get_name() const = 0;
};


// =========================================================
// 2. 行情网关接口 (IMdGateway)
// 专注于订阅与推送
// =========================================================
class IMdGateway : public IBaseGateway {
public:
    virtual ~IMdGateway() = default;

    /**
     * @brief 订阅行情
     * @param symbols 合约代码列表，如 {"rb2405", "hc2405"}
     * 实现类需负责将 vector 转为 CTP 需要的 char*[]
     */
    virtual void subscribe(const std::vector<std::string>& symbols) = 0;

    /**
     * @brief 退订行情
     */
    virtual void unsubscribe(const std::vector<std::string>& symbols) = 0;
};


// =========================================================
// 3. 交易网关接口 (ITraderGateway)
// 专注于订单操作与查询
// =========================================================
class ITraderGateway : public IBaseGateway {
public:
    virtual ~ITraderGateway() = default;

    /**
     * @brief 发送订单
     * @param req 标准化的 Protobuf 订单请求 (包含合约、价格、方向、手数等)
     * @return std::string 本地生成的唯一 OrderID (用于后续追踪)
     */
    virtual std::string send_order(const omni::OrderRequest& req) = 0;

    /**
     * @brief 撤单
     * @param order_id 之前 send_order 返回的 ID，或者是交易所的 ID
     * 实现类需要维护 "本地ID <-> 交易所ID" 的映射关系
     */
    virtual void cancel_order(const std::string& order_id) = 0;

    /**
     * @brief 查询账户资金
     * 这是一个异步操作！结果不会作为 return 返回，而是通过 EventCallback 推送 AccountUpdate
     */
    virtual void query_account() = 0;

    /**
     * @brief 查询持仓
     * 这是一个异步操作！结果通过 EventCallback 推送 PositionUpdate
     */
    virtual void query_position() = 0;
};

} // namespace omni