#pragma once

#include <QPropertyAnimation>
#include <QTimer>
#include <QVector>
#include <QWidget>

#include "settingsmanager.h"

class QPushButton;
class QLabel;
class QSlider;

enum class PanelSize { Compact = 0, Standard = 1, Full = 2 };
enum class DockEdge { Top = 0, Bottom = 1, Left = 2, Right = 3 };

/// A tiny marker-icon grip docked to the screen edge.
/// On hover it opens a compact flyout panel with drawing controls.
///
/// Three configurable panel sizes:
///   Compact  — colors + draw/delete only (~160 × 110)
///   Standard — + tool buttons + size slider  (~165 × 195, roughly square)
///   Full     — larger swatches, bigger buttons (~200 × 240)
class HoverTab : public QWidget {
    Q_OBJECT

public:
    explicit HoverTab(QWidget* parent = nullptr);
    ~HoverTab() override;

    void setDrawingActive(bool active);
    [[nodiscard]] bool isDrawingActive() const { return m_drawingActive; }

    void onDialogClosed();
    void refreshPosition();
    void raiseAll();
    void applyPanelSize(PanelSize sz);

signals:
    void colorChanged(const QColor& color);
    void brushSizeChanged(int size);
    void toolChanged(MarkerTool tool);
    void drawingToggled(bool active);
    void undoRequested();
    void clearRequested();
    void settingsRequested();

protected:
    void paintEvent(QPaintEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void showPanel();
    void hidePanel();
    void buildPanel();
    void applyStyles();
    void repositionPanel();
    void reposition();
    void checkProximity();
    void snapToNearestEdge();

    void updateToolButtons();
    void updateColorButtons();
    void updateSizePreview();
    void updateDrawButton();
    void updateDeleteTooltip();
    void selectColor(const QColor& color);

    static constexpr int kGripSize = 36;

    // ── panel ────────────────────────────────────────────────────────────
    QWidget*            m_panel{nullptr};
    QPropertyAnimation* m_panelAnim{nullptr};
    QTimer*             m_collapseTimer{nullptr};
    QTimer*             m_proximityTimer{nullptr};
    bool                m_dialogOpen{false};
    PanelSize           m_panelSize{PanelSize::Standard};

    // Widgets that hide in Compact mode
    QWidget* m_toolSep{nullptr};
    QWidget* m_toolRow{nullptr};
    QWidget* m_sliderRow{nullptr};

    // ── dock edge + drag ─────────────────────────────────────────────────
    DockEdge m_dockEdge{DockEdge::Right};
    int      m_edgePos{300};
    bool     m_dragging{false};
    QPoint   m_dragOffset;

    // ── controls (inside panel) ──────────────────────────────────────────
    QVector<QPushButton*> m_colorBtns;
    QPushButton* m_customColorBtn{nullptr};
    QPushButton* m_penBtn{nullptr};
    QPushButton* m_highlighterBtn{nullptr};
    QSlider*     m_sizeSlider{nullptr};
    QLabel*      m_sizePreview{nullptr};
    QPushButton* m_drawBtn{nullptr};
    QPushButton* m_deleteBtn{nullptr};
    QPushButton* m_settingsBtn{nullptr};

    // ── tool state ───────────────────────────────────────────────────────
    QColor     m_selectedColor;
    int        m_brushSize{4};
    MarkerTool m_tool{MarkerTool::Pen};
    bool       m_drawingActive{false};

    static inline const QVector<QColor> kPresetColors = {
        QColor("#000000"), QColor("#FFFFFF"), QColor("#FF1744"), QColor("#FF9100"),
        QColor("#FFEA00"), QColor("#00E676"), QColor("#00B0FF"), QColor("#2979FF"),
        QColor("#D500F9"), QColor("#F50057"), QColor("#76FF03"), QColor("#FF6E40"),
    };
};
