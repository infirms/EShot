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

    void listsOnlyRealMicrophoneSources()
    {
        const QString pactl = QStringLiteral(
            "12\talsa_output.headset.monitor\tPipeWire\ts32le 2ch 48000Hz\tRUNNING\n"
            "13\talsa_input.headset.mono-fallback\tPipeWire\ts16le 1ch 48000Hz\tRUNNING\n"
            "14\teasyeffects_source\tPipeWire\tfloat32le 2ch 48000Hz\tIDLE\n");
        QCOMPARE(linuxMicrophoneSources(pactl),
                 QStringList({QStringLiteral("alsa_input.headset.mono-fallback"),
                              QStringLiteral("easyeffects_source")}));
    }
};

QTEST_APPLESS_MAIN(LinuxRecordingSupportTests)
#include "LinuxRecordingSupportTests.moc"
