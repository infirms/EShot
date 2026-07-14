#include <QtTest>

#include "recording/RecordingTimeline.h"

class RecordingTimelineTests : public QObject
{
    Q_OBJECT

private slots:
    void excludesPausedTimeFromElapsedDuration()
    {
        RecordingTimeline timeline;
        timeline.start(1000);
        QCOMPARE(timeline.activeElapsedMs(4000), qint64(3000));

        QVERIFY(timeline.pause(4000));
        QCOMPARE(timeline.activeElapsedMs(9000), qint64(3000));

        QVERIFY(timeline.resume(9000));
        QCOMPARE(timeline.activeElapsedMs(11000), qint64(5000));
    }

    void rejectsDuplicatePauseAndResumeRequests()
    {
        RecordingTimeline timeline;
        timeline.start(1000);

        QVERIFY(timeline.pause(1500));
        QVERIFY(!timeline.pause(1600));
        QVERIFY(timeline.resume(2000));
        QVERIFY(!timeline.resume(2100));
        QVERIFY(!timeline.isPaused());
    }
};

QTEST_APPLESS_MAIN(RecordingTimelineTests)
#include "RecordingTimelineTests.moc"
