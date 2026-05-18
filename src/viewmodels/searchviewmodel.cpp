#include "searchviewmodel.h"

#include <core/commands/commandbus.h>
#include <core/data/commands.h>
#include <core/state/searchstate.h>
#include <utils/debouncer.h>

SearchViewModel::SearchViewModel(CommandBus* commandBus, SearchState* searchState, QObject* parent)
    : QObject(parent),
      _commandBus(commandBus),
      _searchState(searchState),
      _model(this),
      _debouncer(new Debouncer(this)) {
    connect(&_model, &QAbstractItemModel::rowsInserted, this, [this]() { emit resultCountChanged(resultCount()); });
    connect(&_model, &QAbstractItemModel::rowsRemoved, this, [this]() { emit resultCountChanged(resultCount()); });
    connect(&_model, &QAbstractItemModel::modelReset, this, [this]() { emit resultCountChanged(resultCount()); });
    if (_searchState != nullptr) {
        connect(_searchState, &SearchState::queryChanged, this, &SearchViewModel::syncFromState);
        connect(_searchState, &SearchState::searchingChanged, this, &SearchViewModel::syncFromState);
        connect(_searchState, &SearchState::resultCountChanged, this, &SearchViewModel::syncFromState);
        syncFromState();
    }
}

SearchViewModel::~SearchViewModel() = default;

SearchResultModel* SearchViewModel::model() {
    return &_model;
}

const SearchResultModel* SearchViewModel::model() const {
    return &_model;
}

QString SearchViewModel::query() const {
    return _query;
}

bool SearchViewModel::isSearching() const {
    return _searching;
}

int SearchViewModel::resultCount() const {
    return _model.rowCount();
}

void SearchViewModel::setCommandBus(CommandBus* commandBus) {
    _commandBus = commandBus;
}

void SearchViewModel::setSearchState(SearchState* searchState) {
    _searchState = searchState;
    syncFromState();
}

void SearchViewModel::setResults(const QVector<NoteData>& results) {
    _model.setResults(results);
}

void SearchViewModel::search(const QString& query) {
    if (_query != query) {
        _query = query;
        emit queryChanged(_query);
    }
    if (_searchState != nullptr) {
        _searchState->setQuery(query);
    }

    _debouncer->debounce(
        QStringLiteral("notes-search"),
        [this, query]() {
            if (_commandBus == nullptr) {
                emit commandFailed(tr("Command bus is not available."));
                return;
            }
            SearchNotesCommand command;
            command.query = query;
            const CommandResult result = _commandBus->dispatch(command);
            if (!result.success) {
                emit commandFailed(result.errorMessage);
            }
        },
        250);
}

void SearchViewModel::clear() {
    search(QString());
    _model.clear();
}

void SearchViewModel::syncFromState() {
    if (_searchState == nullptr) {
        return;
    }

    if (_query != _searchState->query()) {
        _query = _searchState->query();
        emit queryChanged(_query);
    }
    if (_searching != _searchState->isSearching()) {
        _searching = _searchState->isSearching();
        emit searchingChanged(_searching);
    }
    emit resultCountChanged(resultCount());
}
