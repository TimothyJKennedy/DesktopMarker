#pragma once

#include <QColor>
#include <QPainterPath>

enum class StrokeType { Pen, Highlighter };

/// Immutable value object representing a single drawn stroke.
struct Stroke {
    QPainterPath path;
    QColor       color{Qt::red};
    qreal        width{4.0};
    StrokeType   type{StrokeType::Pen};

    /// Returns the render color — highlighter strokes are semi-transparent.
    [[nodiscard]] QColor effectiveColor() const
    {
        if (type == StrokeType::Highlighter) {
            QColor c = color;
            c.setAlpha(80);
            return c;
        }
        return color;
    }

    /// Returns the pen cap/join style appropriate for this stroke type.
    [[nodiscard]] Qt::PenCapStyle capStyle() const
    {
        return (type == StrokeType::Highlighter) ? Qt::FlatCap : Qt::RoundCap;
    }

    [[nodiscard]] Qt::PenJoinStyle joinStyle() const
    {
        return (type == StrokeType::Highlighter) ? Qt::BevelJoin : Qt::RoundJoin;
    }
};
