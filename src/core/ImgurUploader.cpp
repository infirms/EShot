#include "ImgurUploader.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QDebug>

ImgurUploader::ImgurUploader(QObject *parent) : ImageUploader(parent)
{
    QSettings s("EShot", "EShot");
    m_clientId = s.value("imgurClientId").toString();
}

ImgurUploader::~ImgurUploader()
{
    cancel();
}

QString ImgurUploader::providerDisplayName() const
{
    return QStringLiteral("Imgur");
}

void ImgurUploader::setClientId(const QString &id)
{
    m_clientId = id.trimmed();
    QSettings s("EShot", "EShot");
    s.setValue("imgurClientId", m_clientId);
}

void ImgurUploader::upload()
{
    if (m_reply) {
        emit failed(QStringLiteral("upload already in progress"));
        return;
    }
    if (m_clientId.isEmpty()) {
        finishWithError(QStringLiteral("Imgur requires a Client ID. Use Catbox, Uguu, or 0x0.st for no-key upload."));
        return;
    }
    if (!hasImage()) {
        finishWithError(QStringLiteral("image missing"));
        return;
    }

    emit uploading();

    m_multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QStringLiteral("image/png")));
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant(QStringLiteral("form-data; name=\"image\"; filename=\"%1\"")
                                     .arg(QFileInfo(imagePath()).fileName())));
    QFile *file = new QFile(imagePath(), m_multipart);
    if (!file->open(QIODevice::ReadOnly)) {
        delete m_multipart;
        m_multipart = nullptr;
        finishWithError(QStringLiteral("cannot read image"));
        return;
    }
    imagePart.setBodyDevice(file);
    m_multipart->append(imagePart);

    QNetworkRequest req(QUrl(QStringLiteral("https://api.imgur.com/3/image")));
    req.setRawHeader("Authorization", "Client-ID " + m_clientId.toUtf8());
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    m_reply = nam()->post(req, m_multipart);
    m_multipart->setParent(m_reply);

    QPointer<ImgurUploader> self(this);
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
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            finishWithError(QStringLiteral("invalid response"));
            return;
        }
        QJsonObject obj = doc.object();
        if (!obj.value("success").toBool()) {
            QString err = obj.value("data").toObject().value("error").toString();
            finishWithError(err.isEmpty() ? QStringLiteral("upload failed") : err);
            return;
        }
        QString link = obj.value("data").toObject().value("link").toString();
        QString deletehash = obj.value("data").toObject().value("deletehash").toString();
        QString deleteUrl = deletehash.isEmpty()
            ? QString()
            : QStringLiteral("https://imgur.com/delete/%1").arg(deletehash);
        finishWithSuccess(link, deleteUrl);
    });
}

void ImgurUploader::cancel()
{
    if (m_reply) {
        m_reply->abort();
    }
    m_multipart = nullptr;
    m_reply = nullptr;
}

ImageUploader *createImgurUploader(QObject *parent)
{
    return new ImgurUploader(parent);
}
