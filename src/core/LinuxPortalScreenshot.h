#ifndef LINUXPORTALSCREENSHOT_H
#define LINUXPORTALSCREENSHOT_H

#include <QObject>
#include <QPixmap>
#include <QVariantMap>

class QWidget;
class QEventLoop;
class QScreen;

class LinuxPortalScreenshot : public QObject {
    Q_OBJECT

public:
    static QPixmap grab(QWidget *parent = nullptr, int timeoutMs = 120000);
    static QPixmap grabScreen(QScreen *screen, QWidget *parent = nullptr, int timeoutMs = 5000);
    static QPixmap grabWorkspace(QWidget *parent = nullptr, int timeoutMs = 5000);

private:
    explicit LinuxPortalScreenshot(QObject *parent = nullptr);
    QPixmap waitForScreenshot(QWidget *parent, int timeoutMs);

private slots:
    void onPortalResponse(uint response, const QVariantMap &results);

private:
    QPixmap m_pixmap;
    bool m_finished = false;
    QEventLoop *m_loop = nullptr;
};

#endif
