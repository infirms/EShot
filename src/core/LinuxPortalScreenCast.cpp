#include "LinuxPortalScreenCast.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QGuiApplication>
#include <QTimer>
#include <QWidget>
#include <QWindow>

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QUuid>

#include <fcntl.h>
#include <unistd.h>
#endif

LinuxPortalScreenCast::LinuxPortalScreenCast(QObject *parent)
    : QObject(parent)
{
}

bool LinuxPortalScreenCast::isWaylandSession()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    return isWaylandSessionType(qEnvironmentVariable("XDG_SESSION_TYPE"),
                                QGuiApplication::platformName());
#else
    return false;
#endif
}

bool LinuxPortalScreenCast::isWaylandSessionType(const QString &sessionType,
                                                 const QString &platformName)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    return sessionType.compare(QStringLiteral("wayland"), Qt::CaseInsensitive) == 0
        || platformName.contains(QStringLiteral("wayland"), Qt::CaseInsensitive);
#else
    Q_UNUSED(sessionType);
    Q_UNUSED(platformName);
    return false;
#endif
}

bool LinuxPortalScreenCast::isAvailable()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return false;

    QDBusInterface screencast(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.ScreenCast"),
        bus);
    return screencast.isValid();
#else
    return false;
#endif
}

LinuxPortalScreenCast::Stream LinuxPortalScreenCast::selectStream(QWidget *parent, int timeoutMs)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    LinuxPortalScreenCast portal;
    return portal.select(parent, timeoutMs);
#else
    Q_UNUSED(parent);
    Q_UNUSED(timeoutMs);
    return {};
#endif
}

LinuxPortalScreenCast::Stream LinuxPortalScreenCast::select(QWidget *parent, int timeoutMs)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return {};

    QDBusInterface screencast(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.ScreenCast"),
        bus);
    if (!screencast.isValid())
        return {};

    const QString token = QStringLiteral("eshot_screencast_%1")
        .arg(QUuid::createUuid().toString(QUuid::Id128));
    QVariantMap sessionOptions;
    sessionOptions.insert(QStringLiteral("handle_token"), token + QStringLiteral("_create"));
    sessionOptions.insert(QStringLiteral("session_handle_token"), token + QStringLiteral("_session"));

    QDBusReply<QDBusObjectPath> createReply = screencast.call(QStringLiteral("CreateSession"), sessionOptions);
    if (!createReply.isValid() || !waitForRequest(createReply.value().path(), timeoutMs))
        return {};
    if (m_response != 0)
        return {};

    const QString sessionHandle = objectPathString(m_results.value(QStringLiteral("session_handle")));
    if (sessionHandle.isEmpty())
        return {};

    QVariantMap sourceOptions;
    sourceOptions.insert(QStringLiteral("handle_token"), token + QStringLiteral("_sources"));
    sourceOptions.insert(QStringLiteral("types"), 1u);
    sourceOptions.insert(QStringLiteral("multiple"), false);
    sourceOptions.insert(QStringLiteral("cursor_mode"), 2u);
    QDBusReply<QDBusObjectPath> sourcesReply = screencast.call(
        QStringLiteral("SelectSources"),
        QDBusObjectPath(sessionHandle),
        sourceOptions);
    if (!sourcesReply.isValid() || !waitForRequest(sourcesReply.value().path(), timeoutMs)) {
        closeSession(sessionHandle);
        return {};
    }
    if (m_response != 0) {
        closeSession(sessionHandle);
        return {};
    }

    const bool isX11 = QGuiApplication::platformName().contains(QStringLiteral("xcb"), Qt::CaseInsensitive);
    const QString parentWindow = isX11 && parent && parent->windowHandle()
        ? QStringLiteral("x11:%1").arg(QString::number(parent->windowHandle()->winId(), 16))
        : QString();

    QVariantMap startOptions;
    startOptions.insert(QStringLiteral("handle_token"), token + QStringLiteral("_start"));
    QDBusReply<QDBusObjectPath> startReply = screencast.call(
        QStringLiteral("Start"),
        QDBusObjectPath(sessionHandle),
        parentWindow,
        startOptions);
    if (!startReply.isValid() || !waitForRequest(startReply.value().path(), timeoutMs)) {
        closeSession(sessionHandle);
        return {};
    }
    if (m_response != 0) {
        closeSession(sessionHandle);
        return {};
    }

    Stream stream = firstStreamFromResults(m_results);
    stream.sessionHandle = sessionHandle;
    stream.pipewireFd = openPipeWireRemote(sessionHandle);
    return stream;
#else
    Q_UNUSED(parent);
    Q_UNUSED(timeoutMs);
    return {};
#endif
}

void LinuxPortalScreenCast::closeSession(const QString &sessionHandle)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    if (sessionHandle.isEmpty()) return;
    QDBusInterface session(QStringLiteral("org.freedesktop.portal.Desktop"), sessionHandle,
                           QStringLiteral("org.freedesktop.portal.Session"),
                           QDBusConnection::sessionBus());
    if (session.isValid()) session.call(QStringLiteral("Close"));
#else
    Q_UNUSED(sessionHandle);
#endif
}

bool LinuxPortalScreenCast::waitForRequest(const QString &requestPath, int timeoutMs)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    if (requestPath.isEmpty())
        return false;

    QDBusConnection bus = QDBusConnection::sessionBus();
    m_response = 2;
    m_results.clear();
    m_finished = false;

    const bool connected = bus.connect(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        requestPath,
        QStringLiteral("org.freedesktop.portal.Request"),
        QStringLiteral("Response"),
        this,
        SLOT(onPortalResponse(uint,QVariantMap)));
    if (!connected)
        return false;

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(this, &LinuxPortalScreenCast::destroyed, &loop, &QEventLoop::quit);
    m_loop = &loop;
    timeout.start(timeoutMs);
    loop.exec();
    m_loop = nullptr;

    bus.disconnect(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        requestPath,
        QStringLiteral("org.freedesktop.portal.Request"),
        QStringLiteral("Response"),
        this,
        SLOT(onPortalResponse(uint,QVariantMap)));

    return m_finished;
#else
    Q_UNUSED(requestPath);
    Q_UNUSED(timeoutMs);
    return false;
#endif
}

QSharedPointer<int> LinuxPortalScreenCast::openPipeWireRemote(const QString &sessionHandle) const
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    if (sessionHandle.isEmpty())
        return {};

    QDBusInterface screencast(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.ScreenCast"),
        QDBusConnection::sessionBus());
    if (!screencast.isValid())
        return {};

    QDBusReply<QDBusUnixFileDescriptor> reply = screencast.call(
        QStringLiteral("OpenPipeWireRemote"),
        QDBusObjectPath(sessionHandle),
        QVariantMap());
    if (!reply.isValid() || !reply.value().isValid())
        return {};

    const int duplicated = fcntl(reply.value().fileDescriptor(), F_DUPFD_CLOEXEC, 3);
    if (duplicated < 0)
        return {};

    return QSharedPointer<int>(
        new int(duplicated),
        [](int *fd) {
            if (fd) {
                if (*fd >= 0)
                    close(*fd);
                delete fd;
            }
        });
#else
    Q_UNUSED(sessionHandle);
    return {};
#endif
}

QString LinuxPortalScreenCast::objectPathString(const QVariant &value) const
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    const QDBusObjectPath path = qvariant_cast<QDBusObjectPath>(value);
    if (!path.path().isEmpty())
        return path.path();
#endif
    return value.toString();
}

LinuxPortalScreenCast::Stream LinuxPortalScreenCast::firstStreamFromResults(const QVariantMap &results) const
{
    Stream stream;
    const QVariant streamsValue = results.value(QStringLiteral("streams"));
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    const QDBusArgument argument = streamsValue.value<QDBusArgument>();
    if (argument.currentType() == QDBusArgument::ArrayType) {
        argument.beginArray();
        if (!argument.atEnd()) {
            QVariantMap properties;
            uint nodeId = 0;
            argument.beginStructure();
            argument >> nodeId >> properties;
            argument.endStructure();

            stream.nodeId = nodeId;
            stream.properties = properties;
        }
        argument.endArray();
    }
#endif
    if (!stream.isValid()) {
        const QVariantList streams = streamsValue.toList();
        if (!streams.isEmpty()) {
            const QVariant first = streams.first();
            const QVariantList tuple = first.toList();
            if (tuple.size() >= 2) {
                stream.nodeId = tuple.at(0).toUInt();
                stream.properties = tuple.at(1).toMap();
            }
        }
    }

    stream.pipewireSerial = stream.properties.value(QStringLiteral("pipewire-serial")).toULongLong();
    const QVariant sizeValue = stream.properties.value(QStringLiteral("size"));
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    const QDBusArgument sizeArgument = sizeValue.value<QDBusArgument>();
    if (sizeArgument.currentType() == QDBusArgument::StructureType) {
        int width = 0;
        int height = 0;
        sizeArgument.beginStructure();
        sizeArgument >> width >> height;
        sizeArgument.endStructure();
        if (width > 0 && height > 0)
            stream.size = QSize(width, height);
    }
#endif
    if (!stream.size.isValid()) {
        const QVariantList sizeList = sizeValue.toList();
        if (sizeList.size() >= 2)
            stream.size = QSize(sizeList.at(0).toInt(), sizeList.at(1).toInt());
    }

    const QVariant positionValue = stream.properties.value(QStringLiteral("position"));
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    const QDBusArgument positionArgument = positionValue.value<QDBusArgument>();
    if (positionArgument.currentType() == QDBusArgument::StructureType) {
        int x = 0;
        int y = 0;
        positionArgument.beginStructure();
        positionArgument >> x >> y;
        positionArgument.endStructure();
        stream.position = QPoint(x, y);
    }
#endif
    if (stream.position.isNull()) {
        const QVariantList positionList = positionValue.toList();
        if (positionList.size() >= 2)
            stream.position = QPoint(positionList.at(0).toInt(), positionList.at(1).toInt());
    }
    return stream;
}

void LinuxPortalScreenCast::onPortalResponse(uint response, const QVariantMap &results)
{
    m_response = response;
    m_results = results;
    m_finished = true;
    if (m_loop)
        m_loop->quit();
}
