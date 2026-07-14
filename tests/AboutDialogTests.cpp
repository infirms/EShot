#include <QtTest>
#include <QSignalSpy>
#include "ui/AboutDialog.h"

class AboutDialogTests : public QObject {
    Q_OBJECT
private slots:
    void manualCheckIsForwardedToTheApplicationUpdater()
    {
        AboutDialog dialog;
        QSignalSpy spy(&dialog, &AboutDialog::checkForUpdatesRequested);

        QVERIFY(QMetaObject::invokeMethod(&dialog, "onCheckForUpdates"));

        QCOMPARE(spy.count(), 1);
    }
};

QTEST_MAIN(AboutDialogTests)
#include "AboutDialogTests.moc"
