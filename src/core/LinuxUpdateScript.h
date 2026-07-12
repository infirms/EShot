#ifndef LINUXUPDATESCRIPT_H
#define LINUXUPDATESCRIPT_H

#include <QString>

QString buildLinuxUpdateScript(qint64 processId,
                               const QString &currentAppImage,
                               const QString &downloadedAppImage);

#endif
