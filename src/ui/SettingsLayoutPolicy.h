#pragma once

#include <QList>
#include <QSize>

int settingsDialogWidthForTabs(int tabBarWidth, int horizontalMargins, int baseWidth);
QSize settingsDialogActionButtonSize(const QList<QSize> &sizeHints,
                                     int minimumWidth = 104,
                                     int minimumHeight = 34);
