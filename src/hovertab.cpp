#include "hovertab.h"

#include <QColorDialog>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScreen>
#include <QSlider>
#include <QVBoxLayout>

#include <algorithm>

namespace {

/// Panel background: dark rounded rectangle with translucent support.
class RoundedPanel : public QWidget {
public:
    using QWidget::QWidget;
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(40, 40, 68), 1));
        p.setBrush(QColor(22, 22, 42, 245));
        p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 10, 10);
    }
};

QFrame* makeSep(QWidget* parent)
{
    auto* f = new QFrame(parent);
    f->setFrameShape(QFrame::HLine);
    f->setFixedHeight(1);
    f->setStyleSheet(QStringLiteral("border:none; background:#2a2a3e;"));
    return f;
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
//  Construction / destruction
// ═══════════════════════════════════════════════════════════════════════════

HoverTab::HoverTab(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint
                   | Qt::WindowStaysOnTopHint
                   | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedSize(kGripSize, kGripSize);
    setCursor(Qt::PointingHandCursor);

    auto& s         = SettingsManager::instance();
    m_selectedColor = s.lastColor();
    m_brushSize     = s.brushSize();
    m_tool          = s.markerTool();
    m_dockEdge      = static_cast<DockEdge>(s.dockEdge());
    m_edgePos       = s.tabPositionY();
    m_panelSize     = static_cast<PanelSize>(s.panelSize());

    m_collapseTimer = new QTimer(this);
    m_collapseTimer->setSingleShot(true);
    m_collapseTimer->setInterval(280);
    connect(m_collapseTimer, &QTimer::timeout, this, &HoverTab::hidePanel);

    m_proximityTimer = new QTimer(this);
    m_proximityTimer->setInterval(45);
    connect(m_proximityTimer, &QTimer::timeout, this, &HoverTab::checkProximity);
    m_proximityTimer->start();

    buildPanel();
    applyStyles();
    applyPanelSize(m_panelSize);
    reposition();
    show();
}

HoverTab::~HoverTab()
{
    delete m_panel;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Grip painting  (small marker icon showing selected colour)
// ═══════════════════════════════════════════════════════════════════════════

void HoverTab::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r(1, 1, width() - 2, height() - 2);
    p.setPen(QPen(QColor(50, 50, 80), 1));
    p.setBrush(QColor("#16162a"));
    p.drawRoundedRect(r, 8, 8);

    // Curved marker stroke in the selected colour
    QPainterPath stroke;
    stroke.moveTo(10, height() - 10);
    stroke.cubicTo(width() * 0.38, height() * 0.58,
                   width() * 0.62, height() * 0.42,
                   width() - 10, 10);
    p.setPen(QPen(m_selectedColor, 3.0, Qt::SolidLine, Qt::RoundCap));
    p.setBrush(Qt::NoBrush);
    p.drawPath(stroke);

    // Dot at the pen tip
    p.setPen(Qt::NoPen);
    p.setBrush(m_selectedColor);
    p.drawEllipse(QPointF(10, height() - 10), 2.5, 2.5);

    // Green indicator when drawing is active
    if (m_drawingActive) {
        p.setBrush(QColor("#00E676"));
        p.setPen(QPen(QColor("#16162a"), 1.5));
        p.drawEllipse(QPointF(width() - 7, 7), 4, 4);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  Hover — grip
// ═══════════════════════════════════════════════════════════════════════════

void HoverTab::enterEvent(QEnterEvent* e)
{
    QWidget::enterEvent(e);
    m_collapseTimer->stop();
    showPanel();
}

void HoverTab::leaveEvent(QEvent* e)
{
    QWidget::leaveEvent(e);
    if (!m_dialogOpen && !m_dragging)
        m_collapseTimer->start();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Event filter  (panel hover  +  delete right-click)
// ═══════════════════════════════════════════════════════════════════════════

bool HoverTab::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_panel) {
        if (event->type() == QEvent::Enter)
            m_collapseTimer->stop();
        else if (event->type() == QEvent::Leave && !m_dialogOpen)
            m_collapseTimer->start();
    }

    if (obj == m_deleteBtn && event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::RightButton) {
            if (SettingsManager::instance().deleteMode() ==
                DeleteMode::UndoLeft_ClearRight)
                emit clearRequested();
            else
                emit undoRequested();
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Mouse — drag grip to reposition
// ═══════════════════════════════════════════════════════════════════════════

void HoverTab::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        m_dragging   = true;
        m_dragOffset = e->globalPosition().toPoint() - pos();
    }
    QWidget::mousePressEvent(e);
}

void HoverTab::mouseMoveEvent(QMouseEvent* e)
{
    if (m_dragging) {
        const QPoint newPos = e->globalPosition().toPoint() - m_dragOffset;
        move(newPos);
        if (m_panel->isVisible())
            repositionPanel();
    }
    QWidget::mouseMoveEvent(e);
}

void HoverTab::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_dragging && e->button() == Qt::LeftButton) {
        m_dragging = false;
        snapToNearestEdge();
    }
    QWidget::mouseReleaseEvent(e);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Panel show / hide  (fade animation)
// ═══════════════════════════════════════════════════════════════════════════

void HoverTab::checkProximity()
{
    if (m_dialogOpen || m_dragging) return;

    const QPoint cursor = QCursor::pos();

    // Hot zone extends inward from the screen edge (toward where the panel opens)
    QRect gripZone;
    switch (m_dockEdge) {
    case DockEdge::Right:  gripZone = geometry().adjusted(-25, -25,   8, 25); break;
    case DockEdge::Left:   gripZone = geometry().adjusted( -8, -25,  25, 25); break;
    case DockEdge::Top:    gripZone = geometry().adjusted(-25,  -8,  25, 25); break;
    case DockEdge::Bottom: gripZone = geometry().adjusted(-25, -25,  25,  8); break;
    }

    if (!m_panel->isVisible()) {
        if (gripZone.contains(cursor)) {
            m_collapseTimer->stop();
            showPanel();
        }
    } else {
        const QRect panelZone = m_panel->geometry().adjusted(-8, -8, 8, 8);
        if (gripZone.contains(cursor) || panelZone.contains(cursor)) {
            m_collapseTimer->stop();
        } else if (!m_collapseTimer->isActive()) {
            m_collapseTimer->start();
        }
    }
}

void HoverTab::showPanel()
{
    if (m_panel->isVisible() && m_panel->windowOpacity() >= 0.99)
        return;

    delete m_panelAnim;
    repositionPanel();

    if (!m_panel->isVisible()) {
        m_panel->setWindowOpacity(0.0);
        m_panel->show();
    }
    raise();
    m_panel->raise();

    m_panelAnim = new QPropertyAnimation(m_panel, "windowOpacity", this);
    m_panelAnim->setDuration(120);
    m_panelAnim->setStartValue(m_panel->windowOpacity());
    m_panelAnim->setEndValue(1.0);
    m_panelAnim->start();
}

void HoverTab::hidePanel()
{
    if (m_dialogOpen || !m_panel->isVisible())
        return;

    delete m_panelAnim;
    m_panelAnim = new QPropertyAnimation(m_panel, "windowOpacity", this);
    m_panelAnim->setDuration(80);
    m_panelAnim->setStartValue(m_panel->windowOpacity());
    m_panelAnim->setEndValue(0.0);
    connect(m_panelAnim, &QAbstractAnimation::finished,
            m_panel, &QWidget::hide);
    m_panelAnim->start();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Positioning
// ═══════════════════════════════════════════════════════════════════════════

void HoverTab::reposition()
{
    QScreen* scr = QGuiApplication::primaryScreen();
    const QRect g = scr->geometry();

    switch (m_dockEdge) {
    case DockEdge::Right:
        m_edgePos = std::clamp(m_edgePos, 0, g.height() - kGripSize);
        move(g.x() + g.width() - kGripSize, g.y() + m_edgePos);
        break;
    case DockEdge::Left:
        m_edgePos = std::clamp(m_edgePos, 0, g.height() - kGripSize);
        move(g.x(), g.y() + m_edgePos);
        break;
    case DockEdge::Top:
        m_edgePos = std::clamp(m_edgePos, 0, g.width() - kGripSize);
        move(g.x() + m_edgePos, g.y());
        break;
    case DockEdge::Bottom:
        m_edgePos = std::clamp(m_edgePos, 0, g.width() - kGripSize);
        move(g.x() + m_edgePos, g.y() + g.height() - kGripSize);
        break;
    }
}

void HoverTab::repositionPanel()
{
    QScreen* scr = QGuiApplication::primaryScreen();
    const QRect sg = scr->geometry();
    const int pw = m_panel->width();
    const int ph = m_panel->height();
    constexpr int gap = 6;
    int px = 0, py = 0;

    switch (m_dockEdge) {
    case DockEdge::Right:
        px = x() - pw - gap;
        py = y() + height() / 2 - ph / 2;
        break;
    case DockEdge::Left:
        px = x() + width() + gap;
        py = y() + height() / 2 - ph / 2;
        break;
    case DockEdge::Top:
        px = x() + width() / 2 - pw / 2;
        py = y() + height() + gap;
        break;
    case DockEdge::Bottom:
        px = x() + width() / 2 - pw / 2;
        py = y() - ph - gap;
        break;
    }

    px = std::clamp(px, sg.x() + 4, sg.x() + sg.width()  - pw - 4);
    py = std::clamp(py, sg.y() + 4, sg.y() + sg.height() - ph - 4);
    m_panel->move(px, py);
}

void HoverTab::snapToNearestEdge()
{
    QScreen* scr = QGuiApplication::primaryScreen();
    const QRect g = scr->geometry();
    const QPoint c = pos() + QPoint(kGripSize / 2, kGripSize / 2);

    const int dL = c.x() - g.left();
    const int dR = g.right()  - c.x();
    const int dT = c.y() - g.top();
    const int dB = g.bottom() - c.y();
    const int minD = std::min({dL, dR, dT, dB});

    if (minD == dR) {
        m_dockEdge = DockEdge::Right;
        m_edgePos  = c.y() - g.top() - kGripSize / 2;
    } else if (minD == dL) {
        m_dockEdge = DockEdge::Left;
        m_edgePos  = c.y() - g.top() - kGripSize / 2;
    } else if (minD == dT) {
        m_dockEdge = DockEdge::Top;
        m_edgePos  = c.x() - g.left() - kGripSize / 2;
    } else {
        m_dockEdge = DockEdge::Bottom;
        m_edgePos  = c.x() - g.left() - kGripSize / 2;
    }

    reposition();
    if (m_panel->isVisible())
        repositionPanel();

    SettingsManager::instance().setDockEdge(static_cast<int>(m_dockEdge));
    SettingsManager::instance().setTabPositionY(m_edgePos);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════════════════════

void HoverTab::setDrawingActive(bool active)
{
    m_drawingActive = active;
    updateDrawButton();
    updateDeleteTooltip();
    update();
}

void HoverTab::onDialogClosed()
{
    m_dialogOpen = false;
    updateDeleteTooltip();
}

void HoverTab::refreshPosition() { reposition(); }

void HoverTab::raiseAll()
{
    raise();
    if (m_panel && m_panel->isVisible())
        m_panel->raise();
}

void HoverTab::applyPanelSize(PanelSize sz)
{
    m_panelSize = sz;
    const bool extended = (sz != PanelSize::Compact);

    m_toolSep->setVisible(extended);
    m_toolRow->setVisible(extended);
    m_sliderRow->setVisible(extended);

    // Swatch & button scaling
    const int sw  = (sz == PanelSize::Full) ? 24 : 18;
    const int bh  = (sz == PanelSize::Full) ? 32 : 28;
    for (auto* btn : m_colorBtns) btn->setFixedSize(sw, sw);
    m_customColorBtn->setFixedSize(sw, sw);
    m_drawBtn->setFixedHeight(bh);
    m_deleteBtn->setFixedHeight(bh);

    const int margin = (sz == PanelSize::Full) ? 14 : 10;
    m_panel->layout()->setContentsMargins(margin, margin, margin, margin - 2);

    m_panel->adjustSize();
    m_panel->setFixedSize(m_panel->sizeHint());

    if (m_panel->isVisible())
        repositionPanel();

    SettingsManager::instance().setPanelSize(static_cast<int>(sz));
}

// ═══════════════════════════════════════════════════════════════════════════
//  Build panel
// ═══════════════════════════════════════════════════════════════════════════

void HoverTab::buildPanel()
{
    m_panel = new RoundedPanel(nullptr);
    m_panel->setWindowFlags(Qt::FramelessWindowHint
                            | Qt::WindowStaysOnTopHint
                            | Qt::Tool);
    m_panel->setAttribute(Qt::WA_TranslucentBackground);
    m_panel->setAttribute(Qt::WA_ShowWithoutActivating);
    m_panel->installEventFilter(this);

    auto* lay = new QVBoxLayout(m_panel);
    lay->setContentsMargins(10, 10, 10, 8);
    lay->setSpacing(5);

    // ── Colour palette  (2 rows × 6  +  custom) ─────────────────────────
    auto* colorGrid = new QGridLayout;
    colorGrid->setSpacing(3);
    for (int i = 0; i < kPresetColors.size(); ++i) {
        auto* btn = new QPushButton(m_panel);
        btn->setFixedSize(18, 18);
        btn->setCursor(Qt::PointingHandCursor);
        const QColor c = kPresetColors[i];
        connect(btn, &QPushButton::clicked, this, [this, c] { selectColor(c); });
        colorGrid->addWidget(btn, (i < 6) ? 0 : 1, (i < 6) ? i : i - 6);
        m_colorBtns.append(btn);
    }
    m_customColorBtn = new QPushButton(QStringLiteral("+"), m_panel);
    m_customColorBtn->setFixedSize(18, 18);
    m_customColorBtn->setCursor(Qt::PointingHandCursor);
    m_customColorBtn->setToolTip(tr("Custom colour\u2026"));
    connect(m_customColorBtn, &QPushButton::clicked, this, [this] {
        m_dialogOpen = true;
        m_collapseTimer->stop();
        QColor c = QColorDialog::getColor(m_selectedColor, m_panel, tr("Choose Colour"));
        m_dialogOpen = false;
        if (c.isValid()) selectColor(c);
    });
    colorGrid->addWidget(m_customColorBtn, 0, 6);
    lay->addLayout(colorGrid);

    // ── Separator + tool buttons (hidden in Compact) ─────────────────────
    m_toolSep = makeSep(m_panel);
    lay->addWidget(m_toolSep);

    m_toolRow = new QWidget(m_panel);
    auto* tlay = new QHBoxLayout(m_toolRow);
    tlay->setContentsMargins(0, 0, 0, 0);
    tlay->setSpacing(4);
    m_penBtn = new QPushButton(tr("Pen"), m_panel);
    m_penBtn->setCheckable(true);
    m_penBtn->setFixedHeight(22);
    m_penBtn->setCursor(Qt::PointingHandCursor);
    m_highlighterBtn = new QPushButton(tr("Highlight"), m_panel);
    m_highlighterBtn->setCheckable(true);
    m_highlighterBtn->setFixedHeight(22);
    m_highlighterBtn->setCursor(Qt::PointingHandCursor);
    connect(m_penBtn, &QPushButton::clicked, this, [this] {
        m_tool = MarkerTool::Pen;
        updateToolButtons();
        SettingsManager::instance().setMarkerTool(m_tool);
        emit toolChanged(m_tool);
    });
    connect(m_highlighterBtn, &QPushButton::clicked, this, [this] {
        m_tool = MarkerTool::Highlighter;
        updateToolButtons();
        SettingsManager::instance().setMarkerTool(m_tool);
        emit toolChanged(m_tool);
    });
    tlay->addWidget(m_penBtn);
    tlay->addWidget(m_highlighterBtn);
    lay->addWidget(m_toolRow);

    // ── Slider + preview  (hidden in Compact) ────────────────────────────
    m_sliderRow = new QWidget(m_panel);
    auto* slay = new QHBoxLayout(m_sliderRow);
    slay->setContentsMargins(0, 0, 0, 0);
    slay->setSpacing(4);
    m_sizeSlider = new QSlider(Qt::Horizontal, m_panel);
    m_sizeSlider->setRange(1, 80);
    m_sizeSlider->setValue(m_brushSize);
    m_sizeSlider->setFixedHeight(18);
    connect(m_sizeSlider, &QSlider::valueChanged, this, [this](int v) {
        m_brushSize = v;
        SettingsManager::instance().setBrushSize(v);
        updateSizePreview();
        emit brushSizeChanged(v);
    });
    m_sizePreview = new QLabel(m_panel);
    m_sizePreview->setFixedSize(22, 22);
    m_sizePreview->setAlignment(Qt::AlignCenter);
    slay->addWidget(m_sizeSlider, 1);
    slay->addWidget(m_sizePreview);
    lay->addWidget(m_sliderRow);

    // ── Separator ────────────────────────────────────────────────────────
    lay->addWidget(makeSep(m_panel));

    // ── Draw + Delete ────────────────────────────────────────────────────
    auto* actionLay = new QHBoxLayout;
    actionLay->setSpacing(4);
    m_drawBtn = new QPushButton(m_panel);
    m_drawBtn->setFixedHeight(28);
    m_drawBtn->setCursor(Qt::PointingHandCursor);
    connect(m_drawBtn, &QPushButton::clicked, this, [this] {
        m_drawingActive = !m_drawingActive;
        updateDrawButton();
        emit drawingToggled(m_drawingActive);
    });
    m_deleteBtn = new QPushButton(QStringLiteral("DEL"), m_panel);
    m_deleteBtn->setFixedHeight(28);
    m_deleteBtn->setCursor(Qt::PointingHandCursor);
    m_deleteBtn->installEventFilter(this);
    connect(m_deleteBtn, &QPushButton::clicked, this, [this] {
        if (SettingsManager::instance().deleteMode() ==
            DeleteMode::UndoLeft_ClearRight)
            emit undoRequested();
        else
            emit clearRequested();
    });
    actionLay->addWidget(m_drawBtn, 1);
    actionLay->addWidget(m_deleteBtn, 1);
    lay->addLayout(actionLay);

    // ── Settings gear (bottom-right) ─────────────────────────────────────
    auto* bottomLay = new QHBoxLayout;
    bottomLay->setContentsMargins(0, 0, 0, 0);
    m_settingsBtn = new QPushButton(QStringLiteral("\u2699"), m_panel);
    m_settingsBtn->setFixedSize(20, 20);
    m_settingsBtn->setCursor(Qt::PointingHandCursor);
    m_settingsBtn->setToolTip(tr("Settings"));
    connect(m_settingsBtn, &QPushButton::clicked, this, [this] {
        if (m_drawingActive) {
            m_drawingActive = false;
            updateDrawButton();
            emit drawingToggled(false);
        }
        m_dialogOpen = true;
        m_collapseTimer->stop();
        emit settingsRequested();
    });
    bottomLay->addStretch();
    bottomLay->addWidget(m_settingsBtn);
    lay->addLayout(bottomLay);

    // ── Initial state ────────────────────────────────────────────────────
    updateToolButtons();
    updateColorButtons();
    updateSizePreview();
    updateDrawButton();
    updateDeleteTooltip();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Styles
// ═══════════════════════════════════════════════════════════════════════════

void HoverTab::applyStyles()
{
    static const QString kToolBtn = QStringLiteral(R"(
        QPushButton {
            background:#2a2a3e; color:#b0b0c0; border:1px solid #3a3a4e;
            border-radius:5px; padding:2px 6px; font-weight:bold; font-size:10px;
        }
        QPushButton:hover    { background:#3a3a4e; }
        QPushButton:checked  { background:#3d5afe; color:#fff; border-color:#536dfe; }
    )");
    m_penBtn->setStyleSheet(kToolBtn);
    m_highlighterBtn->setStyleSheet(kToolBtn);

    m_sizeSlider->setStyleSheet(QStringLiteral(R"(
        QSlider::groove:horizontal { background:#2a2a3e; height:4px; border-radius:2px; }
        QSlider::handle:horizontal { background:#d0d0e0; width:12px; height:12px;
                                     margin:-4px 0; border-radius:6px; }
        QSlider::sub-page:horizontal { background:#3d5afe; border-radius:2px; }
    )"));

    m_deleteBtn->setStyleSheet(QStringLiteral(R"(
        QPushButton {
            background:#b71c1c; color:#fff; border:1px solid #c62828;
            border-radius:5px; font-weight:bold; font-size:11px;
        }
        QPushButton:hover   { background:#d32f2f; }
        QPushButton:pressed  { background:#e53935; }
    )"));

    m_settingsBtn->setStyleSheet(QStringLiteral(R"(
        QPushButton {
            background:transparent; color:#606078; border:none;
            font-size:13px;
        }
        QPushButton:hover { color:#a0a0c0; }
    )"));

    m_customColorBtn->setStyleSheet(QStringLiteral(R"(
        QPushButton {
            background:#2a2a3e; color:#b0b0c0; border:2px solid #3a3a4e;
            border-radius:3px; font-weight:bold; font-size:12px;
        }
        QPushButton:hover { border-color:rgba(255,255,255,0.5); }
    )"));

    updateColorButtons();
    updateDrawButton();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Visual helpers
// ═══════════════════════════════════════════════════════════════════════════

void HoverTab::updateToolButtons()
{
    m_penBtn->setChecked(m_tool == MarkerTool::Pen);
    m_highlighterBtn->setChecked(m_tool == MarkerTool::Highlighter);
}

void HoverTab::updateColorButtons()
{
    for (int i = 0; i < m_colorBtns.size(); ++i) {
        const QColor& c = kPresetColors[i];
        const bool sel  = (c == m_selectedColor);
        m_colorBtns[i]->setStyleSheet(
            QStringLiteral("QPushButton{background:%1;border:2px solid %2;"
                           "border-radius:3px;}"
                           "QPushButton:hover{border-color:rgba(255,255,255,0.6);}")
                .arg(c.name(), sel ? QStringLiteral("#ffffff")
                                   : QStringLiteral("transparent")));
    }
}

void HoverTab::updateSizePreview()
{
    constexpr int dim = 22;
    QPixmap pix(dim, dim);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    QColor fill = m_selectedColor;
    if (m_tool == MarkerTool::Highlighter)
        fill.setAlpha(80);
    p.setBrush(fill);
    p.setPen(Qt::NoPen);
    const qreal r = qBound(1.0, m_brushSize / 2.0, dim / 2.0);
    p.drawEllipse(QPointF(dim / 2.0, dim / 2.0), r, r);
    p.end();
    m_sizePreview->setPixmap(pix);
}

void HoverTab::updateDrawButton()
{
    if (m_drawingActive) {
        m_drawBtn->setText(QStringLiteral("STOP"));
        m_drawBtn->setStyleSheet(QStringLiteral(R"(
            QPushButton {
                background:#00C853; color:#000; border:1px solid #00E676;
                border-radius:5px; font-weight:bold; font-size:11px;
            }
            QPushButton:hover { background:#00E676; }
        )"));
    } else {
        m_drawBtn->setText(QStringLiteral("DRAW"));
        m_drawBtn->setStyleSheet(QStringLiteral(R"(
            QPushButton {
                background:#1a3a1a; color:#4CAF50; border:1px solid #2e5a2e;
                border-radius:5px; font-weight:bold; font-size:11px;
            }
            QPushButton:hover { background:#2e5a2e; color:#66BB6A; }
        )"));
    }
}

void HoverTab::updateDeleteTooltip()
{
    if (SettingsManager::instance().deleteMode() ==
        DeleteMode::UndoLeft_ClearRight)
        m_deleteBtn->setToolTip(tr("Left: Undo  \u2022  Right: Clear all"));
    else
        m_deleteBtn->setToolTip(tr("Left: Clear all  \u2022  Right: Undo"));
}

void HoverTab::selectColor(const QColor& color)
{
    m_selectedColor = color;
    SettingsManager::instance().setLastColor(color);
    updateColorButtons();
    updateSizePreview();
    update(); // repaint grip icon with new colour
    emit colorChanged(color);
}
