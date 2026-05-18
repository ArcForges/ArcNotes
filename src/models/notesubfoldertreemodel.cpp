#include "notesubfoldertreemodel.h"

NoteSubFolderTreeModel::NoteSubFolderTreeModel(QObject* parent) : QAbstractItemModel(parent), _root(new Node) {}

NoteSubFolderTreeModel::~NoteSubFolderTreeModel() = default;

QModelIndex NoteSubFolderTreeModel::index(int row, int column, const QModelIndex& parentIndex) const {
    if (row < 0 || column < 0 || column >= columnCount(parentIndex)) {
        return {};
    }

    Node* parentNode = nodeFromIndex(parentIndex);
    if (row >= parentNode->children.count()) {
        return {};
    }

    return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex NoteSubFolderTreeModel::parent(const QModelIndex& childIndex) const {
    if (!childIndex.isValid()) {
        return {};
    }

    Node* childNode = nodeFromIndex(childIndex);
    Node* parentNode = childNode->parent;
    if (parentNode == nullptr || parentNode == _root.get()) {
        return {};
    }

    return createIndex(rowOfNode(parentNode), 0, parentNode);
}

int NoteSubFolderTreeModel::rowCount(const QModelIndex& parentIndex) const {
    return parentIndex.column() > 0 ? 0 : nodeFromIndex(parentIndex)->children.count();
}

int NoteSubFolderTreeModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return 1;
}

QVariant NoteSubFolderTreeModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) {
        return {};
    }

    const Node* node = nodeFromIndex(index);
    switch (role) {
        case Qt::DisplayRole:
        case NameRole:
            return node->subFolder.name;
        case SubFolderIdRole:
            return node->subFolder.id;
        case ParentIdRole:
            return node->subFolder.parentId;
        case CreatedDateRole:
            return node->subFolder.created;
        case ModifiedDateRole:
            return node->subFolder.modified;
        default:
            return {};
    }
}

bool NoteSubFolderTreeModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || role != Qt::EditRole) {
        return false;
    }

    const Node* node = nodeFromIndex(index);
    const QString name = value.toString().trimmed();
    if (node->subFolder.id <= 0 || name.isEmpty() || name == node->subFolder.name) {
        return false;
    }

    emit subFolderRenameRequested(node->subFolder.id, name);
    return true;
}

Qt::ItemFlags NoteSubFolderTreeModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

QHash<int, QByteArray> NoteSubFolderTreeModel::roleNames() const {
    return {{SubFolderIdRole, "subFolderId"},
            {NameRole, "name"},
            {ParentIdRole, "parentId"},
            {CreatedDateRole, "createdDate"},
            {ModifiedDateRole, "modifiedDate"}};
}

NoteSubFolderData NoteSubFolderTreeModel::subFolderForIndex(const QModelIndex& index) const {
    return index.isValid() ? nodeFromIndex(index)->subFolder : NoteSubFolderData();
}

void NoteSubFolderTreeModel::setSubFolders(const QVector<NoteSubFolderData>& subFolders) {
    beginResetModel();
    _root = std::make_unique<Node>();
    _nodes.clear();

    QHash<int, Node*> nodesById;
    for (const NoteSubFolderData& subFolder : subFolders) {
        auto node = std::make_unique<Node>();
        node->subFolder = subFolder;
        nodesById.insert(subFolder.id, node.get());
        _nodes.push_back(std::move(node));
    }

    for (const std::unique_ptr<Node>& node : _nodes) {
        Node* parentNode = nodesById.value(node->subFolder.parentId, _root.get());
        if (parentNode == node.get()) {
            parentNode = _root.get();
        }
        node->parent = parentNode;
        parentNode->children.append(node.get());
    }

    endResetModel();
}

void NoteSubFolderTreeModel::clear() {
    setSubFolders({});
}

NoteSubFolderTreeModel::Node* NoteSubFolderTreeModel::nodeFromIndex(const QModelIndex& index) const {
    return index.isValid() ? static_cast<Node*>(index.internalPointer()) : _root.get();
}

int NoteSubFolderTreeModel::rowOfNode(const Node* node) const {
    if (node == nullptr || node->parent == nullptr) {
        return 0;
    }
    return node->parent->children.indexOf(const_cast<Node*>(node));
}
