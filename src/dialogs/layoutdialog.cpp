/*
 * Copyright (c) 2014-2026 Patrizio Bekerle -- <patrizio@bekerle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 */

#include "layoutdialog.h"

#include <utils/gui.h>
#include <utils/misc.h>
#include <viewmodels/viewmodellocator.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>

LayoutDialog::LayoutDialog(QWidget* parent) : MasterDialog(parent) {
    setWindowTitle(tr("Manage layouts"));
    resize(420, 320);

    auto* mainLayout = new QVBoxLayout(this);
    auto* contentLayout = new QHBoxLayout;
    mainLayout->addLayout(contentLayout);

    _layoutListWidget = new QListWidget(this);
    _layoutListWidget->setAlternatingRowColors(true);
    contentLayout->addWidget(_layoutListWidget);

    auto* buttonLayout = new QVBoxLayout;
    contentLayout->addLayout(buttonLayout);

    auto createButton = [this, buttonLayout](const QString& text, const QString& toolTip, const QString& themeName,
                                             const QString& fallbackPath) {
        auto* button = new QPushButton(text, this);
        button->setToolTip(toolTip);
        button->setIcon(QIcon::fromTheme(themeName, QIcon(fallbackPath)));
        buttonLayout->addWidget(button);
        return button;
    };

    auto* addButton = createButton(tr("Add"), tr("Add layout"), QStringLiteral("list-add"),
                                   QStringLiteral(":/icons/breeze-arcnotes/16x16/list-add.svg"));
    _removeButton = createButton(tr("Remove"), tr("Remove layout"), QStringLiteral("list-remove"),
                                 QStringLiteral(":/icons/breeze-arcnotes/16x16/list-remove.svg"));
    _renameButton = createButton(tr("Rename"), tr("Rename layout"), QStringLiteral("document-edit"),
                                 QStringLiteral(":/icons/breeze-arcnotes/16x16/document-edit.svg"));

    buttonLayout->addStretch();

    _moveUpButton = createButton(tr("Move up"), tr("Move layout up"), QStringLiteral("go-up"),
                                 QStringLiteral(":/icons/breeze-arcnotes/16x16/go-up.svg"));
    _moveDownButton = createButton(tr("Move down"), tr("Move layout down"), QStringLiteral("go-down"),
                                   QStringLiteral(":/icons/breeze-arcnotes/16x16/go-down.svg"));

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    mainLayout->addWidget(buttonBox);

    connect(addButton, &QPushButton::clicked, this, &LayoutDialog::on_addButton_clicked);
    connect(_removeButton, &QPushButton::clicked, this, &LayoutDialog::on_removeButton_clicked);
    connect(_renameButton, &QPushButton::clicked, this, &LayoutDialog::on_renameButton_clicked);
    connect(_moveUpButton, &QPushButton::clicked, this, &LayoutDialog::on_moveUpButton_clicked);
    connect(_moveDownButton, &QPushButton::clicked, this, &LayoutDialog::on_moveDownButton_clicked);
    connect(_layoutListWidget, &QListWidget::currentRowChanged, this,
            &LayoutDialog::on_layoutListWidget_currentRowChanged);
    connect(_layoutListWidget, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem*) { on_layoutListWidget_itemDoubleClicked(); });
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    afterSetupUI();
    reloadLayoutList();
}

LayoutDialog::~LayoutDialog() = default;

void LayoutDialog::reloadLayoutList() {
    SettingsViewModel* viewModel = ViewModelLocator::settingsViewModel();
    const QStringList uuids =
        viewModel == nullptr ? QStringList() : viewModel->persistentSetting(QStringLiteral("layouts")).toStringList();
    const QString currentUuid =
        viewModel == nullptr ? QString() : viewModel->persistentSetting(QStringLiteral("currentLayout")).toString();

    _layoutListWidget->clear();

    for (const QString& uuid : uuids) {
        const QString name =
            viewModel == nullptr
                ? QString()
                : viewModel->persistentSetting(QStringLiteral("layout-") + uuid + QStringLiteral("/name")).toString();

        auto* item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, uuid);
        _layoutListWidget->addItem(item);

        if (uuid == currentUuid) {
            _layoutListWidget->setCurrentItem(item);
        }
    }

    updateButtonStates();
}

void LayoutDialog::updateButtonStates() {
    const int count = _layoutListWidget->count();
    const int currentRow = _layoutListWidget->currentRow();
    const bool hasSelection = currentRow >= 0;

    _removeButton->setEnabled(hasSelection && count > 1);
    _renameButton->setEnabled(hasSelection);
    _moveUpButton->setEnabled(hasSelection && currentRow > 0);
    _moveDownButton->setEnabled(hasSelection && currentRow < count - 1);
}

void LayoutDialog::persistLayoutOrder() {
    QStringList uuids;
    uuids.reserve(_layoutListWidget->count());

    for (int i = 0; i < _layoutListWidget->count(); ++i) {
        uuids << _layoutListWidget->item(i)->data(Qt::UserRole).toString();
    }

    if (SettingsViewModel* viewModel = ViewModelLocator::settingsViewModel()) {
        viewModel->setPersistentSetting(QStringLiteral("layouts"), uuids);
    }
}

void LayoutDialog::on_addButton_clicked() {
    const QString name = QInputDialog::getText(this, tr("Add layout"), tr("Layout name:")).trimmed();

    if (name.isEmpty()) {
        return;
    }

    SettingsViewModel* viewModel = ViewModelLocator::settingsViewModel();
    if (viewModel == nullptr) {
        return;
    }

    const QString uuid = Utils::Misc::createUuidString();
    QStringList uuids = viewModel->persistentSetting(QStringLiteral("layouts")).toStringList();
    uuids.append(uuid);

    viewModel->setPersistentSetting(QStringLiteral("layouts"), uuids);
    viewModel->setPersistentSetting(QStringLiteral("layout-") + uuid + QStringLiteral("/name"), name);

    reloadLayoutList();

    for (int i = 0; i < _layoutListWidget->count(); ++i) {
        if (_layoutListWidget->item(i)->data(Qt::UserRole).toString() == uuid) {
            _layoutListWidget->setCurrentRow(i);
            break;
        }
    }
}

void LayoutDialog::on_removeButton_clicked() {
    QListWidgetItem* item = _layoutListWidget->currentItem();
    if (item == nullptr || _layoutListWidget->count() < 2) {
        return;
    }

    if (Utils::Gui::question(this, tr("Remove layout"), tr("Remove the selected layout?"),
                             QStringLiteral("remove-layout")) != QMessageBox::Yes) {
        return;
    }

    const QString uuid = item->data(Qt::UserRole).toString();
    SettingsViewModel* viewModel = ViewModelLocator::settingsViewModel();
    if (viewModel == nullptr) {
        return;
    }

    QStringList uuids = viewModel->persistentSetting(QStringLiteral("layouts")).toStringList();
    uuids.removeAll(uuid);
    viewModel->setPersistentSetting(QStringLiteral("layouts"), uuids);

    viewModel->removePersistentSetting(QStringLiteral("layout-") + uuid);

    const QString currentUuid = viewModel->persistentSetting(QStringLiteral("currentLayout")).toString();
    if (currentUuid == uuid) {
        viewModel->setPersistentSetting(QStringLiteral("currentLayout"), uuids.isEmpty() ? QString() : uuids.at(0));
    }

    reloadLayoutList();
}

void LayoutDialog::on_renameButton_clicked() {
    QListWidgetItem* item = _layoutListWidget->currentItem();
    if (item == nullptr) {
        return;
    }

    const QString uuid = item->data(Qt::UserRole).toString();
    SettingsViewModel* viewModel = ViewModelLocator::settingsViewModel();
    if (viewModel == nullptr) {
        return;
    }

    const QString oldName =
        viewModel->persistentSetting(QStringLiteral("layout-") + uuid + QStringLiteral("/name")).toString();

    const QString newName =
        QInputDialog::getText(this, tr("Rename layout"), tr("Layout name:"), QLineEdit::Normal, oldName).trimmed();

    if (newName.isEmpty() || newName == oldName) {
        return;
    }

    viewModel->setPersistentSetting(QStringLiteral("layout-") + uuid + QStringLiteral("/name"), newName);

    item->setText(newName);
    updateButtonStates();
}

void LayoutDialog::on_moveUpButton_clicked() {
    const int row = _layoutListWidget->currentRow();
    if (row <= 0) {
        return;
    }

    QListWidgetItem* item = _layoutListWidget->takeItem(row);
    _layoutListWidget->insertItem(row - 1, item);
    _layoutListWidget->setCurrentRow(row - 1);

    persistLayoutOrder();
    updateButtonStates();
}

void LayoutDialog::on_moveDownButton_clicked() {
    const int row = _layoutListWidget->currentRow();
    if (row < 0 || row >= _layoutListWidget->count() - 1) {
        return;
    }

    QListWidgetItem* item = _layoutListWidget->takeItem(row);
    _layoutListWidget->insertItem(row + 1, item);
    _layoutListWidget->setCurrentRow(row + 1);

    persistLayoutOrder();
    updateButtonStates();
}

void LayoutDialog::on_layoutListWidget_currentRowChanged(int currentRow) {
    Q_UNUSED(currentRow)
    updateButtonStates();
}

void LayoutDialog::on_layoutListWidget_itemDoubleClicked() {
    on_renameButton_clicked();
}
