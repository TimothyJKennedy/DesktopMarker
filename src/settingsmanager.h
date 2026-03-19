#pragma once

#include <QColor>
#include <QPoint>
#include <QSettings>
#include <QString>

enum class DeleteMode {
    UndoLeft_ClearRight,   // Mode A (default): left-click = undo, right-click = clear
    ClearLeft_UndoRight    // Mode B: left-click = clear, right-click = undo
};

enum class MarkerTool { Pen, Highlighter };

/// Singleton that persists all user preferences via QSettings.
class SettingsManager {
public:
    static SettingsManager& instance();

    QColor     lastColor() const;
    void       setLastColor(const QColor& color);

    int        brushSize() const;
    void       setBrushSize(int size);

    MarkerTool markerTool() const;
    void       setMarkerTool(MarkerTool tool);

    DeleteMode deleteMode() const;
    void       setDeleteMode(DeleteMode mode);

    int        tabPositionY() const;
    void       setTabPositionY(int y);

    int        dockEdge() const;        // 0=Top, 1=Bottom, 2=Left, 3=Right (default)
    void       setDockEdge(int edge);

    int        panelSize() const;       // 0 = Compact, 1 = Standard, 2 = Full
    void       setPanelSize(int size);

    bool       drawingActive() const;
    void       setDrawingActive(bool active);

private:
    SettingsManager();
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    QSettings m_settings;
};
