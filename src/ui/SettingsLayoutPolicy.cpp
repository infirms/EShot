#include "SettingsLayoutPolicy.h"
#include <QtGlobal>

int settingsDialogWidthForTabs(int tabBarWidth, int horizontalMargins, int baseWidth)
{
    return qMax(baseWidth, tabBarWidth + horizontalMargins);
}
