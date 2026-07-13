#include <QtTest>

#include "core/LinuxAutoStartPolicy.h"

#include <QFile>
#include <QTemporaryDir>

class LinuxAutoStartPolicyTests : public QObject
{
    Q_OBJECT

private slots:
    void prefersExistingOuterAppImage();
    void fallsBackWhenAppImageIsEmpty();
    void fallsBackWhenAppImageDoesNotExist();
};

void LinuxAutoStartPolicyTests::prefersExistingOuterAppImage()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString appImagePath = tempDir.filePath(QStringLiteral("EShot.AppImage"));
    QFile appImage(appImagePath);
    QVERIFY(appImage.open(QIODevice::WriteOnly));
    appImage.close();

    QCOMPARE(LinuxAutoStartPolicy::executablePath(
                 appImagePath, QStringLiteral("/tmp/.mount_EShot/usr/bin/EShot")),
             appImagePath);
}

void LinuxAutoStartPolicyTests::fallsBackWhenAppImageIsEmpty()
{
    const QString executable = QStringLiteral("/opt/eshot/bin/EShot");
    QCOMPARE(LinuxAutoStartPolicy::executablePath(QString(), executable), executable);
}

void LinuxAutoStartPolicyTests::fallsBackWhenAppImageDoesNotExist()
{
    const QString executable = QStringLiteral("/opt/eshot/bin/EShot");
    QCOMPARE(LinuxAutoStartPolicy::executablePath(
                 QStringLiteral("/missing/EShot.AppImage"), executable),
             executable);
}

QTEST_MAIN(LinuxAutoStartPolicyTests)
#include "LinuxAutoStartPolicyTests.moc"
