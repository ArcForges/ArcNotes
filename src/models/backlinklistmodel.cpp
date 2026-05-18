#include "backlinklistmodel.h"

BacklinkListModel::BacklinkListModel(QObject* parent) : QAbstractListModel(parent) {}

int BacklinkListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : _backlinks.count();
}

QVariant BacklinkListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= _backlinks.count()) {
        return {};
    }

    const BacklinkItemData& item = _backlinks.at(index.row());
    switch (role) {
        case Qt::DisplayRole:
        case TitleRole:
            return item.title;
        case NoteIdRole:
            return item.noteId;
        case FileNameRole:
            return item.fileName;
        case ContextRole:
            return item.context;
        default:
            return {};
    }
}

QHash<int, QByteArray> BacklinkListModel::roleNames() const {
    return {{NoteIdRole, "noteId"}, {TitleRole, "title"}, {FileNameRole, "fileName"}, {ContextRole, "context"}};
}

QVector<BacklinkItemData> BacklinkListModel::backlinks() const {
    return _backlinks;
}

void BacklinkListModel::setBacklinks(const QVector<BacklinkItemData>& backlinks) {
    beginResetModel();
    _backlinks = backlinks;
    endResetModel();
}

void BacklinkListModel::clear() {
    setBacklinks({});
}
