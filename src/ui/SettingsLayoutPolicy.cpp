#include "SettingsLayoutPolicy.h"
#include <QtGlobal>

int settingsDialogWidthForTabs(int tabBarWidth, int horizontalMargins, int baseWidth)
{
    return qMax(baseWidth, tabBarWidth + horizontalMargins);
}

QSize settingsDialogActionButtonSize(const QList<QSize> &sizeHints,
                                     int minimumWidth,
                                     int minimumHeight)
{
    int width = minimumWidth;
    int height = minimumHeight;
    for (const QSize &hint : sizeHints) {
        width = qMax(width, hint.width());
        height = qMax(height, hint.height());
    }
    return QSize(width, height);
}
