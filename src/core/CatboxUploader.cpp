#include "CatboxUploader.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QSettings>
#include <QDebug>

CatboxUploader::CatboxUploader(QObject *parent) : ImageUploader(parent)
{
    QSettings s("EShot", "EShot");
    m_userHash = s.value("catboxUserHash").toString();
}

CatboxUploader::~CatboxUploader()
{
    cancel();
}

QString CatboxUploader::providerDisplayName() const
{
    return QStringLiteral("Catbox.moe");
}

QString CatboxUploader::authPlaceholder() const
{
    return QStringLiteral("Optional Catbox user hash");
}

void CatboxUploader::setUserHash(const QString &hash)
{
    m_userHash = hash.trimmed();
    QSettings s("EShot", "EShot");
    s.setValue("catboxUserHash", m_userHash);
}

void CatboxUploader::upload()
{
    if (m_reply) {
        emit failed(QStringLiteral("upload already in progress"));
        return;
    }
    if (!hasImage()) {
        finishWithError(QStringLiteral("image missing"));
        return;
    }

    emit uploading();

    m_multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart reqtypePart;
    reqtypePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                          QVariant(QStringLiteral("form-data; name=\"reqtype\"")));
    reqtypePart.setBody("fileupload");
    m_multipart->append(reqtypePart);

    if (!m_userHash.isEmpty()) {
        QHttpPart userhashPart;
        userhashPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                               QVariant(QStringLiteral("form-data; name=\"userhash\"")));
        userhashPart.setBody(m_userHash.toUtf8());
        m_multipart->append(userhashPart);
    }

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QStringLiteral("image/png")));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QStringLiteral("form-data; name=\"fileToUpload\"; filename=\"%1\"")
                                    .arg(QFileInfo(imagePath()).fileName())));
    QFile *file = new QFile(imagePath(), m_multipart);
    if (!file->open(QIODevice::ReadOnly)) {
        delete m_multipart;
        m_multipart = nullptr;
        finishWithError(QStringLiteral("cannot read image"));
        return;
    }
    filePart.setBodyDevice(file);
    m_multipart->append(filePart);

    QNetworkRequest req(QUrl(QStringLiteral("https://catbox.moe/user/api.php")));
    req.setRawHeader("User-Agent", "EShot/3.0");
    req.setTransferTimeout(60000);

    m_reply = nam()->post(req, m_multipart);
    m_multipart->setParent(m_reply);

    QPointer<CatboxUploader> self(this);
    connect(m_reply, &QNetworkReply::finished, this, [this, self]() {
        if (!self) return;
        QByteArray data = m_reply->readAll();
        int code = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QNetworkReply::NetworkError err = m_reply->error();
        QString errStr = m_reply->errorString();

        QNetworkReply *r = m_reply;
        QHttpMultiPart *mp = m_multipart;
        m_reply = nullptr;
        m_multipart = nullptr;
        r->deleteLater();
        if (mp) mp->deleteLater();

        if (err != QNetworkReply::NoError) {
            finishWithError(QStringLiteral("network error: %1").arg(errStr));
            return;
        }
        if (code != 200) {
            finishWithError(QStringLiteral("http %1").arg(code));
            return;
        }
        QString url = QString::fromUtf8(data).trimmed();
        if (url.isEmpty() || !url.startsWith(QStringLiteral("https://"))) {
            finishWithError(QStringLiteral("unexpected response: ") + url.left(120));
            return;
        }
        finishWithSuccess(url, QString());
    });
}

void CatboxUploader::cancel()
{
    if (m_reply) {
        QNetworkReply *reply = m_reply;
        QHttpMultiPart *multipart = m_multipart;
        m_reply = nullptr;
        m_multipart = nullptr;
        disconnect(reply, nullptr, this, nullptr);
        reply->abort();
        reply->deleteLater();
        if (multipart && multipart->parent() != reply)
            multipart->deleteLater();
        return;
    }
    if (m_multipart) {
        m_multipart->deleteLater();
        m_multipart = nullptr;
    }
}

ImageUploader *createCatboxUploader(QObject *parent)
{
    return new CatboxUploader(parent);
}
