#pragma once

#include <QPoint>
#include <QRect>
#include <QVector>

enum class CaptureSelectionMode {
    FreeRegion,
    Window
};

QRect topmostWindowAt(const QVector<QRect> &windows,
                      const QPoint &point,
                      const QRect &overlayBounds);

bool isWindowSnapClick(const QPoint &pressPosition,
                       const QPoint &releasePosition,
                       int dragThreshold);

QRect windowSnapTargetForMode(CaptureSelectionMode mode,
                              const QVector<QRect> &windows,
                              const QPoint &point,
                              const QRect &overlayBounds);

bool allowsManualSelection(CaptureSelectionMode mode);

int windowSnapAnimationDurationMs();
QRect windowSnapTransitionRect(const QRect &from, const QRect &to, qreal progress);
