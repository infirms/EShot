#ifndef ANNOTATIONENGINE_H
#define ANNOTATIONENGINE_H

#include <QObject>
#include <QPoint>
#include <QRect>
#include <QColor>
#include <QVector>
#include <QPixmap>
#include <QString>
#include <QFont>

class AnnotationEngine : public QObject {
    Q_OBJECT

public:
    enum Tool {
        None = 0,
        Pen,
        Arrow,
        Rectangle,
        Circle,
        Text,
        Blur,
        Highlighter,
        Counter
    };
    Q_ENUM(Tool)

    explicit AnnotationEngine(QObject *parent = nullptr);
    ~AnnotationEngine();

    void setCurrentTool(Tool tool);
    Tool currentTool() const { return m_currentTool; }

    void setColor(const QColor &color);
    QColor color() const { return m_color; }

    void setPenWidth(int width);
    int penWidth() const { return m_penWidth; }

    void setShiftHeld(bool held);

    void beginDraw(const QPoint &pos);
    void continueDraw(const QPoint &pos);
    void endDraw(const QPoint &pos);

    void render(QPainter *painter, const QPoint &offset);
    bool hasAnnotations() const { return !m_annotations.isEmpty(); }

    void clear();
    void undo();
    void redo();

    void addTextAnnotation(const QPoint &pos, const QString &text);
    void addCounterAnnotation(const QPoint &pos);

signals:
    void annotationAdded();

private:
    struct Annotation {
        Tool tool;
        QColor color;
        int penWidth;
        QVector<QPoint> points;
        QString text;
        QRect boundingRect;
        int counterValue = 0;
        bool shiftConstrained = false;
    };

    void drawAnnotation(QPainter *painter, const Annotation &ann, const QPoint &offset);
    void drawBlurEffect(QPainter *painter, const QRect &rect, const QPoint &offset);

    Tool m_currentTool;
    QColor m_color;
    int m_penWidth;
    bool m_shiftHeld;

    QVector<Annotation> m_annotations;
    QVector<Annotation> m_redoStack;
    Annotation m_currentAnnotation;
    bool m_isDrawing;
    int m_counterValue;
};

#endif