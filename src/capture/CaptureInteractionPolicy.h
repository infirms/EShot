#pragma once

#include <QRect>
#include <QSize>

bool shouldReleaseToolForResize(bool handleHit, int currentTool, int noneTool);
int initialAnnotationTool(bool rememberLastTool, int storedTool, int noneTool);
bool shouldShowCaptureHints(bool enabled, bool selecting, bool selectionComplete,
                            bool eyedropperActive);
QRect captureHintRect(const QRect &monitorRect, const QSize &preferredSize);
