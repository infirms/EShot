#include <QtTest/QTest>

#include "capture/CaptureGeometry.h"

class CaptureGeometryTests : public QObject
{
    Q_OBJECT

private slots:
    void mapsPhysicalSelectionOnScaledLeftMonitorToLogicalDisplayRect();
    void leavesPhysicalSelectionOnUnscaledPrimaryMonitorUnchanged();
    void mapsScaledWaylandScreenGeometryToPhysicalCoordinates();
    void cropsRightWaylandScreenFromVirtualPortalImage();
    void reportsPhysicalCapturePixelsOnScaledDisplay();
};

void CaptureGeometryTests::mapsPhysicalSelectionOnScaledLeftMonitorToLogicalDisplayRect()
{
    const QVector<CaptureMonitorGeometry> monitors = {
        {QRect(-1920, 0, 1536, 864), QRect(-1920, 0, 1920, 1080), 1.25},
        {QRect(0, 0, 1920, 1080), QRect(0, 0, 1920, 1080), 1.0}
    };

    const QRect physicalSelection(-1750, 150, 1000, 500);

    QCOMPARE(displayRectFromPhysical(physicalSelection, monitors),
             QRect(-1784, 120, 800, 400));
}

void CaptureGeometryTests::leavesPhysicalSelectionOnUnscaledPrimaryMonitorUnchanged()
{
    const QVector<CaptureMonitorGeometry> monitors = {
        {QRect(-1920, 0, 1536, 864), QRect(-1920, 0, 1920, 1080), 1.25},
        {QRect(0, 0, 1920, 1080), QRect(0, 0, 1920, 1080), 1.0}
    };

    const QRect physicalSelection(240, 180, 960, 540);

    QCOMPARE(displayRectFromPhysical(physicalSelection, monitors), physicalSelection);
}

void CaptureGeometryTests::mapsScaledWaylandScreenGeometryToPhysicalCoordinates()
{
    QCOMPARE(physicalRectFromLogical(QRect(-1920, 0, 1536, 864), 1.25),
             QRect(-2400, 0, 1920, 1080));
}

void CaptureGeometryTests::cropsRightWaylandScreenFromVirtualPortalImage()
{
    QCOMPARE(portalCropRect(QRect(1920, 0, 2560, 1440), QRect(0, 0, 4480, 1440)),
             QRect(1920, 0, 2560, 1440));
}

void CaptureGeometryTests::reportsPhysicalCapturePixelsOnScaledDisplay()
{
    QCOMPARE(snapshotRectFromLogical(QRect(0, 0, 2752, 1152),
                                     QSize(2752, 1152), QSize(3440, 1440), 1.25),
             QRect(0, 0, 3440, 1440));
}

QTEST_APPLESS_MAIN(CaptureGeometryTests)

#include "CaptureGeometryTests.moc"
