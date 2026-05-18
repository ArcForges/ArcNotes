#include "notelistmodel.h"

NoteListModel::NoteListModel(QObject* parent) : QAbstractListModel(parent) {}

int NoteListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : _notes.count();
}

QVariant NoteListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= _notes.count()) {
        return {};
    }

    const NoteData& note = _notes.at(index.row());
    switch (role) {
        case Qt::DisplayRole:
        case TitleRole:
            return note.name;
        case NoteIdRole:
            return note.id;
        case PreviewRole:
            return previewText(note);
        case FileNameRole:
            return note.fileName;
        case ModifiedDateRole:
            return note.modified;
        case CreatedDateRole:
            return note.created;
        case FileSizeRole:
            return note.fileSize;
        case IsDirtyRole:
            return note.hasDirtyData;
        case SubFolderIdRole:
            return note.noteSubFolderId;
        case IsFavoriteRole:
            return note.isFavorite;
        default:
            return {};
    }
}

bool NoteListModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.row() < 0 || index.row() >= _notes.count() || role != Qt::EditRole) {
        return false;
    }

    const QString name = value.toString().trimmed();
    if (name.isEmpty() || name == _notes.at(index.row()).name) {
        return false;
    }

    emit noteRenameRequested(_notes.at(index.row()).id, name);
    return true;
}

Qt::ItemFlags NoteListModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
}

QHash<int, QByteArray> NoteListModel::roleNames() const {
    return {{NoteIdRole, "noteId"},
            {TitleRole, "title"},
            {PreviewRole, "preview"},
            {FileNameRole, "fileName"},
            {ModifiedDateRole, "modifiedDate"},
            {CreatedDateRole, "createdDate"},
            {FileSizeRole, "fileSize"},
            {IsDirtyRole, "isDirty"},
            {SubFolderIdRole, "subFolderId"},
            {IsFavoriteRole, "isFavorite"}};
}

QVector<NoteData> NoteListModel::notes() const {
    return _notes;
}

NoteData NoteListModel::noteAt(int row) const {
    return row >= 0 && row < _notes.count() ? _notes.at(row) : NoteData();
}

int NoteListModel::rowForNoteId(int noteId) const {
    return _rowById.value(noteId, -1);
}

void NoteListModel::setNotes(const QVector<NoteData>& notes) {
    beginResetModel();
    _notes = notes;
    rebuildIndex();
    endResetModel();
}

void NoteListModel::addNote(const NoteData& note) {
    if (_rowById.contains(note.id)) {
        updateNote(note);
        return;
    }

    const int row = _notes.count();
    beginInsertRows(QModelIndex(), row, row);
    _notes.append(note);
    _rowById.insert(note.id, row);
    endInsertRows();
}

void NoteListModel::removeNote(int noteId) {
    const int row = rowForNoteId(noteId);
    if (row < 0) {
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    _notes.removeAt(row);
    rebuildIndex();
    endRemoveRows();
}

void NoteListModel::updateNote(const NoteData& note) {
    const int row = rowForNoteId(note.id);
    if (row < 0) {
        addNote(note);
        return;
    }

    if (_notes.at(row) == note) {
        return;
    }

    _notes[row] = note;
    emit dataChanged(index(row, 0), index(row, 0));
}

void NoteListModel::clear() {
    setNotes({});
}

void NoteListModel::rebuildIndex() {
    _rowById.clear();
    for (int row = 0; row < _notes.count(); ++row) {
        _rowById.insert(_notes.at(row).id, row);
    }
}

QString NoteListModel::previewText(const NoteData& note) const {
    QString preview = note.noteText.simplified();
    constexpr int maxPreviewLength = 160;
    if (preview.length() > maxPreviewLength) {
        preview.truncate(maxPreviewLength);
    }
    return preview;
}
