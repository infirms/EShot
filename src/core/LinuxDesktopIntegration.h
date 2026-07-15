#pragma once

#include <QString>

enum class LinuxDesktopEnvironment {
    Kde,
    Gnome,
    Other
};

namespace LinuxDesktopIntegration {

LinuxDesktopEnvironment detect(const QString &currentDesktop,
                               const QString &sessionDesktop);
bool isWayland(const QString &sessionType, const QString &platformName = QString());
bool useXWaylandOverlay(LinuxDesktopEnvironment desktop, const QString &sessionType);
bool deferFirstRunHotkeyRegistration(LinuxDesktopEnvironment desktop);
bool useGnomeShortcutFallback(LinuxDesktopEnvironment desktop, bool portalAvailable);
QString displayName(LinuxDesktopEnvironment desktop);

}
