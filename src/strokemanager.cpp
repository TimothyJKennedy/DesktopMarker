#include "strokemanager.h"

StrokeManager::StrokeManager(QObject* parent)
    : QObject(parent)
{
}

void StrokeManager::beginStroke(const QPointF& pos, const QColor& color,
                                qreal width, StrokeType type)
{
    Stroke s;
    s.color = color;
    s.width = width;
    s.type  = type;
    s.path.moveTo(pos);
    m_activeStroke = std::move(s);
    m_lastPoint    = pos;
    emit strokesChanged();
}

void StrokeManager::continueStroke(const QPointF& pos)
{
    if (!m_activeStroke)
        return;

    // Cubic interpolation for smooth curves: use midpoint control points
    const QPointF mid = (m_lastPoint + pos) / 2.0;
    m_activeStroke->path.quadTo(m_lastPoint, mid);
    m_lastPoint = pos;
    emit strokesChanged();
}

void StrokeManager::endStroke()
{
    if (!m_activeStroke)
        return;

    m_activeStroke->path.lineTo(m_lastPoint);
    m_strokes.push_back(std::move(*m_activeStroke));
    m_activeStroke.reset();
    emit strokesChanged();
}

void StrokeManager::undoLast()
{
    if (m_strokes.empty())
        return;
    m_strokes.pop_back();
    emit strokesChanged();
}

void StrokeManager::clearAll()
{
    if (m_strokes.empty())
        return;
    m_strokes.clear();
    emit strokesChanged();
}

const Stroke* StrokeManager::activeStroke() const
{
    return m_activeStroke ? &(*m_activeStroke) : nullptr;
}
