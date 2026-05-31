#include "AnnotationEngine.h"
#include <QPainter>
#include <QPainterPath>
#include <QImage>
#include <QtMath>
#include <climits>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QTextBlock>

AnnotationEngine::AnnotationEngine(QObject *parent)
    : QObject(parent)
    , m_currentTool(None)
    , m_color(Qt::red)
    , m_penWidth(3)
    , m_blurIntensity(16)
    , m_shiftHeld(false)
    , m_isDrawing(false)
    , m_counterValue(0)
    , m_selectedIndex(-1)
{
}

AnnotationEngine::~AnnotationEngine() {}

void AnnotationEngine::setCurrentTool(Tool tool) { m_currentTool = tool; }
void AnnotationEngine::setColor(const QColor &color) { m_color = color; }
void AnnotationEngine::setPenWidth(int width) { m_penWidth = qBound(1, width, 20); }
void AnnotationEngine::setBlurIntensity(int intensity) { m_blurIntensity = qBound(4, intensity, 64); }
void AnnotationEngine::setShiftHeld(bool held) { m_shiftHeld = held; }

void AnnotationEngine::beginDraw(const QPoint &pos)
{
    if (m_currentTool == None || m_currentTool == Text || m_currentTool == Counter)
        return;

    m_isDrawing = true;
    m_currentAnnotation = Annotation();
    m_currentAnnotation.tool = m_currentTool;
    m_currentAnnotation.color = m_color;
    m_currentAnnotation.penWidth = m_penWidth;
    m_currentAnnotation.points.append(pos);

    if (m_currentTool == Highlighter)
        m_currentAnnotation.color = QColor(255, 255, 0, 100);
}

void AnnotationEngine::continueDraw(const QPoint &pos)
{
    if (!m_isDrawing || m_currentTool == None) return;

    m_currentAnnotation.shiftConstrained = m_shiftHeld;

    if (m_currentTool == Pen || m_currentTool == Highlighter) {
        m_currentAnnotation.points.append(pos);
    } else {
        if (m_currentAnnotation.points.size() < 2)
            m_currentAnnotation.points.append(pos);
        else
            m_currentAnnotation.points.last() = pos;
    }
}

void AnnotationEngine::endDraw(const QPoint &pos)
{
    if (!m_isDrawing || m_currentTool == None) return;

    m_currentAnnotation.shiftConstrained = m_shiftHeld;

    if (m_currentTool == Pen || m_currentTool == Highlighter) {
        m_currentAnnotation.points.append(pos);
    } else {
        if (m_currentAnnotation.points.size() < 2)
            m_currentAnnotation.points.append(pos);
        else
            m_currentAnnotation.points.last() = pos;
    }

    // Bounding rect
    if (!m_currentAnnotation.points.isEmpty()) {
        int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;
        for (const QPoint &p : m_currentAnnotation.points) {
            minX = qMin(minX, p.x()); minY = qMin(minY, p.y());
            maxX = qMax(maxX, p.x()); maxY = qMax(maxY, p.y());
        }
        m_currentAnnotation.boundingRect = QRect(minX, minY, maxX - minX, maxY - minY);
    }

    if (m_currentAnnotation.points.size() >= 2) {
        QPoint diff = m_currentAnnotation.points.first() - m_currentAnnotation.points.last();
        if (m_currentTool == Pen || m_currentTool == Highlighter ||
            qAbs(diff.x()) > 3 || qAbs(diff.y()) > 3) {
            m_redoStack.clear();
            m_annotations.append(m_currentAnnotation);
            emit annotationAdded();
        }
    }

    m_isDrawing = false;
}

void AnnotationEngine::render(QPainter *painter, const QPoint &offset)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    for (const Annotation &a : m_annotations)
        drawAnnotation(painter, a, offset);
    if (m_isDrawing)
        drawAnnotation(painter, m_currentAnnotation, offset);
    painter->restore();
}

void AnnotationEngine::drawAnnotation(QPainter *painter, const Annotation &ann, const QPoint &offset)
{
    if (ann.points.isEmpty()) return;
    painter->save();

    switch (ann.tool) {
    case Pen: {
        QPen pen(ann.color, ann.penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        if (ann.points.size() > 2) {
            QPainterPath path;
            path.moveTo(ann.points.first() + offset);
            for (int i = 1; i < ann.points.size() - 1; i += 2) {
                QPoint p1 = ann.points[i] + offset;
                QPoint p2 = (i + 1 < ann.points.size()) ? ann.points[i+1] + offset : p1;
                path.quadTo(p1, p2);
            }
            if (ann.points.size() % 2 == 0)
                path.lineTo(ann.points.last() + offset);
            painter->drawPath(path);
        } else if (ann.points.size() == 2) {
            painter->drawLine(ann.points[0] + offset, ann.points[1] + offset);
        } else {
            painter->drawPoint(ann.points.first() + offset);
        }
        break;
    }
    case Arrow: {
        if (ann.points.size() < 2) break;
        QPoint start = ann.points.first() + offset;
        QPoint end = ann.points.last() + offset;
        QPen pen(ann.color, ann.penWidth, Qt::SolidLine, Qt::RoundCap);
        painter->setPen(pen);
        painter->drawLine(start, end);
        double angle = qAtan2(end.y() - start.y(), end.x() - start.x());
        int arrowSize = qMax(12, ann.penWidth * 4);
        QPointF a1 = end - QPointF(qCos(angle - M_PI/6)*arrowSize, qSin(angle - M_PI/6)*arrowSize);
        QPointF a2 = end - QPointF(qCos(angle + M_PI/6)*arrowSize, qSin(angle + M_PI/6)*arrowSize);
        painter->setPen(Qt::NoPen);
        painter->setBrush(ann.color);
        painter->drawPolygon(QPolygonF() << QPointF(end) << a1 << a2);
        break;
    }
    case Rectangle: {
        if (ann.points.size() < 2) break;
        QPoint start = ann.points.first() + offset;
        QPoint end = ann.points.last() + offset;
        QRect r(start, end);
        if (ann.shiftConstrained) {
            int side = qMin(qAbs(r.width()), qAbs(r.height()));
            r.setWidth(r.width() < 0 ? -side : side);
            r.setHeight(r.height() < 0 ? -side : side);
        }
        QPen pen(ann.color, ann.penWidth);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(r.normalized());
        break;
    }
    case Circle: {
        if (ann.points.size() < 2) break;
        QPoint start = ann.points.first() + offset;
        QPoint end = ann.points.last() + offset;
        QRect r(start, end);
        if (ann.shiftConstrained) {
            int side = qMin(qAbs(r.width()), qAbs(r.height()));
            r.setWidth(r.width() < 0 ? -side : side);
            r.setHeight(r.height() < 0 ? -side : side);
        }
        QPen pen(ann.color, ann.penWidth);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(r.normalized());
        break;
    }
    case Line: {
        if (ann.points.size() < 2) break;
        QPoint start = ann.points.first() + offset;
        QPoint end = ann.points.last() + offset;
        QPen pen(ann.color, ann.penWidth);
        painter->setPen(pen);
        painter->drawLine(start, end);
        break;
    }
    case Highlighter: {
        QPen pen(ann.color, ann.penWidth * 5, Qt::SolidLine, Qt::FlatCap, Qt::BevelJoin);
        painter->setPen(pen);
        painter->setOpacity(0.4);
        if (ann.points.size() > 1) {
            QPainterPath path;
            path.moveTo(ann.points.first() + offset);
            for (int i = 1; i < ann.points.size(); ++i)
                path.lineTo(ann.points[i] + offset);
            painter->drawPath(path);
        }
        break;
    }
    case Blur: {
        if (ann.points.size() < 2) break;
        QRect r = QRect(ann.points.first(), ann.points.last()).normalized();
        drawBlurEffect(painter, r, offset);
        break;
    }
    case Text: {
        if (ann.text.isEmpty() || ann.points.isEmpty()) break;
        QFont font("Segoe UI", ann.penWidth * 4 + 8);
        font.setBold(true);
        QPoint tp = ann.points.first() + offset;

        QTextDocument doc;
        doc.setDefaultFont(font);
        doc.setPlainText(ann.text);
        QSizeF docSize = doc.size();

        // Arka plan
        QRect bg(tp.x() - 4, tp.y() - 4,
                 static_cast<int>(docSize.width()) + 8,
                 static_cast<int>(docSize.height()) + 8);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 120));
        painter->drawRoundedRect(bg, 3, 3);

        // Metin
        painter->save();
        painter->translate(tp);
        QAbstractTextDocumentLayout *layout = doc.documentLayout();
        for (int i = 0; i < doc.blockCount(); ++i) {
            QTextBlock block = doc.findBlockByNumber(i);
            QTextLayout *blockLayout = block.layout();
            QTextLine line = blockLayout->lineAt(0);
            painter->setPen(ann.color);
            blockLayout->draw(painter, QPoint(0, 0));
        }
        painter->restore();
        break;
    }
    case Counter: {
        if (ann.points.isEmpty()) break;
        QPoint center = ann.points.first() + offset;
        int radius = 14;
        painter->setPen(QPen(Qt::white, 2));
        painter->setBrush(ann.color);
        painter->drawEllipse(center, radius, radius);
        painter->setPen(Qt::white);
        QFont f("Segoe UI", 10); f.setBold(true);
        painter->setFont(f);
        painter->drawText(QRect(center.x()-radius, center.y()-radius, radius*2, radius*2),
                          Qt::AlignCenter, QString::number(ann.counterValue));
        break;
    }
    case SemiRect: {
        if (ann.points.size() < 2) break;
        QPoint start = ann.points.first() + offset;
        QPoint end = ann.points.last() + offset;
        QRect r(start, end);
        if (ann.shiftConstrained) {
            int side = qMin(qAbs(r.width()), qAbs(r.height()));
            r.setWidth(r.width() < 0 ? -side : side);
            r.setHeight(r.height() < 0 ? -side : side);
        }
        QColor fillColor = ann.color;
        fillColor.setAlpha(80);
        painter->setPen(QPen(ann.color, ann.penWidth));
        painter->setBrush(fillColor);
        painter->drawRect(r.normalized());
        break;
    }
    default: break;
    }

    painter->restore();
}

void AnnotationEngine::clear()
{
    m_annotations.clear();
    m_redoStack.clear();
    m_counterValue = 0;
}

void AnnotationEngine::undo()
{
    if (!m_annotations.isEmpty()) {
        Annotation last = m_annotations.takeLast();
        m_redoStack.append(last);
        if (last.tool == Counter) m_counterValue = qMax(0, m_counterValue - 1);
    }
}

void AnnotationEngine::redo()
{
    if (!m_redoStack.isEmpty()) {
        Annotation last = m_redoStack.takeLast();
        m_annotations.append(last);
        if (last.tool == Counter) m_counterValue = last.counterValue;
    }
}

void AnnotationEngine::addTextAnnotation(const QPoint &pos, const QString &text)
{
    if (text.isEmpty()) return;
    Annotation a;
    a.tool = Text; a.color = m_color; a.penWidth = m_penWidth;
    a.points.append(pos); a.text = text;
    m_redoStack.clear();
    m_annotations.append(a);
    emit annotationAdded();
}

void AnnotationEngine::addCounterAnnotation(const QPoint &pos)
{
    Annotation ann;
    ann.tool = Counter;
    ann.color = m_color;
    ann.penWidth = m_penWidth;
    ann.points.append(pos);
    ann.counterValue = ++m_counterValue;
    m_redoStack.clear();
    m_annotations.append(ann);
    emit annotationAdded();
}

bool AnnotationEngine::eraseAnnotationAt(const QPoint &pos)
{
    for (int i = m_annotations.size() - 1; i >= 0; --i) {
        const Annotation &ann = m_annotations[i];
        if (ann.points.isEmpty()) continue;

        QRect bounds;
        if (ann.tool == Text || ann.tool == Counter) {
            bounds = QRect(ann.points.first(), QSize(80, 30));
            bounds.moveCenter(ann.points.first());
        } else if (ann.points.size() == 1) {
            bounds = QRect(ann.points.first(), QSize(10, 10));
            bounds.moveCenter(ann.points.first());
        } else {
            bounds = QRect(ann.points.first(), ann.points.last()).normalized();
            bounds.adjust(-5, -5, 5, 5);
        }

        if (bounds.contains(pos)) {
            m_redoStack.append(m_annotations[i]);
            m_annotations.removeAt(i);
            return true;
        }
    }
    return false;
}

int AnnotationEngine::findAnnotationAt(const QPoint &pos)
{
    for (int i = m_annotations.size() - 1; i >= 0; --i) {
        const Annotation &ann = m_annotations[i];
        if (ann.points.isEmpty()) continue;

        QRect bounds;
        if (ann.tool == Text || ann.tool == Counter) {
            bounds = QRect(ann.points.first(), QSize(100, 40));
            bounds.moveCenter(ann.points.first());
        } else if (ann.tool == Blur) {
            bounds = QRect(ann.points.first(), ann.points.last()).normalized();
            bounds.adjust(-10, -10, 10, 10);
        } else if (ann.points.size() == 1) {
            bounds = QRect(ann.points.first(), QSize(20, 20));
            bounds.moveCenter(ann.points.first());
        } else {
            // Tüm noktaları kapsayan bounds hesapla
            int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;
            for (const QPoint &p : ann.points) {
                minX = qMin(minX, p.x());
                minY = qMin(minY, p.y());
                maxX = qMax(maxX, p.x());
                maxY = qMax(maxY, p.y());
            }
            bounds = QRect(minX, minY, maxX - minX, maxY - minY);
            bounds.adjust(-10, -10, 10, 10);
        }

        if (bounds.contains(pos))
            return i;
    }
    return -1;
}

void AnnotationEngine::moveAnnotation(int index, const QPoint &delta)
{
    if (index < 0 || index >= m_annotations.size()) return;
    Annotation &ann = m_annotations[index];
    for (int i = 0; i < ann.points.size(); ++i) {
        ann.points[i] += delta;
    }
}

void AnnotationEngine::setSelectedIndex(int index)
{
    m_selectedIndex = index;
}

QRect AnnotationEngine::boundingRectOf(int index) const
{
    if (index < 0 || index >= m_annotations.size()) return QRect();
    const Annotation &ann = m_annotations[index];
    if (ann.points.isEmpty()) return QRect();

    if (ann.tool == Text || ann.tool == Counter) {
        QRect bounds(ann.points.first(), QSize(100, 40));
        bounds.moveCenter(ann.points.first());
        return bounds;
    } else if (ann.tool == Blur || ann.tool == SemiRect) {
        return QRect(ann.points.first(), ann.points.last()).normalized();
    } else {
        int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;
        for (const QPoint &p : ann.points) {
            minX = qMin(minX, p.x()); minY = qMin(minY, p.y());
            maxX = qMax(maxX, p.x()); maxY = qMax(maxY, p.y());
        }
        return QRect(minX, minY, maxX - minX, maxY - minY);
    }
}

void AnnotationEngine::setScreenSnapshot(const QPixmap &snapshot)
{
    m_screenSnapshot = snapshot;
}

void AnnotationEngine::setSelectionRect(const QRect &rect)
{
    m_selectionRect = rect;
}

void AnnotationEngine::drawBlurEffect(QPainter *painter, const QRect &rect, const QPoint &offset)
{
    QRect target = rect.translated(offset);
    painter->save();

    if (target.isEmpty()) {
        painter->restore();
        return;
    }

    // Final capture: painter bir QPixmap üzerine çiziyor
    QPixmap *dev = dynamic_cast<QPixmap*>(painter->device());
    if (dev) {
        QRect clamped = target.intersected(dev->rect());
        if (!clamped.isEmpty()) {
            QPixmap region = dev->copy(clamped);
            if (!region.isNull()) {
                int ps = m_blurIntensity;
                QImage img = region.toImage();
                QImage scaled = img.scaled(qMax(1, img.width() / ps), qMax(1, img.height() / ps),
                                           Qt::IgnoreAspectRatio, Qt::FastTransformation);
                QImage mosaic = scaled.scaled(img.width(), img.height(),
                                              Qt::IgnoreAspectRatio, Qt::FastTransformation);
                painter->drawPixmap(clamped.topLeft(), QPixmap::fromImage(mosaic));
                painter->restore();
                return;
            }
        }
    }

    // Canlı önizleme: m_screenSnapshot'tan al
    if (!m_screenSnapshot.isNull()) {
        QRect sourceRect = target;
        QRect clamped = sourceRect.intersected(m_screenSnapshot.rect());
        if (!clamped.isEmpty()) {
            QPixmap region = m_screenSnapshot.copy(clamped);
            if (!region.isNull()) {
                int ps = m_blurIntensity;
                QImage img = region.toImage();
                QImage scaled = img.scaled(qMax(1, img.width() / ps), qMax(1, img.height() / ps),
                                           Qt::IgnoreAspectRatio, Qt::FastTransformation);
                QImage mosaic = scaled.scaled(img.width(), img.height(),
                                              Qt::IgnoreAspectRatio, Qt::FastTransformation);
                painter->drawPixmap(clamped.topLeft(), QPixmap::fromImage(mosaic));
                painter->restore();
                return;
            }
        }
    }

    // Fallback: gri dolgu
    painter->fillRect(target, QColor(128, 128, 128, 180));
    painter->restore();
}