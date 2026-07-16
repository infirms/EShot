#include <QtTest>

#include "capture/WindowSnapPolicy.h"

class WindowSnapPolicyTests : public QObject
{
    Q_OBJECT

private slots:
    void choosesFirstContainingWindowInZOrder()
    {
        const QVector<QRect> windows = {
            QRect(200, 150, 600, 450),
            QRect(100, 100, 1000, 700)
        };

        QCOMPARE(topmostWindowAt(windows, QPoint(300, 250), QRect(0, 0, 1920, 1080)),
                 windows.first());
    }

    void clipsPartiallyVisibleWindowsAndRejectsInvalidCandidates()
    {
        const QVector<QRect> windows = {
            QRect(),
            QRect(-80, 40, 300, 200)
        };

        QCOMPARE(topmostWindowAt(windows, QPoint(40, 80), QRect(0, 0, 1920, 1080)),
                 QRect(0, 40, 220, 200));
        QVERIFY(topmostWindowAt(windows, QPoint(500, 500), QRect(0, 0, 1920, 1080)).isEmpty());
    }

    void distinguishesClickFromManualDrag()
    {
        QVERIFY(isWindowSnapClick(QPoint(100, 100), QPoint(103, 102), 10));
        QVERIFY(!isWindowSnapClick(QPoint(100, 100), QPoint(112, 100), 10));
    }
};

QTEST_APPLESS_MAIN(WindowSnapPolicyTests)
#include "WindowSnapPolicyTests.moc"
