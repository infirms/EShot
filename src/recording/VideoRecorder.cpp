#include "VideoRecorder.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QStringList>
#include <QVector>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#endif

namespace {
QString defaultSaveDirectory()
{
    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (picturesPath.trimmed().isEmpty())
        picturesPath = QDir::homePath();
    return QDir(picturesPath).filePath(QStringLiteral("EShot"));
}

bool containsDevice(const QStringList &devices, const QString &name)
{
    for (const QString &device : devices) {
        if (device.compare(name, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

QStringList dshowAudioDevices(const QString &ffmpegPath)
{
    QProcess process;
    process.setProgram(ffmpegPath);
    process.setArguments({QStringLiteral("-hide_banner"), QStringLiteral("-list_devices"), QStringLiteral("true"),
                          QStringLiteral("-f"), QStringLiteral("dshow"), QStringLiteral("-i"), QStringLiteral("dummy")});
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start();
    if (!process.waitForFinished(1800))
        process.kill();

    const QString output = QString::fromLocal8Bit(process.readAll());
    QStringList devices;
    QRegularExpression re(QStringLiteral("\"([^\"]+)\"\\s*\\(audio\\)"));
    auto it = re.globalMatch(output);
    while (it.hasNext()) {
        const QString name = it.next().captured(1).trimmed();
        if (!name.isEmpty() && !devices.contains(name))
            devices.append(name);
    }
    return devices;
}

bool ffmpegSupportsFormat(const QString &ffmpegPath, const QString &format)
{
    QProcess process;
    process.setProgram(ffmpegPath);
    process.setArguments({QStringLiteral("-hide_banner"), QStringLiteral("-formats")});
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start();
    if (!process.waitForFinished(1800))
        process.kill();
    const QString output = QString::fromLocal8Bit(process.readAll());
    return output.contains(QRegularExpression(QStringLiteral("\\b%1\\b").arg(QRegularExpression::escape(format))));
}

#ifdef Q_OS_WIN
void writeLe16(QFile &file, quint16 value)
{
    char b[2] = { static_cast<char>(value & 0xff), static_cast<char>((value >> 8) & 0xff) };
    file.write(b, 2);
}

void writeLe32(QFile &file, quint32 value)
{
    char b[4] = {
        static_cast<char>(value & 0xff),
        static_cast<char>((value >> 8) & 0xff),
        static_cast<char>((value >> 16) & 0xff),
        static_cast<char>((value >> 24) & 0xff)
    };
    file.write(b, 4);
}

void recordWasapiLoopback(const QString &path, std::atomic_bool *stopFlag)
{
    HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(init) && init != RPC_E_CHANGED_MODE)
        return;

    IMMDeviceEnumerator *enumerator = nullptr;
    IMMDevice *device = nullptr;
    IAudioClient *client = nullptr;
    IAudioCaptureClient *capture = nullptr;
    WAVEFORMATEX *format = nullptr;
    QFile file(path);
    quint32 dataBytes = 0;

    auto cleanup = [&]() {
        if (client) client->Stop();
        if (format) CoTaskMemFree(format);
        if (capture) capture->Release();
        if (client) client->Release();
        if (device) device->Release();
        if (enumerator) enumerator->Release();
        if (SUCCEEDED(init)) CoUninitialize();
    };

    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator), reinterpret_cast<void **>(&enumerator))) ||
        FAILED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device)) ||
        FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void **>(&client))) ||
        FAILED(client->GetMixFormat(&format)) ||
        FAILED(client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK,
                                  10000000, 0, format, nullptr)) ||
        FAILED(client->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void **>(&capture))) ||
        !file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        cleanup();
        return;
    }

    const quint16 fmtSize = static_cast<quint16>(sizeof(WAVEFORMATEX) + format->cbSize);
    file.write("RIFF", 4); writeLe32(file, 0); file.write("WAVE", 4);
    file.write("fmt ", 4); writeLe32(file, fmtSize);
    file.write(reinterpret_cast<const char *>(format), fmtSize);
    file.write("data", 4); writeLe32(file, 0);
    const qint64 riffSizePos = 4;
    const qint64 dataSizePos = 24 + fmtSize;

    if (FAILED(client->Start())) {
        cleanup();
        return;
    }

    while (!stopFlag->load()) {
        Sleep(10);
        UINT32 packetFrames = 0;
        while (SUCCEEDED(capture->GetNextPacketSize(&packetFrames)) && packetFrames > 0) {
            BYTE *data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;
            if (FAILED(capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr)))
                break;
            const quint32 bytes = frames * format->nBlockAlign;
            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                QByteArray zeros(bytes, 0);
                file.write(zeros);
            } else {
                file.write(reinterpret_cast<const char *>(data), bytes);
            }
            dataBytes += bytes;
            capture->ReleaseBuffer(frames);
        }
    }

    file.seek(riffSizePos); writeLe32(file, 36 + fmtSize - 16 + dataBytes);
    file.seek(dataSizePos); writeLe32(file, dataBytes);
    file.close();
    cleanup();
}
#endif
}

VideoRecorder::VideoRecorder(QObject *parent)
    : QObject(parent)
{
}

VideoRecorder::~VideoRecorder()
{
    if (m_recording)
        cancel();
}

void VideoRecorder::start(const QRect &captureRect, int fps, int maxSeconds, int crf,
                          bool desktopAudioEnabled, int desktopVolume,
                          const QString &desktopAudioDevice,
                          bool microphoneEnabled, int microphoneVolume,
                          const QString &microphoneDevice,
                          const QString &outputPath,
                          const QRect &displayRect)
{
    if (m_recording) {
        emit recordingFailed(QStringLiteral("already recording"));
        return;
    }
    if (captureRect.width() < 8 || captureRect.height() < 8) {
        emit recordingFailed(QStringLiteral("region too small"));
        return;
    }

    m_ffmpegPath = ffmpegPath();
    if (m_ffmpegPath.isEmpty()) {
        emit recordingFailed(QStringLiteral("ffmpeg.exe not found"));
        return;
    }
    const QStringList audioDevices = dshowAudioDevices(m_ffmpegPath);

    m_captureRect = captureRect;
    m_displayRect = displayRect;
    m_fps = qBound(1, fps, 60);
    m_maxSeconds = qMax(0, maxSeconds);
    m_crf = qBound(18, crf, 32);
    m_desktopAudioEnabled = desktopAudioEnabled;
    m_desktopVolume = qBound(0, desktopVolume, 100);
    m_desktopAudioDevice = desktopAudioDevice.trimmed();
    if (m_desktopAudioDevice.isEmpty() || m_desktopAudioDevice == QStringLiteral("virtual-audio-capturer"))
        m_desktopAudioDevice = QStringLiteral("__wasapi__");
    m_systemAudioLoopback = m_desktopAudioEnabled && m_desktopAudioDevice == QStringLiteral("__wasapi__");
    if (m_desktopAudioEnabled && m_desktopAudioDevice != QStringLiteral("__wasapi__") && !containsDevice(audioDevices, m_desktopAudioDevice))
        m_desktopAudioEnabled = false;
    m_microphoneEnabled = microphoneEnabled;
    m_microphoneVolume = qBound(0, microphoneVolume, 100);
    m_microphoneDevice = microphoneDevice.trimmed();
    if (m_microphoneDevice.isEmpty() || m_microphoneDevice == QStringLiteral("default")) {
        m_microphoneDevice = audioDevices.isEmpty() ? QString() : audioDevices.first();
    }
    if (m_microphoneEnabled && (m_microphoneDevice.isEmpty() || !containsDevice(audioDevices, m_microphoneDevice)))
        m_microphoneEnabled = false;
    m_lastElapsedSeconds = -1;
    m_outputPath = outputPath.trimmed().isEmpty() ? makeDefaultOutputPath() : outputPath;
    m_pausedMs = 0;
    m_pauseStartedMs = 0;
    m_paused = false;
    m_stopping = false;
    m_canceling = false;

    QDir dir(QFileInfo(m_outputPath).absolutePath());
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        emit recordingFailed(QStringLiteral("cannot create output directory"));
        return;
    }
    m_videoOnlyPath.clear();
    m_audioPath.clear();
    QString ffmpegOutputPath = m_outputPath;
    if (m_systemAudioLoopback) {
        const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss_zzz"));
        m_videoOnlyPath = dir.filePath(QStringLiteral(".eshot_video_%1.mp4").arg(stamp));
        m_audioPath = dir.filePath(QStringLiteral(".eshot_audio_%1.wav").arg(stamp));
        ffmpegOutputPath = m_videoOnlyPath;
        m_audioStop.store(false);
#ifdef Q_OS_WIN
        m_audioThread = std::thread(recordWasapiLoopback, m_audioPath, &m_audioStop);
#endif
    }

    QStringList args;
    args << QStringLiteral("-y")
         << QStringLiteral("-hide_banner")
         << QStringLiteral("-loglevel") << QStringLiteral("error")
         << QStringLiteral("-f") << QStringLiteral("gdigrab")
         << QStringLiteral("-draw_mouse") << QStringLiteral("1")
         << QStringLiteral("-framerate") << QString::number(m_fps)
         << QStringLiteral("-offset_x") << QString::number(captureRect.x())
         << QStringLiteral("-offset_y") << QString::number(captureRect.y())
         << QStringLiteral("-video_size") << QStringLiteral("%1x%2").arg(captureRect.width()).arg(captureRect.height())
         << QStringLiteral("-i") << QStringLiteral("desktop");

    struct AudioInput {
        int inputIndex;
        int volume;
    };
    QVector<AudioInput> audioInputs;
    int nextInputIndex = 1;
    if (m_desktopAudioEnabled && !m_systemAudioLoopback && m_desktopVolume > 0) {
        if (m_desktopAudioDevice == QStringLiteral("__wasapi__")) {
            args << QStringLiteral("-f") << QStringLiteral("wasapi")
                 << QStringLiteral("-i") << QStringLiteral("default");
        } else {
            args << QStringLiteral("-f") << QStringLiteral("dshow")
                 << QStringLiteral("-i") << QStringLiteral("audio=%1").arg(m_desktopAudioDevice);
        }
        audioInputs.append({nextInputIndex++, m_desktopVolume});
    }
    if (m_microphoneEnabled && m_microphoneVolume > 0) {
        args << QStringLiteral("-f") << QStringLiteral("dshow")
             << QStringLiteral("-i") << QStringLiteral("audio=%1").arg(m_microphoneDevice);
        audioInputs.append({nextInputIndex++, m_microphoneVolume});
    }

    args
         << QStringLiteral("-vf") << QStringLiteral("pad=ceil(iw/2)*2:ceil(ih/2)*2")
         << QStringLiteral("-c:v") << QStringLiteral("libx264")
         << QStringLiteral("-preset") << QStringLiteral("veryfast")
         << QStringLiteral("-crf") << QString::number(m_crf)
         << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p");
    if (audioInputs.size() == 1) {
        const AudioInput input = audioInputs.first();
        args << QStringLiteral("-filter_complex")
             << QStringLiteral("[%1:a]volume=%2[aout]")
                    .arg(input.inputIndex)
                    .arg(QString::number(input.volume / 100.0, 'f', 2))
             << QStringLiteral("-map") << QStringLiteral("0:v")
             << QStringLiteral("-map") << QStringLiteral("[aout]")
             << QStringLiteral("-c:a") << QStringLiteral("aac")
             << QStringLiteral("-b:a") << QStringLiteral("160k");
    } else if (audioInputs.size() == 2) {
        args << QStringLiteral("-filter_complex")
             << QStringLiteral("[%1:a]volume=%2[a1];[%3:a]volume=%4[a2];[a1][a2]amix=inputs=2:duration=longest[aout]")
                    .arg(audioInputs[0].inputIndex)
                    .arg(QString::number(audioInputs[0].volume / 100.0, 'f', 2))
                    .arg(audioInputs[1].inputIndex)
                    .arg(QString::number(audioInputs[1].volume / 100.0, 'f', 2))
             << QStringLiteral("-map") << QStringLiteral("0:v")
             << QStringLiteral("-map") << QStringLiteral("[aout]")
             << QStringLiteral("-c:a") << QStringLiteral("aac")
             << QStringLiteral("-b:a") << QStringLiteral("160k");
    }
    args << ffmpegOutputPath;

    m_process = new QProcess(this);
    m_process->setProgram(m_ffmpegPath);
    m_process->setArguments(args);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::finished, this, &VideoRecorder::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        if (!m_recording)
            return;
        const bool canceled = m_canceling;
        const QString reason = m_process ? m_process->errorString() : QStringLiteral("ffmpeg process error");
        const QString output = m_outputPath;
        stopSystemAudioCapture();
        cleanupProcess();
        if (!output.isEmpty())
            QFile::remove(output);
        if (!canceled)
            emit recordingFailed(reason);
    });

    m_process->start();
    if (!m_process->waitForStarted(3000)) {
        const QString reason = m_process->errorString();
        cleanupProcess();
        emit recordingFailed(reason.isEmpty() ? QStringLiteral("cannot start ffmpeg") : reason);
        return;
    }

    m_recording = true;
    m_elapsed.start();
    emit recordingStarted();
    emit remainingTimeChanged(m_maxSeconds > 0 ? m_maxSeconds : -1);
    emit elapsedTimeChanged(0);

    m_countdownTimer = new QTimer(this);
    m_countdownTimer->setInterval(500);
    connect(m_countdownTimer, &QTimer::timeout, this, [this]() {
        if (!m_recording || m_paused)
            return;
        const int elapsed = static_cast<int>(activeElapsedMs() / 1000);
        if (elapsed != m_lastElapsedSeconds) {
            m_lastElapsedSeconds = elapsed;
            emit elapsedTimeChanged(elapsed);
        }
        if (m_maxSeconds > 0) {
            const int remaining = qMax(0, m_maxSeconds - elapsed);
            emit remainingTimeChanged(remaining);
            if (remaining == 0)
                stop();
        }
    });
    m_countdownTimer->start();
}

void VideoRecorder::stop()
{
    if (!m_recording || !m_process)
        return;
    if (m_paused)
        resume();
    m_stopping = true;
    m_process->write("q\n");
    QTimer::singleShot(2500, this, [this]() {
        if (m_process && m_recording)
            m_process->terminate();
    });
    QTimer::singleShot(5000, this, [this]() {
        if (m_process && m_recording)
            m_process->kill();
    });
}

void VideoRecorder::cancel()
{
    if (!m_recording)
        return;
    m_canceling = true;
    if (m_paused)
        resume();
    if (m_process) {
        m_process->kill();
    } else {
        stopSystemAudioCapture();
        cleanupProcess();
    }
}

void VideoRecorder::pause()
{
    if (!m_recording || m_paused || !m_process)
        return;
    m_pauseStartedMs = m_elapsed.elapsed();
    if (setProcessSuspended(true)) {
        m_paused = true;
        emit pausedChanged(true);
    }
}

void VideoRecorder::resume()
{
    if (!m_recording || !m_paused || !m_process)
        return;
    if (setProcessSuspended(false)) {
        m_pausedMs += qMax<qint64>(0, m_elapsed.elapsed() - m_pauseStartedMs);
        m_pauseStartedMs = 0;
        m_paused = false;
        emit pausedChanged(false);
    }
}

void VideoRecorder::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    if (!m_recording && !m_process)
        return;

    const bool canceled = m_canceling;
    const bool expectedStop = m_stopping || (m_maxSeconds > 0 && activeElapsedMs() / 1000 >= m_maxSeconds);
    const QString output = m_outputPath;
    const QString stderrText = m_process ? QString::fromLocal8Bit(m_process->readAll()).trimmed() : QString();

    stopSystemAudioCapture();

    if (canceled) {
        if (!output.isEmpty())
            QFile::remove(output);
        if (!m_videoOnlyPath.isEmpty())
            QFile::remove(m_videoOnlyPath);
        if (!m_audioPath.isEmpty())
            QFile::remove(m_audioPath);
        cleanupProcess();
        return;
    }

    bool ok = status == QProcess::NormalExit && (exitCode == 0 || expectedStop) &&
        QFileInfo(m_systemAudioLoopback ? m_videoOnlyPath : output).exists();
    if (!ok) {
        if (!output.isEmpty())
            QFile::remove(output);
        if (!m_videoOnlyPath.isEmpty())
            QFile::remove(m_videoOnlyPath);
        if (!m_audioPath.isEmpty())
            QFile::remove(m_audioPath);
        cleanupProcess();
        emit recordingFailed(stderrText.isEmpty() ? QStringLiteral("ffmpeg exited with code %1").arg(exitCode) : stderrText);
        return;
    }

    if (m_systemAudioLoopback)
        ok = muxSystemAudio();

    cleanupProcess();
    if (!ok) {
        emit recordingFailed(QStringLiteral("failed to mux system audio"));
        return;
    }

    emit recordingStopped(output);
}

void VideoRecorder::stopSystemAudioCapture()
{
    m_audioStop.store(true);
    if (m_audioThread.joinable())
        m_audioThread.join();
}

bool VideoRecorder::muxSystemAudio()
{
    if (m_videoOnlyPath.isEmpty() || m_audioPath.isEmpty())
        return false;
    if (!QFileInfo::exists(m_videoOnlyPath))
        return false;
    if (!QFileInfo::exists(m_audioPath) || QFileInfo(m_audioPath).size() < 128) {
        QFile::rename(m_videoOnlyPath, m_outputPath);
        QFile::remove(m_audioPath);
        return QFileInfo::exists(m_outputPath);
    }

    QProcess mux;
    mux.setProgram(m_ffmpegPath);
    QStringList args = {
        QStringLiteral("-y"),
        QStringLiteral("-hide_banner"),
        QStringLiteral("-loglevel"), QStringLiteral("error"),
        QStringLiteral("-i"), m_videoOnlyPath,
        QStringLiteral("-i"), m_audioPath
    };

    QProcess probe;
    probe.setProgram(m_ffmpegPath);
    probe.setArguments({QStringLiteral("-hide_banner"), QStringLiteral("-i"), m_videoOnlyPath});
    probe.setProcessChannelMode(QProcess::MergedChannels);
    probe.start();
    probe.waitForFinished(5000);
    const bool videoHasAudio = QString::fromLocal8Bit(probe.readAll()).contains(QStringLiteral("Audio:"), Qt::CaseInsensitive);

    if (videoHasAudio) {
        args << QStringLiteral("-filter_complex")
             << QStringLiteral("[0:a][1:a]amix=inputs=2:duration=longest[aout]")
             << QStringLiteral("-map") << QStringLiteral("0:v")
             << QStringLiteral("-map") << QStringLiteral("[aout]");
    } else {
        args << QStringLiteral("-map") << QStringLiteral("0:v")
             << QStringLiteral("-map") << QStringLiteral("1:a");
    }
    args << QStringLiteral("-c:v") << QStringLiteral("copy")
         << QStringLiteral("-c:a") << QStringLiteral("aac")
         << QStringLiteral("-shortest")
         << m_outputPath;
    mux.setArguments(args);
    mux.start();
    const bool finished = mux.waitForFinished(30000);
    if (!(finished && mux.exitStatus() == QProcess::NormalExit && mux.exitCode() == 0 && QFileInfo::exists(m_outputPath))) {
        QFile::remove(m_outputPath);
        QFile::rename(m_videoOnlyPath, m_outputPath);
    } else {
        QFile::remove(m_videoOnlyPath);
    }
    QFile::remove(m_audioPath);
    return QFileInfo::exists(m_outputPath);
}

QString VideoRecorder::ffmpegPath() const
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir(appDir).filePath(QStringLiteral("ffmpeg/ffmpeg.exe")),
        QDir(appDir).filePath(QStringLiteral("ffmpeg.exe")),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../third_party/ffmpeg/bin/ffmpeg.exe")),
        QDir::current().filePath(QStringLiteral("third_party/ffmpeg/bin/ffmpeg.exe"))
    };
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path))
            return QFileInfo(path).absoluteFilePath();
    }
    return QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
}

QString VideoRecorder::makeDefaultOutputPath() const
{
    QSettings s("EShot", "EShot");
    QStringList candidates;
    QString configuredDir = s.contains("videoSavePath")
        ? s.value("videoSavePath").toString().trimmed()
        : QDir(defaultSaveDirectory()).filePath(QStringLiteral("Videos"));
    if (configuredDir.isEmpty())
        configuredDir = s.value("savePath").toString().trimmed();
    candidates << (configuredDir.isEmpty() ? defaultSaveDirectory() : configuredDir)
               << QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
               << QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
               << QStandardPaths::writableLocation(QStandardPaths::TempLocation)
               << QDir::homePath();

    const QString fileName = QStringLiteral("EShot_Video_%1.mp4")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss")));

    for (const QString &candidate : candidates) {
        if (candidate.trimmed().isEmpty())
            continue;
        QDir dir(candidate);
        if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
            continue;
        const QString probePath = dir.filePath(QStringLiteral(".eshot_write_test.tmp"));
        QFile probe(probePath);
        if (!probe.open(QIODevice::WriteOnly | QIODevice::Truncate))
            continue;
        probe.close();
        QFile::remove(probePath);
        return dir.filePath(fileName);
    }
    return QDir(QDir::tempPath()).filePath(fileName);
}

qint64 VideoRecorder::activeElapsedMs() const
{
    if (!m_elapsed.isValid())
        return 0;
    qint64 paused = m_pausedMs;
    if (m_paused)
        paused += qMax<qint64>(0, m_elapsed.elapsed() - m_pauseStartedMs);
    return qMax<qint64>(0, m_elapsed.elapsed() - paused);
}

bool VideoRecorder::setProcessSuspended(bool suspended)
{
#ifdef Q_OS_WIN
    if (!m_process)
        return false;
    const DWORD pid = static_cast<DWORD>(m_process->processId());
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return false;

    THREADENTRY32 entry;
    entry.dwSize = sizeof(THREADENTRY32);
    bool touched = false;
    if (Thread32First(snapshot, &entry)) {
        do {
            if (entry.th32OwnerProcessID != pid)
                continue;
            HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, entry.th32ThreadID);
            if (!thread)
                continue;
            if (suspended)
                SuspendThread(thread);
            else
                ResumeThread(thread);
            CloseHandle(thread);
            touched = true;
        } while (Thread32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return touched;
#else
    Q_UNUSED(suspended)
    return false;
#endif
}

void VideoRecorder::cleanupProcess()
{
    m_recording = false;
    m_paused = false;
    m_stopping = false;
    m_canceling = false;
    if (m_countdownTimer) {
        m_countdownTimer->stop();
        m_countdownTimer->deleteLater();
        m_countdownTimer = nullptr;
    }
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
}
