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
 */

#include "notefolderlistwidget.h"

NoteFolderListWidget::NoteFolderListWidget(QWidget* parent) : QListWidget(parent) {}

void NoteFolderListWidget::dropEvent(QDropEvent* e) {
    // finish the move event
    QListWidget::dropEvent(e);

    int itemCount = count();

    if (itemCount == 0) {
        return;
    }

    QVector<int> folderIds;
    folderIds.reserve(itemCount);
    for (int index = 0; index < itemCount; index++) {
        QListWidgetItem* listItem = item(index);
        folderIds.append(listItem->data(Qt::UserRole).toInt());
    }

    emit folderOrderChanged(folderIds);
}
