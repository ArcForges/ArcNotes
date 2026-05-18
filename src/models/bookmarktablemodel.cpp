#include "bookmarktablemodel.h"

BookmarkTableModel::BookmarkTableModel(QObject* parent) : QAbstractTableModel(parent) {}

int BookmarkTableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : _bookmarks.count();
}

int BookmarkTableModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return ColumnCount;
}

QVariant BookmarkTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= _bookmarks.count()) {
        return {};
    }

    const BookmarkItemData& bookmark = _bookmarks.at(index.row());
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case SlotColumn:
                return bookmark.slot;
            case NoteColumn:
                return bookmark.historyItem.noteName;
            case SubFolderColumn:
                return bookmark.historyItem.noteSubFolderPathData;
            case CursorColumn:
                return bookmark.historyItem.cursorPosition;
            default:
                return {};
        }
    }

    switch (role) {
        case SlotRole:
            return bookmark.slot;
        case NoteNameRole:
            return bookmark.historyItem.noteName;
        case SubFolderPathRole:
            return bookmark.historyItem.noteSubFolderPathData;
        case CursorPositionRole:
            return bookmark.historyItem.cursorPosition;
        case RelativeScrollRole:
            return bookmark.historyItem.relativeScrollBarPosition;
        default:
            return {};
    }
}

QVariant BookmarkTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
        case SlotColumn:
            return tr("Slot");
        case NoteColumn:
            return tr("Note");
        case SubFolderColumn:
            return tr("Subfolder");
        case CursorColumn:
            return tr("Position");
        default:
            return {};
    }
}

QHash<int, QByteArray> BookmarkTableModel::roleNames() const {
    return {{SlotRole, "slot"},
            {NoteNameRole, "noteName"},
            {SubFolderPathRole, "subFolderPath"},
            {CursorPositionRole, "cursorPosition"},
            {RelativeScrollRole, "relativeScroll"}};
}

QVector<BookmarkItemData> BookmarkTableModel::bookmarks() const {
    return _bookmarks;
}

void BookmarkTableModel::setBookmarks(const QVector<BookmarkItemData>& bookmarks) {
    beginResetModel();
    _bookmarks = bookmarks;
    endResetModel();
}

void BookmarkTableModel::upsertBookmark(const BookmarkItemData& bookmark) {
    for (int row = 0; row < _bookmarks.count(); ++row) {
        if (_bookmarks.at(row).slot == bookmark.slot) {
            if (_bookmarks.at(row) == bookmark) {
                return;
            }
            _bookmarks[row] = bookmark;
            emit dataChanged(index(row, 0), index(row, ColumnCount - 1));
            return;
        }
    }

    const int row = _bookmarks.count();
    beginInsertRows(QModelIndex(), row, row);
    _bookmarks.append(bookmark);
    endInsertRows();
}

void BookmarkTableModel::removeBookmark(int slot) {
    for (int row = 0; row < _bookmarks.count(); ++row) {
        if (_bookmarks.at(row).slot == slot) {
            beginRemoveRows(QModelIndex(), row, row);
            _bookmarks.removeAt(row);
            endRemoveRows();
            return;
        }
    }
}

void BookmarkTableModel::clear() {
    setBookmarks({});
}
