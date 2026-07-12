#include "ScreenRecorder.h"
#include "GifEncoder.h"
#include "core/LinuxPortalScreenCast.h"

#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QPainter>
#include <QDateTime>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QStringList>
#include <cstring>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#include <fcntl.h>
#endif

namespace {
QString defaultSaveDirectory()
{
    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (picturesPath.trimmed().isEmpty())
        picturesPath = QDir::homePath();
    return QDir(picturesPath).filePath(QStringLiteral("EShot"));
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
bool configurePipeWireRemote(QProcess *process, int fd)
{
    if (!process || fd < 0)
        return false;

    const int flags = fcntl(fd, F_GETFD);
    if (flags < 0)
        return false;
    if (fcntl(fd, F_SETFD, flags & ~FD_CLOEXEC) < 0)
        return false;

    // ponytail: leave the portal PipeWire fd inheritable; add fd allowlisting
    // only if a supported Qt floor gives us it on every Linux target.
    return true;
}
#endif
}

ScreenRecorder::ScreenRecorder(QObject *parent) : QObject(parent) {}

ScreenRecorder::~ScreenRecorder()
{
    if (m_recording) cancel();
}

void ScreenRecorder::start(const QRect &captureRect, int fps, int maxSeconds, int loopCount,
                           const QString &outputPath, const QRect &displayRect)
{
    if (m_recording) {
        emit recordingFailed(QStringLiteral("already recording"));
        return;
    }
    if (captureRect.width() < 8 || captureRect.height() < 8) {
        emit recordingFailed(QStringLiteral("region too small"));
        return;
    }
    if (fps < 1) fps = 1;
    if (fps > 30) fps = 30;
    if (maxSeconds < 0) maxSeconds = 0;

    m_captureRect = captureRect;
    m_displayRect = displayRect;
    m_outputSize = boundedOutputSize(captureRect.size());
    m_fps = fps;
    m_maxSeconds = maxSeconds;
    m_frameCount = 0;
    m_delayCs = qMax(1, qRound(100.0 / m_fps));
    m_outputPath = outputPath;
    m_hasPendingFrame = false;
    m_pendingFrame = QImage();
    m_pendingDelayCs = 0;

    if (m_outputPath.isEmpty()) {
        m_outputPath = makeDefaultOutputPath();
    }

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    if (LinuxPortalScreenCast::isWaylandSession()) {
        startWaylandPortalRecording(captureRect);
        return;
    }
#endif

    if (!initCaptureResources()) {
        emit recordingFailed(QStringLiteral("cannot initialize screen capture"));
        return;
    }

    m_encoder = new GifEncoder(this);
    if (!m_encoder->open(m_outputPath, m_outputSize.width(), m_outputSize.height(), loopCount)) {
        QString err = m_encoder->errorString();
        delete m_encoder;
        m_encoder = nullptr;
        releaseCaptureResources();
        emit recordingFailed(err);
        return;
    }

    m_recording = true;
    m_recordingStartedAt = QDateTime::currentDateTime();
    emit recordingStarted();
    emit remainingTimeChanged(m_maxSeconds);

    m_frameTimer = new QTimer(this);
    m_frameTimer->setTimerType(Qt::PreciseTimer);
    m_frameTimer->setInterval(qMax(1, 1000 / m_fps));
    connect(m_frameTimer, &QTimer::timeout, this, &ScreenRecorder::captureFrame);
    m_frameTimer->start();

    m_countdownTimer = new QTimer(this);
    m_countdownTimer->setInterval(1000);
    connect(m_countdownTimer, &QTimer::timeout, this, [this]() {
        if (m_maxSeconds <= 0) return;
        QDateTime start = m_recordingStartedAt;
        qint64 elapsed = start.msecsTo(QDateTime::currentDateTime());
        int remaining = qMax(0, m_maxSeconds - static_cast<int>(elapsed / 1000));
        emit remainingTimeChanged(remaining);
        if (remaining == 0) {
            emit timeLimitReached();
            finishRecording();
        }
    });
    m_countdownTimer->start();

    QTimer::singleShot(qMin(250, m_frameTimer->interval()), this, &ScreenRecorder::captureFrame);
}

QString ScreenRecorder::makeDefaultOutputPath() const
{
    QSettings s("EShot", "EShot");
    QStringList candidates;
    QString configuredDir = s.contains("gifSavePath")
        ? s.value("gifSavePath").toString().trimmed()
        : QDir(defaultSaveDirectory()).filePath(QStringLiteral("GIFs"));
    if (configuredDir.isEmpty())
        configuredDir = s.value("savePath").toString().trimmed();
    if (!configuredDir.isEmpty()) {
        candidates << configuredDir;
    } else {
        candidates << defaultSaveDirectory();
    }
    candidates << QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
               << QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
               << QStandardPaths::writableLocation(QStandardPaths::TempLocation)
               << QDir::homePath();

    const QString fileName = QStringLiteral("EShot_GIF_%1.gif")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    for (const QString &candidate : candidates) {
        if (candidate.trimmed().isEmpty()) {
            continue;
        }

        QDir dir(candidate);
        if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
            continue;
        }

        const QString probePath = dir.filePath(QStringLiteral(".eshot_write_test.tmp"));
        QFile probe(probePath);
        if (!probe.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            continue;
        }
        probe.close();
        QFile::remove(probePath);
        return dir.filePath(fileName);
    }

    return QDir(QDir::tempPath()).filePath(fileName);
}

void ScreenRecorder::stop()
{
    if (!m_recording) return;
    if (m_portalRecording && m_process) {
        m_process->write("q\n");
        QTimer::singleShot(2500, this, [this]() {
            if (m_process && m_recording)
                m_process->terminate();
        });
        QTimer::singleShot(5000, this, [this]() {
            if (m_process && m_recording)
                m_process->kill();
        });
        return;
    }
    finishRecording();
}

void ScreenRecorder::cancel()
{
    if (!m_recording) return;
    m_recording = false;
    m_portalRecording = false;
    if (m_process) { m_process->kill(); m_process->deleteLater(); m_process = nullptr; }
    if (m_frameTimer)     { m_frameTimer->stop();     m_frameTimer->deleteLater();     m_frameTimer = nullptr; }
    if (m_countdownTimer) { m_countdownTimer->stop(); m_countdownTimer->deleteLater(); m_countdownTimer = nullptr; }
    if (m_encoder)        { delete m_encoder;         m_encoder = nullptr; }
    releaseCaptureResources();
    m_hasPendingFrame = false;
    m_pendingFrame = QImage();
    m_pendingDelayCs = 0;
    if (!m_outputPath.isEmpty() && QFile::exists(m_outputPath)) {
        QFile::remove(m_outputPath);
    }
}

void ScreenRecorder::finishRecording()
{
    if (!m_recording) return;
    m_recording = false;
    if (m_frameTimer)     { m_frameTimer->stop();     m_frameTimer->deleteLater();     m_frameTimer = nullptr; }
    if (m_countdownTimer) { m_countdownTimer->stop(); m_countdownTimer->deleteLater(); m_countdownTimer = nullptr; }
    if (!m_hasPendingFrame) {
        QImage frame = grabScreenRegion(m_captureRect);
        if (!frame.isNull()) {
            m_pendingFrame = frame;
            m_pendingDelayCs = m_delayCs;
            m_hasPendingFrame = true;
        }
    }
    releaseCaptureResources();

    QString savedPath = m_outputPath;
    bool ok = false;
    if (m_encoder) {
        ok = flushPendingFrame() && m_encoder->close();
        QString err = m_encoder->errorString();
        delete m_encoder;
        m_encoder = nullptr;
        m_hasPendingFrame = false;
        m_pendingFrame = QImage();
        m_pendingDelayCs = 0;
        if (!ok) {
            if (QFile::exists(savedPath)) QFile::remove(savedPath);
            emit recordingFailed(err);
            return;
        }
    }
    emit recordingStopped(savedPath);
}

void ScreenRecorder::onPortalProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    if (!m_portalRecording && !m_process)
        return;
    const QString stderrText = m_process ? QString::fromLocal8Bit(m_process->readAll()).trimmed() : QString();
    const QString output = m_outputPath;

    m_recording = false;
    m_portalRecording = false;
    if (m_countdownTimer) { m_countdownTimer->stop(); m_countdownTimer->deleteLater(); m_countdownTimer = nullptr; }
    if (m_process) { m_process->deleteLater(); m_process = nullptr; }

    if (status == QProcess::NormalExit && exitCode == 0 && QFileInfo::exists(output)) {
        emit recordingStopped(output);
        return;
    }

    if (QFileInfo::exists(output))
        QFile::remove(output);
    emit recordingFailed(stderrText.isEmpty() ? QStringLiteral("gstreamer exited with code %1").arg(exitCode) : stderrText);
}

void ScreenRecorder::captureFrame()
{
    if (!m_recording || !m_encoder) return;

    QImage frame = grabScreenRegion(m_captureRect);
    if (frame.isNull()) {
        qWarning() << "ScreenRecorder: grabScreenRegion returned null";
        return;
    }

    if (!m_hasPendingFrame) {
        m_pendingFrame = frame;
        m_pendingDelayCs = m_delayCs;
        m_hasPendingFrame = true;
    } else if (framesEqual(m_pendingFrame, frame) && m_pendingDelayCs < 65000) {
        m_pendingDelayCs += m_delayCs;
    } else {
        if (!flushPendingFrame()) {
            QString err = m_encoder->errorString();
            cancel();
            emit recordingFailed(err);
            return;
        }
        m_pendingFrame = frame;
        m_pendingDelayCs = m_delayCs;
        m_hasPendingFrame = true;
    }
    ++m_frameCount;
    emit frameCaptured(m_frameCount);

    if (m_maxSeconds > 0) {
        qint64 elapsed = m_recordingStartedAt.msecsTo(QDateTime::currentDateTime());
        int remaining = qMax(0, m_maxSeconds - static_cast<int>(elapsed / 1000));
        emit remainingTimeChanged(remaining);
    }
}

bool ScreenRecorder::flushPendingFrame()
{
    if (!m_hasPendingFrame || !m_encoder) return true;
    const bool ok = m_encoder->addFrame(m_pendingFrame, qMax(1, m_pendingDelayCs));
    if (ok) {
        m_hasPendingFrame = false;
        m_pendingFrame = QImage();
        m_pendingDelayCs = 0;
    }
    return ok;
}

bool ScreenRecorder::framesEqual(const QImage &a, const QImage &b) const
{
    if (a.size() != b.size() || a.format() != b.format()) return false;
    if (a.isNull() || b.isNull()) return false;
    const qsizetype bytes = static_cast<qsizetype>(a.bytesPerLine()) * a.height();
    return bytes == static_cast<qsizetype>(b.bytesPerLine()) * b.height()
        && std::memcmp(a.constBits(), b.constBits(), static_cast<size_t>(bytes)) == 0;
}

bool ScreenRecorder::startWaylandPortalRecording(const QRect &captureRect)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    const QString gst = gstLaunchPath();
    if (gst.isEmpty()) {
        emit recordingFailed(QStringLiteral("gstreamer not found"));
        return false;
    }
    if (!LinuxPortalScreenCast::isAvailable()) {
        emit recordingFailed(QStringLiteral("Wayland ScreenCast portal is not available"));
        return false;
    }

    LinuxPortalScreenCast::Stream stream = LinuxPortalScreenCast::selectStream();
    if (!stream.isValid()) {
        emit recordingFailed(QStringLiteral("Wayland screen recording permission was not granted"));
        return false;
    }

    const QSize streamSize = stream.size.isValid() ? stream.size : captureRect.size();
    const QRect relativeRect = captureRect.translated(-stream.position);
    const int left = qBound(0, relativeRect.x(), qMax(0, streamSize.width() - 1));
    const int top = qBound(0, relativeRect.y(), qMax(0, streamSize.height() - 1));
    const int right = qMax(0, streamSize.width() - left - captureRect.width());
    const int bottom = qMax(0, streamSize.height() - top - captureRect.height());
    const QSize outputSize(qMax(8, qMin(m_outputSize.width(), streamSize.width() - left)),
                           qMax(8, qMin(m_outputSize.height(), streamSize.height() - top)));

    const QString targetObject = stream.pipewireSerial > 0
        ? QString::number(stream.pipewireSerial)
        : QString::number(stream.nodeId);
    const int pipewireFd = stream.remoteFd();

    QStringList args;
    args << QStringLiteral("-e")
         << QStringLiteral("pipewiresrc")
         << QStringLiteral("fd=%1").arg(pipewireFd)
         << QStringLiteral("target-object=%1").arg(targetObject)
         << QStringLiteral("do-timestamp=true")
         << QStringLiteral("!")
         << QStringLiteral("queue")
         << QStringLiteral("!")
         << QStringLiteral("videoconvert")
         << QStringLiteral("!")
         << QStringLiteral("videocrop")
         << QStringLiteral("left=%1").arg(left)
         << QStringLiteral("right=%1").arg(right)
         << QStringLiteral("top=%1").arg(top)
         << QStringLiteral("bottom=%1").arg(bottom)
         << QStringLiteral("!")
         << QStringLiteral("videoscale")
         << QStringLiteral("!")
         << QStringLiteral("videorate")
         << QStringLiteral("!")
         << QStringLiteral("video/x-raw,width=%1,height=%2,framerate=%3/1")
                .arg(outputSize.width())
                .arg(outputSize.height())
                .arg(m_fps)
         << QStringLiteral("!")
         << QStringLiteral("gifenc")
         << QStringLiteral("!")
         << QStringLiteral("filesink")
         << QStringLiteral("location=%1").arg(m_outputPath);

    m_process = new QProcess(this);
    m_process->setProgram(gst);
    m_process->setArguments(args);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    if (!configurePipeWireRemote(m_process, pipewireFd)) {
        if (m_process) { m_process->deleteLater(); m_process = nullptr; }
        emit recordingFailed(QStringLiteral("Wayland PipeWire remote could not be opened"));
        return false;
    }
    connect(m_process, &QProcess::finished, this, &ScreenRecorder::onPortalProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        if (!m_recording)
            return;
        const QString reason = m_process ? m_process->errorString() : QStringLiteral("gstreamer process error");
        const QString output = m_outputPath;
        m_recording = false;
        m_portalRecording = false;
        if (m_process) { m_process->deleteLater(); m_process = nullptr; }
        if (!output.isEmpty())
            QFile::remove(output);
        emit recordingFailed(reason);
    });

    m_process->start();
    if (!m_process->waitForStarted(3000)) {
        const QString reason = m_process->errorString();
        if (m_process) { m_process->deleteLater(); m_process = nullptr; }
        emit recordingFailed(reason.isEmpty() ? QStringLiteral("cannot start gstreamer") : reason);
        return false;
    }

    m_recording = true;
    m_portalRecording = true;
    m_recordingStartedAt = QDateTime::currentDateTime();
    emit recordingStarted();
    emit remainingTimeChanged(m_maxSeconds);

    m_countdownTimer = new QTimer(this);
    m_countdownTimer->setInterval(1000);
    connect(m_countdownTimer, &QTimer::timeout, this, [this]() {
        if (m_maxSeconds <= 0)
            return;
        const qint64 elapsed = m_recordingStartedAt.msecsTo(QDateTime::currentDateTime());
        const int remaining = qMax(0, m_maxSeconds - static_cast<int>(elapsed / 1000));
        emit remainingTimeChanged(remaining);
        if (remaining == 0)
            stop();
    });
    m_countdownTimer->start();
    return true;
#else
    Q_UNUSED(captureRect);
    return false;
#endif
}

QString ScreenRecorder::gstLaunchPath() const
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir(appDir).filePath(QStringLiteral("gstreamer/gst-launch-1.0")),
        QDir(appDir).filePath(QStringLiteral("gst-launch-1.0"))
    };
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path))
            return QFileInfo(path).absoluteFilePath();
    }
    return QStandardPaths::findExecutable(QStringLiteral("gst-launch-1.0"));
}

bool ScreenRecorder::initCaptureResources()
{
#ifdef Q_OS_WIN
    releaseCaptureResources();
    if (m_captureRect.width() <= 0 || m_captureRect.height() <= 0) return false;

    m_screenDC = GetDC(nullptr);
    if (!m_screenDC) return false;
    m_memDC = CreateCompatibleDC(m_screenDC);
    if (!m_memDC) {
        releaseCaptureResources();
        return false;
    }
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = m_captureRect.width();
    bi.bmiHeader.biHeight = -m_captureRect.height();
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    m_bits = nullptr;
    m_bitmap = CreateDIBSection(m_screenDC, &bi, DIB_RGB_COLORS, &m_bits, nullptr, 0);
    if (!m_bitmap) {
        releaseCaptureResources();
        return false;
    }
    m_oldBitmap = SelectObject(m_memDC, m_bitmap);
    return m_oldBitmap != nullptr;
#else
    return true;
#endif
}

void ScreenRecorder::releaseCaptureResources()
{
#ifdef Q_OS_WIN
    if (m_memDC && m_oldBitmap) {
        SelectObject(m_memDC, m_oldBitmap);
        m_oldBitmap = nullptr;
    }
    if (m_bitmap) {
        DeleteObject(m_bitmap);
        m_bitmap = nullptr;
    }
    m_bits = nullptr;
    if (m_memDC) {
        DeleteDC(m_memDC);
        m_memDC = nullptr;
    }
    if (m_screenDC) {
        ReleaseDC(nullptr, m_screenDC);
        m_screenDC = nullptr;
    }
#endif
}

QImage ScreenRecorder::grabScreenRegion(const QRect &rect)
{
#ifndef Q_OS_WIN
    QScreen *screen = QGuiApplication::screenAt(rect.center());
    if (!screen) screen = QGuiApplication::primaryScreen();
    if (screen) {
        const QRect sg = screen->geometry();
        QPixmap pix = screen->grabWindow(0,
                                         rect.x() - sg.x(),
                                         rect.y() - sg.y(),
                                         rect.width(),
                                         rect.height());
        if (!pix.isNull()) {
            QImage img = pix.toImage().convertToFormat(QImage::Format_RGB32);
            if (m_outputSize.isValid() && img.size() != m_outputSize)
                img = img.scaled(m_outputSize, Qt::IgnoreAspectRatio, Qt::FastTransformation);
            return img;
        }
    }
#endif

#ifdef Q_OS_WIN
    int sx = rect.x();
    int sy = rect.y();
    int sw = rect.width();
    int sh = rect.height();
    if (sw <= 0 || sh <= 0) return QImage();

    if (!m_screenDC || !m_memDC || !m_bitmap || !m_bits) return QImage();
    BOOL ok = BitBlt(m_memDC, 0, 0, sw, sh, m_screenDC, sx, sy, SRCCOPY | CAPTUREBLT);
    if (!ok) return QImage();

    QImage img(static_cast<uchar *>(m_bits), sw, sh, sw * 4, QImage::Format_RGB32);
    if (m_outputSize.isValid() && m_outputSize != QSize(sw, sh))
        return img.scaled(m_outputSize, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    return img.copy();
#else
    return QImage();
#endif
}

QSize ScreenRecorder::boundedOutputSize(const QSize &sourceSize) const
{
    if (sourceSize.isEmpty()) return sourceSize;

    QSettings s("EShot", "EShot");
    const int maxSide = qBound(320, s.value("recordingMaxSide", 1280).toInt(), 3840);
    if (sourceSize.width() <= maxSide && sourceSize.height() <= maxSide)
        return sourceSize;

    QSize output = sourceSize;
    output.scale(maxSide, maxSide, Qt::KeepAspectRatio);
    output.setWidth(qMax(8, output.width()));
    output.setHeight(qMax(8, output.height()));
    return output;
}
