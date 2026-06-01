#include "qt_time_helper_server.h"
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QtTimeHelperServer server;
    QString errorMessage;
    if (!server.start(&errorMessage))
    {
        qCritical() << "[qt_time_helper] 启动失败:" << errorMessage;
        return 1;
    }

    qInfo() << "[qt_time_helper] 服务已启动";
    return app.exec();
}
