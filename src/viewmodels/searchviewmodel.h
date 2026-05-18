#pragma once

#include <models/searchresultmodel.h>

#include <QObject>
#include <QString>

class CommandBus;
class Debouncer;
class SearchState;

class SearchViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE search NOTIFY queryChanged)
    Q_PROPERTY(bool isSearching READ isSearching NOTIFY searchingChanged)
    Q_PROPERTY(int resultCount READ resultCount NOTIFY resultCountChanged)

public:
    explicit SearchViewModel(CommandBus* commandBus = nullptr, SearchState* searchState = nullptr,
                             QObject* parent = nullptr);
    ~SearchViewModel() override;

    SearchResultModel* model();
    const SearchResultModel* model() const;
    QString query() const;
    bool isSearching() const;
    int resultCount() const;

    void setCommandBus(CommandBus* commandBus);
    void setSearchState(SearchState* searchState);

public slots:
    void setResults(const QVector<NoteData>& results);
    void search(const QString& query);
    void clear();

signals:
    void queryChanged(const QString& query);
    void searchingChanged(bool searching);
    void resultCountChanged(int resultCount);
    void commandFailed(const QString& message);

private:
    void syncFromState();

    CommandBus* _commandBus = nullptr;
    SearchState* _searchState = nullptr;
    SearchResultModel _model;
    Debouncer* _debouncer = nullptr;
    QString _query;
    bool _searching = false;
};
