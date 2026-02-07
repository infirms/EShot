#ifndef PINNEDWINDOW_H
#define PINNEDWINDOW_H

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QMenu>

class PinnedWindow : public QWidget {
    Q_OBJECT

public:
    explicit PinnedWindow(const QPixmap &pixmap, const QPoint &screenPos, QWidget *parent = nullptr);
    ~PinnedWindow();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    QPixmap m_pixmap;
    QPoint m_dragOffset;
    bool m_dragging;
    bool m_hovered;

    // Kapatma butonu alanı
    QRect closeButtonRect() const;
};

#endif