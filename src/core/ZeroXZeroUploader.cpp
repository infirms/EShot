#include "ZeroXZeroUploader.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>
#include <QPointer>

ZeroXZeroUploader::ZeroXZeroUploader(QObject *parent) : ImageUploader(parent) {}

ZeroXZeroUploader::~ZeroXZeroUploader()
{
    cancel();
}

QString ZeroXZeroUploader::providerDisplayName() const
{
    return QStringLiteral("0x0.st");
}

void ZeroXZeroUploader::upload()
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
                       QVariant(QStringLiteral("form-data; name=\"file\"; filename=\"%1\"")
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

    QNetworkRequest req(QUrl(QStringLiteral("https://0x0.st")));
    req.setRawHeader("User-Agent", "EShot/2.4");
    req.setTransferTimeout(60000);

    m_reply = nam()->post(req, m_multipart);
    m_multipart->setParent(m_reply);

    QPointer<ZeroXZeroUploader> self(this);
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

        QString url = QString::fromUtf8(data).trimmed();
        if (url.isEmpty() || !url.startsWith(QStringLiteral("https://"))) {
            finishWithError(QStringLiteral("unexpected response: ") + url.left(120));
            return;
        }
        finishWithSuccess(url);
    });
}

void ZeroXZeroUploader::cancel()
{
    if (m_reply) {
        m_reply->abort();
    }
    m_multipart = nullptr;
    m_reply = nullptr;
}

ImageUploader *createZeroXZeroUploader(QObject *parent)
{
    return new ZeroXZeroUploader(parent);
}
