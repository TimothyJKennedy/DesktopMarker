#include "settingsmanager.h"

#include <algorithm>

SettingsManager::SettingsManager()
    : m_settings(QSettings::IniFormat, QSettings::UserScope,
                 QStringLiteral("DesktopMarker"), QStringLiteral("DesktopMarker"))
{
}

SettingsManager& SettingsManager::instance()
{
    static SettingsManager s;
    return s;
}

// --- Color ----------------------------------------------------------------

QColor SettingsManager::lastColor() const
{
    return m_settings.value(QStringLiteral("tool/color"), QColor(Qt::red)).value<QColor>();
}

void SettingsManager::setLastColor(const QColor& color)
{
    m_settings.setValue(QStringLiteral("tool/color"), color);
}

// --- Brush size -----------------------------------------------------------

int SettingsManager::brushSize() const
{
    return std::clamp(m_settings.value(QStringLiteral("tool/brushSize"), 4).toInt(), 1, 80);
}

void SettingsManager::setBrushSize(int size)
{
    m_settings.setValue(QStringLiteral("tool/brushSize"), std::clamp(size, 1, 80));
}

// --- Marker tool ----------------------------------------------------------

MarkerTool SettingsManager::markerTool() const
{
    return static_cast<MarkerTool>(
        m_settings.value(QStringLiteral("tool/markerTool"), 0).toInt());
}

void SettingsManager::setMarkerTool(MarkerTool tool)
{
    m_settings.setValue(QStringLiteral("tool/markerTool"), static_cast<int>(tool));
}

// --- Delete mode ----------------------------------------------------------

DeleteMode SettingsManager::deleteMode() const
{
    return static_cast<DeleteMode>(
        m_settings.value(QStringLiteral("behavior/deleteMode"), 0).toInt());
}

void SettingsManager::setDeleteMode(DeleteMode mode)
{
    m_settings.setValue(QStringLiteral("behavior/deleteMode"), static_cast<int>(mode));
}

// --- Tab position ---------------------------------------------------------

int SettingsManager::tabPositionY() const
{
    return m_settings.value(QStringLiteral("ui/tabPositionY"), 300).toInt();
}

void SettingsManager::setTabPositionY(int y)
{
    m_settings.setValue(QStringLiteral("ui/tabPositionY"), y);
}

// --- Dock edge ------------------------------------------------------------

int SettingsManager::dockEdge() const
{
    return std::clamp(m_settings.value(QStringLiteral("ui/dockEdge"), 3).toInt(), 0, 3);
}

void SettingsManager::setDockEdge(int edge)
{
    m_settings.setValue(QStringLiteral("ui/dockEdge"), std::clamp(edge, 0, 3));
}

// --- Panel size -----------------------------------------------------------

int SettingsManager::panelSize() const
{
    return std::clamp(m_settings.value(QStringLiteral("ui/panelSize"), 1).toInt(), 0, 2);
}

void SettingsManager::setPanelSize(int size)
{
    m_settings.setValue(QStringLiteral("ui/panelSize"), std::clamp(size, 0, 2));
}

// --- Drawing active flag (not persisted across sessions) ------------------

static bool s_drawingActive = false;

bool SettingsManager::drawingActive() const
{
    return s_drawingActive;
}

void SettingsManager::setDrawingActive(bool active)
{
    s_drawingActive = active;
}
