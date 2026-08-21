# 军工盾 Qt 客户端 — DESIGN

> **定位**：Qt 客户端的架构设计、模块边界、协议映射与功能清单。  
> **维护**：新增或修改功能后同步更新本文档（§12 功能清单、§13 扩展模式、§15 变更记录）。  
> **最后更新**：2026-08-12  
> **代码根目录**：`qt/`

---

## 1. 系统背景

无人机侦测与反制（帧打一体）嵌入式客户端，运行于板端全屏（Jetson/Ubuntu），通过 **TCP** 连接设备服务端，本地 **SQLite** 持久化侦测历史。

| 项 | 值 |
|----|-----|
| 构建 | `qmake` + `make`，`qt.pro` |
| 默认连接 | `10.0.76.209:5555`（`app_config.h`） |
| 历史库 | `{AppDataLocation}/history.db` |
| 鉴权 | `{AppConfigLocation}/auth.ini`（SHA-256；默认 `admin` / `root123`） |
| 首页地图 | QWebEngine → `assets/web/test_map.html` |
| IDE 跳转 | `compile_flags.txt`（clangd，无需编译） |

### 辅助文档

| 文件 | 用途 |
|------|------|
| `readme.md` | 源码阅读顺序 |
| `信号分层.md` | 系统级 vs 业务级信号 |
| `前端结构说明.md` | 首页 JS 模块划分 |
| 根目录 `数据协议1.md` | 完整协议说明 |

---

## 2. 设计目标与约束

### 目标

- 板端全屏运行，五页导航（首页 / 历史 / 白名单 / 统计 / 设置）
- 实时显示设备状态、地图目标、干扰与打击操作
- 设置页覆盖设备全量配置
- 侦测历史本地落库、查询、回放

### 架构约束

| 层级 | 职责 | 禁止 |
|------|------|------|
| **Page** | UI；发 `request*`；收 `update*` | 直接调用 `TcpManager` |
| **Coordinator** | Page ↔ TcpManager 信号接线 | 协议编解码、操作 UI |
| **Service** | payload 编解码；经回调 `sendFrame` | 操作 UI |
| **TcpManager** | Socket、帧解析、路由、重连 | 业务逻辑 |
| **AppController** | 装配模块；广播连接状态与全局设备信息 | 页面级业务 |

---

## 3. 总体架构

```
MainWindow
├── TopNavBar                 导航 / 时钟 / 设备状态弹窗
├── QStackedWidget            五页切换（index 与 TopNavBar 严格对应）
└── AppController
    ├── TcpManager
    │   ├── DroneOpsService
    │   ├── DeviceOpsService
    │   ├── CalibrationService
    │   ├── SpectrumService
    │   └── SettingsProtocolService
    ├── HomeCoordinator       → HomePage
    ├── HistoryCoordinator    → HistoryPage + HistoryRepository
    ├── SettingsCoordinator   → SettingsPage + LocalTimeServiceClient
    ├── DatabaseManager
    ├── HistoryRepository
    └── WhitelistRepository   → HomePage 地图告警评估
```

### 页面索引

| Index | 页面 | 类 | 状态 |
|-------|------|-----|------|
| 0 | 首页 | `HomePage` | ✅ |
| 1 | 侦测历史 | `HistoryPage` | ✅ |
| 2 | 白名单 | `WhitelistPage` | ✅ CRUD + 分页 |
| 3 | 报表统计 | `StatsPage` | ⬜ 占位 |
| 4 | 系统设置 | `SettingsPage` | ✅ |

---

## 4. 启动与装配

**入口**：`main.cpp` → 全屏 `MainWindow`

**装配顺序**（`mainwindow.cpp`）：

1. 创建五页 + `TopNavBar` + `QStackedWidget`
2. `AppController(home, history, settings)` → `connectToDevice()`
3. 绑定导航、首页全屏、顶栏设备信息

**AppController**（`app_controller.cpp`）：

- 初始化 SQLite + `HistoryRepository`
- 创建三个 Coordinator 并 `setupConnections()`
- 系统信号：`deviceStatusChanged`、`deviceStatusInfoUpdated`（DataType=2）

---

## 5. 模块设计

### 5.1 首页（HomePage）

**文件**：`src/views/home/home_page.*`、`video_takeover/*`、`home_web_bridge.*`、`bottom_console.*`、`assets/web/js/*`

**组成**：

- `QWebEngineView`：Leaflet 地图（缩放/平移/全屏/测距/图层）
- `BottomConsole`：通信/导航干扰按钮
- **Web 目标面板**（`target-panel`，`test_map.html`）：无人机目标列表与操作（含「图传接管」按钮）
- **Qt 图传浮层**（`VideoTakeoverWidget`，锚定在 **`.map-pane` 区域右上**，避开右侧 336px 目标面板）：544×272 源比例、`KeepAspectRatio` 绘制；仅面板区域拦截鼠标

**图传模块（方案 B — Facade）**：`VideoTakeoverFacade` 组合：

| 组件 | 职责 |
|------|------|
| `VideoTakeoverPanelController` | 289/290/291 状态机、25fps 节流、3s 首帧超时 |
| `VideoFramePipeline` | 后台 JPEG 解码 + frame token 防乱序 |
| `VideoTakeoverWidget` | Qt overlay UI（meta、画面、关闭按钮） |
| `HomeWebBridge` | 仅 290 → JS（同步 Web 目标面板按钮状态） |

`HomePage` 持有 Facade 指针并转发 toast / `requestDroneVideoTakeover`；`HomeCoordinator` 直连 Facade 的 `on290` / `on291` / `onConnectionLost`。

**Page → Coordinator 信号**：

| 信号 | 业务 |
|------|------|
| `commJammingToggled` / `navJammingToggled` | 干扰 mode 0/1 |
| `requestDroneDirectionFinding` | 测向 |
| `requestDronePrecisionStrike` | 精准打击 |
| `requestDroneWideBandJamming` | 宽频干扰 |
| `requestDroneVideoTakeover` | 图传接管 |
| `fullscreenChanged` | 地图全屏（隐藏顶栏） |

**图传流程（289/290/291）**：

```
JS CMD:VIDEO_TAKEOVER → HomeWebBridge → VideoTakeoverFacade::onUserRequest
  → Controller::onUserRequest → HomeCoordinator → TcpManager::setDroneVideoTakeover(289)
  → 290 → Facade::on290 → Controller::onDeviceResponse + sendVideoTakeoverResponse(Web 按钮态)
  → 291 → Facade::on291 → Controller::onVideoFrame (40ms 节流)
  → Pipeline::submitFrame → Worker 解码 → Widget::displayFrame
```

**图传 UI 状态**：

- 开启/等待首帧：Qt 浮层显示 meta（目标 ID、频点、状态）；Controller 内 3s 首帧超时 → toast
- 关闭：Web 目标面板按钮或 Qt 浮层「关闭」→ `CMD:VIDEO_TAKEOVER:0:…` → 290 成功后隐藏 Qt 浮层
- 首帧到达后停止超时计时，meta 含更新时间与分辨率
- TCP 断开：清帧 + meta 提示「连接已断开，请重新开启图传」

**目标过期**：跟随 `SettingsPage::warningRemoveTimeChanged` → `setWarningRemoveTimeSeconds`

### 5.2 侦测历史（HistoryPage）

**文件**：`src/views/history/history_page.*`、`pilot_location_dialog.*`、`coordinators/history_coordinator.*`、`repositories/history_repository.*`

**Coordinator 职责**：

- 订阅 `droneTargetReported`（DataType 56）→ 会话跟踪 + 落库
- 2s 定时器标记过期会话（超时 = 预警消除时间）
- 响应查询 / 详情 / 回放 / 清空

**会话 key**：`{baseKey}|{yyyyMMddHHmmsszzz}`

**数据模型**（`history_page.h`）：

- `HistoryRecord`：摘要（型号、SN、频率、飞手位置、停留时长等）
- `HistoryDetailEntry`：每次更新的快照（坐标、距离、高度、时间）

**数据库表**：

- `detection_history`（`record_key` UNIQUE）
- `detection_history_detail`

**飞手位置**：表格点击坐标 → `PilotLocationDialog`（Leaflet 地图 marker + 高德 URI 二维码，`pilot_location.html`）；无效坐标时提示，不打开地图。

### 5.3 系统设置（SettingsPage）

**文件**：`src/views/settings/settings_page.*`、`coordinators/settings_coordinator.*`、各子目录

**鉴权**：密码门 → `SettingsUserRole::Admin` / `Root` → 不同侧边栏

| 子页面 | Admin | Root | DataType 域 |
|--------|-------|------|-------------|
| 设备设置 | ✅ | ✅ | GPS 57/59、全频扫描 193/195、IP 25/27、TCP 237/239 |
| 侦测频段 | ✅ | ✅ | 8/10 |
| 模式选择 | ✅ | ✅ | 61/63、130/132、221/223、181/183、214/216、254/252 |
| 固件版本号 | ✅ | ✅ | 14/15 |
| 数据采集 | — | ✅ | 18/19（FTP 上传） |
| 频谱图开关 | — | ✅ | 65–69、218–220 |
| 角度校准 | ✅ | ✅ | 31–38 |
| 打击频率设置 | — | ✅ | 96/98 |
| 打击状态 | ✅ | ✅ | 102/103 |
| 信源参数 | — | ✅ | 105/107 |
| 功放设置 | — | ✅ | 118/120 |
| 测向定标值 | — | ✅ | 124/126 |
| 告警历史 | ✅ | ✅ | 116/117 |
| 机型库 | ✅ | ✅ | 201–208 |
| 授权信息 | — | ✅ | 136/137 |
| 系统功能 | ✅ | ✅ | 蜂鸣器 92/94、本地时间、闪烁、重启 29、密码、预警时间 |
| 系统日志 | ✅ | ✅ | 本地展示 |

**信号命名约定**：

- 发出：`requestSaveXxx` / `requestQueryXxx`
- 接收：`updateXxxSaveResult` / `updateXxxQueried`
- TcpManager：`xxxSetResponse` / `xxxQueried` / `xxxReported`

**地图选点**：`MapPickerDialog` → JS 写 `document.title` → Qt 读坐标回填

---

## 6. 网络与协议

### 6.1 帧格式

`protocol_types.h`：`0xEEEEEEEE` + 29B 头 + payload + 5B 尾（checksum + `0xAAAAAAAA`）

### 6.2 分发结构

| 文件 | 范围 |
|------|------|
| `tcp_manager_base_reports.cpp` | 设备信息 1/2 |
| `tcp_manager_device_delegates.cpp` | 无人机、干扰、图传 |
| `tcp_manager_settings_delegates.cpp` | 设置类 query/set |
| `tcp_manager_spectrum_delegates.cpp` | 频谱 |

### 6.3 设备主动上报

| DT | 含义 | 信号 |
|----|------|------|
| 1 | 旧版设备信息 74B | `deviceInfoParsed` → 首页地图 |
| 2 | 完整状态 183B | `deviceInfoParsed` → 顶栏 |
| 56 | 无人机目标 | `droneTargetReported` |
| 69 | 时频谱 | `spectrumDataReported` |
| 104 | 干扰状态 | `deviceJammingModeReported` |
| 113 | 测向功率 | `droneDirectionPowerReported` |
| 220 | 全频谱 JSON | `fullSpectrumReported` |
| 291 | 图传 JPG | `droneVideoImageReported` |

### 6.4 首页操作协议

| 功能 | 发送 | 响应 | 上报 |
|------|------|------|------|
| 通信/导航干扰 | 100 | 101 | 104 |
| 精准打击 | 109 | 110 | — |
| 测向 | 111 | 112 | 113 |
| 宽频干扰 | 114 | 115 | — |
| 图传接管 | 289 | 290 | 291 |

### 6.5 Service 分工

| Service | DataType |
|---------|----------|
| `DroneOpsService` | 56, 109–115, 289–291 |
| `DeviceOpsService` | 29–30, 92–95, 100–104, 116–117, 136–137 |
| `CalibrationService` | 31–38 |
| `SpectrumService` | 65–69, 218–220 |
| `SettingsProtocolService` | GPS、频段、模式、网络、机型库、功放等 |
| `LocalTimeServiceClient` | 本地（`qt_time_helper/`，非 TCP） |

---

## 7. 数据流模式

### A. 首页下发

```
HomePage::request* → HomeCoordinator → TcpManager → Service → sendFrame
```

### B. 首页上报

```
Device → TcpManager → Service::emit → HomeCoordinator → HomePage::update*
  → HomeWebBridge::runJavaScript → qt_bridge.js
```

### C. 设置页

```
SettingsPage::request* → SettingsCoordinator → TcpManager
  → 响应 → SettingsPage::update*
```

### D. 历史落库

```
DataType 56 → HistoryCoordinator::handleDroneTargetReported
  → HistoryRepository::upsertRecord / appendDetailEntry
  → HistoryPage::upsertRecord（实时刷新表格）
```

### E. Web → Qt（title 命令）

```
JS setQtCommandTitle('CMD:…') → HomeWebBridge::handleTitleCommand → emit → HomePage → Coordinator
```

### F. 系统级（仅 AppController）

```
TcpManager::connected/disconnected/error → deviceStatusChanged
DataType 2 → deviceStatusInfoUpdated → TopNavBar
```

---

## 8. Web 桥接契约

**原则**：`qt_bridge.js` 与 `home_web_bridge.cpp` 的全局函数名必须保持一致。

### Qt → JS

| JS 函数 | C++ |
|---------|-----|
| `updateMarker` / `updateDashboard` | `sendDeviceInfo` |
| `updateDroneTargetFromQt` | `sendDroneTarget` |
| `setWarningClearDelayMs` | `sendWarningRemoveTimeSeconds` |
| `updateDroneDirectionFindingResponseFromQt` | `sendDirectionFindingResponse` |
| `updateDroneDirectionPowerReportFromQt` | `sendDirectionPowerReport` |
| `updateDronePrecisionStrikeResponseFromQt` | `sendPrecisionStrikeResponse` |
| `updateDroneWideBandJammingResponseFromQt` | `sendWideBandJammingResponse` |
| `updateDroneVideoTakeoverResponseFromQt` | `sendVideoTakeoverResponse`（290 同步 Web 按钮态） |

> 注：`showVideoTakeoverPanel` / `updateVideoTakeoverFrame` 等 Web 图传 DOM API 为历史遗留，当前画面由 Qt `VideoTakeoverWidget` 绘制。

### JS → Qt（`document.title`）

| 前缀 | 含义 |
|------|------|
| `CMD:DIRECTION_FINDING:` | 测向 |
| `CMD:PRECISION_STRIKE:` | 精准打击 |
| `CMD:WIDE_JAM:` | 宽频干扰 |
| `CMD:VIDEO_TAKEOVER:` | 图传接管 |
| `CMD:FULLSCREEN_ON/OFF` | 全屏 |

### JS 模块（见 `前端结构说明.md`）

`map_bootstrap.js` → `target_store.js` → `target_panel.js` → `target_actions.js` → `map_core.js` → `qt_bridge.js`

**地图浮层 DOM**（均在 `.map-pane` 内）：

| 元素 | 位置 | z-index | 说明 |
|------|------|---------|------|
| `.custom-controls` | 左下 | 1000 | 缩放/全屏/测距等 |
| `#video-takeover-panel` | 右上 | 1000 | 历史 Web 图传 DOM（当前未用于 291 显示） |
| `.dashboard-container` | 右下 | 1000 | 俯仰角 / 水平角 |
| `#direction-dialog` | 顶部居中 | 1200 | 测向进度 |
| `#map-alarm-overlay` | 全覆盖 | 1100 | 非白名单目标告警闪烁（仅首页地图区） |

**地图告警闪烁**（P1，对齐 `web-ppl` 首页 `.warning`）：

```
showFlash = AlarmPreferences.screenFlashEnabled   // QSettings 持久化
         && homePageVisible                       // stackedWidget index == 0
         && mapPageLoaded
         && ∃ pendingDroneTarget where !WhitelistRepository.containsForTarget(target)
```

Qt：`HomePage::evaluateMapAlarmFlash()` → `HomeWebBridge::setMapAlarmFlashActive()`（地图 `loadFinished` 时 `evaluateMapAlarmFlash(true)` 同步 Web 状态）  
Web：`setMapAlarmFlashActive()` 切换 `#map-alarm-overlay` 的 `map-alarm-overlay--active` 类

---

## 9. 信号分层

| 留在 AppController | 下沉 Coordinator | 下沉 Service |
|--------------------|------------------|--------------|
| `deviceStatusChanged` | 各页 `request*` / `update*` | 协议 parse/encode |
| `deviceStatusInfoUpdated` | 页面间联动（如预警时间） | — |

详见 `信号分层.md`。

---

## 10. 功能清单

### 首页 ✅

- [x] 地图交互（缩放/平移/全屏/测距/图层）
- [x] 设备位置与姿态
- [x] 实时目标 marker + Web 目标面板
- [x] 目标过期清除
- [x] 通信/导航干扰 + 状态同步
- [x] 测向 / 精准打击 / 宽频干扰
- [x] 图传接管 + Qt 地图浮层显示（544×272，`VideoTakeoverFacade`）
- [x] 地图告警闪烁（非白名单目标 + 设置开启 + 仅首页 `.map-pane`）
- [ ] 电子围栏 / 防控区域绘制
- [ ] JS 侧完整详情面板（已实现于 Web `target-panel`）

### 侦测历史 ✅

- [x] 分页表格 + 筛选
- [x] 详情 / 单条轨迹回放
- [x] 清空记录
- [x] 实时落库

### 白名单 ✅

- [x] `WhitelistRepository` + SQLite（CRUD、`containsForTarget` 供首页告警匹配）
- [x] CRUD 页面（序号 / 序列号 / 有效时间 / 有效区域 / 备注 / 操作 + 分页）
- [x] 新增/编辑弹窗（永久有效 + 日期范围、不限制 + 地图绘制区域；应用内 overlay，非原生 `QDialog::exec`）
- [x] 共享 `DateTimePickerPopup`（与历史页同款日历/时分秒；点外/Esc 关闭；应用级 eventFilter）
- [x] 地图区域 WebEngine **懒加载**（仅取消「不限制」时创建 `whitelist_area.html`）
- [x] 首页目标详情「白名单」一键加入（P2，`CMD:WHITELIST_ADD`）
- [x] 有效时间 / 有效区域**匹配逻辑**（P3：`containsForTarget` 校验当前时间与无人机经纬度）

**P3 匹配规则（`containsForTarget`）：**

1. 先用 `targetUniqueId` / `targetId` / `targetName` 在库中查找白名单记录（命中 `serial_number` 或 `record_key` 任一即可）。
2. **有效时间**：`permanent` 始终生效；`range|开始|结束` 则当前系统时间须落在区间内。
3. **有效区域**：`unlimited` 始终生效；自定义 JSON 区域则无人机 `latitude/longitude` 须落在区域内（多边形射线法 / 圆 Haversine 米 / 矩形 bounds）。坐标无效（0,0 或越界）时**不视为白名单**。
4. 仅当序列号命中且时间、区域均满足时，首页才不闪烁告警。

**实现要点 / 已知坑：**

- 写入 SQLite 时，`QString` 默认构造为 **null**（非 `""`），绑参会变成 SQL `NULL`；`model_name` 等 `NOT NULL` 字段须用 `sqlTextValue()` 或显式赋 `""`（见 `whitelist_repository.cpp`）。
- 旧板端库表缺列时，`ensureSchema()` 会 `ALTER TABLE` 补列并回填 `created_at` / `updated_at` / 空字符串。

### 报表统计 ⬜

- [ ] 全部（参考 `web-ppl/views/statistics`）

### 系统设置 ✅

- [x] 全部 17 个子模块（见 §5.3）

---

## 11. 扩展模式

### 11.1 新增首页 / 无人机操作

```
protocol_types.h          新 payload（如需）
drone_ops_service.*       encode/decode + signal
tcp_manager.h             public 方法
tcp_manager_device_delegates.cpp   case 分支
home_coordinator.*        TcpManager ↔ VideoTakeoverFacade
home_page.*               地图 / 目标 / toast；持有 Facade
video_takeover/*          Facade + Controller + Pipeline + Widget
home_web_bridge.* + qt_bridge.js   网页交互（290 按钮态 + CMD 解析）
DESIGN.md §10 + §15      更新清单与变更记录
```

### 11.2 新增设置项

```
settings_protocol_service.* + protocol_types.h
tcp_manager + tcp_manager_settings_delegates.cpp
views/settings/<name>/    子页面
settings_page.cpp         侧边栏 + bindSettingsViewSignals
settings_coordinator.*    双向 connect
```

### 11.3 新增历史字段

```
history_page.h            HistoryRecord / HistoryDetailEntry
history_repository.*      表结构 + SQL
history_coordinator.*     字段映射
```

### 11.4 新增顶级页面

```
views/<name>/
mainwindow.* + top_nav_bar.*
app_controller.*          新建 Coordinator（如需）
qt.pro                    SOURCES / HEADERS / INCLUDEPATH
```

### 11.5 变更记录模板

```markdown
| YYYY-MM-DD | 功能名 | 说明 |
```

---

## 12. 快速定位

| 改什么 | 文件 |
|--------|------|
| 页面切换 / 顶栏 | `mainwindow.cpp`, `top_nav_bar.cpp` |
| TCP / 重连 | `tcp_manager.cpp`, `app_config.h` |
| DataType 解析 | `network/dispatch/tcp_manager_*_delegates.cpp` |
| 首页 / 地图 / 图传 | `home_page.cpp`, `video_takeover/*`, `home_web_bridge.*`, `assets/web/test_map.html` |
| 历史表格 / 落库 / 飞手位置 | `history_page.cpp`, `pilot_location_dialog.*`, `assets/web/pilot_location.html`, `history_coordinator.cpp`, `history_repository.cpp` |
| 设置子模块 | `views/settings/<模块>/`, `settings_coordinator.cpp` |
| 协议结构体 | `protocol_types.h` |
| 密码 / 权限 | `settings_page.cpp`, `settings_role.h` |

---

## 13. 目录结构

```
qt/
├── DESIGN.md                 ← 本文档
├── qt.pro
├── compile_flags.txt
├── assets/web/               首页 & 回放 HTML/JS
├── src/
│   ├── main.cpp / mainwindow.*
│   ├── app_controller.* / app_config.h / device_status.h
│   ├── components/           top_nav_bar, datetime_picker_popup
│   ├── coordinators/         home, history, settings
│   ├── database/             database_manager
│   ├── preferences/          alarm_preferences (QSettings)
│   ├── repositories/         history_repository, whitelist_repository
│   ├── network/
│   │   ├── core/             tcp_manager, protocol_types
│   │   └── dispatch/         *_delegates.cpp
│   ├── services/             drone/device/calibration/spectrum/settings/local_time
│   └── views/
│       ├── home/ history/ whitelist/ statistics/ settings/
└── qt_time_helper/           本地时间 sidecar
```

---

## 14. 待实现（Roadmap）

参考 Web 版 `web-ppl/`：

1. ~~白名单 CRUD + 分页~~ ✅
2. 报表统计（饼图、KPI、折线、热力图）
3. 首页电子围栏绘制
4. ~~白名单有效时间 / 有效区域匹配（P3）~~ ✅

---

## 15. 变更记录

| 日期 | 变更 |
|------|------|
| 2026-07-30 | 图传方案 B | 抽取 `video_takeover/` 模块：`VideoTakeoverFacade` + `VideoFramePipeline` + `VideoTakeoverWidget`；`HomePage` 瘦身；Coordinator 直连 Facade |
| 2026-08-13 | 白名单删除确认 | 删除/错误提示改为应用内 overlay，避免全屏下原生 `QMessageBox` 露出桌面 |
| 2026-08-13 | 白名单新增弹窗卡顿 | `DateTimePickerPopup` 应用级 eventFilter（点外/Esc 关闭）；地图 WebEngine 懒加载；关闭/切换/resize 时 `hidePopup` |
| 2026-08-12 | 白名单新增弹窗 | 完整表单：有效时间（永久/日期范围）、有效区域（不限制/地图绘制）；新增 `whitelist_area.html/js` |
| 2026-08-12 | 白名单 P1+P2 | `WhitelistRepository` CRUD + `WhitelistPage` 分页表格；首页 `containsForTarget` 告警匹配；目标详情「白名单」按钮经 `CMD:WHITELIST_ADD` 一键加入 |
| 2026-08-12 | 地图告警闪烁 | P1 落地：Web 四边渐变闪烁 + `WhitelistRepository` + `AlarmPreferences`；移除旧 `ScreenFlashOverlay` |
| 2026-08-12 | 飞手位置 | 移除「复制坐标」按钮 |
| 2026-08-12 | 飞手位置 | `PilotLocationDialog`：Web 地图 + 高德导航二维码；新增 `pilot_location.html/js`、`qrcode.js` |
| 2026-08-12 | 侦测历史 | 移除未实现的批量回放工具栏按钮；单条行内回放保留 |
| 2026-08-12 | 侦测历史 | 移除未实现的「导出 Excel」按钮与相关占位 |
| 2026-08-12 | P1 图传结构 | 抽取 `VideoTakeoverPanelController`（状态机 + Web 推送 + 3s 首帧超时）；`updateVideoTakeoverOverlayStatus` 更名为 `syncVideoTakeoverPanelMeta` |
| 2026-08-12 | P0 代码清理 | 移除 `HomePage` Qt `rightPanel` 死代码；清理 `tcp_manager` `[DEBUG-E]` 调试日志 |
| 2026-08-12 | 图传面板位置 | `#video-takeover-panel` 由左上改为右上锚定（`map.css`） |
| 2026-08-12 | 图传 UI 方案 A | 移除 Qt `VideoTakeoverOverlay` 全屏模态；图传面板迁入 Web `.map-pane`（`#video-takeover-panel`），与俯仰角 dashboard 同层；`HomeWebBridge` 增加 show/hide/meta/frame 推送；清理 `[Video291]` 调试日志与 `video_frame_debug.h` |
| 2026-08-11 | 图传 291 | 修复 TCP 收包误把 JPEG 二进制当 hex 文本转换；增强 Video291 打桩日志（后于 2026-08-12 已移除打桩） |
| 2026-08-01 | 图传浮层缩小至最大 800×540（后于 2026-08-12 改为 Web 小窗，不再使用 Qt overlay） |
| 2026-08-01 | 初始功能梳理 |

<!-- 新功能在此追加 -->
