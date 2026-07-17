#include "ApplicationTheme.h"

#include <QApplication>
#include <QColor>

QPalette eshotApplicationPalette(bool dark, bool highContrast)
{
    QPalette p;
    if (highContrast) {
        p.setColor(QPalette::Window, Qt::black);
        p.setColor(QPalette::WindowText, Qt::white);
        p.setColor(QPalette::Base, Qt::black);
        p.setColor(QPalette::AlternateBase, QColor(30, 30, 30));
        p.setColor(QPalette::ToolTipBase, Qt::black);
        p.setColor(QPalette::ToolTipText, Qt::yellow);
        p.setColor(QPalette::Text, Qt::white);
        p.setColor(QPalette::Button, Qt::black);
        p.setColor(QPalette::ButtonText, Qt::white);
        p.setColor(QPalette::BrightText, Qt::red);
        p.setColor(QPalette::Link, Qt::cyan);
        p.setColor(QPalette::Highlight, Qt::yellow);
        p.setColor(QPalette::HighlightedText, Qt::black);
    } else if (dark) {
        p.setColor(QPalette::Window, QColor(53, 53, 53));
        p.setColor(QPalette::WindowText, Qt::white);
        p.setColor(QPalette::Base, QColor(42, 42, 42));
        p.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
        p.setColor(QPalette::ToolTipBase, QColor(53, 53, 53));
        p.setColor(QPalette::ToolTipText, Qt::white);
        p.setColor(QPalette::Text, Qt::white);
        p.setColor(QPalette::Button, QColor(53, 53, 53));
        p.setColor(QPalette::ButtonText, Qt::white);
        p.setColor(QPalette::BrightText, Qt::red);
        p.setColor(QPalette::Link, QColor(42, 130, 218));
        p.setColor(QPalette::Highlight, QColor(42, 130, 218));
        p.setColor(QPalette::HighlightedText, Qt::black);
    } else {
        p.setColor(QPalette::Window, QColor(240, 240, 240));
        p.setColor(QPalette::WindowText, Qt::black);
        p.setColor(QPalette::Base, Qt::white);
        p.setColor(QPalette::AlternateBase, QColor(233, 233, 233));
        p.setColor(QPalette::ToolTipBase, QColor(255, 255, 220));
        p.setColor(QPalette::ToolTipText, Qt::black);
        p.setColor(QPalette::Text, Qt::black);
        p.setColor(QPalette::Button, QColor(240, 240, 240));
        p.setColor(QPalette::ButtonText, Qt::black);
        p.setColor(QPalette::BrightText, Qt::red);
        p.setColor(QPalette::Link, QColor(0, 0, 255));
        p.setColor(QPalette::Highlight, QColor(0, 120, 215));
        p.setColor(QPalette::HighlightedText, Qt::white);
    }
    return p;
}

void applyEShotApplicationTheme(QApplication &application, bool dark, bool highContrast)
{
    application.setPalette(eshotApplicationPalette(dark, highContrast));
}
