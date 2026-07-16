#include <QtTest>

#include "recording/RecordingSettingsPolicy.h"

class RecordingSettingsPolicyTests : public QObject
{
    Q_OBJECT

private slots:
    void exposesDistinctGifAndVideoFpsLimits()
    {
        QCOMPARE(gifRecordingFpsLimit(), 30);
        QCOMPARE(videoRecordingFpsLimit(), 60);
    }

    void preservesExplicitAudioChoices()
    {
        QVERIFY(!initialAudioEnabled(true, false, false, {}, RecordingAudioSource::Desktop));
        QVERIFY(initialAudioEnabled(true, true, false, {}, RecordingAudioSource::Microphone));
    }

    void migratesEveryLegacyAudioMode()
    {
        QVERIFY(!initialAudioEnabled(false, false, true, QStringLiteral("none"), RecordingAudioSource::Desktop));
        QVERIFY(initialAudioEnabled(false, false, true, QStringLiteral("desktop"), RecordingAudioSource::Desktop));
        QVERIFY(!initialAudioEnabled(false, false, true, QStringLiteral("desktop"), RecordingAudioSource::Microphone));
        QVERIFY(initialAudioEnabled(false, false, true, QStringLiteral("microphone"), RecordingAudioSource::Microphone));
        QVERIFY(initialAudioEnabled(false, false, true, QStringLiteral("both"), RecordingAudioSource::Desktop));
        QVERIFY(initialAudioEnabled(false, false, true, QStringLiteral("both"), RecordingAudioSource::Microphone));
    }

    void enablesBothAudioSourcesForNewUsers()
    {
        QVERIFY(initialAudioEnabled(false, false, false, {}, RecordingAudioSource::Desktop));
        QVERIFY(initialAudioEnabled(false, false, false, {}, RecordingAudioSource::Microphone));
    }
};

QTEST_APPLESS_MAIN(RecordingSettingsPolicyTests)
#include "RecordingSettingsPolicyTests.moc"
