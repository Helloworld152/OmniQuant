# OmniQuant (Project Aegis) - 开发状态文档

* 注释和对话都用中文
* 每次创建cmake都要加上导出compilejson

这份文档由 Gemini 生成，记录了项目的核心设计选择、当前进度及后续开发指南。

## 1. 系统架构综述

项目采用 **“漏斗型”分层架构**，旨在平衡极速接入与业务复杂性。

| 组件 | 语言 | 角色 | 通信协议 | 关键技术 |
| :--- | :--- | :--- | :--- | :--- |
| **Gateway** | C++ 17 | 柜台接入 / 数据清洗 | ZMQ (PUSH) | `cppzmq`, `Protobuf` |
| **Core Router**| Rust | 逻辑路由 / 状态聚合 | ZMQ (PULL) + AMQP | `tokio`, `prost`, `lapin` |
| **Backend API**| Python | 业务逻辑 / 状态快照 | AMQP + WS | `FastAPI`, `aio-pika`, `Redis` |
| **Dashboard**  | Vue 3 | 实时监控面板 | WebSocket | `Vite`, `Tailwind`, `Ag-Grid` |

---

## 2. 当前进度 (Phase 2: Core Logic)

### 已完成 ✅
- [x] **统一协议定义**：在 `protocol/omni.proto` 中定义了 `EventFrame` 通用数据结构。
- [x] **跨语言通信**：打通了 C++ (Gateway) 到 Rust (Core) 的二进制 Protobuf 传输链路。
- [x] **环境适配**：解决了 Ubuntu 系统库与 Anaconda 环境下 Protobuf 的版本冲突及 GLIBCXX 链接问题。
- [x] **基础设施自动化**：提供 `docker-compose.yml` 一键启动 RabbitMQ/Redis/Postgres。
- [x] **启动脚本**：`start_all.sh` 可一键编译并运行 C++ 和 Rust 核心链路。
- [x] **CTP 真实接入**：已完善 `gateway_ctp`，实现了 `MdHandler` (行情) 和 `TraderHandler` (交易) 的完整逻辑。
- [x] **后端消费逻辑**：`consumer.py` 已实现从 RabbitMQ 解析 Protobuf 并通过 WebSocket 广播 JSON 数据。
- [x] **前端实现**：Web Dashboard 已完成，支持实时行情表格、持仓监控、委托/成交日志展示。

### 待进行 ⏳
- [ ] **状态持久化**：在 Python 端实现 Redis 持仓累加逻辑 (目前仅为广播，重启后数据丢失)。
- [ ] **历史数据存储**：将行情数据写入 TimescaleDB/Postgres。

---

## 3. 关键配置说明

### Protobuf 编译 (C++)
由于系统中存在 Anaconda 环境，`gateway_ctp/CMakeLists.txt` 已被配置为：
- 强制使用 `/usr/bin/protoc` (v3.6.1)。
- 链接系统路径下的 `libprotobuf`。
- 这样避免了新版 Protobuf 引入的 `absl` 复杂依赖问题。

### 运行指令
```bash
# 启动基础设施
docker-compose up -d

# 一键运行核心链路测试 (C++ -> Rust)
./start_all.sh
```

---

## 4. 下一步开发路线 (Phase 2.5)

### 1. 状态持久化 (Redis)
1. 在 `backend_api` 中引入 Redis 客户端。
2. 在 `consumer.py` 收到 `TradeUpdate` 时，原子更新 Redis 中的持仓数据。
3. 在 `manager.py` 中增加 HTTP API，允许前端在 WebSocket 连接建立前获取当前持仓快照。

### 2. 系统稳定性测试
1. 模拟 CTP 断线重连，验证 `gateway_ctp` 的健壮性。
2. 验证前端 WebSocket 在后端重启后的重连机制。

---

**Last Updated**: 2026-01-24
**Status**: Phase 2 Complete / Optimization Pending
