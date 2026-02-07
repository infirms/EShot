#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QStandardPaths>
#include <QGroupBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QApplication>
#include <QDir>
#include <QDateTime>
#include <QListWidgetItem>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("EShot Ayarları");
    setMinimumSize(560, 540);
    setMaximumSize(750, 650);

    m_settings = new QSettings("EShot", "EShot", this);
    setupUI();
    loadSettings();
}

SettingsDialog::~SettingsDialog() {}

QString SettingsDialog::resolvePatternPreview(const QString &pattern) const
{
    QDateTime now = QDateTime::currentDateTime();
    QString r = pattern;
    r.replace("%Y", now.toString("yyyy"));
    r.replace("%y", now.toString("yy"));
    r.replace("%M", now.toString("MM"));
    r.replace("%D", now.toString("dd"));
    r.replace("%h", now.toString("HH"));
    r.replace("%m", now.toString("mm"));
    r.replace("%s", now.toString("ss"));
    r.replace("%T", "WindowTitle");
    return r;
}

void SettingsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    QLabel *titleLabel = new QLabel("EShot Ayarları");
    QFont tf = titleLabel->font(); tf.setPointSize(16); tf.setBold(true);
    titleLabel->setFont(tf);
    mainLayout->addWidget(titleLabel);

    QTabWidget *tabs = new QTabWidget(this);
    tabs->addTab(createGeneralTab(), "Genel");
    tabs->addTab(createCaptureTab(), "Yakalama");
    tabs->addTab(createAppearanceTab(), "Görünüm");
    tabs->addTab(createInterfaceTab(), "Arayüz");
    mainLayout->addWidget(tabs);

    // Butonlar
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *resetBtn = new QPushButton("Varsayılana Sıfırla");
    resetBtn->setStyleSheet("color: #ff6b6b;");
    connect(resetBtn, &QPushButton::clicked, this, &SettingsDialog::onReset);
    btnLayout->addWidget(resetBtn);

    QPushButton *cancelBtn = new QPushButton("İptal");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    QPushButton *saveBtn = new QPushButton("Kaydet");
    saveBtn->setDefault(true);
    saveBtn->setStyleSheet(R"(
        QPushButton { background-color: #0078D4; color: white; border: none;
                      padding: 8px 24px; border-radius: 4px; font-weight: bold; }
        QPushButton:hover { background-color: #1a8cff; }
    )");
    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSave);
    btnLayout->addWidget(saveBtn);

    mainLayout->addLayout(btnLayout);
}

QWidget* SettingsDialog::createGeneralTab()
{
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);

    // Kayıt dizini
    QGroupBox *pathGroup = new QGroupBox("Kayıt Dizini");
    QHBoxLayout *pathLayout = new QHBoxLayout(pathGroup);
    m_savePathEdit = new QLineEdit();
    m_savePathEdit->setPlaceholderText("Ekran görüntülerinin kaydedileceği dizin");
    QPushButton *browseBtn = new QPushButton("Gözat...");
    connect(browseBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowse);
    pathLayout->addWidget(m_savePathEdit);
    pathLayout->addWidget(browseBtn);
    layout->addWidget(pathGroup);

    // Dosya adı şablonu
    QGroupBox *fnGroup = new QGroupBox("Dosya Adı Şablonu");
    QVBoxLayout *fnLayout = new QVBoxLayout(fnGroup);

    m_filenamePatternEdit = new QLineEdit();
    m_filenamePatternEdit->setPlaceholderText("Screenshot_%Y-%M-%D_%h-%m-%s");
    connect(m_filenamePatternEdit, &QLineEdit::textChanged, this, &SettingsDialog::onFilenamePatternChanged);
    fnLayout->addWidget(m_filenamePatternEdit);

    m_patternPreviewLabel = new QLabel();
    m_patternPreviewLabel->setStyleSheet("color: #888; font-style: italic;");
    fnLayout->addWidget(m_patternPreviewLabel);

    QLabel *helpLabel = new QLabel(
        "<b>Değişkenler:</b> %Y (yıl 4h), %y (yıl 2h), %M (ay), %D (gün), "
        "%h (saat), %m (dakika), %s (saniye), %T (pencere başlığı)"
    );
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("color: #999; font-size: 11px;");
    fnLayout->addWidget(helpLabel);

    layout->addWidget(fnGroup);

    // Genel seçenekler
    QGroupBox *genGroup = new QGroupBox("Genel Seçenekler");
    QVBoxLayout *genLayout = new QVBoxLayout(genGroup);

    m_autoStartCheck = new QCheckBox("Windows ile birlikte başlat");
    genLayout->addWidget(m_autoStartCheck);

    m_showNotificationsCheck = new QCheckBox("Bildirim göster");
    genLayout->addWidget(m_showNotificationsCheck);

    m_playSoundCheck = new QCheckBox("Yakalama sesi çal");
    genLayout->addWidget(m_playSoundCheck);

    m_copyPathAfterSaveCheck = new QCheckBox("Kaydettikten sonra dosya yolunu panoya kopyala");
    genLayout->addWidget(m_copyPathAfterSaveCheck);

    layout->addWidget(genGroup);
    layout->addStretch();
    return tab;
}

QWidget* SettingsDialog::createCaptureTab()
{
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);

    // Format
    QGroupBox *fmtGroup = new QGroupBox("Dosya Formatı");
    QFormLayout *fmtLayout = new QFormLayout(fmtGroup);

    m_formatCombo = new QComboBox();
    m_formatCombo->addItem("PNG (Kayıpsız)", "PNG");
    m_formatCombo->addItem("JPEG (Küçük dosya)", "JPEG");
    m_formatCombo->addItem("BMP (Sıkıştırmasız)", "BMP");
    fmtLayout->addRow("Format:", m_formatCombo);

    QHBoxLayout *qLayout = new QHBoxLayout();
    m_qualitySlider = new QSlider(Qt::Horizontal);
    m_qualitySlider->setRange(10, 100);
    m_qualitySpin = new QSpinBox();
    m_qualitySpin->setRange(10, 100);
    m_qualitySpin->setSuffix("%");
    connect(m_qualitySlider, &QSlider::valueChanged, m_qualitySpin, &QSpinBox::setValue);
    connect(m_qualitySpin, QOverload<int>::of(&QSpinBox::valueChanged), m_qualitySlider, &QSlider::setValue);
    qLayout->addWidget(m_qualitySlider);
    qLayout->addWidget(m_qualitySpin);
    fmtLayout->addRow("JPEG Kalitesi:", qLayout);

    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx) {
        bool jpeg = (m_formatCombo->itemData(idx).toString() == "JPEG");
        m_qualitySlider->setEnabled(jpeg);
        m_qualitySpin->setEnabled(jpeg);
    });

    layout->addWidget(fmtGroup);

    // Yakalama
    QGroupBox *capGroup = new QGroupBox("Yakalama Ayarları");
    QFormLayout *capLayout = new QFormLayout(capGroup);

    m_delaySpin = new QSpinBox();
    m_delaySpin->setRange(0, 10000);
    m_delaySpin->setSingleStep(500);
    m_delaySpin->setSuffix(" ms");
    m_delaySpin->setSpecialValueText("Gecikme yok");
    capLayout->addRow("Gecikme:", m_delaySpin);

    m_copyAfterCaptureCheck = new QCheckBox("Yakaladıktan sonra panoya kopyala");
    capLayout->addRow(m_copyAfterCaptureCheck);

    m_closeAfterCopyCheck = new QCheckBox("Kopyaladıktan sonra overlay'i kapat");
    capLayout->addRow(m_closeAfterCopyCheck);

    layout->addWidget(capGroup);
    layout->addStretch();
    return tab;
}

QWidget* SettingsDialog::createAppearanceTab()
{
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);

    // Tema
    QGroupBox *themeGroup = new QGroupBox("Tema");
    QVBoxLayout *themeLayout = new QVBoxLayout(themeGroup);
    m_darkModeCheck = new QCheckBox("Koyu tema kullan (yeniden başlatma gerekir)");
    themeLayout->addWidget(m_darkModeCheck);
    layout->addWidget(themeGroup);

    // Overlay
    QGroupBox *overlayGroup = new QGroupBox("Overlay Ayarları");
    QFormLayout *overlayLayout = new QFormLayout(overlayGroup);

    // Opaklık slider — 0-255 arası
    QHBoxLayout *opLayout = new QHBoxLayout();
    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(0, 255);
    m_opacitySlider->setTickInterval(25);
    m_opacityValueLabel = new QLabel("39%");
    m_opacityValueLabel->setFixedWidth(40);
    connect(m_opacitySlider, &QSlider::valueChanged, [this](int val) {
        int pct = qRound(val * 100.0 / 255.0);
        m_opacityValueLabel->setText(QString("%1%").arg(pct));
    });
    opLayout->addWidget(m_opacitySlider);
    opLayout->addWidget(m_opacityValueLabel);
    overlayLayout->addRow("Arka Plan Opaklığı:", opLayout);

    m_crosshairStyleCombo = new QComboBox();
    m_crosshairStyleCombo->addItem("Kesikli çizgi", "dash");
    m_crosshairStyleCombo->addItem("Düz çizgi", "solid");
    m_crosshairStyleCombo->addItem("Kapalı", "none");
    overlayLayout->addRow("Crosshair:", m_crosshairStyleCombo);

    layout->addWidget(overlayGroup);
    layout->addStretch();
    return tab;
}

QWidget* SettingsDialog::createInterfaceTab()
{
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);

    QGroupBox *toolsGroup = new QGroupBox("Araç Çubuğu Görünürlüğü");
    QVBoxLayout *toolsLayout = new QVBoxLayout(toolsGroup);

    QLabel *infoLabel = new QLabel("Toolbar'da gösterilecek araçları seçin:");
    infoLabel->setStyleSheet("color: #999;");
    toolsLayout->addWidget(infoLabel);

    m_toolVisibilityList = new QListWidget();
    m_toolVisibilityList->setAlternatingRowColors(true);

    // Tüm araçlar
    struct ToolInfo { QString key; QString label; };
    QVector<ToolInfo> tools = {
        {"Pen",         "✏️ Kalem"},
        {"Arrow",       "➡️ Ok"},
        {"Rectangle",   "⬜ Dikdörtgen"},
        {"Circle",      "⭕ Çember / Elips"},
        {"Text",        "🔤 Metin"},
        {"Highlighter", "🖍️ Vurgulayıcı"},
        {"Blur",        "🔲 Bulanıklaştır"},
        {"Counter",     "🔢 Numara Sayacı"},
    };

    for (const auto &t : tools) {
        QListWidgetItem *item = new QListWidgetItem(t.label);
        item->setData(Qt::UserRole, t.key);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked); // loadSettings'te güncellenir
        m_toolVisibilityList->addItem(item);
    }

    toolsLayout->addWidget(m_toolVisibilityList);

    // Select All / Deselect All
    QHBoxLayout *selBtnLayout = new QHBoxLayout();
    QPushButton *selectAllBtn = new QPushButton("Tümünü Seç");
    QPushButton *deselectAllBtn = new QPushButton("Tümünü Kaldır");
    connect(selectAllBtn, &QPushButton::clicked, this, &SettingsDialog::onSelectAllTools);
    connect(deselectAllBtn, &QPushButton::clicked, this, &SettingsDialog::onDeselectAllTools);
    selBtnLayout->addWidget(selectAllBtn);
    selBtnLayout->addWidget(deselectAllBtn);
    selBtnLayout->addStretch();
    toolsLayout->addLayout(selBtnLayout);

    layout->addWidget(toolsGroup);
    layout->addStretch();
    return tab;
}



void SettingsDialog::loadSettings()
{
    QString defPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    m_savePathEdit->setText(m_settings->value("savePath", defPath).toString());
    m_filenamePatternEdit->setText(m_settings->value("filenamePattern", "Screenshot_%Y-%M-%D_%h-%m-%s").toString());
    onFilenamePatternChanged(m_filenamePatternEdit->text());

    m_autoStartCheck->setChecked(m_settings->value("autoStart", false).toBool());
    m_showNotificationsCheck->setChecked(m_settings->value("showNotifications", true).toBool());
    m_playSoundCheck->setChecked(m_settings->value("playSound", false).toBool());
    m_copyPathAfterSaveCheck->setChecked(m_settings->value("copyPathAfterSave", false).toBool());

    QString fmt = m_settings->value("imageFormat", "PNG").toString();
    int fi = m_formatCombo->findData(fmt);
    if (fi >= 0) m_formatCombo->setCurrentIndex(fi);

    int q = m_settings->value("imageQuality", 95).toInt();
    m_qualitySlider->setValue(q);
    m_qualitySpin->setValue(q);
    m_delaySpin->setValue(m_settings->value("captureDelay", 0).toInt());
    m_copyAfterCaptureCheck->setChecked(m_settings->value("copyAfterCapture", true).toBool());
    m_closeAfterCopyCheck->setChecked(m_settings->value("closeAfterCopy", true).toBool());

    bool jpeg = (fmt == "JPEG");
    m_qualitySlider->setEnabled(jpeg);
    m_qualitySpin->setEnabled(jpeg);

    m_darkModeCheck->setChecked(m_settings->value("darkMode", true).toBool());

    int opacity = m_settings->value("overlayOpacity", 100).toInt();
    m_opacitySlider->setValue(opacity);

    QString cross = m_settings->value("crosshairStyle", "dash").toString();
    int ci = m_crosshairStyleCombo->findData(cross);
    if (ci >= 0) m_crosshairStyleCombo->setCurrentIndex(ci);

    // Araç görünürlüğü
    QStringList visibleTools = m_settings->value("visibleTools",
        QStringList{"Pen","Arrow","Rectangle","Circle","Text","Highlighter","Blur","Counter"})
        .toStringList();

    for (int i = 0; i < m_toolVisibilityList->count(); ++i) {
        QListWidgetItem *item = m_toolVisibilityList->item(i);
        QString key = item->data(Qt::UserRole).toString();
        item->setCheckState(visibleTools.contains(key) ? Qt::Checked : Qt::Unchecked);
    }
}

void SettingsDialog::onFilenamePatternChanged(const QString &text)
{
    QString preview = resolvePatternPreview(text);
    m_patternPreviewLabel->setText("Önizleme: " + preview + ".png");
}

void SettingsDialog::onBrowse()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Klasör Seç", m_savePathEdit->text());
    if (!dir.isEmpty()) m_savePathEdit->setText(dir);
}

void SettingsDialog::onSelectAllTools()
{
    for (int i = 0; i < m_toolVisibilityList->count(); ++i)
        m_toolVisibilityList->item(i)->setCheckState(Qt::Checked);
}

void SettingsDialog::onDeselectAllTools()
{
    for (int i = 0; i < m_toolVisibilityList->count(); ++i)
        m_toolVisibilityList->item(i)->setCheckState(Qt::Unchecked);
}

void SettingsDialog::onSave()
{
    QString savePath = m_savePathEdit->text();
    if (!savePath.isEmpty()) {
        QDir dir(savePath);
        if (!dir.exists() && !dir.mkpath(".")) {
            QMessageBox::warning(this, "Hata", "Kayıt dizini oluşturulamadı: " + savePath);
            return;
        }
    }

    m_settings->setValue("savePath", savePath);
    m_settings->setValue("filenamePattern", m_filenamePatternEdit->text());
    m_settings->setValue("autoStart", m_autoStartCheck->isChecked());
    m_settings->setValue("showNotifications", m_showNotificationsCheck->isChecked());
    m_settings->setValue("playSound", m_playSoundCheck->isChecked());
    m_settings->setValue("copyPathAfterSave", m_copyPathAfterSaveCheck->isChecked());

    m_settings->setValue("imageFormat", m_formatCombo->currentData().toString());
    m_settings->setValue("imageQuality", m_qualitySpin->value());
    m_settings->setValue("captureDelay", m_delaySpin->value());
    m_settings->setValue("copyAfterCapture", m_copyAfterCaptureCheck->isChecked());
    m_settings->setValue("closeAfterCopy", m_closeAfterCopyCheck->isChecked());

    m_settings->setValue("darkMode", m_darkModeCheck->isChecked());
    m_settings->setValue("overlayOpacity", m_opacitySlider->value());
    m_settings->setValue("crosshairStyle", m_crosshairStyleCombo->currentData().toString());

    // Araç görünürlüğü
    QStringList visibleTools;
    for (int i = 0; i < m_toolVisibilityList->count(); ++i) {
        QListWidgetItem *item = m_toolVisibilityList->item(i);
        if (item->checkState() == Qt::Checked)
            visibleTools.append(item->data(Qt::UserRole).toString());
    }
    m_settings->setValue("visibleTools", visibleTools);

    // Windows otomatik başlatma
#ifdef Q_OS_WIN
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                   QSettings::NativeFormat);
    if (m_autoStartCheck->isChecked()) {
        QString appPath = QCoreApplication::applicationFilePath().replace('/', '\\');
        reg.setValue("EShot", "\"" + appPath + "\"");
    } else {
        reg.remove("EShot");
    }
#endif

    m_settings->sync();
    accept();
}

void SettingsDialog::onReset()
{
    if (QMessageBox::question(this, "Sıfırla",
            "Tüm ayarlar varsayılan değerlere sıfırlansın mı?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
        m_settings->clear();
        m_settings->sync();
        loadSettings();
    }
}