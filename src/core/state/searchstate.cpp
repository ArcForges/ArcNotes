#include "searchstate.h"

SearchState::SearchState(QObject* parent) : QObject(parent) {}

QString SearchState::query() const {
    return _query;
}

bool SearchState::isSearching() const {
    return _searching;
}

int SearchState::resultCount() const {
    return _resultNoteIds.count();
}

QString SearchState::searchMode() const {
    return _searchMode;
}

QVector<int> SearchState::resultNoteIds() const {
    return _resultNoteIds;
}

void SearchState::setQuery(const QString& query) {
    if (_query == query) {
        return;
    }

    _query = query;
    emit queryChanged(_query);
}

void SearchState::setSearching(bool searching) {
    if (_searching == searching) {
        return;
    }

    _searching = searching;
    emit searchingChanged(_searching);
}

void SearchState::setSearchMode(const QString& mode) {
    if (_searchMode == mode) {
        return;
    }

    _searchMode = mode;
    emit searchModeChanged(_searchMode);
}

void SearchState::setResultNoteIds(const QVector<int>& noteIds) {
    if (_resultNoteIds == noteIds) {
        return;
    }

    const int previousCount = resultCount();
    _resultNoteIds = noteIds;
    emit resultNoteIdsChanged(_resultNoteIds);
    if (previousCount != resultCount()) {
        emit resultCountChanged(resultCount());
    }
}

void SearchState::clearResults() {
    setResultNoteIds(QVector<int>());
}
