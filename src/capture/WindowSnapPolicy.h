#pragma once

#include <QPoint>
#include <QRect>
#include <QVector>

QRect topmostWindowAt(const QVector<QRect> &windows,
                      const QPoint &point,
                      const QRect &overlayBounds);

bool isWindowSnapClick(const QPoint &pressPosition,
                       const QPoint &releasePosition,
                       int dragThreshold);
