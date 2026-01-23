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

## 2. 当前进度 (Phase 1: Skeleton)

### 已完成 ✅
- [x] **统一协议定义**：在 `protocol/omni.proto` 中定义了 `EventFrame` 通用数据结构。
- [x] **跨语言通信**：打通了 C++ (Gateway) 到 Rust (Core) 的二进制 Protobuf 传输链路。
- [x] **环境适配**：解决了 Ubuntu 系统库与 Anaconda 环境下 Protobuf 的版本冲突及 GLIBCXX 链接问题。
- [x] **基础设施自动化**：提供 `docker-compose.yml` 一键启动 RabbitMQ/Redis/Postgres。
- [x] **启动脚本**：`start_all.sh` 可一键编译并运行 C++ 和 Rust 核心链路。

### 待进行 ⏳
- [ ] **CTP 真实接入**：将 `gateway_mock` 替换为真实的 CTP API 逻辑。
- [ ] **后端消费逻辑**：完善 Python 端的 `consumer.py`，实现 MQ 到 WebSocket 的转发。
- [ ] **状态持久化**：在 Python 端实现 Redis 持仓累加逻辑。

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

## 4. 下一步开发路线 (Phase 2)

### 1. CTP 接入
1. 将 CTP SDK (`.so` 和 `.h`) 放入 `gateway_ctp/lib`。
2. 修改 `gateway_ctp/src/main.cpp`，实现 `CThostFtdcMdSpi` 和 `CThostFtdcTraderSpi`。
3. 在回调函数中封装 `omni::EventFrame` 并通过 ZMQ 发出。

### 2. 后端增强
1. 运行 `protocol/scripts/gen_protos.sh` 生成 Python 代码。
2. 完善 `backend_api/app/consumer.py` 中的反序列化逻辑。
3. 实现 WebSocket 广播，将行情推送到 `web_dashboard`。

### 3. 前端实现
1. 使用 `protobufjs` 在前端解析二进制流（或由 Python 转为 JSON 推送以降低复杂度）。
2. 构建实时行情表，实现“红涨绿跌”效果。

---

**Last Updated**: 2026-01-23
**Status**: Phase 1 Complete / Functional Skeleton
