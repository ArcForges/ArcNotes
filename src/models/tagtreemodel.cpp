#include "tagtreemodel.h"

#include <QColor>
#include <QIcon>

namespace {
constexpr int AllNotesTagId = -1;
constexpr int AllUntaggedNotesTagId = -2;

TagData specialTag(int id, const QString& name) {
    TagData tag;
    tag.id = id;
    tag.name = name;
    return tag;
}

QIcon tagIcon() {
    return QIcon::fromTheme(QStringLiteral("tag"), QIcon(QStringLiteral(":icons/breeze-arcnotes/16x16/tag.svg")));
}

QIcon specialTagIcon() {
    return QIcon::fromTheme(QStringLiteral("edit-copy"),
                            QIcon(QStringLiteral(":icons/breeze-arcnotes/16x16/edit-copy.svg")));
}
}  // namespace

TagTreeModel::TagTreeModel(QObject* parent) : QAbstractItemModel(parent), _root(new Node) {}

TagTreeModel::~TagTreeModel() = default;

QModelIndex TagTreeModel::index(int row, int column, const QModelIndex& parentIndex) const {
    if (row < 0 || column < 0 || column >= columnCount(parentIndex)) {
        return {};
    }

    Node* parentNode = nodeFromIndex(parentIndex);
    if (row >= parentNode->children.count()) {
        return {};
    }

    return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex TagTreeModel::parent(const QModelIndex& childIndex) const {
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

int TagTreeModel::rowCount(const QModelIndex& parentIndex) const {
    return parentIndex.column() > 0 ? 0 : nodeFromIndex(parentIndex)->children.count();
}

int TagTreeModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return 2;
}

QVariant TagTreeModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) {
        return {};
    }

    const Node* node = nodeFromIndex(index);
    switch (role) {
        case Qt::DisplayRole:
            if (index.column() == 1) {
                return node->noteCount > 0 ? QString::number(node->noteCount) : QString();
            }
            return node->tag.name;
        case NameRole:
            return node->tag.name;
        case Qt::DecorationRole:
            if (index.column() == 0) {
                return node->tag.id < 0 ? specialTagIcon() : tagIcon();
            }
            return {};
        case Qt::ForegroundRole:
            if (index.column() == 1) {
                return QColor(Qt::gray);
            }
            return {};
        case TagIdRole:
            return node->tag.id;
        case NoteCountRole:
            return node->noteCount;
        case ColorRole:
            return node->tag.color;
        case ParentIdRole:
            return node->tag.parentId;
        default:
            return {};
    }
}

bool TagTreeModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.column() != 0 || role != Qt::EditRole) {
        return false;
    }

    const Node* node = nodeFromIndex(index);
    const QString name = value.toString().trimmed();
    if (node->tag.id <= 0 || name.isEmpty() || name == node->tag.name) {
        return false;
    }

    emit tagRenameRequested(node->tag.id, name);
    return true;
}

Qt::ItemFlags TagTreeModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    Qt::ItemFlags itemFlags = QAbstractItemModel::flags(index);
    const Node* node = nodeFromIndex(index);
    if (node->tag.id > 0 && index.column() == 0) {
        itemFlags |= Qt::ItemIsEditable;
    }
    return itemFlags;
}

QHash<int, QByteArray> TagTreeModel::roleNames() const {
    return {{TagIdRole, "tagId"},
            {NameRole, "name"},
            {NoteCountRole, "noteCount"},
            {ColorRole, "color"},
            {ParentIdRole, "parentId"}};
}

TagData TagTreeModel::tagForIndex(const QModelIndex& index) const {
    return index.isValid() ? nodeFromIndex(index)->tag : TagData();
}

void TagTreeModel::setTags(const QVector<TagData>& tags, const QHash<int, int>& noteCounts) {
    beginResetModel();
    _root = std::make_unique<Node>();
    _nodes.clear();

    QHash<int, Node*> nodesById;
    auto addNode = [this, &nodesById, &noteCounts](const TagData& tag) {
        auto node = std::make_unique<Node>();
        node->tag = tag;
        node->noteCount = noteCounts.value(tag.id);
        nodesById.insert(tag.id, node.get());
        _nodes.push_back(std::move(node));
    };

    addNode(specialTag(AllNotesTagId, tr("All notes")));

    for (const TagData& tag : tags) {
        addNode(tag);
    }

    if (noteCounts.value(AllUntaggedNotesTagId) > 0) {
        addNode(specialTag(AllUntaggedNotesTagId, tr("Untagged notes")));
    }

    for (const std::unique_ptr<Node>& node : _nodes) {
        Node* parentNode = node->tag.id < 0 ? _root.get() : nodesById.value(node->tag.parentId, _root.get());
        if (parentNode == node.get()) {
            parentNode = _root.get();
        }
        node->parent = parentNode;
        parentNode->children.append(node.get());
    }

    endResetModel();
}

void TagTreeModel::clear() {
    setTags({});
}

TagTreeModel::Node* TagTreeModel::nodeFromIndex(const QModelIndex& index) const {
    return index.isValid() ? static_cast<Node*>(index.internalPointer()) : _root.get();
}

int TagTreeModel::rowOfNode(const Node* node) const {
    if (node == nullptr || node->parent == nullptr) {
        return 0;
    }
    return node->parent->children.indexOf(const_cast<Node*>(node));
}
