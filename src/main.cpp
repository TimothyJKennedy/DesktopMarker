#include <QApplication>
#include <QAbstractNativeEventFilter>
#include <QGuiApplication>
#include <QScreen>

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
