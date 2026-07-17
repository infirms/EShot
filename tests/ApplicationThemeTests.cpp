#include <QtTest>

#include "ui/ApplicationTheme.h"

class ApplicationThemeTests : public QObject {
    Q_OBJECT
private slots:
    void createsExplicitPalettes()
    {
        QCOMPARE(eshotApplicationPalette(true, false).color(QPalette::Window), QColor(53, 53, 53));
        QCOMPARE(eshotApplicationPalette(false, false).color(QPalette::Base), QColor(Qt::white));
        QCOMPARE(eshotApplicationPalette(false, true).color(QPalette::Highlight), QColor(Qt::yellow));
    }
};

QTEST_APPLESS_MAIN(ApplicationThemeTests)
#include "ApplicationThemeTests.moc"
