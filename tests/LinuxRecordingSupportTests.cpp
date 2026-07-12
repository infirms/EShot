#include <QtTest>

#include "recording/LinuxRecordingSupport.h"

class LinuxRecordingSupportTests : public QObject {
    Q_OBJECT
private slots:
    void usesPortalNodeIdAsPipeWirePath()
    {
        QCOMPARE(pipeWireSourcePath(77), QStringLiteral("path=77"));
        QVERIFY(pipeWireSourcePath(0).isEmpty());
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
