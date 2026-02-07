#include "PinnedWindow.h"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QDebug>

PinnedWindow::PinnedWindow(const QPixmap &pixmap, const QPoint &screenPos, QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
    , m_pixmap(pixmap)
    , m_dragging(false)
    , m_hovered(false)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setMouseTracking(true);
    setCursor(Qt::SizeAllCursor);

    setFixedSize(m_pixmap.size());
    move(screenPos);
    show();

    qDebug() << "[PinnedWindow] Created at" << screenPos << "size:" << m_pixmap.size();
}

PinnedWindow::~PinnedWindow()
{
    qDebug() << "[PinnedWindow] Destroyed";
}

QRect PinnedWindow::closeButtonRect() const
{
    int s = 20;
    return QRect(width() - s - 4, 4, s, s);
}

void PinnedWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    // Gölge kenarlık
    painter.setPen(QPen(QColor(0, 0, 0, 80), 2));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));

    // Pixmap
    painter.drawPixmap(0, 0, m_pixmap);

    // Hover durumunda kapatma butonu
    if (m_hovered) {
        // Üst kenarda yarı saydam bant
        painter.fillRect(0, 0, width(), 28, QColor(0, 0, 0, 120));

        // X butonu
        QRect closeRect = closeButtonRect();
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(220, 50, 50, 200));
        painter.drawRoundedRect(closeRect, 4, 4);

        // X işareti
        painter.setPen(QPen(Qt::white, 2));
        int m = 5;
        painter.drawLine(closeRect.left() + m, closeRect.top() + m,
                         closeRect.right() - m, closeRect.bottom() - m);
        painter.drawLine(closeRect.right() - m, closeRect.top() + m,
                         closeRect.left() + m, closeRect.bottom() - m);

        // "Pin" etiketi
        painter.setPen(Qt::white);
        QFont f = painter.font();
        f.setPointSize(8);
        f.setBold(true);
        painter.setFont(f);
        painter.drawText(6, 18, "📌 Pinned");
    }
}

void PinnedWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Kapatma butonuna tıklama
        if (m_hovered && closeButtonRect().contains(event->pos())) {
            close();
            return;
        }
        m_dragging = true;
        m_dragOffset = event->pos();
    }
}

void PinnedWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        move(event->globalPosition().toPoint() - m_dragOffset);
    }

    // Hover durumunu kontrol et (kapatma butonu için)
    bool wasHovered = m_hovered;
    // mouse tracking açık olduğundan her harekette buraya gelir
    if (wasHovered != m_hovered) update();
}

void PinnedWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
}

void PinnedWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
    }
}

void PinnedWindow::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    m_hovered = true;
    update();
}

void PinnedWindow::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_hovered = false;
    update();
}

void PinnedWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    QAction *copyAction = menu.addAction("Kopyala");
    connect(copyAction, &QAction::triggered, [this]() {
        QGuiApplication::clipboard()->setPixmap(m_pixmap);
    });

    menu.addSeparator();

    QAction *closeAction = menu.addAction("Kapat");
    connect(closeAction, &QAction::triggered, this, &QWidget::close);

    menu.exec(event->globalPos());
}