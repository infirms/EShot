#include <QtTest>
#include "ui/SettingsLayoutPolicy.h"

class SettingsLayoutPolicyTests : public QObject {
    Q_OBJECT
private slots:
    void expandsDialogToFitTabs()
    {
        QCOMPARE(settingsDialogWidthForTabs(680, 24, 560), 704);
        QCOMPARE(settingsDialogWidthForTabs(420, 24, 560), 560);
    }
};

QTEST_APPLESS_MAIN(SettingsLayoutPolicyTests)
#include "SettingsLayoutPolicyTests.moc"
