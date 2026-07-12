#include "VisualSearch.h"

#include <QUrlQuery>

VisualSearchProvider visualSearchProviderFromSettings(const QString &key)
{
    if (key == QStringLiteral("yandex"))
        return VisualSearchProvider::YandexImages;
    return VisualSearchProvider::GoogleLens;
}

QString visualSearchProviderKey(VisualSearchProvider provider)
{
    if (provider == VisualSearchProvider::YandexImages)
        return QStringLiteral("yandex");
    return QStringLiteral("google");
}

QString visualSearchDisplayName(VisualSearchProvider provider)
{
    if (provider == VisualSearchProvider::YandexImages)
        return QStringLiteral("Yandex Images");
    return QStringLiteral("Google Lens");
}

QString visualSearchTooltip(VisualSearchProvider provider)
{
    if (provider == VisualSearchProvider::YandexImages)
        return QStringLiteral("Search with Yandex Images");
    return QStringLiteral("Search with Google Lens");
}

QString visualSearchIconPath(VisualSearchProvider provider)
{
    if (provider == VisualSearchProvider::YandexImages)
        return QStringLiteral(":/icons/yandex_search.svg");
    return QStringLiteral(":/icons/search.svg");
}

QUrl visualSearchResultUrl(VisualSearchProvider provider, const QUrl &imageUrl)
{
    QUrl result(provider == VisualSearchProvider::YandexImages
                    ? QStringLiteral("https://yandex.com/images/search")
                    : QStringLiteral("https://lens.google.com/uploadbyurl"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("url"), imageUrl.toString(QUrl::FullyEncoded));
    result.setQuery(query);
    return result;
}

QSize visualSearchUploadSize(const QSize &sourceSize, int maximumLongEdge)
{
    if (!sourceSize.isValid() || maximumLongEdge <= 0)
        return QSize();
    if (qMax(sourceSize.width(), sourceSize.height()) <= maximumLongEdge)
        return sourceSize;
    return sourceSize.scaled(maximumLongEdge, maximumLongEdge, Qt::KeepAspectRatio);
}
