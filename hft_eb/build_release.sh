#!/bin/bash
set -e

# 创建构建目录
BUILD_DIR="bin"
mkdir -p $BUILD_DIR

echo ">>> Starting Release Build..."

# 进入目录并执行 CMake
cd $BUILD_DIR
cmake -DCMAKE_BUILD_TYPE=Release ..

# 编译
make -j$(nproc)

echo ">>> Build Complete. Binary is at $BUILD_DIR/hft_engine"
