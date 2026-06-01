#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <QString>
#include <QtGlobal>

struct ConnectionConfig
{
    QString host;
    quint16 port = 0;
    int reconnectIntervalMs = 1000;
};

class AppConfig
{
  public:
    static ConnectionConfig defaultConnectionConfig()
    {
        ConnectionConfig config;
        config.host = "10.0.76.189";
        config.port = 5555;
        config.reconnectIntervalMs = 1000;
        return config;
    }
};

#endif // APP_CONFIG_H
