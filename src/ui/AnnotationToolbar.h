#ifndef ANNOTATIONTOOLBAR_H
#define ANNOTATIONTOOLBAR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMap>
#include <QColor>
#include <QStringList>

class AnnotationToolbar : public QWidget {
    Q_OBJECT

public:
    explicit AnnotationToolbar(QWidget *parent = nullptr);
    ~AnnotationToolbar();

signals:
    void toolSelected(int toolId);
    void copyRequested();
    void saveRequested();
    void closeRequested();
    void pinRequested();
    void undoRequested();
    void redoRequested();
    void colorChanged(const QColor &color);
    void penWidthChanged(int width);

private slots:
    void onToolButtonClicked();
    void onActionButtonClicked();
    void onColorButtonClicked();
    void onWidthSliderChanged(int value);

private:
    void setupUI();
    void applyStyles();
    QPushButton* createToolButton(const QString &iconPath, const QString &tooltip,
                                  int toolId, const QString &settingsKey);
    QPushButton* createActionButton(const QString &iconPath, const QString &tooltip,
                                    const QString &action);
    QPushButton* createColorButton(const QColor &color);
    QWidget* createSeparator();

    // Araç görünürlük kontrolü
    bool isToolVisible(const QString &key) const;

    QHBoxLayout *m_layout;
    QMap<int, QPushButton*> m_toolButtons;
    QPushButton *m_colorButton;
    int m_currentToolId;
    QColor m_currentColor;
    QStringList m_visibleTools;
};

#endif