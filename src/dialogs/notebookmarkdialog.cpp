#include "notebookmarkdialog.h"

#include <utils/gui.h>

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QIcon>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

NoteBookmarkDialog::NoteBookmarkDialog(QWidget* parent) : MasterDialog(parent), _model(this) {
    setObjectName(QStringLiteral("NoteBookmarkDialog"));
    resize(520, 320);
    setWindowTitle(tr("Note Bookmarks"));

    auto* layout = new QVBoxLayout(this);

    _bookmarkTableWidget = new QTableView(this);
    _bookmarkTableWidget->setObjectName(QStringLiteral("bookmarkTableWidget"));
    _bookmarkTableWidget->setModel(&_model);
    _bookmarkTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    _bookmarkTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    _bookmarkTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _bookmarkTableWidget->setAlternatingRowColors(true);
    _bookmarkTableWidget->setSortingEnabled(false);
    _bookmarkTableWidget->hideColumn(BookmarkTableModel::SubFolderColumn);
    _bookmarkTableWidget->horizontalHeader()->setSectionResizeMode(BookmarkTableModel::SlotColumn,
                                                                   QHeaderView::ResizeToContents);
    _bookmarkTableWidget->horizontalHeader()->setSectionResizeMode(BookmarkTableModel::NoteColumn,
                                                                   QHeaderView::Stretch);
    _bookmarkTableWidget->horizontalHeader()->setSectionResizeMode(BookmarkTableModel::CursorColumn,
                                                                   QHeaderView::ResizeToContents);
    layout->addWidget(_bookmarkTableWidget);

    _buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    _buttonBox->setObjectName(QStringLiteral("buttonBox"));
    _buttonBox->setOrientation(Qt::Horizontal);

    QPushButton* jumpButton = _buttonBox->addButton(tr("Jump to bookmark"), QDialogButtonBox::ActionRole);
    jumpButton->setIcon(QIcon::fromTheme(QStringLiteral("go-next"),
                                         QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/go-next.svg"))));
    jumpButton->setToolTip(tr("Jump to the selected bookmark in the main window"));

    QPushButton* deleteButton = _buttonBox->addButton(tr("Delete bookmark"), QDialogButtonBox::ActionRole);
    deleteButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete"),
                                           QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/edit-delete.svg"))));
    deleteButton->setToolTip(tr("Delete the selected bookmark"));

    QPushButton* reloadButton = _buttonBox->addButton(tr("Reload"), QDialogButtonBox::ActionRole);
    reloadButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh"),
                                           QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/view-refresh.svg"))));
    reloadButton->setToolTip(tr("Reload the bookmark list"));

    layout->addWidget(_buttonBox);

    connect(_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(jumpButton, &QPushButton::clicked, this, &NoteBookmarkDialog::onJumpButtonClicked);
    connect(deleteButton, &QPushButton::clicked, this, &NoteBookmarkDialog::onDeleteButtonClicked);
    connect(reloadButton, &QPushButton::clicked, this, &NoteBookmarkDialog::reloadRequested);
    connect(_bookmarkTableWidget, &QTableView::doubleClicked, this,
            [this](const QModelIndex&) { onJumpButtonClicked(); });

    _bookmarkTableWidget->installEventFilter(this);
    afterSetupUI();
}

void NoteBookmarkDialog::setBookmarks(const QVector<BookmarkItemData>& bookmarks) {
    _model.setBookmarks(bookmarks);
}

int NoteBookmarkDialog::selectedSlot() const {
    if (_bookmarkTableWidget == nullptr || !_bookmarkTableWidget->currentIndex().isValid()) {
        return -1;
    }

    return _bookmarkTableWidget->currentIndex()
        .siblingAtColumn(BookmarkTableModel::SlotColumn)
        .data(BookmarkTableModel::SlotRole)
        .toInt();
}

void NoteBookmarkDialog::onJumpButtonClicked() {
    const int slot = selectedSlot();
    if (slot >= 0) {
        emit jumpToBookmarkRequested(slot);
    }
}

void NoteBookmarkDialog::onDeleteButtonClicked() {
    const int slot = selectedSlot();
    if (slot < 0) {
        return;
    }

    const auto answer = Utils::Gui::question(this, tr("Delete bookmark"),
                                             tr("Are you sure you want to delete the bookmark at slot %1?").arg(slot),
                                             QStringLiteral("delete-note-bookmark"));
    if (answer == QMessageBox::Yes) {
        emit deleteBookmarkRequested(slot);
    }
}

bool NoteBookmarkDialog::eventFilter(QObject* watched, QEvent* event) {
    if (watched == _bookmarkTableWidget && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Delete) {
            onDeleteButtonClicked();
            return true;
        }
    }

    return MasterDialog::eventFilter(watched, event);
}
