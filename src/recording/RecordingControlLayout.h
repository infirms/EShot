#ifndef RECORDINGCONTROLLAYOUT_H
#define RECORDINGCONTROLLAYOUT_H

#include <QRect>
#include <QSize>

enum class RecordingControlPlacement {
    Below,
    Above,
    Inside
};

enum class RecordingOverlayVisibility {
    AlwaysVisible,
    RevealWhilePaused
};

struct RecordingControlLayout {
    QRect controlRect;
    RecordingControlPlacement placement = RecordingControlPlacement::Below;
    bool compact = false;
};

RecordingControlLayout recordingControlLayout(const QRect &captureRect,
                                               const QRect &screenRect,
                                               const QSize &preferredSize,
                                               const QSize &compactSize);

RecordingOverlayVisibility recordingOverlayVisibilityPolicy(
    RecordingControlPlacement placement,
    bool platformCanExcludeOverlay);

#endif
