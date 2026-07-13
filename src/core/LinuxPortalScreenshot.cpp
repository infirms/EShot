#include "LinuxPortalScreenshot.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QImage>
#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>
#include <QWidget>
#include <QWindow>

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QUuid>
#include <fcntl.h>
#include <unistd.h>
#endif

LinuxPortalScreenshot::LinuxPortalScreenshot(QObject *parent)
    : QObject(parent)
{
}

QPixmap LinuxPortalScreenshot::grab(QWidget *parent, int timeoutMs)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    LinuxPortalScreenshot portal;
    return portal.waitForScreenshot(parent, timeoutMs);
#else
    Q_UNUSED(parent);
    Q_UNUSED(timeoutMs);
    return QPixmap();
#endif
}

QPixmap LinuxPortalScreenshot::grabScreen(QScreen *screen, QWidget *parent, int timeoutMs)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    Q_UNUSED(parent);
    if (!screen)
        return QPixmap();

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return QPixmap();

    int pipeFds[2] = {-1, -1};
    if (pipe2(pipeFds, O_CLOEXEC) != 0)
        return QPixmap();

    QDBusMessage message = QDBusMessage::createMethodCall(
        QStringLiteral("org.kde.KWin.ScreenShot2"),
        QStringLiteral("/org/kde/KWin/ScreenShot2"),
        QStringLiteral("org.kde.KWin.ScreenShot2"),
        QStringLiteral("CaptureScreen"));
    QVariantMap options;
    options.insert(QStringLiteral("native-resolution"), true);
    options.insert(QStringLiteral("include-cursor"), false);
    message.setArguments(QList<QVariant>{QVariant(screen->name()), QVariant(options),
                          QVariant::fromValue(QDBusUnixFileDescriptor(pipeFds[1]))});
    close(pipeFds[1]);

    QDBusPendingCallWatcher watcher(bus.asyncCall(message, timeoutMs));
    QEventLoop loop;
    QPixmap result;
    QObject::connect(&watcher, &QDBusPendingCallWatcher::finished, &loop, [&]() {
        QDBusPendingReply<QVariantMap> reply = watcher;
        if (reply.isError()) {
            close(pipeFds[0]);
            loop.quit();
            return;
        }

        const QVariantMap metadata = reply.value();
        const int width = metadata.value(QStringLiteral("width")).toInt();
        const int height = metadata.value(QStringLiteral("height")).toInt();
        const int format = metadata.value(QStringLiteral("format")).toInt();
        const qreal scale = metadata.value(QStringLiteral("scale"), 1.0).toReal();
        if (metadata.value(QStringLiteral("type")).toString() != QStringLiteral("raw")
            || width <= 0 || height <= 0
            || format <= QImage::Format_Invalid || format >= QImage::NImageFormats) {
            close(pipeFds[0]);
            loop.quit();
            return;
        }

        QImage image(width, height, static_cast<QImage::Format>(format));
        image.setDevicePixelRatio(scale > 0.0 ? scale : 1.0);
        QFile file;
        if (file.open(pipeFds[0], QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)
            && file.read(reinterpret_cast<char *>(image.bits()), image.sizeInBytes())
                   == image.sizeInBytes()) {
            result = QPixmap::fromImage(image);
        }
        loop.quit();
    });
    loop.exec();
    return result;
#else
    Q_UNUSED(screen);
    Q_UNUSED(parent);
    Q_UNUSED(timeoutMs);
    return QPixmap();
#endif
}

QPixmap LinuxPortalScreenshot::waitForScreenshot(QWidget *parent, int timeoutMs)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return QPixmap();

    QDBusInterface screenshot(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Screenshot"),
        bus);
    if (!screenshot.isValid())
        return QPixmap();

    const QString token = QStringLiteral("eshot_%1")
        .arg(QUuid::createUuid().toString(QUuid::Id128));
    QVariantMap options;
    options.insert(QStringLiteral("handle_token"), token);
    options.insert(QStringLiteral("interactive"), false);
    options.insert(QStringLiteral("modal"), false);

    const bool isX11 = QGuiApplication::platformName().contains(QStringLiteral("xcb"), Qt::CaseInsensitive);
    const QString parentWindow = isX11 && parent && parent->windowHandle()
        ? QStringLiteral("x11:%1").arg(QString::number(parent->windowHandle()->winId(), 16))
        : QString();
    QDBusReply<QDBusObjectPath> reply = screenshot.call(
        QStringLiteral("Screenshot"),
        parentWindow,
        options);
    if (!reply.isValid())
        return QPixmap();

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    m_loop = &loop;
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(this, &LinuxPortalScreenshot::destroyed, &loop, &QEventLoop::quit);

    const QString requestPath = reply.value().path();
    const bool connected = bus.connect(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        requestPath,
        QStringLiteral("org.freedesktop.portal.Request"),
        QStringLiteral("Response"),
        this,
        SLOT(onPortalResponse(uint,QVariantMap)));
    if (!connected)
        return QPixmap();

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

    return m_pixmap;
#else
    Q_UNUSED(parent);
    Q_UNUSED(timeoutMs);
    return QPixmap();
#endif
}

void LinuxPortalScreenshot::onPortalResponse(uint response, const QVariantMap &results)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    m_finished = true;
    if (m_loop)
        m_loop->quit();
    if (response != 0)
        return;

    const QUrl uri = results.value(QStringLiteral("uri")).toUrl();
    if (!uri.isLocalFile())
        return;

    const QString path = uri.toLocalFile();
    if (!QFile::exists(path))
        return;

    QPixmap pixmap(path);
    if (!pixmap.isNull()) {
        pixmap.setDevicePixelRatio(1.0);
        m_pixmap = pixmap;
    }
#else
    Q_UNUSED(response);
    Q_UNUSED(results);
#endif
}
