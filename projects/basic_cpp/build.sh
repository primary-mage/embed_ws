#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "[INFO] 清理旧构建..."
rm -rf build

echo "[INFO] cmake 配置..."
cmake -B build -S .

echo "[INFO] 编译..."
cmake --build build

echo "[INFO] 完成! 可执行文件: build/main"
echo ""
echo "试试:"
echo "  ./build/main sys"
echo "  ./build/main proc"
echo "  ./build/main cpu"
echo "  ./build/main list ."
