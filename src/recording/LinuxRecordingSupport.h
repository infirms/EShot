#pragma once

#include <QSize>
#include <QStringList>

QString pipeWireSourcePath(uint nodeId);
QSize evenRecordingSize(const QSize &size);
QStringList linuxMicrophoneSources(const QString &pactlShortSources);
QStringList discoverLinuxMicrophoneSources();
