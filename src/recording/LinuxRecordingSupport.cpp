#include "LinuxRecordingSupport.h"

#include <QProcess>

QString pipeWireSourcePath(uint nodeId)
{
    return nodeId > 0 ? QStringLiteral("path=%1").arg(nodeId) : QString();
}

QSize evenRecordingSize(const QSize &size)
{
    if (!size.isValid()) return QSize();
    return QSize(qMax(8, size.width() - (size.width() % 2)),
                 qMax(8, size.height() - (size.height() % 2)));
}

QStringList linuxMicrophoneSources(const QString &pactlShortSources)
{
    QStringList result;
    const QStringList lines = pactlShortSources.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QStringList columns = line.split('\t');
        if (columns.size() < 2) continue;
        const QString name = columns.at(1).trimmed();
        if (name.isEmpty() || name.endsWith(QStringLiteral(".monitor"), Qt::CaseInsensitive)) continue;
        if (!result.contains(name)) result.append(name);
    }
    return result;
}

QStringList discoverLinuxMicrophoneSources()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    QProcess pactl;
    pactl.start(QStringLiteral("pactl"), {QStringLiteral("list"), QStringLiteral("short"), QStringLiteral("sources")});
    if (!pactl.waitForFinished(2000) || pactl.exitCode() != 0) return {};
    return linuxMicrophoneSources(QString::fromLocal8Bit(pactl.readAllStandardOutput()));
#else
    return {};
#endif
}
