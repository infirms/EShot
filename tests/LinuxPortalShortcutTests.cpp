#include <QtTest>

#include "core/LinuxPortalGlobalShortcuts.h"

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
};

QTEST_APPLESS_MAIN(LinuxPortalShortcutTests)
#include "LinuxPortalShortcutTests.moc"
