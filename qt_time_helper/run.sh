#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_NAME="qt_time_helper"
SERVICE_NAME="qt-time-helper"
INSTALL_BIN="/usr/local/bin/${PROJECT_NAME}"
INSTALL_SERVICE="/etc/systemd/system/${SERVICE_NAME}.service"
SOCKET_PATH="/tmp/${PROJECT_NAME}.sock"

if [ "$(id -u)" -eq 0 ]; then
    SUDO=""
else
    SUDO="sudo"
fi

NPROC="$(nproc 2>/dev/null || echo 4)"

echo "==================================="
echo "  Build And Deploy ${PROJECT_NAME}"
echo "==================================="

cd "${SCRIPT_DIR}"

echo "[1/7] Run qmake..."
qmake qt_time_helper.pro

echo "[2/7] Build project (make -j${NPROC})..."
make -j"${NPROC}"

echo "[3/7] Stop old service..."
${SUDO} systemctl stop "${SERVICE_NAME}" >/dev/null 2>&1 || true

echo "[4/7] Remove stale socket..."
${SUDO} rm -f "${SOCKET_PATH}"

echo "[5/7] Install binary to ${INSTALL_BIN}..."
${SUDO} install -m 0755 "${PROJECT_NAME}" "${INSTALL_BIN}"

echo "[6/7] Install service file..."
${SUDO} install -m 0644 "deploy/${SERVICE_NAME}.service" "${INSTALL_SERVICE}"

echo "[7/7] Reload and enable service..."
${SUDO} systemctl daemon-reload
${SUDO} systemctl enable "${SERVICE_NAME}" >/dev/null 2>&1 || true

echo "[8/8] Start service..."
${SUDO} systemctl restart "${SERVICE_NAME}"

echo "==================================="
echo "Deploy finished."
echo "Service status:"
${SUDO} systemctl --no-pager --full status "${SERVICE_NAME}" || true
echo "Socket status:"
ls -l "${SOCKET_PATH}" 2>/dev/null || echo "${SOCKET_PATH} not found"
echo "==================================="
