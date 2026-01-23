#!/bin/bash

# 获取项目根目录
ROOT_DIR=$(cd "$(dirname "$0")" && pwd)

# 定义清理函数
cleanup() {
    echo ""
    echo "=== 正在关闭所有服务 ==="
    
    # 杀掉 Python 后端
    if [ -n "$BACKEND_PID" ]; then
        echo "停止 Backend API (PID: $BACKEND_PID)..."
        kill $BACKEND_PID 2>/dev/null
    fi

    # 杀掉 Vue 前端
    if [ -n "$FRONTEND_PID" ]; then
        echo "停止 Web Dashboard (PID: $FRONTEND_PID)..."
        kill $FRONTEND_PID 2>/dev/null
    fi

    # 杀掉 Core/Gateway (由 start_all.sh 启动的，通常 start_all.sh 退出会带走它们，
    # 但为了保险，我们再杀一次名字)
    pkill -f "gateway_ctp"
    pkill -f "omni_core_router"
    
    echo "所有服务已停止。"
    exit
}

# 捕获退出信号
trap cleanup SIGINT SIGTERM

echo "=== OmniQuant 一键启动 ==="

# 1. 启动 Python 后端
echo "[1/3] 启动 Backend API (使用 venv)..."
cd "$ROOT_DIR/backend_api"
# 使用虚拟环境中的 uvicorn，指定端口 9999
./venv/bin/uvicorn app.main:app --host 0.0.0.0 --port 9999 > "$ROOT_DIR/backend.log" 2>&1 &
BACKEND_PID=$!
echo "Backend 运行中 (PID: $BACKEND_PID), 日志: backend.log"

# 2. 启动 Vue 前端
echo "[2/3] 启动 Web Dashboard..."
cd "$ROOT_DIR/web_dashboard"
# npm install # 首次运行需解开
npm run dev > "$ROOT_DIR/frontend.log" 2>&1 &
FRONTEND_PID=$!
echo "Frontend 运行中 (PID: $FRONTEND_PID), 访问: http://localhost:5173"

# 3. 启动 Core & Gateway (保留在前台显示核心日志)
echo "[3/3] 启动 Core & Gateway..."
echo "---------------------------------------------------"
echo "系统全开！按 Ctrl+C 退出所有服务。"
echo "---------------------------------------------------"

# 回到根目录运行 start_all.sh
cd "$ROOT_DIR"
./start_all.sh

# 等待 start_all.sh 结束（通常它会一直运行直到 Ctrl+C）
wait
