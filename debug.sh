#!/bin/bash

# 发生错误时立即退出
set -e

echo "==================================="
echo "  DEBUG 构建军工盾 Qt 上位机程序"
echo "==================================="

# 1. 清理旧的编译文件
echo "[1/3] 清理旧的编译产物..."
make clean > /dev/null 2>&1 || true

# 清理历史上遗留在项目根目录的 qmake/make 中间文件
echo "      清理根目录遗留的 .o / moc / qrc / ui 文件..."
find . -maxdepth 1 -type f \( \
    -name "*.o" -o \
    -name "moc_*.cpp" -o \
    -name "moc_*.o" -o \
    -name "qrc_*.cpp" -o \
    -name "qrc_*.o" -o \
    -name "ui_*.h" \
\) -delete

# 清理新的独立构建目录，避免旧缓存影响重新生成
rm -rf .build

# 2. 生成 Debug Makefile
echo "[2/3] 正在执行 qmake (CONFIG+=debug)..."
qmake "CONFIG+=debug" qt_test.pro

# 3. 多线程编译 Debug 版本
echo "[3/3] 正在编译 Debug 版本 (make -j4)..."
make -j4

echo "==================================="
echo "Debug 编译完成。"
echo "下一步可执行："
echo "  gdb ./qt_test"
echo "如果需要图形界面环境，可在 gdb 中执行："
echo "  set env DISPLAY :0"
echo "==================================="
