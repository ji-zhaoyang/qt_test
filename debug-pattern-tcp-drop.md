# [OPEN] pattern-tcp-drop

## 背景
- 现象：点击“开始采集”后，主 TCP 连接被远端关闭，随后一段时间持续 `ConnectionRefusedError`。
- 期望：点击“开始采集”仅发送 `DataType=18` 采集任务，不应导致主 TCP 连接断开。

## 当前假设
1. `DataType=18` 的 JSON 参数或字段顺序与设备端预期不一致，设备端收到后主动关闭连接。
2. 设备端在处理采集命令时会重启或拉起内部服务，因此主连接先被关闭，短时间内端口拒绝连接。
3. Qt 端除 `18` 之外还触发了其他链路，导致设备状态切换或服务退出。
4. 采集请求中的某个具体字段值触发设备端异常分支，例如 `filename`、`type`、`path` 或 FTP 参数。
5. `sendFrame()` 组出来的二进制帧内容与能工作的工程仍有差异，设备端在解析时直接异常退出。

## 调试计划
1. 只增加终端打桩，不改业务逻辑。
2. 记录点击采集、请求参数、发送帧摘要、Socket 报错、断开、重连。
3. 复现一次后，根据终端日志判定是哪条假设成立。

## 结果记录
- 已复现
- `DEBUG-A/B/C` 证实点击采集后确实发送了 `DataType=18`
- 发送内容：
  - `ip=10.9.0.20`
  - `port=21`
  - `user=finsung`
  - `password=finsung`
  - `path=/home/finsung/qt`
  - `time=1`
  - `channel=1`
  - `type=1`
  - `filename=11`
  - `freq=0.0`
- `DEBUG-D/E` 证实：
  - 最后一次发送的 `dataType=18`
  - 远端先 `RemoteHostClosedError`
  - 随后出现连续 `ConnectionRefusedError`
  - 断开前最后一次接收的仍是 `dataType=2`
- 当前倾向：
  - 假设 2 强支持：设备端在处理 `18` 后服务退出/重启
  - 假设 3 基本排除：没有证据表明点击采集时额外发送了别的业务帧
  - 假设 1 / 4 / 5 仍需继续通过设备端日志或更小步参数试验确认
- 追加试验：将 `freq` 从 `0.0` 改为 `1.0`
  - `DEBUG-A/B/C` 证实本轮确实发送了 `freq=1.0`
  - 现象未变：仍然是 `RemoteHostClosedError` 后跟连续 `ConnectionRefusedError`
  - 结论：`freq=0.0` 不是根因，至少不是主要根因
