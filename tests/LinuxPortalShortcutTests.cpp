#include <QtTest>

#include "core/LinuxPortalGlobalShortcuts.h"
#include "core/LinuxKGlobalAccelShortcuts.h"
#include "core/LinuxPortalScreenCast.h"

class LinuxPortalShortcutTests : public QObject {
    Q_OBJECT

private slots:
    void formatsPreferredTriggers()
    {
        QCOMPARE(LinuxPortalGlobalShortcuts::preferredTrigger(0, VK_SNAPSHOT), QStringLiteral("Print"));
        QCOMPARE(LinuxPortalGlobalShortcuts::preferredTrigger(
                     MOD_CONTROL | MOD_ALT, 'P'),
                 QStringLiteral("CTRL+ALT+p"));
        QCOMPARE(LinuxPortalGlobalShortcuts::preferredTrigger(MOD_SHIFT, '7'),
                 QStringLiteral("SHIFT+7"));
        QCOMPARE(LinuxPortalGlobalShortcuts::preferredTrigger(MOD_CONTROL, VK_F12),
                 QStringLiteral("CTRL+F12"));
        QVERIFY(LinuxPortalGlobalShortcuts::preferredTrigger(0, 0).isEmpty());
    }

    void prefersPortalThenFallsBackToX11()
    {
        QCOMPARE(LinuxPortalGlobalShortcuts::preferredBackend(true, true),
                 LinuxHotkeyBackend::Portal);
        QCOMPARE(LinuxPortalGlobalShortcuts::preferredBackend(true, false),
                 LinuxHotkeyBackend::Portal);
        QCOMPARE(LinuxPortalGlobalShortcuts::preferredBackend(false, true),
                 LinuxHotkeyBackend::X11);
        QCOMPARE(LinuxPortalGlobalShortcuts::preferredBackend(false, false),
                 LinuxHotkeyBackend::Unavailable);
    }

    void detectsWaylandBehindXwayland()
    {
        QVERIFY(LinuxPortalScreenCast::isWaylandSessionType(QStringLiteral("wayland"),
                                                             QStringLiteral("xcb")));
        QVERIFY(LinuxPortalScreenCast::isWaylandSessionType(QString(),
                                                             QStringLiteral("wayland")));
        QVERIFY(!LinuxPortalScreenCast::isWaylandSessionType(QStringLiteral("x11"),
                                                              QStringLiteral("xcb")));
    }

    void detectsKdeDesktop()
    {
        QVERIFY(LinuxKGlobalAccelShortcuts::isKdeDesktop(QStringLiteral("KDE")));
        QVERIFY(LinuxKGlobalAccelShortcuts::isKdeDesktop(QStringLiteral("plasma:KDE")));
        QVERIFY(!LinuxKGlobalAccelShortcuts::isKdeDesktop(QStringLiteral("GNOME")));
    }

    void usesStableKdeActionIds()
    {
        const QStringList id = LinuxKGlobalAccelShortcuts::actionId(1);
        QCOMPARE(id.value(0), QStringLiteral("io.github.benoks.EShot"));
        QCOMPARE(id.value(1), QStringLiteral("eshot_capture"));
        QCOMPARE(id.value(2), QStringLiteral("EShot"));
        QCOMPARE(id.value(3), QStringLiteral("Capture"));
    }
};

QTEST_APPLESS_MAIN(LinuxPortalShortcutTests)
#include "LinuxPortalShortcutTests.moc"
