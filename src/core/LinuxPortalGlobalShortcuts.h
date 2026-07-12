#ifndef LINUXPORTALGLOBALSHORTCUTS_H
#define LINUXPORTALGLOBALSHORTCUTS_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QVariantMap>

#include "PlatformHotkey.h"

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
class QDBusObjectPath;
#endif

struct PortalShortcut {
    QString id;
    QVariantMap options;
};

enum class LinuxHotkeyBackend {
    Portal,
    X11,
    Unavailable
};

class LinuxPortalGlobalShortcuts : public QObject {
    Q_OBJECT

public:
    explicit LinuxPortalGlobalShortcuts(QObject *parent = nullptr);

    bool isAvailable() const;
    void setShortcuts(const QHash<int, QPair<UINT, UINT>> &shortcuts);
    bool requestRebind();
    static QString preferredTrigger(UINT modifiers, UINT virtualKey);
    static LinuxHotkeyBackend preferredBackend(bool portalAvailable, bool x11Available);

signals:
    void shortcutActivated(int id);

private slots:
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    void onCreateSessionResponse(uint response, const QVariantMap &results);
    void onBindShortcutsResponse(uint response, const QVariantMap &results);
    void onActivated(const QDBusObjectPath &sessionHandle, const QString &shortcutId,
                     qulonglong timestamp, const QVariantMap &options);
#endif

private:
    void createSession();
    void bindShortcuts();
    void closeSession();
    QString shortcutIdForInt(int id) const;
    QString descriptionForInt(int id) const;
    QHash<int, QPair<UINT, UINT>> m_shortcuts;
    QHash<QString, int> m_ids;
    QString m_sessionHandle;
    bool m_available = false;
    bool m_createPending = false;
    bool m_bindPending = false;
    bool m_bindCompleted = false;
};

#endif
