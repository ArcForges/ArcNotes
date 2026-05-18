#pragma once

#include <core/data/trashitemdata.h>

#include <QAbstractListModel>
#include <QVector>

class TrashItemModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        TrashItemIdRole = Qt::UserRole + 1,
        FileNameRole,
        FileSizeRole,
        SubFolderPathRole,
        SubFolderIdRole,
        CreatedDateRole,
        FullPathRole
    };
    Q_ENUM(Role)

    explicit TrashItemModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVector<TrashItemData> trashItems() const;

public slots:
    void setTrashItems(const QVector<TrashItemData>& trashItems);
    void removeTrashItem(int trashItemId);
    void clear();

private:
    QVector<TrashItemData> _trashItems;
};
