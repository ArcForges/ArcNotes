#pragma once

#include <core/data/trashitemdata.h>

#include <QString>
#include <QVector>

class LocalTrashDialogHost {
public:
    virtual ~LocalTrashDialogHost() = default;

    [[nodiscard]] virtual QVector<TrashItemData> localTrashItems() const = 0;
    [[nodiscard]] virtual QString localTrashItemText(int trashItemId) const = 0;
    [[nodiscard]] virtual QString localTrashItemRestorationPath(int trashItemId) const = 0;
    [[nodiscard]] virtual bool localTrashItemFileExists(int trashItemId) const = 0;
    virtual bool localTrashRestoreItems(const QVector<int>& trashItemIds) = 0;
    virtual bool localTrashRemoveItems(const QVector<int>& trashItemIds) = 0;
};
