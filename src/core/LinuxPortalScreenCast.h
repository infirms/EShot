#ifndef LINUXPORTALSCREENCAST_H
#define LINUXPORTALSCREENCAST_H

#include <QObject>
#include <QPoint>
#include <QSharedPointer>
#include <QSize>
#include <QVariantMap>

class QWidget;
class QEventLoop;

class LinuxPortalScreenCast : public QObject {
    Q_OBJECT

public:
    struct Stream {
        uint nodeId = 0;
        quint64 pipewireSerial = 0;
        QSharedPointer<int> pipewireFd;
        QPoint position;
        QSize size;
        QVariantMap properties;
        QString sessionHandle;
        bool isValid() const { return nodeId != 0 || pipewireSerial != 0; }
        int remoteFd() const { return pipewireFd ? *pipewireFd : -1; }
    };

    explicit LinuxPortalScreenCast(QObject *parent = nullptr);

    static bool isWaylandSession();
    static bool isWaylandSessionType(const QString &sessionType, const QString &platformName);
    static bool isAvailable();
    static Stream selectStream(QWidget *parent = nullptr, int timeoutMs = 120000);
    static void closeSession(const QString &sessionHandle);

private slots:
    void onPortalResponse(uint response, const QVariantMap &results);

private:
    Stream select(QWidget *parent, int timeoutMs);
    bool waitForRequest(const QString &requestPath, int timeoutMs);
    QSharedPointer<int> openPipeWireRemote(const QString &sessionHandle) const;
    QString objectPathString(const QVariant &value) const;
    Stream firstStreamFromResults(const QVariantMap &results) const;

    uint m_response = 2;
    QVariantMap m_results;
    bool m_finished = false;
    QEventLoop *m_loop = nullptr;
};

#endif
