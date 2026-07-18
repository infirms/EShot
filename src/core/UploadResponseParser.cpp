#include "UploadResponseParser.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace {
bool isTmpFilesHttpsUrl(const QUrl &url)
{
    return url.scheme() == QStringLiteral("https") &&
           url.host().compare(QStringLiteral("tmpfiles.org"), Qt::CaseInsensitive) == 0;
}
}

QUrl tmpFilesLandingUrl(const QByteArray &response)
{
    const QUrl url(QJsonDocument::fromJson(response)
                       .object()
                       .value(QStringLiteral("data"))
                       .toObject()
                       .value(QStringLiteral("url"))
                       .toString());
    return isTmpFilesHttpsUrl(url) ? url : QUrl();
}

QUrl tmpFilesDirectUrl(const QByteArray &html)
{
    static const QRegularExpression directUrlPattern(
        QStringLiteral(R"(https://tmpfiles\.org/dl/[^\s\"'<>]+)"),
        QRegularExpression::CaseInsensitiveOption);
    QString page = QString::fromUtf8(html);
    page.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    const QRegularExpressionMatch match = directUrlPattern.match(page);
    if (!match.hasMatch())
        return {};
    const QUrl url(match.captured());
    return isTmpFilesHttpsUrl(url) ? url : QUrl();
}
