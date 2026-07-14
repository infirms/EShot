#include <QtTest>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>

#include "core/LinuxPortalScreenshot.h"
#include "core/LinuxScreenshotPolicy.h"

class LinuxScreenshotPolicyTests : public QObject
{
    Q_OBJECT

private slots:
    void detectsKdeWaylandSession()
    {
        QVERIFY(LinuxScreenshotPolicy::isKdeWaylandSession(
            QStringLiteral("KDE"), QString(), QStringLiteral("wayland")));
        QVERIFY(LinuxScreenshotPolicy::isKdeWaylandSession(
            QStringLiteral("KDE:GNOME"), QString(), QStringLiteral("Wayland")));
        QVERIFY(LinuxScreenshotPolicy::isKdeWaylandSession(
            QString(), QStringLiteral("plasma"), QStringLiteral("wayland")));
        QVERIFY(!LinuxScreenshotPolicy::isKdeWaylandSession(
            QStringLiteral("GNOME"), QStringLiteral("KDE"), QStringLiteral("wayland")));
        QVERIFY(!LinuxScreenshotPolicy::isKdeWaylandSession(
            QStringLiteral("KDE"), QString(), QStringLiteral("x11")));
    }

    void preparesKWinPermissionForAnyKdeWaylandExecutable()
    {
        const QString executable = QStringLiteral("/tmp/.mount_EShot.test/usr/bin/EShot");

        QVERIFY(LinuxScreenshotPolicy::shouldPrepareKWinPermission(
            QStringLiteral("KDE"), QString(), QStringLiteral("wayland"),
            QStringLiteral("/home/user/EShot.AppImage"), executable));
        QVERIFY(!LinuxScreenshotPolicy::shouldPrepareKWinPermission(
            QStringLiteral("GNOME"), QString(), QStringLiteral("wayland"),
            QStringLiteral("/home/user/EShot.AppImage"), executable));
        QVERIFY(!LinuxScreenshotPolicy::shouldPrepareKWinPermission(
            QStringLiteral("KDE"), QString(), QStringLiteral("x11"),
            QStringLiteral("/home/user/EShot.AppImage"), executable));
        QVERIFY(LinuxScreenshotPolicy::shouldPrepareKWinPermission(
            QStringLiteral("KDE"), QString(), QStringLiteral("wayland"),
            QString(), executable));
    }

    void buildsHiddenKWinPermissionDesktopEntry()
    {
        const QString entry = LinuxScreenshotPolicy::kwinPermissionDesktopEntry(
            QStringLiteral("/tmp/.mount_EShot.test/usr/bin/EShot"));

        QVERIFY(entry.contains(QStringLiteral("NoDisplay=true\n")));
        QVERIFY(entry.contains(QStringLiteral(
            "Exec=\"/tmp/.mount_EShot.test/usr/bin/EShot\"\n")));
        QVERIFY(entry.contains(QStringLiteral(
            "X-KDE-DBUS-Restricted-Interfaces=org.kde.KWin.ScreenShot2\n")));
    }

    void installsKWinPermissionDesktopEntryAtomically()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        QString desktopPath;
        QString error;
        QVERIFY(LinuxScreenshotPolicy::installKWinPermissionDesktopEntry(
            directory.path(), QStringLiteral("/tmp/.mount_EShot.test/usr/bin/EShot"),
            &desktopPath, &error));
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QCOMPARE(desktopPath, directory.filePath(
            QStringLiteral("io.github.benoks.EShot.KWinScreenshot.desktop")));

        QFile desktopFile(desktopPath);
        QVERIFY(desktopFile.open(QIODevice::ReadOnly | QIODevice::Text));
        QCOMPARE(QString::fromUtf8(desktopFile.readAll()),
                 LinuxScreenshotPolicy::kwinPermissionDesktopEntry(
                     QStringLiteral("/tmp/.mount_EShot.test/usr/bin/EShot")));
    }

    void buildsCursorFreeSpectacleArguments()
    {
        const QStringList arguments = LinuxScreenshotPolicy::spectacleWorkspaceArguments(
            QStringLiteral("/tmp/EShot capture.png"));

        QCOMPARE(arguments,
                 QStringList({QStringLiteral("--fullscreen"),
                              QStringLiteral("--background"),
                              QStringLiteral("--nonotify"),
                              QStringLiteral("--output"),
                              QStringLiteral("/tmp/EShot capture.png")}));
        QVERIFY(!arguments.contains(QStringLiteral("--pointer")));
    }

    void rejectsCursorBearingFallbackOnKdeWayland()
    {
        QVERIFY(!LinuxScreenshotPolicy::allowCursorBearingFallback(true));
        QVERIFY(LinuxScreenshotPolicy::allowCursorBearingFallback(false));
    }

    void presentsOverlayOnlyForUsableCapture()
    {
        QVERIFY(LinuxScreenshotPolicy::canPresentCapture(true));
        QVERIFY(!LinuxScreenshotPolicy::canPresentCapture(false));
    }

    void loadsAndCleansUpSpectacleWorkspaceCapture()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString sourcePath = directory.filePath(QStringLiteral("source.png"));
        QImage source(13, 7, QImage::Format_ARGB32_Premultiplied);
        source.fill(QColor(12, 34, 56));
        QVERIFY(source.save(sourcePath));

        const QString executablePath = directory.filePath(QStringLiteral("spectacle"));
        QFile executable(executablePath);
        QVERIFY(executable.open(QIODevice::WriteOnly | QIODevice::Text));
        executable.write(
            "#!/bin/sh\n"
            "output=\n"
            "while [ \"$#\" -gt 0 ]; do\n"
            "  if [ \"$1\" = \"--output\" ]; then shift; output=$1; fi\n"
            "  shift\n"
            "done\n"
            "/usr/bin/cp \"$ESHOT_TEST_SPECTACLE_IMAGE\" \"$output\"\n");
        executable.close();
        QVERIFY(executable.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                          | QFileDevice::ExeOwner));

        const QByteArray oldPath = qgetenv("PATH");
        qputenv("PATH", directory.path().toLocal8Bit());
        qputenv("ESHOT_TEST_SPECTACLE_IMAGE", sourcePath.toLocal8Bit());
        const QPixmap capture = LinuxPortalScreenshot::grabSpectacleWorkspace(nullptr, 5000);
        qputenv("PATH", oldPath);
        qunsetenv("ESHOT_TEST_SPECTACLE_IMAGE");

        QVERIFY(!capture.isNull());
        QCOMPARE(capture.size(), QSize(13, 7));
        const QStringList leftovers = QDir(directory.path()).entryList(
            {QStringLiteral("eshot-spectacle-*.png")}, QDir::Files);
        QVERIFY(leftovers.isEmpty());
    }
};

QTEST_MAIN(LinuxScreenshotPolicyTests)
#include "LinuxScreenshotPolicyTests.moc"
