#include "RecordingControlLayout.h"

#include <QtGlobal>

namespace {
constexpr int ScreenMargin = 8;
constexpr int ControlGap = 8;
constexpr int InsideInset = 12;

int clampedControlX(const QRect &captureRect, const QRect &screenRect, int width)
{
    const int minimum = screenRect.left() + ScreenMargin;
    const int maximum = screenRect.right() - ScreenMargin - width + 1;
    return qBound(minimum, captureRect.center().x() - (width - 1) / 2,
                  qMax(minimum, maximum));
}
}

RecordingControlLayout recordingControlLayout(const QRect &captureRect,
                                               const QRect &screenRect,
                                               const QSize &preferredSize,
                                               const QSize &compactSize)
{
    RecordingControlLayout result;
    const int availableWidth = qMax(1, screenRect.width() - ScreenMargin * 2);
    result.compact = preferredSize.width() > availableWidth;

    QSize size = result.compact ? compactSize : preferredSize;
    size.setWidth(qMin(size.width(), availableWidth));
    const int x = clampedControlX(captureRect, screenRect, size.width());

    const int belowTop = captureRect.bottom() + 1 + ControlGap;
    if (belowTop + size.height() - 1 <= screenRect.bottom() - ScreenMargin) {
        result.placement = RecordingControlPlacement::Below;
        result.controlRect = QRect(QPoint(x, belowTop), size);
        return result;
    }

    const int aboveTop = captureRect.top() - ControlGap - size.height();
    if (aboveTop >= screenRect.top() + ScreenMargin) {
        result.placement = RecordingControlPlacement::Above;
        result.controlRect = QRect(QPoint(x, aboveTop), size);
        return result;
    }

    result.placement = RecordingControlPlacement::Inside;
    const int insideTop = qBound(screenRect.top() + ScreenMargin,
                                 captureRect.bottom() - InsideInset - size.height() + 1,
                                 qMax(screenRect.top() + ScreenMargin,
                                      screenRect.bottom() - ScreenMargin - size.height() + 1));
    result.controlRect = QRect(QPoint(x, insideTop), size);
    return result;
}

RecordingOverlayVisibility recordingOverlayVisibilityPolicy(
    RecordingControlPlacement placement,
    bool platformCanExcludeOverlay)
{
    if (placement == RecordingControlPlacement::Inside && !platformCanExcludeOverlay)
        return RecordingOverlayVisibility::RevealWhilePaused;
    return RecordingOverlayVisibility::AlwaysVisible;
}
