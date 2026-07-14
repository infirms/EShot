#pragma once

#include <QtGlobal>

struct SettingsHotkeyDefinition {
    quint32 modifiers = 0;
    quint32 virtualKey = 0;
};

bool settingsHotkeyChanged(const SettingsHotkeyDefinition &proposed,
                           const SettingsHotkeyDefinition &saved);
