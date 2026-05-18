#pragma once

#include <core/data/notehistorydata.h>

#include <QAbstractTableModel>
#include <QVector>

struct BookmarkItemData {
    int slot = 0;
    NoteHistoryItemData historyItem;

    bool operator==(const BookmarkItemData& other) const {
        return slot == other.slot && historyItem == other.historyItem;
    }
};

class BookmarkTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { SlotColumn = 0, NoteColumn, SubFolderColumn, CursorColumn, ColumnCount };
    Q_ENUM(Column)

    enum Role { SlotRole = Qt::UserRole + 1, NoteNameRole, SubFolderPathRole, CursorPositionRole, RelativeScrollRole };
    Q_ENUM(Role)

    explicit BookmarkTableModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QVector<BookmarkItemData> bookmarks() const;

public slots:
    void setBookmarks(const QVector<BookmarkItemData>& bookmarks);
    void upsertBookmark(const BookmarkItemData& bookmark);
    void removeBookmark(int slot);
    void clear();

private:
    QVector<BookmarkItemData> _bookmarks;
};
