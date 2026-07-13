#include "LinuxAutoStartPolicy.h"

#include <QFileInfo>

QString LinuxAutoStartPolicy::executablePath(const QString &appImagePath,
                                             const QString &applicationFilePath)
{
    const QFileInfo appImage(appImagePath);
    if (!appImagePath.isEmpty() && appImage.isFile())
        return appImage.absoluteFilePath();

    return applicationFilePath;
}
