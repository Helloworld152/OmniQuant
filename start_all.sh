#!/bin/bash

# 获取脚本所在绝对路径
ROOT_DIR=$(cd "$(dirname "$0")" && pwd)
SDK_LIB_DIR="$ROOT_DIR/gateway_ctp/ctp_sdk/lib"

# --- 自动清理旧进程 ---
echo "正在清理旧进程..."
pkill -f "gateway_ctp" || true
pkill -f "omni_core_router" || true
# 给一点时间释放端口
sleep 1
# --------------------

# --- 加载环境变量 ---
if [ -f "$ROOT_DIR/.env" ]; then
    echo "正在加载配置文件: .env"
    set -o allexport
    source "$ROOT_DIR/.env"
    set +o allexport
else
    echo "警告: 未找到 .env 文件，将使用默认配置。"
fi
# --------------------

echo "=== 1. 编译 C++ 网关 (CTP) ==="
mkdir -p "$ROOT_DIR/gateway_ctp/build"
cd "$ROOT_DIR/gateway_ctp/build"
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

echo ""
echo "=== 2. 编译 Rust 核心路由 ==="
cd "$ROOT_DIR/core_router"
cargo build

echo ""
echo "=== 3. 启动组件 ==="
# 在后台启动 Rust Core
echo "正在启动 Core Router..."
cd "$ROOT_DIR/core_router"
cargo run &
CORE_PID=$!

# 等待一下确保 Core 端口已绑定
sleep 2

# 在前台启动 C++ Gateway
echo "正在启动 Gateway (CTP)..."
echo "使用库路径: $SDK_LIB_DIR"

# 设置动态库路径并启动
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SDK_LIB_DIR
"$ROOT_DIR/gateway_ctp/build/gateway_ctp"

# 清理
kill $CORE_PID
