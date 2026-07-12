#pragma once

#include <QRect>
#include <QVector>

struct CaptureMonitorGeometry
{
    QRect logical;
    QRect physical;
    qreal scale = 1.0;
};

QRect physicalRectFromLogical(const QRect &logicalRect, qreal scale);
QRect portalCropRect(const QRect &screenPhysicalRect, const QRect &virtualPhysicalRect);

QRect displayRectFromPhysical(const QRect &physicalRect,
                              const QVector<CaptureMonitorGeometry> &monitors);
