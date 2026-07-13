#ifndef LINUXSCREENSHOTPOLICY_H
#define LINUXSCREENSHOTPOLICY_H

#include <QString>
#include <QStringList>

namespace LinuxScreenshotPolicy {

bool isKdeWaylandSession(const QString &currentDesktop,
                         const QString &sessionDesktop,
                         const QString &sessionType);
bool shouldPrepareKWinPermission(const QString &currentDesktop,
                                 const QString &sessionDesktop,
                                 const QString &sessionType,
                                 const QString &appImagePath,
                                 const QString &executablePath);
QString kwinPermissionDesktopEntry(const QString &executablePath);
bool installKWinPermissionDesktopEntry(const QString &applicationsDirectory,
                                       const QString &executablePath,
                                       QString *desktopPath = nullptr,
                                       QString *error = nullptr);
QStringList spectacleWorkspaceArguments(const QString &outputPath);
bool allowCursorBearingFallback(bool kdeWaylandSession);
bool canPresentCapture(bool captureSucceeded);

}

#endif
