#pragma once

#include <core/data/notedata.h>

#include <QAbstractListModel>
#include <QHash>
#include <QVector>

class NoteListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        NoteIdRole = Qt::UserRole + 1,
        TitleRole,
        PreviewRole,
        FileNameRole,
        ModifiedDateRole,
        CreatedDateRole,
        FileSizeRole,
        IsDirtyRole,
        SubFolderIdRole,
        IsFavoriteRole
    };
    Q_ENUM(Role)

    explicit NoteListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVector<NoteData> notes() const;
    NoteData noteAt(int row) const;
    int rowForNoteId(int noteId) const;

signals:
    void noteRenameRequested(int noteId, const QString& name);

public slots:
    void setNotes(const QVector<NoteData>& notes);
    void addNote(const NoteData& note);
    void removeNote(int noteId);
    void updateNote(const NoteData& note);
    void clear();

private:
    void rebuildIndex();
    QString previewText(const NoteData& note) const;

    QVector<NoteData> _notes;
    QHash<int, int> _rowById;
};
