#ifndef QT_TIME_HELPER_SERVER_H
#define QT_TIME_HELPER_SERVER_H

#include <QObject>

class QLocalServer;
class QLocalSocket;

class QtTimeHelperServer : public QObject
{
    Q_OBJECT

  public:
    explicit QtTimeHelperServer(QObject *parent = nullptr);
    bool start(QString *errorMessage);

  private slots:
    void onNewConnection();
    void onSocketReadyRead();
    void onSocketDisconnected();

  private:
    void writeResponse(QLocalSocket *socket, bool success, const QString &message);
    bool isPeerAllowed(QLocalSocket *socket, QString *errorMessage) const;
    bool processRequest(QLocalSocket *socket, const QByteArray &requestBytes);
    bool handlePing(QLocalSocket *socket);
    bool handleSetSystemTime(QLocalSocket *socket, const QString &dateTimeText, const QString &timezoneId);

    QLocalServer *server;
};

#endif // QT_TIME_HELPER_SERVER_H
