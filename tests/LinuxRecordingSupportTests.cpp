#include <QtTest>

#include "recording/LinuxRecordingSupport.h"

class LinuxRecordingSupportTests : public QObject {
    Q_OBJECT
private slots:
    void usesStablePipeWireSerialWhenAvailable()
    {
        QCOMPARE(pipeWireSourcePath(77, 0), QStringLiteral("path=77"));
        QCOMPARE(pipeWireSourcePath(77, 9001), QStringLiteral("target-object=9001"));
        QCOMPARE(pipeWireSourcePath(0, 9001), QStringLiteral("target-object=9001"));
        QVERIFY(pipeWireSourcePath(0, 0).isEmpty());
    }

    void mapsLogicalPortalGeometryToPhysicalPixels()
    {
        const PortalCropGeometry crop = portalCropGeometry(
            QRect(200, 100, 800, 400), QRect(100, 50, 400, 200),
            QPoint(0, 0), QSize(1920, 1080), QSize(800, 400));
        QVERIFY(crop.valid);
        QCOMPARE(crop.left, 200);
        QCOMPARE(crop.top, 100);
        QCOMPARE(crop.right, 3840 - 200 - 800);
        QCOMPARE(crop.bottom, 2160 - 100 - 400);
        QCOMPARE(crop.outputSize, QSize(800, 400));
    }

    void rejectsAStreamFromTheWrongMonitor()
    {
        const PortalCropGeometry crop = portalCropGeometry(
            QRect(2000, 100, 500, 300), QRect(2000, 100, 500, 300),
            QPoint(0, 0), QSize(1920, 1080), QSize(500, 300));
        QVERIFY(!crop.valid);
    }

    void forcesEncoderFriendlyEvenDimensions()
    {
        QCOMPARE(evenRecordingSize(QSize(801, 603)), QSize(800, 602));
        QCOMPARE(evenRecordingSize(QSize(8, 8)), QSize(8, 8));
    }

    void choosesAnInstalledAacEncoder()
    {
        QCOMPARE(preferredGstAacEncoder({QStringLiteral("avenc_aac"), QStringLiteral("faac")}),
                 QStringLiteral("avenc_aac"));
        QCOMPARE(preferredGstAacEncoder({QStringLiteral("voaacenc"), QStringLiteral("fdkaacenc")}),
                 QStringLiteral("fdkaacenc"));
        QVERIFY(preferredGstAacEncoder({}).isEmpty());
    }

    void exposesFriendlyMicrophoneLabelsAndIds()
    {
        const QString pactl = QStringLiteral(
            "Source #12\n\tName: alsa_output.headset.monitor\n\tDescription: Monitor of Headset\n\tMonitor of Sink: alsa_output.headset\n"
            "Source #13\n\tName: alsa_input.headset.mono-fallback\n\tDescription: G432 Gaming Headset Mono\n\tMonitor of Sink: n/a\n"
            "Source #14\n\tName: easyeffects_source\n\tDescription: Easy Effects Source\n\tMonitor of Sink: n/a\n");
        const auto devices = linuxMicrophoneDevices(pactl);
        QCOMPARE(devices.size(), 2);
        QCOMPARE(devices.at(0).first, QStringLiteral("G432 Gaming Headset Mono"));
        QCOMPARE(devices.at(0).second, QStringLiteral("alsa_input.headset.mono-fallback"));
        QCOMPARE(devices.at(1).first, QStringLiteral("Easy Effects Source"));
        QCOMPARE(devices.at(1).second, QStringLiteral("easyeffects_source"));
    }
};

QTEST_APPLESS_MAIN(LinuxRecordingSupportTests)
#include "LinuxRecordingSupportTests.moc"
