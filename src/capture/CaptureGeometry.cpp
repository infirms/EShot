#include "CaptureGeometry.h"

#include <QtMath>

namespace {
const CaptureMonitorGeometry *monitorForPhysicalRect(
    const QRect &rect,
    const QVector<CaptureMonitorGeometry> &monitors)
{
    const QPoint center = rect.center();
    const CaptureMonitorGeometry *nearest = nullptr;
    qint64 nearestDistance = 0;

    for (const CaptureMonitorGeometry &monitor : monitors) {
        if (!monitor.logical.isValid() || !monitor.physical.isValid() || monitor.scale <= 0.0)
            continue;
        if (monitor.physical.contains(center))
            return &monitor;

        const QPoint candidate = monitor.physical.center();
        const qint64 dx = static_cast<qint64>(candidate.x()) - center.x();
        const qint64 dy = static_cast<qint64>(candidate.y()) - center.y();
        const qint64 distance = dx * dx + dy * dy;
        if (!nearest || distance < nearestDistance) {
            nearest = &monitor;
            nearestDistance = distance;
        }
    }

    return nearest;
}
}

QRect physicalRectFromLogical(const QRect &logicalRect, qreal scale)
{
    if (!logicalRect.isValid() || scale <= 0.0)
        return QRect();

    return QRect(qRound(logicalRect.x() * scale),
                 qRound(logicalRect.y() * scale),
                 qRound(logicalRect.width() * scale),
                 qRound(logicalRect.height() * scale));
}

QRect portalCropRect(const QRect &screenPhysicalRect, const QRect &virtualPhysicalRect)
{
    if (!screenPhysicalRect.isValid() || !virtualPhysicalRect.isValid())
        return QRect();

    return screenPhysicalRect.translated(-virtualPhysicalRect.topLeft());
}

QRect displayRectFromPhysical(const QRect &physicalRect,
                              const QVector<CaptureMonitorGeometry> &monitors)
{
    if (!physicalRect.isValid())
        return physicalRect;

    const CaptureMonitorGeometry *monitor = monitorForPhysicalRect(physicalRect, monitors);
    if (!monitor)
        return physicalRect;

    const auto logicalX = [monitor](int physicalX) {
        return monitor->logical.x()
            + qRound((physicalX - monitor->physical.x()) / monitor->scale);
    };
    const auto logicalY = [monitor](int physicalY) {
        return monitor->logical.y()
            + qRound((physicalY - monitor->physical.y()) / monitor->scale);
    };

    const int left = logicalX(physicalRect.x());
    const int top = logicalY(physicalRect.y());
    const int right = logicalX(physicalRect.x() + physicalRect.width());
    const int bottom = logicalY(physicalRect.y() + physicalRect.height());
    return QRect(left, top, qMax(0, right - left), qMax(0, bottom - top));
}
