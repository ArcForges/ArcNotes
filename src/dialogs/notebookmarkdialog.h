#pragma once

#include <models/bookmarktablemodel.h>

#include "masterdialog.h"

class QDialogButtonBox;
class QTableView;

class NoteBookmarkDialog : public MasterDialog {
    Q_OBJECT

public:
    explicit NoteBookmarkDialog(QWidget* parent = nullptr);

    void setBookmarks(const QVector<BookmarkItemData>& bookmarks);

signals:
    void jumpToBookmarkRequested(int slot);
    void deleteBookmarkRequested(int slot);
    void reloadRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    int selectedSlot() const;
    void onJumpButtonClicked();
    void onDeleteButtonClicked();

    BookmarkTableModel _model;
    QTableView* _bookmarkTableWidget = nullptr;
    QDialogButtonBox* _buttonBox = nullptr;
};
