#ifndef LOCAL_TIME_SERVICE_CLIENT_H
#define LOCAL_TIME_SERVICE_CLIENT_H

#include <QDateTime>
#include <QObject>

class QJsonObject;

struct LocalTimeServiceResult
{
    bool success = false;
    QString message;
};

class LocalTimeServiceClient : public QObject
{
    Q_OBJECT

  public:
    explicit LocalTimeServiceClient(QObject *parent = nullptr);

    LocalTimeServiceResult checkAvailability() const;
    LocalTimeServiceResult setSystemTime(const QDateTime &dateTime, const QString &timezoneId) const;
    static QString serverName();

  private:
    LocalTimeServiceResult sendRequest(const QJsonObject &request) const;
};

#endif // LOCAL_TIME_SERVICE_CLIENT_H
