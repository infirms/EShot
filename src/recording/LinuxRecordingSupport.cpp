#include "LinuxRecordingSupport.h"

#include <QProcess>

QString pipeWireSourcePath(uint nodeId, quint64 pipewireSerial)
{
    if (pipewireSerial > 0)
        return QStringLiteral("target-object=%1").arg(pipewireSerial);
    return nodeId > 0 ? QStringLiteral("path=%1").arg(nodeId) : QString();
}

PortalCropGeometry portalCropGeometry(const QRect &captureRect,
                                      const QRect &displayRect,
                                      const QPoint &streamPosition,
                                      const QSize &streamDisplaySize,
                                      const QSize &requestedOutputSize)
{
    PortalCropGeometry geometry;
    if (!captureRect.isValid() || !requestedOutputSize.isValid())
        return geometry;

    QSize sourcePixelSize;
    if (displayRect.isValid() && streamDisplaySize.isValid()) {
        const QRect streamDisplayRect(streamPosition, streamDisplaySize);
        if (!streamDisplayRect.contains(displayRect))
            return geometry;

        const qreal scaleX = captureRect.width() / static_cast<qreal>(displayRect.width());
        const qreal scaleY = captureRect.height() / static_cast<qreal>(displayRect.height());
        sourcePixelSize = QSize(qRound(streamDisplaySize.width() * scaleX),
                                qRound(streamDisplaySize.height() * scaleY));
        geometry.left = qRound((displayRect.x() - streamPosition.x()) * scaleX);
        geometry.top = qRound((displayRect.y() - streamPosition.y()) * scaleY);
    } else {
        sourcePixelSize = streamDisplaySize.isValid() ? streamDisplaySize : captureRect.size();
        geometry.left = captureRect.x() - streamPosition.x();
        geometry.top = captureRect.y() - streamPosition.y();
    }

    if (geometry.left < 0 || geometry.top < 0
        || geometry.left + captureRect.width() > sourcePixelSize.width()
        || geometry.top + captureRect.height() > sourcePixelSize.height()) {
        return {};
    }

    geometry.right = sourcePixelSize.width() - geometry.left - captureRect.width();
    geometry.bottom = sourcePixelSize.height() - geometry.top - captureRect.height();
    geometry.outputSize = evenRecordingSize(QSize(
        qMin(requestedOutputSize.width(), captureRect.width()),
        qMin(requestedOutputSize.height(), captureRect.height())));
    geometry.valid = geometry.outputSize.isValid();
    return geometry;
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
