#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QPixmap>
#include <QDebug>
#include <QSettings>
#include <QPalette>
#include <QList>
#include <QPointer>
#include <QCommandLineParser>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QTimer>
#include <QPainter>

#include "core/HotkeyManager.h"
#include "core/TranslationManager.h"
#include "capture/CaptureOverlay.h"
#include "capture/PinnedWindow.h"
#include "capture/PinManager.h"
#include "ui/SettingsDialog.h"
#include "ui/AboutDialog.h"
#include "ui/FirstRunWizard.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class EShotApp : public QObject {
    Q_OBJECT

public:
    EShotApp(QObject *parent = nullptr) : QObject(parent)
    {
        TranslationManager::init();
        loadSettings();
        setupTrayIcon();
        setupOverlay();
        setupHotkey();
        checkForUpdates();
    }

    ~EShotApp()
    {
        if (m_trayIcon) m_trayIcon->hide();
        if (m_overlay) { delete m_overlay; m_overlay = nullptr; }
    }

public slots:
    void onCaptureRequested()
    {
        if (m_overlay && m_overlay->isVisible()) return;
        if (m_overlay) m_overlay->startCapture();
    }

    void onCaptureCompleted(const QPixmap &pixmap)
    {
        if (m_trayIcon && m_showNotifications) {
            m_trayIcon->showMessage(
                TranslationManager::notifCaptureTitle(),
                TranslationManager::notifCaptureMsg(pixmap.width(), pixmap.height()),
                QSystemTrayIcon::Information, 2000);
        }
    }

    void onCaptureCancelled() {}

    void onTrayActivated(QSystemTrayIcon::ActivationReason reason)
    {
        if (reason == QSystemTrayIcon::DoubleClick) onCaptureRequested();
    }

    void onQuitAction() { qApp->quit(); }

    void onSettingsRequested()
    {
        SettingsDialog dlg;
        if (dlg.exec() == QDialog::Accepted) {
            loadSettings();
            rebuildTrayMenu();
            if (m_overlay) m_overlay->refreshUI();
        }
    }

    void onAboutRequested()
    {
        AboutDialog dlg;
        dlg.exec();
    }

    void onCloseAllPins()
    {
        for (auto &w : m_pinnedWindows) {
            if (w) w->close();
        }
        m_pinnedWindows.clear();
    }

private:
    void loadSettings()
    {
        QSettings s("EShot", "EShot");
        m_showNotifications = s.value("showNotifications", true).toBool();
    }

    void rebuildTrayMenu()
    {
        if (!m_trayMenu) return;
        m_trayMenu->clear();

        QAction *captureAction = m_trayMenu->addAction(QIcon(":/icons/copy.svg"), TranslationManager::trayCapture());
        connect(captureAction, &QAction::triggered, this, &EShotApp::onCaptureRequested);
        QAction *settingsAction = m_trayMenu->addAction(QIcon(":/icons/gear.svg"), TranslationManager::traySettings());
        connect(settingsAction, &QAction::triggered, this, &EShotApp::onSettingsRequested);
        QAction *aboutAction = m_trayMenu->addAction(QIcon(":/icons/pen.svg"), TranslationManager::trayAbout());
        connect(aboutAction, &QAction::triggered, this, &EShotApp::onAboutRequested);
        m_trayMenu->addSeparator();

        // Tüm sabitlemeleri kapat
        QAction *closeAllAction = m_trayMenu->addAction(QIcon(":/icons/close.svg"), TranslationManager::pinnedCloseAll());
        connect(closeAllAction, &QAction::triggered, this, &EShotApp::onCloseAllPins);
        m_trayMenu->addSeparator();

        QAction *quitAction = m_trayMenu->addAction(QIcon(":/icons/close.svg"), TranslationManager::trayQuit());
        connect(quitAction, &QAction::triggered, this, &EShotApp::onQuitAction);

        m_trayIcon->setToolTip(QString("%1 v%2").arg(TranslationManager::appTitle(), QCoreApplication::applicationVersion()));
    }

    void setupTrayIcon()
    {
        m_trayIcon = new QSystemTrayIcon(this);

        QSettings s("EShot", "EShot");
        QString iconStyle = s.value("trayIconStyle", "dark").toString();

        // Tepsi simgesi stili
        QIcon trayIcon;
        if (iconStyle == "light") {
            // Açık tema simgesi - beyaz kalem
            QPixmap pix(":/icons/pen.svg");
            if (!pix.isNull()) {
                QPixmap colored(pix.size());
                colored.fill(Qt::white);
                colored.setMask(pix.createMaskFromColor(Qt::transparent));
                trayIcon = QIcon(colored);
            }
        } else {
            trayIcon = QIcon(":/icons/pen.svg");
        }

        if (trayIcon.isNull()) {
            QPixmap pix(32, 32); pix.fill(Qt::blue);
            trayIcon = QIcon(pix);
        }
        m_trayIcon->setIcon(trayIcon);
        m_trayIcon->setToolTip(QString("%1 v%2").arg(TranslationManager::appTitle(), QCoreApplication::applicationVersion()));

        m_trayMenu = new QMenu();
        rebuildTrayMenu();

        m_trayIcon->setContextMenu(m_trayMenu);
        connect(m_trayIcon, &QSystemTrayIcon::activated, this, &EShotApp::onTrayActivated);
        m_trayIcon->show();
    }

    void setupOverlay()
    {
        m_overlay = new CaptureOverlay();
        connect(m_overlay, &CaptureOverlay::captureCompleted, this, &EShotApp::onCaptureCompleted);
        connect(m_overlay, &CaptureOverlay::captureCancelled, this, &EShotApp::onCaptureCancelled);
        connect(m_overlay, &CaptureOverlay::pinnedWindowCreated, this, [this](PinnedWindow *w) {
            m_pinnedWindows.append(QPointer<PinnedWindow>(w));
        });
    }

    void setupHotkey()
    {
        connect(&HotkeyManager::instance(), &HotkeyManager::captureRequested,
                this, &EShotApp::onCaptureRequested);
    }

    void checkForUpdates()
    {
        QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
        connect(mgr, &QNetworkAccessManager::finished, this, [this, mgr](QNetworkReply *reply) {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray data = reply->readAll();
                QJsonDocument doc = QJsonDocument::fromJson(data);
                if (doc.isObject()) {
                    QJsonObject obj = doc.object();
                    QString latestTag = obj["tag_name"].toString();
                    if (latestTag.startsWith("v") || latestTag.startsWith("V"))
                        latestTag = latestTag.mid(1);
                    QString currentVersion = QCoreApplication::applicationVersion();
                    qDebug() << "[EShot] Update check: current=" << currentVersion << "latest=" << latestTag;
                    if (!latestTag.isEmpty() && latestTag != currentVersion) {
                        qDebug() << "[EShot] Update available:" << latestTag;
                        m_updateAvailable = true;
                        m_latestVersion = latestTag;
                        setTrayIconUpdate();
                    } else {
                        qDebug() << "[EShot] Already up to date";
                    }
                }
            } else {
                qDebug() << "[EShot] Update check failed:" << reply->errorString();
            }
            reply->deleteLater();
            mgr->deleteLater();
        });
        mgr->get(QNetworkRequest(QUrl("https://api.github.com/repos/Benoks/EShot/releases/latest")));
    }

    void setTrayIconUpdate()
    {
        if (!m_trayIcon) return;
        // Sarı kalem ikonu oluştur
        QPixmap base(":/icons/pen.svg");
        if (!base.isNull()) {
            QPixmap yellow(base.size());
            yellow.fill(Qt::transparent);
            QPainter p(&yellow);
            p.setCompositionMode(QPainter::CompositionMode_SourceOver);
            // Orijinal ikonu çiz
            p.drawPixmap(0, 0, base);
            // Sarı renk maskesi uygula
            p.setCompositionMode(QPainter::CompositionMode_SourceIn);
            p.fillRect(yellow.rect(), QColor(255, 200, 0));
            p.end();
            m_trayIcon->setIcon(QIcon(yellow));
        }
        m_trayIcon->setToolTip(QString("%1 v%2 — %3").arg(
            TranslationManager::appTitle(),
            QCoreApplication::applicationVersion(),
            TranslationManager::updateTitle()));
    }

    void setTrayIconNormal()
    {
        if (!m_trayIcon) return;
        QSettings s("EShot", "EShot");
        QString iconStyle = s.value("trayIconStyle", "dark").toString();
        QIcon trayIcon;
        if (iconStyle == "light") {
            QPixmap pix(":/icons/pen.svg");
            if (!pix.isNull()) {
                QPixmap colored(pix.size());
                colored.fill(Qt::white);
                colored.setMask(pix.createMaskFromColor(Qt::transparent));
                trayIcon = QIcon(colored);
            }
        } else {
            trayIcon = QIcon(":/icons/pen.svg");
        }
        if (trayIcon.isNull()) {
            QPixmap pix(32, 32); pix.fill(Qt::blue);
            trayIcon = QIcon(pix);
        }
        m_trayIcon->setIcon(trayIcon);
        m_trayIcon->setToolTip(QString("%1 v%2").arg(TranslationManager::appTitle(), QCoreApplication::applicationVersion()));
    }

    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;
    CaptureOverlay *m_overlay = nullptr;
    bool m_showNotifications = true;
    bool m_updateAvailable = false;
    QString m_latestVersion;
    QList<QPointer<PinnedWindow>> m_pinnedWindows;
};

#include "main.moc"

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#endif
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("EShot");
    app.setApplicationVersion("2.2.0");
    app.setOrganizationName("EShot");
    app.setQuitOnLastWindowClosed(false);
    app.setStyle("Fusion");
    app.setWindowIcon(QIcon(":/icons/pen.svg"));

    // Komut satırı argümanları
    QCommandLineParser parser;
    parser.setApplicationDescription("EShot - Screenshot Tool");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption captureOption("capture", "Capture screenshot immediately.");
    parser.addOption(captureOption);
    QCommandLineOption saveOption("save", "Save screenshot to specified path.", "path");
    parser.addOption(saveOption);
    parser.process(app);

    QSettings settings("EShot", "EShot");
    bool highContrast = settings.value("highContrast", false).toBool();
    bool darkMode = settings.value("darkMode", true).toBool();

    if (highContrast || darkMode) {
        QPalette p;
        if (highContrast) {
            p.setColor(QPalette::Window, Qt::black);
            p.setColor(QPalette::WindowText, Qt::white);
            p.setColor(QPalette::Base, Qt::black);
            p.setColor(QPalette::AlternateBase, QColor(30,30,30));
            p.setColor(QPalette::ToolTipBase, Qt::black);
            p.setColor(QPalette::ToolTipText, Qt::yellow);
            p.setColor(QPalette::Text, Qt::white);
            p.setColor(QPalette::Button, Qt::black);
            p.setColor(QPalette::ButtonText, Qt::white);
            p.setColor(QPalette::BrightText, Qt::red);
            p.setColor(QPalette::Link, Qt::cyan);
            p.setColor(QPalette::Highlight, Qt::yellow);
            p.setColor(QPalette::HighlightedText, Qt::black);
        } else {
            p.setColor(QPalette::Window, QColor(53,53,53));
            p.setColor(QPalette::WindowText, Qt::white);
            p.setColor(QPalette::Base, QColor(42,42,42));
            p.setColor(QPalette::AlternateBase, QColor(66,66,66));
            p.setColor(QPalette::ToolTipBase, QColor(53,53,53));
            p.setColor(QPalette::ToolTipText, Qt::white);
            p.setColor(QPalette::Text, Qt::white);
            p.setColor(QPalette::Button, QColor(53,53,53));
            p.setColor(QPalette::ButtonText, Qt::white);
            p.setColor(QPalette::BrightText, Qt::red);
            p.setColor(QPalette::Link, QColor(42,130,218));
            p.setColor(QPalette::Highlight, QColor(42,130,218));
            p.setColor(QPalette::HighlightedText, Qt::black);
        }
        app.setPalette(p);
    }

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "[EShot] System tray is not available!";
        return 1;
    }

    EShotApp eshotApp;

    // İlk çalıştırma sihirbazı
    if (FirstRunWizard::shouldShow()) {
        FirstRunWizard wizard;
        if (wizard.exec() == QDialog::Accepted) {
            // Sihirbaz tamamlandı
        }
    }

    // Komut satırı işlemleri
    if (parser.isSet(saveOption)) {
        QString savePath = parser.value(saveOption);
        QSettings s("EShot", "EShot");
        s.setValue("savePath", QFileInfo(savePath).absolutePath());
    }
    if (parser.isSet(captureOption) || parser.isSet(saveOption)) {
        QMetaObject::invokeMethod(&eshotApp, "onCaptureRequested", Qt::QueuedConnection);
    }

    return app.exec();
}
