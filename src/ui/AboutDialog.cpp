#include "AboutDialog.h"
#include "../core/TranslationManager.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QIcon>

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(TranslationManager::aboutTitle());
    setWindowIcon(QIcon(":/icons/pen.svg"));
    setFixedSize(340, 340);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setupUI();
}

AboutDialog::~AboutDialog() {}

void AboutDialog::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(24, 20, 24, 20);

    // SVG ikon — high-DPI render
    QLabel *iconLabel = new QLabel();
    QIcon appIcon(":/icons/pen.svg");
    if (!appIcon.isNull()) {
        QPixmap pix = appIcon.pixmap(96, 96);
        iconLabel->setPixmap(pix);
    }
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    // İsim
    QLabel *nameLabel = new QLabel("EShot");
    QFont nameFont = nameLabel->font();
    nameFont.setPointSize(18);
    nameFont.setBold(true);
    nameLabel->setFont(nameFont);
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setStyleSheet("color: #ffffff;");
    layout->addWidget(nameLabel);

    // Sürüm
    QLabel *verLabel = new QLabel(QString("%1 %2").arg(TranslationManager::version(), QApplication::applicationVersion()));
    verLabel->setAlignment(Qt::AlignCenter);
    verLabel->setStyleSheet("color: #999999; font-size: 11px; margin-bottom: 2px;");
    layout->addWidget(verLabel);

    // Tanım
    QLabel *descLabel = new QLabel(TranslationManager::aboutDesc());
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setStyleSheet("color: #bbbbbb; font-size: 12px;");
    layout->addWidget(descLabel);

    // GitHub linki
    QLabel *linkLabel = new QLabel(
        "<a href='https://github.com/Benoks/EShot' style='color: #5B9BD5; text-decoration: none;'>"
        "GitHub</a>"
    );
    linkLabel->setAlignment(Qt::AlignCenter);
    linkLabel->setOpenExternalLinks(true);
    linkLabel->setStyleSheet("font-size: 11px; margin-top: 4px;");
    layout->addWidget(linkLabel);

    layout->addStretch();

    // Güncelleme durumu
    QLabel *updateStatusLabel = new QLabel();
    updateStatusLabel->setAlignment(Qt::AlignCenter);
    updateStatusLabel->setStyleSheet("color: #888888; font-size: 11px;");
    updateStatusLabel->setMinimumHeight(14);
    layout->addWidget(updateStatusLabel);

    // Güncelleme kontrolü
    QPushButton *checkUpdateBtn = new QPushButton(TranslationManager::checkForUpdates());
    checkUpdateBtn->setCursor(Qt::PointingHandCursor);
    checkUpdateBtn->setFixedHeight(32);
    checkUpdateBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #0078D4;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 6px 16px;
            font-weight: bold;
            font-size: 12px;
        }
        QPushButton:hover { background-color: #1a8cff; }
        QPushButton:disabled { background-color: #555; color: #aaa; }
    )");
    connect(checkUpdateBtn, &QPushButton::clicked, this, [checkUpdateBtn, updateStatusLabel]() {
        checkUpdateBtn->setEnabled(false);
        checkUpdateBtn->setText("...");

        QNetworkAccessManager *mgr = new QNetworkAccessManager(checkUpdateBtn);
        connect(mgr, &QNetworkAccessManager::finished, checkUpdateBtn, [mgr, checkUpdateBtn, updateStatusLabel](QNetworkReply *reply) {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray data = reply->readAll();
                QJsonDocument doc = QJsonDocument::fromJson(data);
                if (doc.isObject()) {
                    QJsonObject obj = doc.object();
                    QString latestTag = obj["tag_name"].toString();
                    if (latestTag.startsWith("v") || latestTag.startsWith("V"))
                        latestTag = latestTag.mid(1);
                    QString currentVersion = QApplication::applicationVersion();
                    if (!latestTag.isEmpty() && latestTag != currentVersion) {
                        updateStatusLabel->setText(TranslationManager::updateMessage(latestTag));
                        updateStatusLabel->setStyleSheet("color: #ff9800; font-size: 11px;");
                    } else {
                        updateStatusLabel->setText(TranslationManager::upToDate());
                        updateStatusLabel->setStyleSheet("color: #4caf50; font-size: 11px;");
                    }
                }
            } else {
                updateStatusLabel->setText(reply->errorString());
                updateStatusLabel->setStyleSheet("color: #ff5252; font-size: 11px;");
            }
            checkUpdateBtn->setEnabled(true);
            checkUpdateBtn->setText(TranslationManager::checkForUpdates());
            reply->deleteLater();
            mgr->deleteLater();
        });
        mgr->get(QNetworkRequest(QUrl("https://api.github.com/repos/Benoks/EShot/releases/latest")));
    });
    layout->addWidget(checkUpdateBtn);

    // Kapat
    QPushButton *closeBtn = new QPushButton(TranslationManager::cancel());
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFixedHeight(32);
    closeBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3a3a3a;
            color: white;
            border: 1px solid #505050;
            border-radius: 4px;
            padding: 6px 12px;
            font-size: 12px;
        }
        QPushButton:hover { background-color: #4a4a4a; border-color: #666; }
    )");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn);
}
