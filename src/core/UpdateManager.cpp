#include "UpdateManager.h"
#include "TranslationManager.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QVersionNumber>

namespace {
bool isNewerVersion(const QString &latest, const QString &current)
{
    QVersionNumber latestVersion = QVersionNumber::fromString(latest.trimmed());
    QVersionNumber currentVersion = QVersionNumber::fromString(current.trimmed());
    if (latestVersion.isNull() || currentVersion.isNull())
        return latest.trimmed() != current.trimmed();
    return QVersionNumber::compare(latestVersion, currentVersion) > 0;
}

QString psSingleQuote(QString value)
{
    value.replace('\'', "''");
    return QStringLiteral("'") + value + QStringLiteral("'");
}
}

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
    setStatus(TranslationManager::updateStatusIdle());
}

UpdateManager::~UpdateManager()
{
    if (m_checkReply) m_checkReply->abort();
    if (m_downloadReply) m_downloadReply->abort();
    if (m_downloadFile) {
        m_downloadFile->close();
        delete m_downloadFile;
    }
}

void UpdateManager::setStatus(const QString &status)
{
    if (m_statusText == status)
        return;
    m_statusText = status;
    emit statusChanged();
}

void UpdateManager::checkForUpdates(bool manual)
{
    if (isBusy())
        return;

    m_checking = true;
    setStatus(TranslationManager::updateStatusChecking());

    QUrl url(QStringLiteral("https://api.github.com/repos/Benoks/EShot/releases/latest"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("_t"), QString::number(QDateTime::currentMSecsSinceEpoch()));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", "EShot-Updater");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_checkReply = m_network->get(request);
    connect(m_checkReply, &QNetworkReply::finished, this, [this, manual]() {
        QNetworkReply *reply = m_checkReply;
        m_checkReply = nullptr;
        m_checking = false;

        if (reply->error() != QNetworkReply::NoError) {
            const QString msg = reply->errorString();
            reply->deleteLater();
            m_installAfterCheck = false;
            setStatus(TranslationManager::updateStatusFailed(msg));
            emit failed(msg);
            return;
        }

        const QByteArray data = reply->readAll();
        reply->deleteLater();
        parseRelease(data, manual);
    });
}

void UpdateManager::parseRelease(const QByteArray &data, bool manual)
{
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        const QString msg = TranslationManager::updateInvalidResponse();
        m_installAfterCheck = false;
        setStatus(TranslationManager::updateStatusFailed(msg));
        emit failed(msg);
        return;
    }

    const QJsonObject obj = doc.object();
    QString latestTag = obj.value(QStringLiteral("tag_name")).toString();
    if (latestTag.startsWith('v', Qt::CaseInsensitive))
        latestTag = latestTag.mid(1);

    m_latestVersion = latestTag;
    m_releaseUrl = obj.value(QStringLiteral("html_url")).toString();
    m_installerUrl = selectInstallerUrl(obj.value(QStringLiteral("assets")).toArray(), &m_installerName, &m_installerSize);

    const QString currentVersion = QCoreApplication::applicationVersion();
    m_updateAvailable = !latestTag.isEmpty() && isNewerVersion(latestTag, currentVersion);

    if (m_updateAvailable) {
        setStatus(TranslationManager::updateStatusAvailable(latestTag));
    } else {
        if (manual)
            setStatus(TranslationManager::updateStatusUpToDate());
        else
            setStatus(TranslationManager::updateStatusIdle());
    }

    emit updateCheckFinished(m_updateAvailable, m_latestVersion);
    emit statusChanged();

    if (m_installAfterCheck) {
        m_installAfterCheck = false;
        if (m_updateAvailable)
            QTimer::singleShot(0, this, &UpdateManager::installUpdate);
    }
}

QString UpdateManager::selectInstallerUrl(const QJsonArray &assets, QString *assetName, qint64 *assetSize) const
{
    const QString arch = QSysInfo::currentCpuArchitecture().toLower();
    const QString wanted = arch.contains(QStringLiteral("arm")) ? QStringLiteral("arm64") : QStringLiteral("x64");

    for (const QJsonValue &value : assets) {
        const QJsonObject asset = value.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        const QString lower = name.toLower();
        if (!lower.endsWith(QStringLiteral(".exe")))
            continue;
        if (!lower.contains(QStringLiteral("setup")))
            continue;
        if (!lower.contains(wanted))
            continue;

        if (assetName) *assetName = name;
        if (assetSize) *assetSize = static_cast<qint64>(asset.value(QStringLiteral("size")).toDouble());
        return asset.value(QStringLiteral("browser_download_url")).toString();
    }

    return QString();
}

QString UpdateManager::updateCacheDir() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (dir.trimmed().isEmpty())
        dir = QDir::tempPath() + QStringLiteral("/EShot");
    dir = QDir(dir).filePath(QStringLiteral("updates"));
    QDir().mkpath(dir);
    return dir;
}

void UpdateManager::installUpdate()
{
    if (isBusy())
        return;
    if (!m_updateAvailable) {
        m_installAfterCheck = true;
        checkForUpdates(true);
        return;
    }
    if (m_installerUrl.isEmpty()) {
        const QString msg = TranslationManager::updateNoInstaller();
        setStatus(TranslationManager::updateStatusFailed(msg));
        emit failed(msg);
        return;
    }
    downloadInstaller();
}

void UpdateManager::downloadInstaller()
{
    m_downloading = true;
    setStatus(TranslationManager::updateStatusDownloading());

    const QString fileName = m_installerName.isEmpty()
        ? QStringLiteral("EShot_Update_%1.exe").arg(m_latestVersion)
        : m_installerName;
    const QString path = QDir(updateCacheDir()).filePath(fileName);
    m_downloadFile = new QFile(path);
    if (!m_downloadFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        const QString msg = m_downloadFile->errorString();
        delete m_downloadFile;
        m_downloadFile = nullptr;
        m_downloading = false;
        setStatus(TranslationManager::updateStatusFailed(msg));
        emit failed(msg);
        return;
    }

    QNetworkRequest request{QUrl(m_installerUrl)};
    request.setRawHeader("User-Agent", "EShot-Updater");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    m_downloadReply = m_network->get(request);

    connect(m_downloadReply, &QNetworkReply::readyRead, this, [this]() {
        if (m_downloadFile && m_downloadReply)
            m_downloadFile->write(m_downloadReply->readAll());
    });
    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, &UpdateManager::downloadProgress);
    connect(m_downloadReply, &QNetworkReply::finished, this, &UpdateManager::finishDownload);
}

void UpdateManager::finishDownload()
{
    QNetworkReply *reply = m_downloadReply;
    m_downloadReply = nullptr;
    m_downloading = false;

    if (m_downloadFile && reply)
        m_downloadFile->write(reply->readAll());

    const QString path = m_downloadFile ? m_downloadFile->fileName() : QString();
    if (m_downloadFile) {
        m_downloadFile->flush();
        m_downloadFile->close();
    }

    if (!reply || reply->error() != QNetworkReply::NoError) {
        const QString msg = reply ? reply->errorString() : TranslationManager::updateInvalidResponse();
        if (m_downloadFile) {
            delete m_downloadFile;
            m_downloadFile = nullptr;
        }
        if (!path.isEmpty())
            QFile::remove(path);
        if (reply) reply->deleteLater();
        setStatus(TranslationManager::updateStatusFailed(msg));
        emit failed(msg);
        return;
    }
    reply->deleteLater();

    const qint64 size = QFileInfo(path).size();
    if (size <= 0 || (m_installerSize > 0 && size != m_installerSize)) {
        if (m_downloadFile) {
            delete m_downloadFile;
            m_downloadFile = nullptr;
        }
        QFile::remove(path);
        const QString msg = TranslationManager::updateInvalidDownload();
        setStatus(TranslationManager::updateStatusFailed(msg));
        emit failed(msg);
        return;
    }

    delete m_downloadFile;
    m_downloadFile = nullptr;
    launchInstaller(path);
}

void UpdateManager::launchInstaller(const QString &installerPath)
{
    m_installing = true;
    setStatus(TranslationManager::updateStatusInstalling());

    const QString exePath = QCoreApplication::applicationFilePath();
    const QString scriptPath = QDir(updateCacheDir()).filePath(
        QStringLiteral("install_update_%1.ps1").arg(QDateTime::currentMSecsSinceEpoch()));
    const QString logPath = QDir(updateCacheDir()).filePath(QStringLiteral("install_update.log"));

    QString script;
    script += QStringLiteral("$ErrorActionPreference = 'SilentlyContinue'\r\n");
    script += QStringLiteral("$pidToWait = %1\r\n").arg(QCoreApplication::applicationPid());
    script += QStringLiteral("$installer = %1\r\n").arg(psSingleQuote(QDir::toNativeSeparators(installerPath)));
    script += QStringLiteral("$exe = %1\r\n").arg(psSingleQuote(QDir::toNativeSeparators(exePath)));
    script += QStringLiteral("$log = %1\r\n").arg(psSingleQuote(QDir::toNativeSeparators(logPath)));
    script += QStringLiteral("try { Wait-Process -Id $pidToWait -Timeout 45 } catch {}\r\n");
    script += QStringLiteral("$args = @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-',('/LOG=' + $log))\r\n");
    script += QStringLiteral("$p = Start-Process -FilePath $installer -ArgumentList $args -Wait -PassThru\r\n");
    script += QStringLiteral("if ($p.ExitCode -eq 0) { Start-Sleep -Milliseconds 800; if (Test-Path $exe) { Start-Process -FilePath $exe -ArgumentList '--silent' } }\r\n");
    script += QStringLiteral("Remove-Item -LiteralPath $installer -Force\r\n");
    script += QStringLiteral("Remove-Item -LiteralPath $PSCommandPath -Force\r\n");

    QFile file(scriptPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        const QString msg = file.errorString();
        m_installing = false;
        setStatus(TranslationManager::updateStatusFailed(msg));
        emit failed(msg);
        return;
    }
    file.write(script.toUtf8());
    file.close();

    const QString program = QStringLiteral("powershell.exe");
    const QStringList args = {
        QStringLiteral("-NoProfile"),
        QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
        QStringLiteral("-WindowStyle"), QStringLiteral("Hidden"),
        QStringLiteral("-File"), scriptPath
    };

    if (!QProcess::startDetached(program, args)) {
        m_installing = false;
        const QString msg = TranslationManager::updateCannotLaunchInstaller();
        setStatus(TranslationManager::updateStatusFailed(msg));
        emit failed(msg);
        return;
    }

    setStatus(TranslationManager::updateStatusRestarting());
    emit installerLaunched();
    QTimer::singleShot(500, qApp, &QCoreApplication::quit);
}
