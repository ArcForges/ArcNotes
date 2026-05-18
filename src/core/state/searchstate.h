#pragma once

#include <QObject>
#include <QString>
#include <QVector>

class SearchState : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(bool isSearching READ isSearching WRITE setSearching NOTIFY searchingChanged)
    Q_PROPERTY(int resultCount READ resultCount NOTIFY resultCountChanged)
    Q_PROPERTY(QString searchMode READ searchMode WRITE setSearchMode NOTIFY searchModeChanged)

public:
    explicit SearchState(QObject* parent = nullptr);

    [[nodiscard]] QString query() const;
    [[nodiscard]] bool isSearching() const;
    [[nodiscard]] int resultCount() const;
    [[nodiscard]] QString searchMode() const;
    [[nodiscard]] QVector<int> resultNoteIds() const;

public slots:
    void setQuery(const QString& query);
    void setSearching(bool searching);
    void setSearchMode(const QString& mode);
    void setResultNoteIds(const QVector<int>& noteIds);
    void clearResults();

signals:
    void queryChanged(const QString& query);
    void searchingChanged(bool searching);
    void resultCountChanged(int resultCount);
    void searchModeChanged(const QString& mode);
    void resultNoteIdsChanged(const QVector<int>& noteIds);

private:
    QString _query;
    bool _searching = false;
    QString _searchMode;
    QVector<int> _resultNoteIds;
};
