#include "LinuxScreenshotPolicy.h"

namespace LinuxScreenshotPolicy {

bool isKdeWaylandSession(const QString &currentDesktop,
                         const QString &sessionDesktop,
                         const QString &sessionType)
{
    const QString desktop = currentDesktop.trimmed().isEmpty()
        ? sessionDesktop : currentDesktop;
    const bool isKde = desktop.contains(QStringLiteral("KDE"), Qt::CaseInsensitive)
        || desktop.contains(QStringLiteral("Plasma"), Qt::CaseInsensitive);
    return isKde
        && sessionType.compare(QStringLiteral("wayland"), Qt::CaseInsensitive) == 0;
}

QStringList spectacleWorkspaceArguments(const QString &outputPath)
{
    return {QStringLiteral("--fullscreen"),
            QStringLiteral("--background"),
            QStringLiteral("--nonotify"),
            QStringLiteral("--output"),
            outputPath};
}

bool allowCursorBearingFallback(bool kdeWaylandSession)
{
    return !kdeWaylandSession;
}

bool canPresentCapture(bool captureSucceeded)
{
    return captureSucceeded;
}

}
