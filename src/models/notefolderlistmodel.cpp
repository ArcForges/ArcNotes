#include "notefolderlistmodel.h"

NoteFolderListModel::NoteFolderListModel(QObject* parent) : QAbstractListModel(parent) {}

int NoteFolderListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : _folders.count();
}

QVariant NoteFolderListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= _folders.count()) {
        return {};
    }

    const NoteFolderData& folder = _folders.at(index.row());
    switch (role) {
        case Qt::DisplayRole:
        case NameRole:
            return folder.name;
        case FolderIdRole:
            return folder.id;
        case LocalPathRole:
            return folder.localPath;
        case PriorityRole:
            return folder.priority;
        case ActiveTagIdRole:
            return folder.activeTagId;
        case ShowSubfoldersRole:
            return folder.showSubfolders;
        case AllSubfoldersRole:
            return folder.allSubfolders;
        default:
            return {};
    }
}

QHash<int, QByteArray> NoteFolderListModel::roleNames() const {
    return {{FolderIdRole, "folderId"},          {NameRole, "name"},
            {LocalPathRole, "localPath"},        {PriorityRole, "priority"},
            {ActiveTagIdRole, "activeTagId"},    {ShowSubfoldersRole, "showSubfolders"},
            {AllSubfoldersRole, "allSubfolders"}};
}

QVector<NoteFolderData> NoteFolderListModel::folders() const {
    return _folders;
}

int NoteFolderListModel::rowForFolderId(int folderId) const {
    return _rowById.value(folderId, -1);
}

void NoteFolderListModel::setFolders(const QVector<NoteFolderData>& folders) {
    beginResetModel();
    _folders = folders;
    rebuildIndex();
    endResetModel();
}

void NoteFolderListModel::updateFolder(const NoteFolderData& folder) {
    const int row = rowForFolderId(folder.id);
    if (row < 0) {
        const int insertRow = _folders.count();
        beginInsertRows(QModelIndex(), insertRow, insertRow);
        _folders.append(folder);
        _rowById.insert(folder.id, insertRow);
        endInsertRows();
        return;
    }

    if (_folders.at(row) == folder) {
        return;
    }

    _folders[row] = folder;
    emit dataChanged(index(row, 0), index(row, 0));
}

void NoteFolderListModel::removeFolder(int folderId) {
    const int row = rowForFolderId(folderId);
    if (row < 0) {
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    _folders.removeAt(row);
    rebuildIndex();
    endRemoveRows();
}

void NoteFolderListModel::clear() {
    setFolders({});
}

void NoteFolderListModel::rebuildIndex() {
    _rowById.clear();
    for (int row = 0; row < _folders.count(); ++row) {
        _rowById.insert(_folders.at(row).id, row);
    }
}
