#include "UguuUploader.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

UguuUploader::UguuUploader(QObject *parent) : ImageUploader(parent) {}

UguuUploader::~UguuUploader()
{
    cancel();
}

QString UguuUploader::providerDisplayName() const
{
    return QStringLiteral("Uguu.se");
}

void UguuUploader::upload()
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

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QStringLiteral("image/png")));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QStringLiteral("form-data; name=\"files[]\"; filename=\"%1\"")
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

    QNetworkRequest req(QUrl(QStringLiteral("https://uguu.se/upload")));
    req.setRawHeader("User-Agent", "EShot/2.4");
    req.setTransferTimeout(60000);

    m_reply = nam()->post(req, m_multipart);
    m_multipart->setParent(m_reply);

    QPointer<UguuUploader> self(this);
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
        if (code < 200 || code >= 300) {
            finishWithError(QStringLiteral("http %1").arg(code));
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(data);
        QString url;
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            QJsonArray files = obj.value(QStringLiteral("files")).toArray();
            if (!files.isEmpty())
                url = files.first().toObject().value(QStringLiteral("url")).toString();
        }

        if (url.isEmpty() || !url.startsWith(QStringLiteral("https://"))) {
            QString text = QString::fromUtf8(data).trimmed();
            finishWithError(QStringLiteral("unexpected response: ") + text.left(120));
            return;
        }
        finishWithSuccess(url);
    });
}

void UguuUploader::cancel()
{
    if (m_reply) {
        m_reply->abort();
    }
    m_multipart = nullptr;
    m_reply = nullptr;
}

ImageUploader *createUguuUploader(QObject *parent)
{
    return new UguuUploader(parent);
}
