#pragma once

#include <QWidget>
#include "strokemanager.h"

/// Full-screen transparent overlay that spans all monitors.
/// Renders strokes from a StrokeManager and handles mouse drawing input.
/// When drawing is disabled the window is completely click-through.
class DrawingOverlay : public QWidget {
    Q_OBJECT

public:
    explicit DrawingOverlay(StrokeManager* strokeMgr, QWidget* parent = nullptr);

    void setDrawingEnabled(bool enabled);
    [[nodiscard]] bool isDrawingEnabled() const { return m_drawingEnabled; }

    /// Recompute geometry to cover the full virtual desktop.
    void repositionOnScreens();

    /// Rebuild the custom brush-circle cursor.
    void updateBrushCursor();

    void setCurrentColor(const QColor& c)       { m_currentColor = c; }
    void setCurrentWidth(qreal w)                { m_currentWidth = w; updateBrushCursor(); }
    void setCurrentStrokeType(StrokeType t)      { m_currentType = t; }

signals:
    /// Emitted when the user presses Esc while drawing is active.
    void drawingToggleRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setClickThrough(bool through);
    void renderStroke(QPainter& painter, const Stroke& stroke) const;

    StrokeManager* m_strokeMgr;
    bool           m_drawingEnabled{false};
    bool           m_isDrawing{false};

    QColor     m_currentColor{Qt::red};
    qreal      m_currentWidth{4.0};
    StrokeType m_currentType{StrokeType::Pen};
};
