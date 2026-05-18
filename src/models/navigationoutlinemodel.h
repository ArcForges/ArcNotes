#pragma once

#include <QAbstractItemModel>
#include <QHash>
#include <QString>
#include <QVector>
#include <memory>
#include <vector>

struct NavigationOutlineItemData {
    QString title;
    int level = 0;
    int position = 0;
    int line = 0;

    bool operator==(const NavigationOutlineItemData& other) const {
        return title == other.title && level == other.level && position == other.position && line == other.line;
    }
};

class NavigationOutlineModel : public QAbstractItemModel {
    Q_OBJECT

public:
    enum Role { TitleRole = Qt::UserRole + 1, LevelRole, PositionRole, LineRole };
    Q_ENUM(Role)

    explicit NavigationOutlineModel(QObject* parent = nullptr);
    ~NavigationOutlineModel() override;

    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex& index) const override;
    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QVector<NavigationOutlineItemData> headings() const;

public slots:
    void setHeadings(const QVector<NavigationOutlineItemData>& headings);
    void clear();

private:
    struct Node {
        NavigationOutlineItemData item;
        Node* parent = nullptr;
        QVector<Node*> children;
    };

    [[nodiscard]] Node* nodeFromIndex(const QModelIndex& index) const;
    int rowOfNode(const Node* node) const;

    std::unique_ptr<Node> _root;
    std::vector<std::unique_ptr<Node>> _nodes;
    QVector<NavigationOutlineItemData> _headings;
};
