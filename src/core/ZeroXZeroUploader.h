#ifndef ZEROXZEROUPLOADER_H
#define ZEROXZEROUPLOADER_H

#include "ImageUploader.h"

class QNetworkReply;
class QHttpMultiPart;

class ZeroXZeroUploader : public ImageUploader {
    Q_OBJECT

public:
    explicit ZeroXZeroUploader(QObject *parent = nullptr);
    ~ZeroXZeroUploader() override;

    Provider provider() const override { return Provider::ZeroXZero; }
    QString providerDisplayName() const override;

    void upload() override;
    void cancel() override;

private:
    QNetworkReply *m_reply = nullptr;
    QHttpMultiPart *m_multipart = nullptr;
};

ImageUploader *createZeroXZeroUploader(QObject *parent = nullptr);

#endif
