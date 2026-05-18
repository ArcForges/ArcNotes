#pragma once

#include <core/data/tagdata.h>

#include <QAbstractItemModel>
#include <QHash>
#include <QVector>
#include <memory>
#include <vector>

class TagTreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    enum Role { TagIdRole = Qt::UserRole + 1, NameRole, NoteCountRole, ColorRole, ParentIdRole };
    Q_ENUM(Role)

    explicit TagTreeModel(QObject* parent = nullptr);
    ~TagTreeModel() override;

    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex& index) const override;
    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] TagData tagForIndex(const QModelIndex& index) const;

signals:
    void tagRenameRequested(int tagId, const QString& name);

public slots:
    void setTags(const QVector<TagData>& tags, const QHash<int, int>& noteCounts = {});
    void clear();

private:
    struct Node {
        TagData tag;
        int noteCount = 0;
        Node* parent = nullptr;
        QVector<Node*> children;
    };

    [[nodiscard]] Node* nodeFromIndex(const QModelIndex& index) const;
    int rowOfNode(const Node* node) const;

    std::unique_ptr<Node> _root;
    std::vector<std::unique_ptr<Node>> _nodes;
};
