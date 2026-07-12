#include <QtTest>
#include "capture/CaptureInteractionPolicy.h"

class CaptureInteractionPolicyTests : public QObject {
    Q_OBJECT
private slots:
    void resizeHandleReleasesActiveTool()
    {
        QVERIFY(shouldReleaseToolForResize(true, 3, 0));
        QVERIFY(!shouldReleaseToolForResize(false, 3, 0));
        QVERIFY(!shouldReleaseToolForResize(true, 0, 0));
    }

    void toolPersistenceDefaultsToNone()
    {
        QCOMPARE(initialAnnotationTool(false, 5, 0), 0);
        QCOMPARE(initialAnnotationTool(true, 5, 0), 5);
    }
};

QTEST_APPLESS_MAIN(CaptureInteractionPolicyTests)
#include "CaptureInteractionPolicyTests.moc"
