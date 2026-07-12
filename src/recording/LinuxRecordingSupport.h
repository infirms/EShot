#pragma once

#include <QSize>
#include <QStringList>

QString pipeWireSourcePath(uint nodeId);
QSize evenRecordingSize(const QSize &size);
QString preferredGstAacEncoder(const QStringList &availableElements);
QString discoverGstAacEncoder();
QList<QPair<QString, QString>> linuxMicrophoneDevices(const QString &pactlSources);
QList<QPair<QString, QString>> discoverLinuxMicrophoneDevices();
