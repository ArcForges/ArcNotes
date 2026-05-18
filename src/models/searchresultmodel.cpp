#include "searchresultmodel.h"

SearchResultModel::SearchResultModel(QObject* parent) : QAbstractListModel(parent) {}

int SearchResultModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : _results.count();
}

QVariant SearchResultModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= _results.count()) {
        return {};
    }

    const NoteData& note = _results.at(index.row());
    switch (role) {
        case Qt::DisplayRole:
        case TitleRole:
            return note.name;
        case NoteIdRole:
            return note.id;
        case FileNameRole:
            return note.fileName;
        case MatchPreviewRole:
            return previewText(note);
        case ModifiedDateRole:
            return note.modified;
        case SubFolderIdRole:
            return note.noteSubFolderId;
        default:
            return {};
    }
}

QHash<int, QByteArray> SearchResultModel::roleNames() const {
    return {{NoteIdRole, "noteId"},
            {TitleRole, "title"},
            {FileNameRole, "fileName"},
            {MatchPreviewRole, "matchPreview"},
            {ModifiedDateRole, "modifiedDate"},
            {SubFolderIdRole, "subFolderId"}};
}

QVector<NoteData> SearchResultModel::results() const {
    return _results;
}

void SearchResultModel::setResults(const QVector<NoteData>& results) {
    beginResetModel();
    _results = results;
    endResetModel();
}

void SearchResultModel::clear() {
    setResults({});
}

QString SearchResultModel::previewText(const NoteData& note) const {
    QString preview = note.noteText.simplified();
    constexpr int maxPreviewLength = 180;
    if (preview.length() > maxPreviewLength) {
        preview.truncate(maxPreviewLength);
    }
    return preview;
}
