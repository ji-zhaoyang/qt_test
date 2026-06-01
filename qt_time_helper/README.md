# qt_time_helper

`qt_time_helper` 是给 `qt_test` 提供本地高权限设时能力的辅助服务。

## 作用

- 监听本机 `QLocalSocket`
- 接收 `qt_test` 发来的设时请求
- 以 root 权限执行：
  - `timedatectl set-timezone`
  - `timedatectl set-time`
  - `date -s` 作为回退
  - `hwclock -w`

## 通信协议

请求：

```json
{"action":"set_system_time","datetime":"2026-06-01 12:34:56","timezone":"Asia/Shanghai"}
```

探活请求：

```json
{"action":"ping"}
```

响应：

```json
{"success":true,"message":"本机时间设置成功"}
```

## 构建

```bash
cd /path/to/qt_time_helper
qmake qt_time_helper.pro
make -j$(nproc)
```

## 安装

```bash
sudo cp qt_time_helper /usr/local/bin/qt_time_helper
sudo cp deploy/qt-time-helper.service /etc/systemd/system/qt-time-helper.service
sudo systemctl daemon-reload
sudo systemctl enable qt-time-helper
sudo systemctl start qt-time-helper
```

## 查看状态

```bash
systemctl status qt-time-helper
journalctl -u qt-time-helper -f
```

## 安全限制

- 仅支持白名单动作：`ping`、`set_system_time`
- 时区必须是系统支持的 `IANA` 时区 ID
- 时间范围限制为 `2020-01-01` 到 `2100-12-31`
- 本地 socket 不再只限 owner 访问，真正的访问控制由服务端调用方校验负责
- Linux 下仅允许白名单进程名通过本地 socket 调用，当前默认允许：
  - `qt_test`
  - `qt_testApp`
