#include <QApplication>
#include <QAbstractNativeEventFilter>
#include <QGuiApplication>
#include <QLabel>
#include <QPainter>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

#include <functional>

#include "drawingoverlay.h"
#include "hovertab.h"
#include "settingsdialog.h"
#include "settingsmanager.h"
#include "strokemanager.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

// ═══════════════════════════════════════════════════════════════════════════
//  Global hotkey  (Ctrl+Shift+D  →  WM_HOTKEY on Windows)
// ═══════════════════════════════════════════════════════════════════════════

static constexpr int kHotkeyId = 1;

class HotkeyFilter : public QAbstractNativeEventFilter {
public:
    std::function<void()> onToggle;

    bool nativeEventFilter(const QByteArray& eventType,
                           void* message, qintptr* /*result*/) override
    {
#ifdef Q_OS_WIN
        if (eventType == "windows_generic_MSG") {
            const auto* msg = static_cast<MSG*>(message);
            if (msg->message == WM_HOTKEY && msg->wParam == kHotkeyId) {
                if (onToggle) onToggle();
                return true;
            }
        }
#else
        Q_UNUSED(eventType); Q_UNUSED(message);
#endif
        return false;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════════════════════════

static StrokeType toStrokeType(MarkerTool t)
{
    return t == MarkerTool::Pen ? StrokeType::Pen : StrokeType::Highlighter;
}

// ═══════════════════════════════════════════════════════════════════════════
//  main
// ═══════════════════════════════════════════════════════════════════════════

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("DesktopMarker"));
    app.setOrganizationName(QStringLiteral("DesktopMarker"));
    app.setQuitOnLastWindowClosed(false);

    // ── Splash screen ────────────────────────────────────────────────────
    auto* splash = new QWidget(nullptr, Qt::FramelessWindowHint
                                        | Qt::WindowStaysOnTopHint);
    splash->setAttribute(Qt::WA_TranslucentBackground);
    splash->setAttribute(Qt::WA_DeleteOnClose);
    splash->setFixedSize(320, 340);

    auto* slay = new QVBoxLayout(splash);
    slay->setAlignment(Qt::AlignCenter);
    slay->setSpacing(10);
    slay->setContentsMargins(30, 28, 30, 24);

    auto* bg = new QFrame(splash);
    bg->setStyleSheet(QStringLiteral(
        "QFrame{background:qlineargradient(y1:0,y2:1,stop:0 #1a1a32,stop:1 #0e0e1e);"
        "border:1px solid #2a2a4a; border-radius:16px;}"));
    bg->setFixedSize(320, 340);
    bg->move(0, 0);
    bg->lower();

    QPixmap icon(QStringLiteral(":/icon.png"));
    auto* iconLabel = new QLabel(splash);
    iconLabel->setPixmap(icon.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setAlignment(Qt::AlignCenter);
    slay->addWidget(iconLabel);

    auto* title = new QLabel(QStringLiteral("DesktopMarker"), splash);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral(
        "color:#e0e0f0; font-size:22px; font-weight:bold; background:transparent;"));
    slay->addWidget(title);

    auto* ver = new QLabel(QStringLiteral("v1.0.2"), splash);
    ver->setAlignment(Qt::AlignCenter);
    ver->setStyleSheet(QStringLiteral(
        "color:#6a6a8a; font-size:13px; background:transparent;"));
    slay->addWidget(ver);

    slay->addSpacing(6);

    auto* credit = new QLabel(QStringLiteral("Created by Timothy J Kennedy  \u00b7  2026"), splash);
    credit->setAlignment(Qt::AlignCenter);
    credit->setStyleSheet(QStringLiteral(
        "color:#8080a0; font-size:11px; background:transparent;"));
    slay->addWidget(credit);

    splash->move(QGuiApplication::primaryScreen()->geometry().center()
                 - QPoint(160, 170));
    splash->show();
    app.processEvents();

    QTimer::singleShot(2500, splash, &QWidget::close);

    // ── Core objects ─────────────────────────────────────────────────────
    StrokeManager  strokeMgr;
    DrawingOverlay overlay(&strokeMgr);
    HoverTab       tab;

    auto& settings = SettingsManager::instance();
    overlay.setCurrentColor(settings.lastColor());
    overlay.setCurrentWidth(static_cast<qreal>(settings.brushSize()));
    overlay.setCurrentStrokeType(toStrokeType(settings.markerTool()));

    // ── Drawing-mode toggle (single source of truth) ─────────────────────
    auto toggleDrawing = [&](bool active) {
        overlay.setDrawingEnabled(active);
        tab.setDrawingActive(active);
        if (active) {
            overlay.raise();                // bring overlay into the topmost stack
            overlay.activateWindow();       // so Esc key reaches the overlay
        }
        tab.raiseAll();                     // grip + panel stay above overlay
    };

    // ── Tab → Overlay wiring ─────────────────────────────────────────────
    QObject::connect(&tab, &HoverTab::colorChanged,
                     &overlay, &DrawingOverlay::setCurrentColor);

    QObject::connect(&tab, &HoverTab::brushSizeChanged, [&](int size) {
        overlay.setCurrentWidth(static_cast<qreal>(size));
    });

    QObject::connect(&tab, &HoverTab::toolChanged, [&](MarkerTool tool) {
        overlay.setCurrentStrokeType(toStrokeType(tool));
    });

    QObject::connect(&tab, &HoverTab::drawingToggled, toggleDrawing);

    // ── Tab → StrokeManager ──────────────────────────────────────────────
    QObject::connect(&tab, &HoverTab::undoRequested,
                     &strokeMgr, &StrokeManager::undoLast);
    QObject::connect(&tab, &HoverTab::clearRequested,
                     &strokeMgr, &StrokeManager::clearAll);

    // ── Tab → Settings dialog ────────────────────────────────────────────
    QObject::connect(&tab, &HoverTab::settingsRequested, [&] {
        SettingsDialog dlg;
        dlg.exec();                         // modal — blocks until closed
        tab.onDialogClosed();
        // Re-apply panel size in case the user changed it
        tab.applyPanelSize(
            static_cast<PanelSize>(SettingsManager::instance().panelSize()));
    });

    // ── Overlay → toggle off (Esc pressed while drawing) ─────────────────
    QObject::connect(&overlay, &DrawingOverlay::drawingToggleRequested, [&] {
        toggleDrawing(false);
    });

    // ── Screen geometry changes ──────────────────────────────────────────
    auto repositionAll = [&] {
        overlay.repositionOnScreens();
        tab.refreshPosition();
    };

    QObject::connect(qApp, &QGuiApplication::primaryScreenChanged, repositionAll);
    QObject::connect(qApp, &QGuiApplication::screenAdded,
                     [&](QScreen* screen) {
        repositionAll();
        QObject::connect(screen, &QScreen::geometryChanged, repositionAll);
    });
    QObject::connect(qApp, &QGuiApplication::screenRemoved, repositionAll);

    for (QScreen* screen : QGuiApplication::screens())
        QObject::connect(screen, &QScreen::geometryChanged, repositionAll);

    // ── Global hotkey  (Ctrl + Shift + D) ────────────────────────────────
    HotkeyFilter hotkeyFilter;
    hotkeyFilter.onToggle = [&] {
        toggleDrawing(!overlay.isDrawingEnabled());
    };
    app.installNativeEventFilter(&hotkeyFilter);

#ifdef Q_OS_WIN
    RegisterHotKey(nullptr, kHotkeyId,
                   MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, 'D');
#endif

    // ── Ensure initial z-order: tab always above overlay ─────────────────
    tab.raiseAll();

    // ── Run ──────────────────────────────────────────────────────────────
    const int ret = app.exec();

#ifdef Q_OS_WIN
    UnregisterHotKey(nullptr, kHotkeyId);
#endif

    return ret;
}
