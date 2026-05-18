#pragma once

#include <core/data/notesubfolderdata.h>

#include <QAbstractItemModel>
#include <QHash>
#include <QVector>
#include <memory>
#include <vector>

class NoteSubFolderTreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    enum Role { SubFolderIdRole = Qt::UserRole + 1, NameRole, ParentIdRole, CreatedDateRole, ModifiedDateRole };
    Q_ENUM(Role)

    explicit NoteSubFolderTreeModel(QObject* parent = nullptr);
    ~NoteSubFolderTreeModel() override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QHash<int, QByteArray> roleNames() const override;

    NoteSubFolderData subFolderForIndex(const QModelIndex& index) const;

signals:
    void subFolderRenameRequested(int subFolderId, const QString& name);

public slots:
    void setSubFolders(const QVector<NoteSubFolderData>& subFolders);
    void clear();

private:
    struct Node {
        NoteSubFolderData subFolder;
        Node* parent = nullptr;
        QVector<Node*> children;
    };

    Node* nodeFromIndex(const QModelIndex& index) const;
    int rowOfNode(const Node* node) const;

    std::unique_ptr<Node> _root;
    std::vector<std::unique_ptr<Node>> _nodes;
};
