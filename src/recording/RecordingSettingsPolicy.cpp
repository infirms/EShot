#include "RecordingSettingsPolicy.h"

int gifRecordingFpsLimit()
{
    return 30;
}

int videoRecordingFpsLimit()
{
    return 60;
}

bool initialAudioEnabled(bool hasExplicitSetting,
                         bool explicitEnabled,
                         bool hasLegacyMode,
                         const QString &legacyMode,
                         RecordingAudioSource source)
{
    if (hasExplicitSetting)
        return explicitEnabled;
    if (!hasLegacyMode)
        return true;

    const QString normalizedMode = legacyMode.trimmed().toLower();
    if (normalizedMode == QStringLiteral("both"))
        return true;
    if (source == RecordingAudioSource::Desktop)
        return normalizedMode == QStringLiteral("desktop");
    return normalizedMode == QStringLiteral("microphone");
}
