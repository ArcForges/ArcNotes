#include "navigationoutlinemodel.h"

NavigationOutlineModel::NavigationOutlineModel(QObject* parent) : QAbstractItemModel(parent), _root(new Node) {}

NavigationOutlineModel::~NavigationOutlineModel() = default;

QModelIndex NavigationOutlineModel::index(int row, int column, const QModelIndex& parentIndex) const {
    if (row < 0 || column < 0 || column >= columnCount(parentIndex)) {
        return {};
    }

    Node* parentNode = nodeFromIndex(parentIndex);
    if (row >= parentNode->children.count()) {
        return {};
    }

    return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex NavigationOutlineModel::parent(const QModelIndex& childIndex) const {
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

int NavigationOutlineModel::rowCount(const QModelIndex& parent) const {
    return parent.column() > 0 ? 0 : nodeFromIndex(parent)->children.count();
}

int NavigationOutlineModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return 1;
}

QVariant NavigationOutlineModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) {
        return {};
    }

    const NavigationOutlineItemData& item = nodeFromIndex(index)->item;
    switch (role) {
        case Qt::DisplayRole:
        case TitleRole:
            return item.title;
        case LevelRole:
            return item.level;
        case PositionRole:
            return item.position;
        case LineRole:
            return item.line;
        default:
            return {};
    }
}

QHash<int, QByteArray> NavigationOutlineModel::roleNames() const {
    return {{TitleRole, "title"}, {LevelRole, "level"}, {PositionRole, "position"}, {LineRole, "line"}};
}

QVector<NavigationOutlineItemData> NavigationOutlineModel::headings() const {
    return _headings;
}

void NavigationOutlineModel::setHeadings(const QVector<NavigationOutlineItemData>& headings) {
    beginResetModel();
    _headings = headings;
    _root = std::make_unique<Node>();
    _nodes.clear();

    QHash<int, Node*> lastHeadingByLevel;
    for (const NavigationOutlineItemData& heading : headings) {
        auto node = std::make_unique<Node>();
        node->item = heading;

        Node* parentNode = _root.get();
        for (int level = heading.level - 1; level >= 1; --level) {
            parentNode = lastHeadingByLevel.value(level, nullptr);
            if (parentNode != nullptr) {
                break;
            }
        }
        if (parentNode == nullptr) {
            parentNode = _root.get();
        }

        node->parent = parentNode;
        parentNode->children.append(node.get());
        lastHeadingByLevel.insert(heading.level, node.get());

        for (int level = heading.level + 1; level <= 6; ++level) {
            lastHeadingByLevel.remove(level);
        }

        _nodes.push_back(std::move(node));
    }

    endResetModel();
}

void NavigationOutlineModel::clear() {
    setHeadings({});
}

NavigationOutlineModel::Node* NavigationOutlineModel::nodeFromIndex(const QModelIndex& index) const {
    return index.isValid() ? static_cast<Node*>(index.internalPointer()) : _root.get();
}

int NavigationOutlineModel::rowOfNode(const Node* node) const {
    if (node == nullptr || node->parent == nullptr) {
        return 0;
    }
    return node->parent->children.indexOf(const_cast<Node*>(node));
}
