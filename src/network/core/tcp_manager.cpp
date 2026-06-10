#include "tcp_manager.h"
#include "services/calibration_service.h"
#include "services/device_ops_service.h"
#include "services/drone_ops_service.h"
#include "services/spectrum_service.h"
#include "services/settings_protocol_service.h"
#include <QDateTime>
#include <QDebug>
#include <QTimer>
#include <cstdlib>
#include <cstring>

namespace
{
QString socketErrorText(QAbstractSocket::SocketError error)
{
    switch (error)
    {
    case QAbstractSocket::ConnectionRefusedError:
        return "ConnectionRefusedError";
    case QAbstractSocket::RemoteHostClosedError:
        return "RemoteHostClosedError";
    case QAbstractSocket::HostNotFoundError:
        return "HostNotFoundError";
    case QAbstractSocket::SocketAccessError:
        return "SocketAccessError";
    case QAbstractSocket::SocketResourceError:
        return "SocketResourceError";
    case QAbstractSocket::SocketTimeoutError:
        return "SocketTimeoutError";
    case QAbstractSocket::DatagramTooLargeError:
        return "DatagramTooLargeError";
    case QAbstractSocket::NetworkError:
        return "NetworkError";
    case QAbstractSocket::AddressInUseError:
        return "AddressInUseError";
    case QAbstractSocket::SocketAddressNotAvailableError:
        return "SocketAddressNotAvailableError";
    case QAbstractSocket::UnsupportedSocketOperationError:
        return "UnsupportedSocketOperationError";
    case QAbstractSocket::ProxyAuthenticationRequiredError:
        return "ProxyAuthenticationRequiredError";
    case QAbstractSocket::SslHandshakeFailedError:
        return "SslHandshakeFailedError";
    case QAbstractSocket::UnfinishedSocketOperationError:
        return "UnfinishedSocketOperationError";
    case QAbstractSocket::ProxyConnectionRefusedError:
        return "ProxyConnectionRefusedError";
    case QAbstractSocket::ProxyConnectionClosedError:
        return "ProxyConnectionClosedError";
    case QAbstractSocket::ProxyConnectionTimeoutError:
        return "ProxyConnectionTimeoutError";
    case QAbstractSocket::ProxyNotFoundError:
        return "ProxyNotFoundError";
    case QAbstractSocket::ProxyProtocolError:
        return "ProxyProtocolError";
    case QAbstractSocket::OperationError:
        return "OperationError";
    case QAbstractSocket::SslInternalError:
        return "SslInternalError";
    case QAbstractSocket::SslInvalidUserDataError:
        return "SslInvalidUserDataError";
    case QAbstractSocket::TemporaryError:
        return "TemporaryError";
    case QAbstractSocket::UnknownSocketError:
    default:
        return "UnknownSocketError";
    }
}

} // namespace

TcpManager::TcpManager(QObject *parent)
    : QObject(parent),
      droneOpsService(new DroneOpsService(this)),
      deviceOpsService(new DeviceOpsService(this)),
      calibrationService(new CalibrationService(this)),
      spectrumService(new SpectrumService(this)),
      settingsProtocolService(new SettingsProtocolService(this))
{
    qRegisterMetaType<SpectrumReportData>("SpectrumReportData");
    qRegisterMetaType<FullSpectrumReportData>("FullSpectrumReportData");
    qRegisterMetaType<StrikeFrequencyBandList>("StrikeFrequencyBandList");
    qRegisterMetaType<PowerAmplifierParamList>("PowerAmplifierParamList");
    qRegisterMetaType<DirectionCalibrationValueList>("DirectionCalibrationValueList");
    qRegisterMetaType<AlarmHistoryInfo>("AlarmHistoryInfo");
    qRegisterMetaType<DeviceUsageInfo>("DeviceUsageInfo");
    qRegisterMetaType<SignalSourceParamsConfig>("SignalSourceParamsConfig");
    qRegisterMetaType<PatternUploadRequest>("PatternUploadRequest");
    qRegisterMetaType<ModelLibraryPageResult>("ModelLibraryPageResult");
    qRegisterMetaType<ModelLibraryUpdateRequest>("ModelLibraryUpdateRequest");

    tcpSocket = new QTcpSocket(this);

    // 初始化断线重连定时器
    reconnectTimer = new QTimer(this);
    reconnectTimer->setSingleShot(true);
    reconnectTimer->setInterval(reconnectIntervalMs);
    connect(reconnectTimer, &QTimer::timeout, this, &TcpManager::onReconnectTimeout);

    // 绑定底层 socket 信号到当前类的槽函数
    connect(tcpSocket, &QTcpSocket::connected, this, &TcpManager::onSocketConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &TcpManager::onSocketDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &TcpManager::onSocketReadyRead);
    connect(tcpSocket, static_cast<void (QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error), this,
            &TcpManager::onSocketError);

    droneOpsService->setFrameSender(
        [this](uint16_t dataType, const QByteArray &payload)
        {
            sendFrame(dataType, payload);
        });
    connect(droneOpsService, &DroneOpsService::droneTargetReported, this, &TcpManager::droneTargetReported);
    connect(droneOpsService, &DroneOpsService::droneDirectionFindingResponse,
            this, &TcpManager::droneDirectionFindingResponse);
    connect(droneOpsService, &DroneOpsService::droneDirectionPowerReported,
            this, &TcpManager::droneDirectionPowerReported);
    connect(droneOpsService, &DroneOpsService::dronePrecisionStrikeResponse,
            this, &TcpManager::dronePrecisionStrikeResponse);
    connect(droneOpsService, &DroneOpsService::droneWideBandJammingResponse,
            this, &TcpManager::droneWideBandJammingResponse);

    deviceOpsService->setFrameSender(
        [this](uint16_t dataType, const QByteArray &payload)
        {
            sendFrame(dataType, payload);
        });
    connect(deviceOpsService, &DeviceOpsService::deviceJammingModeSetResponse,
            this, &TcpManager::deviceJammingModeSetResponse);
    connect(deviceOpsService, &DeviceOpsService::deviceJammingModeReported,
            this, &TcpManager::deviceJammingModeReported);
    connect(deviceOpsService, &DeviceOpsService::deviceJammingStatusQueried,
            this, &TcpManager::deviceJammingStatusQueried);
    connect(deviceOpsService, &DeviceOpsService::alarmHistoryQueried,
            this, &TcpManager::alarmHistoryQueried);
    connect(deviceOpsService, &DeviceOpsService::deviceUsageInfoQueried,
            this, &TcpManager::deviceUsageInfoQueried);
    connect(deviceOpsService, &DeviceOpsService::buzzerEnabledSetResponse,
            this, &TcpManager::buzzerEnabledSetResponse);
    connect(deviceOpsService, &DeviceOpsService::buzzerEnabledQueried,
            this, &TcpManager::buzzerEnabledQueried);
    connect(deviceOpsService, &DeviceOpsService::deviceRebootResponse,
            this, &TcpManager::deviceRebootResponse);

    calibrationService->setFrameSender(
        [this](uint16_t dataType, const QByteArray &payload)
        {
            sendFrame(dataType, payload);
        });
    connect(calibrationService, &CalibrationService::compassCalibrationResponse,
            this, &TcpManager::compassCalibrationResponse);

    spectrumService->setFrameSender(
        [this](uint16_t dataType, const QByteArray &payload)
        {
            sendFrame(dataType, payload);
        });
    connect(spectrumService, &SpectrumService::spectrogramSwitchResponse,
            this, &TcpManager::spectrogramSwitchResponse);
    connect(spectrumService, &SpectrumService::spectrumDataReported,
            this, &TcpManager::spectrumDataReported);
    connect(spectrumService, &SpectrumService::fullSpectrumSwitchResponse,
            this, &TcpManager::fullSpectrumSwitchResponse);
    connect(spectrumService, &SpectrumService::fullSpectrumReported,
            this, &TcpManager::fullSpectrumReported);

    settingsProtocolService->setFrameSender(
        [this](uint16_t dataType, const QByteArray &payload)
        {
            sendFrame(dataType, payload);
        });
    connect(settingsProtocolService, &SettingsProtocolService::deviceGpsQueried,
            this, &TcpManager::deviceGpsQueried);
    connect(settingsProtocolService, &SettingsProtocolService::deviceGpsSetResponse,
            this, &TcpManager::deviceGpsSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::detectBandsSetResponse,
            this, &TcpManager::detectBandsSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::detectBandsQueried,
            this, &TcpManager::detectBandsQueried);
    connect(settingsProtocolService, &SettingsProtocolService::droneReportModeSetResponse,
            this, &TcpManager::droneReportModeSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::droneReportModeQueried,
            this, &TcpManager::droneReportModeQueried);
    connect(settingsProtocolService, &SettingsProtocolService::suppressionModeSetResponse,
            this, &TcpManager::suppressionModeSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::suppressionModeQueried,
            this, &TcpManager::suppressionModeQueried);
    connect(settingsProtocolService, &SettingsProtocolService::o4ServerModeSetResponse,
            this, &TcpManager::o4ServerModeSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::o4ServerModeQueried,
            this, &TcpManager::o4ServerModeQueried);
    connect(settingsProtocolService, &SettingsProtocolService::uavCategoryDisplayModeSetResponse,
            this, &TcpManager::uavCategoryDisplayModeSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::uavCategoryDisplayModeQueried,
            this, &TcpManager::uavCategoryDisplayModeQueried);
    connect(settingsProtocolService, &SettingsProtocolService::dataEnableSetResponse,
            this, &TcpManager::dataEnableSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::dataEnableQueried,
            this, &TcpManager::dataEnableQueried);
    connect(settingsProtocolService, &SettingsProtocolService::featureModesSetResponse,
            this, &TcpManager::featureModesSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::featureModesQueried,
            this, &TcpManager::featureModesQueried);
    connect(settingsProtocolService, &SettingsProtocolService::fullScanParamsSetResponse,
            this, &TcpManager::fullScanParamsSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::fullScanParamsQueried,
            this, &TcpManager::fullScanParamsQueried);
    connect(settingsProtocolService, &SettingsProtocolService::deviceIpSetResponse,
            this, &TcpManager::deviceIpSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::deviceIpQueried,
            this, &TcpManager::deviceIpQueried);
    connect(settingsProtocolService, &SettingsProtocolService::tcpServerIpSetResponse,
            this, &TcpManager::tcpServerIpSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::tcpServerIpQueried,
            this, &TcpManager::tcpServerIpQueried);
    connect(settingsProtocolService, &SettingsProtocolService::modelLibraryModeSetResponse,
            this, &TcpManager::modelLibraryModeSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::modelLibraryModeQueried,
            this, &TcpManager::modelLibraryModeQueried);
    connect(settingsProtocolService, &SettingsProtocolService::modelLibraryRecordSetResponse,
            this, &TcpManager::modelLibraryRecordSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::modelLibraryRecordsQueried,
            this, &TcpManager::modelLibraryRecordsQueried);
    connect(settingsProtocolService, &SettingsProtocolService::strikeFrequencyBandsSetResponse,
            this, &TcpManager::strikeFrequencyBandsSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::strikeFrequencyBandsQueried,
            this, &TcpManager::strikeFrequencyBandsQueried);
    connect(settingsProtocolService, &SettingsProtocolService::powerAmplifierParamsSetResponse,
            this, &TcpManager::powerAmplifierParamsSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::powerAmplifierParamsQueried,
            this, &TcpManager::powerAmplifierParamsQueried);
    connect(settingsProtocolService, &SettingsProtocolService::directionCalibrationValuesSetResponse,
            this, &TcpManager::directionCalibrationValuesSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::directionCalibrationValuesQueried,
            this, &TcpManager::directionCalibrationValuesQueried);
    connect(settingsProtocolService, &SettingsProtocolService::signalSourceParamsSetResponse,
            this, &TcpManager::signalSourceParamsSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::signalSourceParamsQueried,
            this, &TcpManager::signalSourceParamsQueried);
    connect(settingsProtocolService, &SettingsProtocolService::patternUploadResponse,
            this, &TcpManager::patternUploadResponse);
    connect(settingsProtocolService, &SettingsProtocolService::firmwareVersionsQueried,
            this, &TcpManager::firmwareVersionsQueried);
}

TcpManager::~TcpManager()
{
    if (tcpSocket->isOpen())
    {
        tcpSocket->close();
    }
}

// 连接：建连入口与重连参数维护。
void TcpManager::connectToServer(const QString &ip, quint16 port)
{
    lastConnectAttemptAt = QDateTime::currentDateTime();
    lastConnectedIp = ip;
    lastConnectedPort = port;

    if (tcpSocket->state() == QAbstractSocket::ConnectedState)
    {
        tcpSocket->disconnectFromHost();
    }

    tcpSocket->connectToHost(ip, port);
}

void TcpManager::setReconnectIntervalMs(int intervalMs)
{
    reconnectIntervalMs = qMax(200, intervalMs);
    reconnectTimer->setInterval(reconnectIntervalMs);
}

void TcpManager::sendCommand(const QString &cmd)
{
    if (tcpSocket->state() == QAbstractSocket::ConnectedState)
    {
        tcpSocket->write(cmd.toUtf8());
        qDebug() << "[TcpManager] 已发送数据:" << cmd.trimmed();
    }
    else
    {
        qDebug() << "[TcpManager] 发送失败，未连接服务器";
    }
}

// 发包：统一封装协议头、载荷、校验和与包尾。
void TcpManager::sendFrame(uint16_t dataType, const QByteArray &data)
{
    // #region debug-point C:send-frame-entry
    if (dataType == 18)
    {
        qDebug().noquote()
            << QStringLiteral("[DEBUG-C] tcp_manager.cpp:sendFrame | 进入发送 | dataType=%1 payloadSize=%2 socketState=%3 target=%4:%5")
                   .arg(QString::number(dataType),
                        QString::number(data.size()),
                        socketStateText(),
                        lastConnectedIp,
                        QString::number(lastConnectedPort));
        qDebug().noquote()
            << QStringLiteral("[DEBUG-C] tcp_manager.cpp:sendFrame | payloadUtf8=%1").arg(QString::fromUtf8(data));
        qDebug().noquote()
            << QStringLiteral("[DEBUG-C] tcp_manager.cpp:sendFrame | payloadHex=%1")
                   .arg(QString::fromLatin1(data.toHex(' ').toUpper()));
    }
    // #endregion

    if (tcpSocket->state() != QAbstractSocket::ConnectedState)
    {
        qDebug() << "[TcpManager] 未连接服务器，无法发送命令 (DataType:" << dataType << ") 当前状态:" << socketStateText()
                 << "目标地址:" << lastConnectedIp << ":" << lastConnectedPort;
        return;
    }

    ProtocolHeader header;
    header.startFlag = 0xEEEEEEEE;
    header.version = 0x0302;
    header.length = sizeof(ProtocolHeader) + data.size() + sizeof(ProtocolTail);

    QDateTime now = QDateTime::currentDateTime();
    QDate date = now.date();
    QTime time = now.time();

    header.year = date.year();
    header.month = date.month();
    header.day = date.day();
    header.hour = time.hour();
    header.minute = time.minute();
    header.second = time.second();
    header.millisecond = time.msec();
    header.dataType = dataType;

    static uint64_t s_packageNum = 0;
    header.packageNum = ++s_packageNum;

    QByteArray frame;
    frame.append(reinterpret_cast<const char *>(&header), sizeof(ProtocolHeader));
    frame.append(data);

    // 校验和：帧头与数据部分和取低字节（不包括开始标志部分4字节）
    uint8_t checksum = 0;
    for (int i = 4; i < frame.size(); ++i)
    {
        checksum += static_cast<uint8_t>(frame.at(i));
    }

    ProtocolTail tail;
    tail.checksum = checksum;
    tail.endFlag = 0xAAAAAAAA;

    frame.append(reinterpret_cast<const char *>(&tail), sizeof(ProtocolTail));

    tcpSocket->write(frame);
    tcpSocket->flush();
    rememberSentFrame(dataType, frame.size());

    // #region debug-point C:send-frame-written
    if (dataType == 18)
    {
        qDebug().noquote()
            << QStringLiteral("[DEBUG-C] tcp_manager.cpp:sendFrame | 已写入socket | frameSize=%1 checksum=%2 packageNum=%3")
                   .arg(QString::number(frame.size()),
                        QString::number(static_cast<int>(checksum)),
                        QString::number(header.packageNum));
        qDebug().noquote()
            << QStringLiteral("[DEBUG-C] tcp_manager.cpp:sendFrame | frameHex=%1")
                   .arg(QString::fromLatin1(frame.toHex(' ').toUpper()));
    }
    // #endregion
}

bool TcpManager::isConnected() const
{
    return tcpSocket->state() == QAbstractSocket::ConnectedState;
}

void TcpManager::onSocketConnected()
// 连接：底层 socket 状态变化与重连定时器管理。
{
    lastConnectedAt = QDateTime::currentDateTime();
    lastConnectedAt = QDateTime::currentDateTime();
    lastSocketError = QAbstractSocket::UnknownSocketError;
    lastSocketErrorText.clear();
    qDebug() << "[TcpManager] TCP 客户端连接成功！";
    reconnectTimer->stop(); // 连接成功，停止重连定时器
    emit connected();
}

void TcpManager::onSocketDisconnected()
{
    lastDisconnectedAt = QDateTime::currentDateTime();
    // #region debug-point E:socket-disconnected
    qDebug().noquote()
        << QStringLiteral("[DEBUG-E] tcp_manager.cpp:onSocketDisconnected | 连接断开 | lastSentDataType=%1 lastSentFrameLength=%2 "
                          "lastReceivedDataType=%3 lastReceivedFrameLength=%4 socketState=%5 lastError=%6")
               .arg(QString::number(lastSentDataType),
                    QString::number(lastSentFrameLength),
                    QString::number(lastReceivedDataType),
                    QString::number(lastReceivedFrameLength),
                    socketStateText(),
                    lastSocketErrorText);
    // #endregion
    qDebug() << "[TcpManager] TCP 连接已断开！";
    emit disconnected();

    // 断开后启动重连机制
    if (!lastConnectedIp.isEmpty() && lastConnectedPort != 0)
    {
        const int delayMs = nextReconnectDelayMs();
        reconnectTimer->start(delayMs);
    }
}

void TcpManager::onReconnectTimeout()
{
    if (tcpSocket->state() == QAbstractSocket::ConnectedState)
    {
        reconnectTimer->stop();
        return;
    }

    if (lastConnectedIp.isEmpty() || lastConnectedPort == 0)
    {
        return;
    }

    if (tcpSocket->state() != QAbstractSocket::UnconnectedState)
    {
        tcpSocket->abort();
    }

    qDebug() << "[TcpManager] 正在尝试重新连接...";
    // #region debug-point E:reconnect-attempt
    qDebug().noquote()
        << QStringLiteral("[DEBUG-E] tcp_manager.cpp:onReconnectTimeout | 开始重连 | target=%1:%2 lastSentDataType=%3 socketState=%4")
               .arg(lastConnectedIp,
                    QString::number(lastConnectedPort),
                    QString::number(lastSentDataType),
                    socketStateText());
    // #endregion
    tcpSocket->connectToHost(lastConnectedIp, lastConnectedPort);
}

// 收包：读取 socket 数据并兼容调试助手发送的十六进制字符串。
void TcpManager::onSocketReadyRead()
{
    QByteArray data = tcpSocket->readAll();

    // 【关键修复】：由于网络调试助手可能发送的是 "EE EE" 这样的 ASCII 字符串，
    // 我们需要判断并将其转换为真正的二进制十六进制数据
    QByteArray realData;
    if (data.contains("EE EE") || data.contains("ee ee"))
    {
        // 如果发现明显的十六进制字符串特征，去除空格并转换为 Hex
        data.replace(" ", "");
        realData = QByteArray::fromHex(data);
    }
    else
    {
        realData = data;
    }

    m_buffer.append(realData); // 追加到缓冲区，处理粘包/半包

    parseBuffer();
}

// 收包：在缓冲区内完成粘包/半包处理、校验并提取完整协议帧。
void TcpManager::parseBuffer()
{
    // 循环解析缓冲区中的所有完整数据帧
    while (static_cast<size_t>(m_buffer.size()) >= sizeof(ProtocolHeader))
    {
        // 1. 寻找包头 0xEEEEEEEE
        int startIndex = -1;
        for (int i = 0; i <= m_buffer.size() - 4; ++i)
        {
            uint32_t flag = 0;
            std::memcpy(&flag, m_buffer.constData() + i, sizeof(flag));
            if (flag == 0xEEEEEEEE)
            {
                startIndex = i;
                break;
            }
        }

        // 如果找不到包头，保留最后 3 个字节，避免下次收包时错过被截断的帧头。
        if (startIndex == -1)
        {
            const int keepBytes = qMin(3, m_buffer.size());
            if (keepBytes > 0)
            {
                m_buffer = m_buffer.right(keepBytes);
            }
            else
            {
                m_buffer.clear();
            }
            return;
        }

        // 丢弃包头之前的垃圾数据
        if (startIndex > 0)
        {
            m_buffer.remove(0, startIndex);
        }

        // 此时 m_buffer[0] 开始一定是 0xEEEEEEEE
        if (static_cast<size_t>(m_buffer.size()) < sizeof(ProtocolHeader))
        {
            return; // 长度不够一个头，继续等数据
        }

        ProtocolHeader headerValue;
        std::memcpy(&headerValue, m_buffer.constData(), sizeof(headerValue));
        const ProtocolHeader *header = &headerValue;
        uint32_t frameLen = header->length;

        // 检查数据包长度是否合理
        if (frameLen < sizeof(ProtocolHeader) + sizeof(ProtocolTail) || frameLen > 1024 * 1024)
        {
            m_buffer.remove(0, 4); // 丢弃这个错误的包头，继续找下一个
            continue;
        }

        // 判断缓冲区数据是否已经收全了这整个包
        if (static_cast<uint32_t>(m_buffer.size()) < frameLen)
        {
            return; // 半包，等待下一次 readAll
        }

        // 先在缓冲区上校验，确认完整帧有效后再从缓冲区移除，避免错误长度把后续好包一并删掉。
        ProtocolTail tailValue;
        std::memcpy(&tailValue, m_buffer.constData() + frameLen - sizeof(ProtocolTail), sizeof(tailValue));
        if (tailValue.endFlag != 0xAAAAAAAA)
        {
            m_buffer.remove(0, 1);
            continue; // 当前帧头无效，滑动 1 字节继续找下一个
        }

        // 校验 Checksum (实现对接收帧校验和的验证)
        uint8_t calculatedChecksum = 0;
        // Checksum从 startFlag 之后开始算（第4个字节），直到 checksum 字段之前
        for (uint32_t i = 4; i < frameLen - sizeof(ProtocolTail); ++i)
        {
            calculatedChecksum += static_cast<uint8_t>(m_buffer.at(static_cast<int>(i)));
        }

        if (tailValue.checksum != calculatedChecksum)
        {
            m_buffer.remove(0, 1);
            continue; // 当前帧头无效，滑动 1 字节继续找下一个
        }

        // 提取出一个完整的数据包
        QByteArray frameData = m_buffer.left(frameLen);
        m_buffer.remove(0, frameLen); // 从缓冲区移除已处理的包

        ProtocolHeader frameHeaderValue;
        std::memcpy(&frameHeaderValue, frameData.constData(), sizeof(frameHeaderValue));
        header = &frameHeaderValue;

        // 业务分发
        dispatchProtocol(header, frameData);
    }
}

// 主分发：把完整协议帧路由到各个模块子分发函数。
void TcpManager::dispatchProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    rememberReceivedFrame(header->dataType, frameData.size());

    if (dispatchDeviceBaseProtocol(header, frameData) || dispatchSystemProtocol(header, frameData) ||
        dispatchAngleCalibrationProtocol(header, frameData) || dispatchSpectrumSwitchProtocol(header, frameData) ||
        dispatchStrikeFrequencyProtocol(header, frameData) || dispatchPowerAmplifierProtocol(header, frameData) ||
        dispatchDirectionCalibrationProtocol(header, frameData) ||
        dispatchSignalSourceParamsProtocol(header, frameData) ||
        dispatchDataCollectionProtocol(header, frameData) ||
        dispatchFirmwareProtocol(header, frameData) ||
        dispatchModelLibraryProtocol(header, frameData) ||
        dispatchDeviceOpsProtocol(header, frameData) ||
        dispatchGpsProtocol(header, frameData) || dispatchDetectBandProtocol(header, frameData) ||
        dispatchModeSelectProtocol(header, frameData) || dispatchNetworkConfigProtocol(header, frameData))
    {
        return;
    }

    qDebug().noquote()
        << QStringLiteral("[TcpManager] 未处理收帧 DataType=%1 FrameHex=%2")
               .arg(header->dataType)
               .arg(QString::fromLatin1(frameData.toHex(' ').toUpper()));
}

void TcpManager::onSocketError(QTcpSocket::SocketError socketError)
{
    lastSocketError = socketError;
    lastSocketErrorText = tcpSocket->errorString();
    QString errorStr = tcpSocket->errorString();
    // #region debug-point D:socket-error
    qDebug().noquote()
        << QStringLiteral("[DEBUG-D] tcp_manager.cpp:onSocketError | socket错误 | error=%1 code=%2 errorString=%3 "
                          "socketState=%4 lastSentDataType=%5 lastSentFrameLength=%6 lastReceivedDataType=%7")
               .arg(socketErrorText(socketError),
                    QString::number(static_cast<int>(socketError)),
                    errorStr,
                    socketStateText(),
                    QString::number(lastSentDataType),
                    QString::number(lastSentFrameLength),
                    QString::number(lastReceivedDataType));
    // #endregion
    qDebug() << "[TcpManager] TCP 连接错误:" << socketErrorText(socketError) << "(" << static_cast<int>(socketError)
             << ")" << errorStr;

    if (!lastConnectedIp.isEmpty() && lastConnectedPort != 0 && tcpSocket->state() != QAbstractSocket::ConnectedState)
    {
        const int delayMs = nextReconnectDelayMs();
        reconnectTimer->start(delayMs);
    }

    emit errorOccurred(errorStr);
}

void TcpManager::rememberSentFrame(uint16_t dataType, int frameLength)
{
    lastSentDataType = dataType;
    lastSentFrameLength = frameLength;
    lastSentAt = QDateTime::currentDateTime();
}

void TcpManager::rememberReceivedFrame(uint16_t dataType, int frameLength)
{
    lastReceivedDataType = dataType;
    lastReceivedFrameLength = frameLength;
    lastReceivedAt = QDateTime::currentDateTime();
}

QString TcpManager::socketStateText() const
{
    switch (tcpSocket->state())
    {
    case QAbstractSocket::UnconnectedState:
        return "UnconnectedState";
    case QAbstractSocket::HostLookupState:
        return "HostLookupState";
    case QAbstractSocket::ConnectingState:
        return "ConnectingState";
    case QAbstractSocket::ConnectedState:
        return "ConnectedState";
    case QAbstractSocket::BoundState:
        return "BoundState";
    case QAbstractSocket::ClosingState:
        return "ClosingState";
    case QAbstractSocket::ListeningState:
        return "ListeningState";
    default:
        return "UnknownState";
    }
}

int TcpManager::nextReconnectDelayMs() const
{
    const int jitterMs = 300;
    return reconnectIntervalMs + (std::rand() % (jitterMs + 1));
}
