#ifndef APPLICATIONTHEME_H
#define APPLICATIONTHEME_H

#include <QPalette>

class QApplication;

QPalette eshotApplicationPalette(bool dark, bool highContrast);
void applyEShotApplicationTheme(QApplication &application, bool dark, bool highContrast);

#endif
