#include "WindowSnapPolicy.h"

QRect topmostWindowAt(const QVector<QRect> &windows,
                      const QPoint &point,
                      const QRect &overlayBounds)
{
    if (!overlayBounds.isValid() || !overlayBounds.contains(point))
        return {};

    for (const QRect &window : windows) {
        if (!window.isValid())
            continue;
        const QRect visible = window.intersected(overlayBounds);
        if (visible.isValid() && visible.contains(point))
            return visible;
    }
    return {};
}

bool isWindowSnapClick(const QPoint &pressPosition,
                       const QPoint &releasePosition,
                       int dragThreshold)
{
    return dragThreshold > 0
        && (releasePosition - pressPosition).manhattanLength() < dragThreshold;
}
