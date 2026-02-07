#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QSettings>
#include <QTabWidget>
#include <QLabel>
#include <QListWidget>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

private slots:
    void onBrowse();
    void onSave();
    void onReset();
    void onFilenamePatternChanged(const QString &text);
    void onSelectAllTools();
    void onDeselectAllTools();

private:
    void loadSettings();
    void setupUI();
    QWidget* createGeneralTab();
    QWidget* createCaptureTab();
    QWidget* createAppearanceTab();
    QWidget* createInterfaceTab();
    QWidget* createAboutTab();

    QString resolvePatternPreview(const QString &pattern) const;

    QSettings *m_settings;

    // Genel
    QLineEdit *m_savePathEdit;
    QLineEdit *m_filenamePatternEdit;
    QLabel *m_patternPreviewLabel;
    QCheckBox *m_autoStartCheck;
    QCheckBox *m_showNotificationsCheck;
    QCheckBox *m_playSoundCheck;
    QCheckBox *m_copyPathAfterSaveCheck;

    // Yakalama
    QComboBox *m_formatCombo;
    QSlider *m_qualitySlider;
    QSpinBox *m_qualitySpin;
    QSpinBox *m_delaySpin;
    QCheckBox *m_copyAfterCaptureCheck;
    QCheckBox *m_closeAfterCopyCheck;

    // Görünüm
    QCheckBox *m_darkModeCheck;
    QSlider *m_opacitySlider;
    QLabel *m_opacityValueLabel;
    QComboBox *m_crosshairStyleCombo;

    // Arayüz - Araç görünürlüğü
    QListWidget *m_toolVisibilityList;
};

#endif