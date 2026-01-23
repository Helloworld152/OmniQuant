#!/bin/bash

# 获取脚本所在绝对路径
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROTOCOL_DIR="$SCRIPT_DIR/.."
BACKEND_DIR="$SCRIPT_DIR/../../backend_api/app/generated"

echo "=== 生成 Python Protobuf 代码 ==="
echo "协议目录: $PROTOCOL_DIR"
echo "输出目录: $BACKEND_DIR"

mkdir -p "$BACKEND_DIR"

# 使用 protoc 生成 Python 代码
# -I 指定 import 搜索路径
# --python_out 指定输出路径
# 最后一个参数是 proto 文件路径
protoc --python_out="$BACKEND_DIR" -I "$PROTOCOL_DIR" "$PROTOCOL_DIR/omni.proto"

# 创建 __init__.py 使其成为一个 Python 包
touch "$BACKEND_DIR/__init__.py"

echo "生成完成。"