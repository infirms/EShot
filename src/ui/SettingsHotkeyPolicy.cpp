#include "SettingsHotkeyPolicy.h"

bool settingsHotkeyChanged(const SettingsHotkeyDefinition &proposed,
                           const SettingsHotkeyDefinition &saved)
{
    return proposed.modifiers != saved.modifiers || proposed.virtualKey != saved.virtualKey;
}
