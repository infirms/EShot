#include <QtTest>

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QToolButton>

#include "core/TranslationManager.h"
#include "recording/RecordingIndicator.h"

class RecordingIndicatorTests : public QObject
{
    Q_OBJECT

private slots:
    void createsARealRecordingControlBar()
    {
        TranslationManager::setLanguage(TranslationManager::Turkish, false);
        RecordingIndicator indicator(QRect(120, 120, 640, 360), nullptr, 2, true);

        auto *bar = indicator.findChild<QFrame *>(QStringLiteral("recordingControlBar"));
        auto *status = indicator.findChild<QLabel *>(QStringLiteral("recordingStatusLabel"));
        auto *pause = indicator.findChild<QToolButton *>(QStringLiteral("recordingPauseButton"));
        auto *stop = indicator.findChild<QPushButton *>(QStringLiteral("recordingStopButton"));
        auto *cancel = indicator.findChild<QToolButton *>(QStringLiteral("recordingCancelButton"));
        auto *details = indicator.findChild<QToolButton *>(QStringLiteral("recordingDetailsButton"));

        QVERIFY(bar);
        QVERIFY(status);
        QVERIFY(pause);
        QVERIFY(stop);
        QVERIFY(cancel);
        QVERIFY(details);
        QCOMPARE(bar->height(), 42);
        QCOMPARE(indicator.property("recordingBorderWidth").toInt(), 2);
        QVERIFY(!pause->icon().isNull());
        QVERIFY(!stop->icon().isNull());
        QVERIFY(!cancel->icon().isNull());
        QVERIFY(!details->icon().isNull());

        const QString style = bar->styleSheet();
        QVERIFY(style.contains(QStringLiteral("background-color: #2d2d2d")));
        QVERIFY(style.contains(QStringLiteral("border: 1px solid #404040")));
        QVERIFY(style.contains(QStringLiteral("background-color: #3a3a3a")));
        QVERIFY(style.contains(QStringLiteral("border: 1px solid #505050")));
        QCOMPARE(stop->text(), QStringLiteral("Durdur"));
        QCOMPARE(details->toolTip(), QString::fromUtf8("Ayrıntılar"));
        TranslationManager::setLanguage(TranslationManager::English, false);
    }

    void buttonsEmitTheirDedicatedSignals()
    {
        RecordingIndicator indicator(QRect(120, 120, 640, 360), nullptr, 2, true);
        auto *pause = indicator.findChild<QToolButton *>(QStringLiteral("recordingPauseButton"));
        auto *stop = indicator.findChild<QPushButton *>(QStringLiteral("recordingStopButton"));
        auto *cancel = indicator.findChild<QToolButton *>(QStringLiteral("recordingCancelButton"));
        QVERIFY(pause);
        QVERIFY(stop);
        QVERIFY(cancel);

        QSignalSpy pauseSpy(&indicator, &RecordingIndicator::pauseRequested);
        QSignalSpy stopSpy(&indicator, &RecordingIndicator::stopRequested);
        QSignalSpy cancelSpy(&indicator, &RecordingIndicator::cancelRequested);
        QTest::mouseClick(pause, Qt::LeftButton);
        QTest::mouseClick(stop, Qt::LeftButton);
        QTest::mouseClick(cancel, Qt::LeftButton);

        QCOMPARE(pauseSpy.count(), 1);
        QCOMPARE(stopSpy.count(), 1);
        QCOMPARE(cancelSpy.count(), 1);
    }

    void pauseButtonChangesToResumeState()
    {
        RecordingIndicator indicator(QRect(120, 120, 640, 360), nullptr, 2, true);
        auto *pause = indicator.findChild<QToolButton *>(QStringLiteral("recordingPauseButton"));
        QVERIFY(pause);

        indicator.setPaused(true);
        QCOMPARE(pause->property("recordingState").toString(), QStringLiteral("resume"));

        QSignalSpy resumeSpy(&indicator, &RecordingIndicator::resumeRequested);
        QTest::mouseClick(pause, Qt::LeftButton);
        QCOMPARE(resumeSpy.count(), 1);
    }

    void pausesCaptureBeforePresentingInsideControls()
    {
        QScreen *screen = QGuiApplication::primaryScreen();
        QVERIFY(screen);
        RecordingIndicator indicator(screen->geometry(), nullptr, 2, true,
                                     RecordingIndicatorMode::Video);
        QVERIFY(indicator.requiresCaptureSafePresentation());

        QSignalSpy pauseSpy(&indicator, &RecordingIndicator::pauseRequested);
        indicator.startCaptureSafePresentation();
        QCOMPARE(pauseSpy.count(), 1);
        QVERIFY(indicator.property("captureSafeInside").toBool());
        auto *bar = indicator.findChild<QFrame *>(QStringLiteral("recordingControlBar"));
        QVERIFY(bar);
        QVERIFY(!bar->isVisible());

        indicator.setPaused(true);
        QTRY_VERIFY_WITH_TIMEOUT(bar->isVisible(), 200);
    }

};

QTEST_MAIN(RecordingIndicatorTests)
#include "RecordingIndicatorTests.moc"
