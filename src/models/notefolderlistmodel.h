#pragma once

#include <core/data/notefolderdata.h>

#include <QAbstractListModel>
#include <QHash>
#include <QVector>

class NoteFolderListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        FolderIdRole = Qt::UserRole + 1,
        NameRole,
        LocalPathRole,
        PriorityRole,
        ActiveTagIdRole,
        ShowSubfoldersRole,
        AllSubfoldersRole
    };
    Q_ENUM(Role)

    explicit NoteFolderListModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QVector<NoteFolderData> folders() const;
    [[nodiscard]] int rowForFolderId(int folderId) const;

public slots:
    void setFolders(const QVector<NoteFolderData>& folders);
    void updateFolder(const NoteFolderData& folder);
    void removeFolder(int folderId);
    void clear();

private:
    void rebuildIndex();

    QVector<NoteFolderData> _folders;
    QHash<int, int> _rowById;
};
