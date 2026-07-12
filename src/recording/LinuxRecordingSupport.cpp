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

QString preferredGstAacEncoder(const QStringList &availableElements)
{
    const QStringList preference = {QStringLiteral("fdkaacenc"), QStringLiteral("avenc_aac"),
                                    QStringLiteral("faac"), QStringLiteral("voaacenc")};
    for (const QString &encoder : preference)
        if (availableElements.contains(encoder)) return encoder;
    return {};
}

QString discoverGstAacEncoder()
{
    QStringList available;
    for (const QString &encoder : {QStringLiteral("fdkaacenc"), QStringLiteral("avenc_aac"),
                                   QStringLiteral("faac"), QStringLiteral("voaacenc")}) {
        QProcess inspect;
        inspect.setProgram(QStringLiteral("gst-inspect-1.0"));
        inspect.setArguments({encoder});
        inspect.setStandardOutputFile(QProcess::nullDevice());
        inspect.setStandardErrorFile(QProcess::nullDevice());
        inspect.start();
        if (inspect.waitForFinished(2000) && inspect.exitCode() == 0)
            available.append(encoder);
    }
    return preferredGstAacEncoder(available);
}

QList<QPair<QString, QString>> linuxMicrophoneDevices(const QString &pactlSources)
{
    QList<QPair<QString, QString>> result;
    QString name;
    QString description;
    QString monitor;
    auto flush = [&]() {
        if (!name.isEmpty() && !name.endsWith(QStringLiteral(".monitor"), Qt::CaseInsensitive)
            && (monitor.isEmpty() || monitor == QStringLiteral("n/a"))) {
            QString label = description.trimmed();
            if (label.isEmpty() || label == QStringLiteral("(null)")) label = name;
            result.append(qMakePair(label, name));
        }
        name.clear(); description.clear(); monitor.clear();
    };
    const QStringList lines = pactlSources.split('\n');
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("Source #"))) { flush(); continue; }
        if (trimmed.startsWith(QStringLiteral("Name:"))) name = trimmed.mid(5).trimmed();
        else if (trimmed.startsWith(QStringLiteral("Description:"))) description = trimmed.mid(12).trimmed();
        else if (trimmed.startsWith(QStringLiteral("Monitor of Sink:"))) monitor = trimmed.mid(16).trimmed();
    }
    flush();
    return result;
}

QList<QPair<QString, QString>> discoverLinuxMicrophoneDevices()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    QProcess pactl;
    pactl.start(QStringLiteral("pactl"), {QStringLiteral("list"), QStringLiteral("sources")});
    if (!pactl.waitForFinished(2000) || pactl.exitCode() != 0) return {};
    return linuxMicrophoneDevices(QString::fromLocal8Bit(pactl.readAllStandardOutput()));
#else
    return {};
#endif
}
