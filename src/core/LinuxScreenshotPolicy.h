#ifndef LINUXSCREENSHOTPOLICY_H
#define LINUXSCREENSHOTPOLICY_H

#include <QString>
#include <QStringList>

namespace LinuxScreenshotPolicy {

bool isKdeWaylandSession(const QString &currentDesktop,
                         const QString &sessionDesktop,
                         const QString &sessionType);
QStringList spectacleWorkspaceArguments(const QString &outputPath);
bool allowCursorBearingFallback(bool kdeWaylandSession);
bool canPresentCapture(bool captureSucceeded);

}

#endif
