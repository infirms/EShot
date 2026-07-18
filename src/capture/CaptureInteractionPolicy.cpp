#include "CaptureInteractionPolicy.h"

bool shouldReleaseToolForResize(bool handleHit, int currentTool, int noneTool)
{
    return handleHit && currentTool != noneTool;
}

int initialAnnotationTool(bool rememberLastTool, int storedTool, int noneTool)
{
    return rememberLastTool ? storedTool : noneTool;
}

bool shouldShowCaptureHints(bool enabled, bool selecting, bool selectionComplete,
                            bool eyedropperActive)
{
    return enabled && !selecting && !selectionComplete && !eyedropperActive;
}

QRect captureHintRect(const QRect &monitorRect, const QSize &preferredSize)
{
    if (!monitorRect.isValid() || preferredSize.isEmpty())
        return {};

    constexpr int HorizontalMargin = 16;
    constexpr int TopMargin = 24;
    const int width = qMin(preferredSize.width(),
                           qMax(0, monitorRect.width() - HorizontalMargin * 2));
    const int height = qMin(preferredSize.height(), monitorRect.height());
    const int x = monitorRect.left() + (monitorRect.width() - width) / 2;
    const int y = monitorRect.top() + qMin(TopMargin, qMax(0, monitorRect.height() - height));
    return QRect(x, y, width, height);
}

int quickSettingsTabHeight(int textWidth, int availableHeight)
{
    constexpr int MinimumHeight = 118;
    constexpr int TextPadding = 32;
    constexpr int ScreenMargins = 64;
    const int preferredHeight = qMax(MinimumHeight, qMax(0, textWidth) + TextPadding);
    const int maximumHeight = qMax(28, availableHeight - ScreenMargins);
    return qMin(preferredHeight, maximumHeight);
}

bool shouldForwardCaptureKeyFromManagedProxy(bool textEditorVisible)
{
    return !textEditorVisible;
}

bool shouldDetachModalFromOverlay(bool xwaylandOverlay)
{
    return xwaylandOverlay;
}
