#include <QtTest>
#include "ui/SettingsHotkeyPolicy.h"

class SettingsHotkeyPolicyTests : public QObject {
    Q_OBJECT
private slots:
    void unchangedHotkeyDoesNotNeedReregistration()
    {
        const SettingsHotkeyDefinition saved { 3, 44 };
        QVERIFY(!settingsHotkeyChanged(saved, saved));
    }

    void changedHotkeyNeedsReregistration()
    {
        const SettingsHotkeyDefinition saved { 3, 44 };
        QVERIFY(settingsHotkeyChanged({ 3, 45 }, saved));
        QVERIFY(settingsHotkeyChanged({ 2, 44 }, saved));
    }
};

QTEST_APPLESS_MAIN(SettingsHotkeyPolicyTests)
#include "SettingsHotkeyPolicyTests.moc"
