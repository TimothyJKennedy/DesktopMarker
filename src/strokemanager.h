#pragma once

#include <QObject>
#include <optional>
#include <vector>

#include "stroke.h"

/// Owns the collection of strokes and provides undo / clear operations.
/// Emits strokesChanged() after every mutation so overlays can repaint.
class StrokeManager : public QObject {
    Q_OBJECT

public:
    explicit StrokeManager(QObject* parent = nullptr);

    void beginStroke(const QPointF& pos, const QColor& color, qreal width, StrokeType type);
    void continueStroke(const QPointF& pos);
    void endStroke();

    void undoLast();
    void clearAll();

    [[nodiscard]] const std::vector<Stroke>& strokes() const { return m_strokes; }
    [[nodiscard]] bool hasActiveStroke() const { return m_activeStroke.has_value(); }
    [[nodiscard]] const Stroke* activeStroke() const;
    [[nodiscard]] bool isEmpty() const { return m_strokes.empty(); }

signals:
    void strokesChanged();

private:
    std::vector<Stroke>  m_strokes;
    std::optional<Stroke> m_activeStroke;
    QPointF               m_lastPoint;
};
