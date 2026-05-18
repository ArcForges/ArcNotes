#pragma once

#include <core/data/notedata.h>

#include <QAbstractListModel>
#include <QVector>

class SearchResultModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        NoteIdRole = Qt::UserRole + 1,
        TitleRole,
        FileNameRole,
        MatchPreviewRole,
        ModifiedDateRole,
        SubFolderIdRole
    };
    Q_ENUM(Role)

    explicit SearchResultModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVector<NoteData> results() const;

public slots:
    void setResults(const QVector<NoteData>& results);
    void clear();

private:
    QString previewText(const NoteData& note) const;

    QVector<NoteData> _results;
};
