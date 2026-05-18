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

#pragma once

#include "masterdialog.h"

class QListWidget;
class QListWidgetItem;
class QPushButton;

class LayoutDialog : public MasterDialog {
    Q_OBJECT

public:
    explicit LayoutDialog(QWidget* parent = nullptr);
    ~LayoutDialog() override;

private slots:
    void on_addButton_clicked();
    void on_removeButton_clicked();
    void on_renameButton_clicked();
    void on_moveUpButton_clicked();
    void on_moveDownButton_clicked();
    void on_layoutListWidget_currentRowChanged(int currentRow);
    void on_layoutListWidget_itemDoubleClicked();

private:
    QListWidget* _layoutListWidget = nullptr;
    QPushButton* _removeButton = nullptr;
    QPushButton* _renameButton = nullptr;
    QPushButton* _moveUpButton = nullptr;
    QPushButton* _moveDownButton = nullptr;

    void reloadLayoutList();
    void updateButtonStates();
    void persistLayoutOrder();
};
