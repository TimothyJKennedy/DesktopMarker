#pragma once

#include <QDialog>

/// Minimal settings dialog — lets the user choose the Delete button behavior
/// (Mode A / Mode B) and offers a Quit button for a clean exit.
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
};
