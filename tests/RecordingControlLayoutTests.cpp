#include <QtTest>

#include "recording/RecordingControlLayout.h"

class RecordingControlLayoutTests : public QObject
{
    Q_OBJECT

private slots:
    void placesControlsBelowWhenThereIsRoom()
    {
        const QRect screen(0, 0, 1920, 1080);
        const QRect capture(500, 250, 800, 450);

        const RecordingControlLayout layout = recordingControlLayout(
            capture, screen, QSize(310, 42), QSize(214, 42));

        QCOMPARE(layout.placement, RecordingControlPlacement::Below);
        QCOMPARE(layout.controlRect.top(), capture.bottom() + 9);
        QCOMPARE(layout.controlRect.center().x(), capture.center().x());
        QVERIFY(!layout.compact);
    }

    void movesControlsAboveWhenBottomSpaceIsUnavailable()
    {
        const QRect screen(0, 0, 1920, 1080);
        const QRect capture(560, 620, 800, 450);

        const RecordingControlLayout layout = recordingControlLayout(
            capture, screen, QSize(310, 42), QSize(214, 42));

        QCOMPARE(layout.placement, RecordingControlPlacement::Above);
        QCOMPARE(layout.controlRect.bottom(), capture.top() - 9);
    }

    void placesControlsInsideForFullscreenCapture()
    {
        const QRect screen(0, 0, 1920, 1080);

        const RecordingControlLayout layout = recordingControlLayout(
            screen, screen, QSize(310, 42), QSize(214, 42));

        QCOMPARE(layout.placement, RecordingControlPlacement::Inside);
        QCOMPARE(layout.controlRect.bottom(), screen.bottom() - 12);
        QCOMPARE(layout.controlRect.center().x(), screen.center().x());
    }

    void usesCompactControlsOnNarrowScreens()
    {
        const QRect screen(0, 0, 250, 600);
        const QRect capture(10, 100, 230, 320);

        const RecordingControlLayout layout = recordingControlLayout(
            capture, screen, QSize(310, 42), QSize(214, 42));

        QVERIFY(layout.compact);
        QCOMPARE(layout.controlRect.width(), 214);
        QVERIFY(screen.adjusted(8, 0, -8, 0).contains(layout.controlRect.topLeft()));
        QVERIFY(screen.adjusted(8, 0, -8, 0).contains(layout.controlRect.topRight()));
    }

    void clampsControlsToTheScreenHorizontally()
    {
        const QRect screen(-1920, 0, 1920, 1080);
        const QRect capture(-1910, 220, 300, 400);

        const RecordingControlLayout layout = recordingControlLayout(
            capture, screen, QSize(310, 42), QSize(214, 42));

        QCOMPARE(layout.controlRect.left(), screen.left() + 8);
        QVERIFY(layout.controlRect.right() <= screen.right() - 8);
    }

    void hidesInsideControlsWhenThePlatformCannotExcludeThemFromCapture()
    {
        QCOMPARE(recordingOverlayVisibilityPolicy(RecordingControlPlacement::Inside, false),
                 RecordingOverlayVisibility::RevealWhilePaused);
        QCOMPARE(recordingOverlayVisibilityPolicy(RecordingControlPlacement::Inside, true),
                 RecordingOverlayVisibility::AlwaysVisible);
        QCOMPARE(recordingOverlayVisibilityPolicy(RecordingControlPlacement::Below, false),
                 RecordingOverlayVisibility::AlwaysVisible);
        QCOMPARE(recordingOverlayVisibilityPolicy(RecordingControlPlacement::Above, false),
                 RecordingOverlayVisibility::AlwaysVisible);
    }
};

QTEST_APPLESS_MAIN(RecordingControlLayoutTests)
#include "RecordingControlLayoutTests.moc"
