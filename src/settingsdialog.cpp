#include "settingsdialog.h"
#include "settingsmanager.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("DesktopMarker Settings"));
    setFixedSize(380, 360);

    auto* root = new QVBoxLayout(this);
    root->setSpacing(12);

    // ── Delete-mode group ────────────────────────────────────────────────
    auto* delGroup = new QGroupBox(tr("Delete Button Behavior"), this);
    auto* dlay     = new QVBoxLayout(delGroup);
    dlay->setSpacing(6);

    auto* modeA = new QRadioButton(
        tr("Mode A :  Left = Undo  \u2022  Right = Clear all"), delGroup);
    auto* modeB = new QRadioButton(
        tr("Mode B :  Left = Clear all  \u2022  Right = Undo"), delGroup);

    const auto curDel = SettingsManager::instance().deleteMode();
    modeA->setChecked(curDel == DeleteMode::UndoLeft_ClearRight);
    modeB->setChecked(curDel == DeleteMode::ClearLeft_UndoRight);
    dlay->addWidget(modeA);
    dlay->addWidget(modeB);
    root->addWidget(delGroup);

    // ── Panel size group ─────────────────────────────────────────────────
    auto* sizeGroup = new QGroupBox(tr("Panel Size"), this);
    auto* slay      = new QVBoxLayout(sizeGroup);
    slay->setSpacing(6);

    auto* szCompact  = new QRadioButton(tr("Compact  \u2014  colours + draw/delete only"), sizeGroup);
    auto* szStandard = new QRadioButton(tr("Standard  \u2014  + tools + brush slider"), sizeGroup);
    auto* szFull     = new QRadioButton(tr("Full  \u2014  larger swatches & buttons"), sizeGroup);

    const int curSz = SettingsManager::instance().panelSize();
    szCompact->setChecked(curSz == 0);
    szStandard->setChecked(curSz == 1);
    szFull->setChecked(curSz == 2);
    slay->addWidget(szCompact);
    slay->addWidget(szStandard);
    slay->addWidget(szFull);
    root->addWidget(sizeGroup);

    // ── Hotkey info ──────────────────────────────────────────────────────
    auto* info = new QLabel(tr("Global hotkey:  Ctrl + Shift + D  (toggle drawing)"), this);
    info->setStyleSheet(QStringLiteral("color:#808090; font-size:10px; padding-left:4px;"));
    root->addWidget(info);

    root->addStretch();

    // ── OK / Cancel ──────────────────────────────────────────────────────
    auto* bbox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(bbox);

    connect(bbox, &QDialogButtonBox::accepted, this,
            [this, modeA, szCompact, szStandard]() {
        SettingsManager::instance().setDeleteMode(
            modeA->isChecked() ? DeleteMode::UndoLeft_ClearRight
                               : DeleteMode::ClearLeft_UndoRight);
        int sz = szCompact->isChecked() ? 0 : szStandard->isChecked() ? 1 : 2;
        SettingsManager::instance().setPanelSize(sz);
        accept();
    });
    connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // ── Quit button ──────────────────────────────────────────────────────
    auto* quitBtn = new QPushButton(tr("Quit DesktopMarker"), this);
    quitBtn->setFixedHeight(28);
    root->addWidget(quitBtn);
    connect(quitBtn, &QPushButton::clicked, qApp, &QApplication::quit);

    // ── Dark theme ───────────────────────────────────────────────────────
    setStyleSheet(QStringLiteral(R"(
        QDialog {
            background:#1e1e2e; color:#d0d0e0;
        }
        QGroupBox {
            color:#a0a0c0; border:1px solid #3a3a4e; border-radius:6px;
            margin-top:14px; padding-top:18px; font-weight:bold;
        }
        QGroupBox::title {
            subcontrol-origin:margin; left:12px; padding:0 4px;
        }
        QRadioButton              { color:#c0c0d0; spacing:8px; padding:3px 0; }
        QRadioButton::indicator   { width:14px; height:14px; border-radius:7px;
                                    border:2px solid #5a5a7a; }
        QRadioButton::indicator:checked {
            background:#3d5afe; border-color:#536dfe;
        }
        QPushButton {
            background:#2a2a3e; color:#c0c0d0; border:1px solid #3a3a4e;
            border-radius:4px; padding:6px 16px;
        }
        QPushButton:hover   { background:#3a3a4e; }
        QPushButton:pressed  { background:#4a4a5e; }
    )"));
}
