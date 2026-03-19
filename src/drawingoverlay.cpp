#include "drawingoverlay.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

DrawingOverlay::DrawingOverlay(StrokeManager* strokeMgr, QWidget* parent)
    : QWidget(parent)
    , m_strokeMgr(strokeMgr)
{
    setWindowFlags(Qt::FramelessWindowHint
                   | Qt::WindowStaysOnTopHint
                   | Qt::Tool
                   | Qt::BypassWindowManagerHint);

    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setMouseTracking(true);

    connect(m_strokeMgr, &StrokeManager::strokesChanged,
            this, qOverload<>(&QWidget::update));

    repositionOnScreens();
    show();
    setClickThrough(true);
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

void DrawingOverlay::repositionOnScreens()
{
    QRect virt;
    for (QScreen* scr : QGuiApplication::screens())
        virt = virt.united(scr->geometry());
    setGeometry(virt);
}

// ---------------------------------------------------------------------------
// Drawing-mode toggle
// ---------------------------------------------------------------------------

void DrawingOverlay::setDrawingEnabled(bool enabled)
{
    if (m_drawingEnabled == enabled)
        return;

    m_drawingEnabled = enabled;
    setClickThrough(!enabled);

    if (enabled) {
        updateBrushCursor();
    } else {
        unsetCursor();
        if (m_isDrawing) {
            m_strokeMgr->endStroke();
            m_isDrawing = false;
        }
    }

    update();
}

// ---------------------------------------------------------------------------
// Custom brush-size cursor
// ---------------------------------------------------------------------------

void DrawingOverlay::updateBrushCursor()
{
    if (!m_drawingEnabled)
        return;

    const int diam = qMax(static_cast<int>(m_currentWidth), 6);
    const int pad  = 3;
    const int sz   = diam + pad * 2;

    QPixmap pix(sz, sz);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(0, 0, 0, 180), 1.4));
    p.drawEllipse(QRectF(pad, pad, diam, diam));
    p.setPen(QPen(QColor(255, 255, 255, 220), 0.7));
    p.drawEllipse(QRectF(pad, pad, diam, diam));

    const qreal c = sz / 2.0;
    p.setPen(QPen(QColor(255, 255, 255, 180), 0.5));
    p.drawLine(QPointF(c - 2.5, c), QPointF(c + 2.5, c));
    p.drawLine(QPointF(c, c - 2.5), QPointF(c, c + 2.5));
    p.end();

    setCursor(QCursor(pix, sz / 2, sz / 2));
}

// ---------------------------------------------------------------------------
// Click-through (platform-specific)
// ---------------------------------------------------------------------------

void DrawingOverlay::setClickThrough(bool through)
{
#ifdef Q_OS_WIN
    auto hwnd = reinterpret_cast<HWND>(winId());
    LONG_PTR ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    if (through)
        ex |= WS_EX_TRANSPARENT;
    else
        ex &= ~WS_EX_TRANSPARENT;
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex);
    // Force Windows to re-evaluate the extended style for hit-testing
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
#else
    setWindowFlag(Qt::WindowTransparentForInput, through);
    show();
#endif
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void DrawingOverlay::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Layered windows (WA_TranslucentBackground) use per-pixel alpha for
    // hit-testing.  Paint a nearly-invisible fill so the entire overlay
    // area registers mouse hits while drawing mode is active.
    if (m_drawingEnabled)
        painter.fillRect(rect(), QColor(0, 0, 0, 1));

    for (const auto& s : m_strokeMgr->strokes())
        renderStroke(painter, s);

    if (const Stroke* active = m_strokeMgr->activeStroke())
        renderStroke(painter, *active);
}

void DrawingOverlay::renderStroke(QPainter& painter, const Stroke& stroke) const
{
    QPen pen(stroke.effectiveColor(), stroke.width,
             Qt::SolidLine, stroke.capStyle(), stroke.joinStyle());
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(stroke.path);
}

// ---------------------------------------------------------------------------
// Mouse events
// ---------------------------------------------------------------------------

void DrawingOverlay::mousePressEvent(QMouseEvent* event)
{
    if (!m_drawingEnabled || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    m_isDrawing = true;
    m_strokeMgr->beginStroke(event->position(),
                             m_currentColor, m_currentWidth, m_currentType);
}

void DrawingOverlay::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_isDrawing)
        return;
    m_strokeMgr->continueStroke(event->position());
}

void DrawingOverlay::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_isDrawing || event->button() != Qt::LeftButton)
        return;
    m_isDrawing = false;
    m_strokeMgr->endStroke();
}

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------

void DrawingOverlay::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape && m_drawingEnabled) {
        emit drawingToggleRequested();
        return;
    }
    QWidget::keyPressEvent(event);
}
