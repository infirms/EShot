#pragma once

#include <QString>

enum class RecordingAudioSource
{
    Desktop,
    Microphone
};

int gifRecordingFpsLimit();
int videoRecordingFpsLimit();

bool initialAudioEnabled(bool hasExplicitSetting,
                         bool explicitEnabled,
                         bool hasLegacyMode,
                         const QString &legacyMode,
                         RecordingAudioSource source);
