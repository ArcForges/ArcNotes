#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

struct BacklinkItemData {
    int noteId = 0;
    QString title;
    QString fileName;
    QString context;

    bool operator==(const BacklinkItemData& other) const {
        return noteId == other.noteId && title == other.title && fileName == other.fileName && context == other.context;
    }
};

class BacklinkListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role { NoteIdRole = Qt::UserRole + 1, TitleRole, FileNameRole, ContextRole };
    Q_ENUM(Role)

    explicit BacklinkListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVector<BacklinkItemData> backlinks() const;

public slots:
    void setBacklinks(const QVector<BacklinkItemData>& backlinks);
    void clear();

private:
    QVector<BacklinkItemData> _backlinks;
};
