#include "trashitemmodel.h"

TrashItemModel::TrashItemModel(QObject* parent) : QAbstractListModel(parent) {}

int TrashItemModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : _trashItems.count();
}

QVariant TrashItemModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= _trashItems.count()) {
        return {};
    }

    const TrashItemData& item = _trashItems.at(index.row());
    switch (role) {
        case Qt::DisplayRole:
        case FileNameRole:
            return item.fileName;
        case TrashItemIdRole:
            return item.id;
        case FileSizeRole:
            return item.fileSize;
        case SubFolderPathRole:
            return item.noteSubFolderPathData;
        case SubFolderIdRole:
            return item.noteSubFolderId;
        case CreatedDateRole:
            return item.created;
        case FullPathRole:
            return item.fullNoteFilePath;
        default:
            return {};
    }
}

QHash<int, QByteArray> TrashItemModel::roleNames() const {
    return {{TrashItemIdRole, "trashItemId"}, {FileNameRole, "fileName"},
            {FileSizeRole, "fileSize"},       {SubFolderPathRole, "subFolderPath"},
            {SubFolderIdRole, "subFolderId"}, {CreatedDateRole, "createdDate"},
            {FullPathRole, "fullPath"}};
}

QVector<TrashItemData> TrashItemModel::trashItems() const {
    return _trashItems;
}

void TrashItemModel::setTrashItems(const QVector<TrashItemData>& trashItems) {
    beginResetModel();
    _trashItems = trashItems;
    endResetModel();
}

void TrashItemModel::removeTrashItem(int trashItemId) {
    for (int row = 0; row < _trashItems.count(); ++row) {
        if (_trashItems.at(row).id == trashItemId) {
            beginRemoveRows(QModelIndex(), row, row);
            _trashItems.removeAt(row);
            endRemoveRows();
            return;
        }
    }
}

void TrashItemModel::clear() {
    setTrashItems({});
}
