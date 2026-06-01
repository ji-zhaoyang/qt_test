#!/bin/bash

# 发生错误时立即退出
set -e

echo "==================================="
echo "  🚀 开始编译军工盾 Qt 上位机程序"
echo "==================================="

# 1. 清理旧的编译文件
echo "[1/4] 清理旧的编译产物..."
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

# 2. 生成 Makefile
echo "[2/4] 正在执行 qmake..."
qmake qt_test.pro

# 3. 多线程编译
echo "[3/4] 正在编译 (make -j4)..."
make -j4

# 4. 运行程序
echo "[4/4] 编译成功！准备启动程序..."
echo "==================================="

# 导出显示环境变量 (适用于 Jetson 等通过 SSH 远程启动或多屏环境)
export DISPLAY=:0

# 启动程序
./qt_test
