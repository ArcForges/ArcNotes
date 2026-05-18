#include "storedattachmentsdialog.h"

#include <utils/gui.h>
#include <utils/misc.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "storedfiledialoghost.h"

namespace {
QString noteTooltip(const NoteData& note) {
    QString toolTipText = QObject::tr("<strong>%1</strong><br />last modified: %3<br />created: %2<br />file size: %4")
                              .arg(note.fileName, note.fileCreated.toString(), note.fileLastModified.toString(),
                                   Utils::Misc::toHumanReadableByteSize(note.fileSize));

    if (!note.relativeNoteSubFolderPath.isEmpty()) {
        toolTipText += QObject::tr("<br />path: %1").arg(note.relativeNoteSubFolderPath);
    }

#ifdef QT_DEBUG
    toolTipText += QStringLiteral("<br />id: %1").arg(note.id);
#endif

    return toolTipText;
}

QVector<int> noteIds(const QVector<NoteData>& notes) {
    QVector<int> ids;
    ids.reserve(notes.count());
    for (const NoteData& note : notes) {
        if (note.id > 0) {
            ids.append(note.id);
        }
    }
    return ids;
}
}  // namespace

StoredAttachmentsDialog::StoredAttachmentsDialog(QWidget* parent)
    : MasterDialog(parent), _host(dynamic_cast<StoredFileDialogHost*>(parent)) {
    buildUi();
    afterSetupUI();
    _infoFrame->setEnabled(false);
    _fileTreeWidget->installEventFilter(this);
    refreshAttachmentFiles();
}

StoredAttachmentsDialog::~StoredAttachmentsDialog() = default;

void StoredAttachmentsDialog::buildUi() {
    setWindowTitle(tr("Stored attachments"));
    resize(831, 617);

    auto* layout = new QGridLayout(this);
    auto* leftFrame = new QFrame(this);
    leftFrame->setFrameShape(QFrame::NoFrame);
    auto* leftLayout = new QGridLayout(leftFrame);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    _searchLineEdit = new QLineEdit(leftFrame);
    _searchLineEdit->setPlaceholderText(tr("Find attachment"));
    _searchLineEdit->setClearButtonEnabled(true);
    _searchLineEdit->setStyleSheet(
        QStringLiteral("QLineEdit { border: none; padding: 2px 5px 2px 27px; "
                       "background-image: url(:/images/search-notes.svg); background-repeat: no-repeat; "
                       "background-position: center left; margin-right: 0px; }"));
    _fileTreeWidget = new QTreeWidget(leftFrame);
    _fileTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    _fileTreeWidget->setEditTriggers(QAbstractItemView::EditKeyPressed);
    _fileTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _fileTreeWidget->setRootIsDecorated(false);
    _fileTreeWidget->setHeaderHidden(true);
    _fileTreeWidget->setHeaderLabels({QStringLiteral("1")});
    _progressBar = new QProgressBar(leftFrame);
    _progressBar->setValue(0);
    _orphanedCheckBox = new QCheckBox(tr("Only show orphaned attachments"), leftFrame);
    _orphanedCheckBox->setToolTip(tr("Only show attachments that are not used in notes"));
    _currentNoteCheckBox = new QCheckBox(tr("Only show from current note"), leftFrame);
    _currentNoteCheckBox->setToolTip(tr("Only show attachments that are used in the current note"));

    auto* deleteButton = new QPushButton(tr("Delete"), leftFrame);
    deleteButton->setToolTip(tr("Delete selected attachments"));
    deleteButton->setIcon(QIcon::fromTheme(QStringLiteral("list-remove"),
                                           QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/list-remove.svg"))));
    auto* insertButton = new QPushButton(tr("Add to current note"), leftFrame);
    insertButton->setToolTip(tr("Add selected attachments to the current note"));
    insertButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add"),
                                           QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/list-add.svg"))));
    auto* openFileButton = new QPushButton(tr("Open &attachment"), leftFrame);
    openFileButton->setIcon(QIcon::fromTheme(
        QStringLiteral("mail-attachment"), QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/mail-attachment.svg"))));
    auto* openFolderButton = new QPushButton(tr("Open &folder"), leftFrame);
    openFolderButton->setToolTip(tr("Open the attachments folder"));
    openFolderButton->setIcon(QIcon::fromTheme(QStringLiteral("folder"),
                                               QIcon(QStringLiteral(":/icons/breeze-dark-arcnotes/16x16/folder.svg"))));
    auto* refreshButton = new QPushButton(tr("Refresh"), leftFrame);
    refreshButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh"),
                                            QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/view-refresh.svg"))));

    leftLayout->addWidget(_searchLineEdit, 1, 0);
    leftLayout->addWidget(_fileTreeWidget, 2, 0);
    leftLayout->addWidget(_progressBar, 3, 0);
    leftLayout->addWidget(_orphanedCheckBox, 4, 0);
    leftLayout->addWidget(_currentNoteCheckBox, 5, 0);
    leftLayout->addWidget(deleteButton, 6, 0);
    leftLayout->addWidget(insertButton, 7, 0);
    leftLayout->addWidget(openFileButton, 8, 0);
    leftLayout->addWidget(openFolderButton, 9, 0);
    leftLayout->addWidget(refreshButton, 10, 0);

    _infoFrame = new QFrame(this);
    _infoFrame->setFrameShape(QFrame::NoFrame);
    auto* infoLayout = new QGridLayout(_infoFrame);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    _pathLabel = new QLabel(QStringLiteral("?"), _infoFrame);
    _pathLabel->setWordWrap(true);
    _pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    _sizeLabel = new QLabel(QStringLiteral("?"), _infoFrame);
    _sizeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    _typeLabel = new QLabel(QStringLiteral("?"), _infoFrame);
    _typeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    infoLayout->addWidget(new QLabel(tr("File path:"), _infoFrame), 0, 1);
    infoLayout->addWidget(_pathLabel, 0, 2);
    infoLayout->addWidget(new QLabel(tr("File size:"), _infoFrame), 1, 1);
    infoLayout->addWidget(_sizeLabel, 1, 2);
    infoLayout->addWidget(new QLabel(tr("File type:"), _infoFrame), 2, 1);
    infoLayout->addWidget(_typeLabel, 2, 2);
    infoLayout->setRowStretch(3, 1);

    _notesFrame = new QFrame(_infoFrame);
    _notesFrame->setFrameShape(QFrame::NoFrame);
    auto* notesLayout = new QVBoxLayout(_notesFrame);
    notesLayout->setContentsMargins(0, 0, 0, 0);
    notesLayout->addWidget(new QLabel(tr("Used in notes:"), _notesFrame));
    _noteTreeWidget = new QTreeWidget(_notesFrame);
    _noteTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    _noteTreeWidget->setRootIsDecorated(false);
    _noteTreeWidget->setSortingEnabled(true);
    _noteTreeWidget->setHeaderHidden(true);
    _noteTreeWidget->setHeaderLabels({QStringLiteral("Note")});
    notesLayout->addWidget(_noteTreeWidget);
    infoLayout->addWidget(_notesFrame, 4, 1, 1, 2);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    layout->addWidget(leftFrame, 0, 0);
    layout->addWidget(_infoFrame, 0, 1);
    layout->addWidget(buttonBox, 1, 0, 1, 2);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 2);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(_fileTreeWidget, &QTreeWidget::currentItemChanged, this,
            &StoredAttachmentsDialog::on_fileTreeWidget_currentItemChanged);
    connect(deleteButton, &QPushButton::clicked, this, &StoredAttachmentsDialog::on_deleteButton_clicked);
    connect(insertButton, &QPushButton::clicked, this, &StoredAttachmentsDialog::on_insertButton_clicked);
    connect(openFileButton, &QPushButton::clicked, this, &StoredAttachmentsDialog::on_openFileButton_clicked);
    connect(openFolderButton, &QPushButton::clicked, this, &StoredAttachmentsDialog::on_openFolderButton_clicked);
    connect(_fileTreeWidget, &QTreeWidget::customContextMenuRequested, this,
            &StoredAttachmentsDialog::on_fileTreeWidget_customContextMenuRequested);
    connect(_fileTreeWidget, &QTreeWidget::itemDoubleClicked, this,
            &StoredAttachmentsDialog::on_fileTreeWidget_itemDoubleClicked);
    connect(_noteTreeWidget, &QTreeWidget::itemDoubleClicked, this,
            &StoredAttachmentsDialog::on_noteTreeWidget_itemDoubleClicked);
    connect(refreshButton, &QPushButton::clicked, this, &StoredAttachmentsDialog::on_refreshButton_clicked);
    connect(_fileTreeWidget, &QTreeWidget::itemChanged, this, &StoredAttachmentsDialog::on_fileTreeWidget_itemChanged);
    connect(_searchLineEdit, &QLineEdit::textChanged, this, &StoredAttachmentsDialog::on_searchLineEdit_textChanged);
    connect(_noteTreeWidget, &QTreeWidget::customContextMenuRequested, this,
            &StoredAttachmentsDialog::on_noteTreeWidget_customContextMenuRequested);
    connect(_orphanedCheckBox, &QCheckBox::toggled, this, &StoredAttachmentsDialog::on_orphanedCheckBox_toggled);
    connect(_currentNoteCheckBox, &QCheckBox::toggled, this, &StoredAttachmentsDialog::on_currentNoteCheckBox_toggled);
}

void StoredAttachmentsDialog::refreshAttachmentFiles() {
    if (_host == nullptr) {
        _progressBar->setValue(_progressBar->maximum());
        return;
    }

    const QString attachmentsPath = _host->storedFileDialogAttachmentsPath();
    QDir attachmentsDir(attachmentsPath);

    if (!attachmentsDir.exists()) {
        _progressBar->setValue(_progressBar->maximum());
        return;
    }

    QStringList attachmentFiles;
    QVector<NoteData> noteList;

    if (_currentNoteOnly) {
        if (_host == nullptr) {
            return;
        }

        const NoteData note = _host->storedFileDialogCurrentNote();
        if (note.id > 0) {
            attachmentFiles = _host->storedFileDialogAttachmentFiles(note);
            noteList = {note};
        }
    } else {
        attachmentFiles = attachmentsDir.entryList(QStringList(QStringLiteral("*")), QDir::Files, QDir::Time);
        noteList = _host->storedFileDialogAllNotes();
    }

    attachmentFiles.removeDuplicates();
    _fileNoteList.clear();

    _progressBar->setMaximum(noteList.count());
    _progressBar->setValue(0);
    _progressBar->show();

    for (const NoteData& note : noteList) {
        const QStringList attachmentsFileList = _host->storedFileDialogAttachmentFiles(note);

        for (const QString& fileName : attachmentsFileList) {
            if (_orphanedAttachmentsOnly) {
                attachmentFiles.removeAll(fileName);
            }

            if (!_fileNoteList[fileName].contains(note)) {
                _fileNoteList[fileName].append(note);
            }
        }

        _progressBar->setValue(_progressBar->value() + 1);
    }

    _progressBar->hide();
    _fileTreeWidget->clear();

    for (const QString& fileName : attachmentFiles) {
        auto* item = new QTreeWidgetItem();
        item->setText(0, fileName);
        item->setData(0, Qt::UserRole, fileName);
        item->setFlags(item->flags() | Qt::ItemIsEditable);

        const QString filePath = getFilePath(item);
        QFileInfo info(filePath);
        item->setToolTip(0, tr("Last modified at %1").arg(info.lastModified().toString()) + QStringLiteral("\n") +
                                Utils::Misc::toHumanReadableByteSize(info.size()));

        _fileTreeWidget->addTopLevelItem(item);
    }

    on_searchLineEdit_textChanged(_searchLineEdit->text());
    _fileTreeWidget->sortItems(0, Qt::AscendingOrder);

    if (!attachmentFiles.isEmpty()) {
        auto* event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Home, Qt::NoModifier);
        QApplication::postEvent(_fileTreeWidget, event);
    } else {
        loadCurrentFileDetails();
    }
}

void StoredAttachmentsDialog::on_fileTreeWidget_currentItemChanged(QTreeWidgetItem* current,
                                                                   QTreeWidgetItem* previous) {
    Q_UNUSED(current)
    Q_UNUSED(previous)
    loadCurrentFileDetails();
}

void StoredAttachmentsDialog::loadCurrentFileDetails() {
    QTreeWidgetItem* current = _fileTreeWidget->currentItem();

    if (current == nullptr) {
        _infoFrame->setEnabled(false);
        _pathLabel->clear();
        _typeLabel->clear();
        _sizeLabel->clear();
        _notesFrame->hide();
        return;
    }

    const QString filePath = getFilePath(current);
    _pathLabel->setText(filePath);

    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(filePath);
    _typeLabel->setText(type.comment());

    QFileInfo fileInfo(filePath);
    _sizeLabel->setText(Utils::Misc::toHumanReadableByteSize(fileInfo.size()));
    _infoFrame->setEnabled(true);

    const QString fileName = current->text(0);
    if (_fileNoteList.contains(fileName)) {
        const auto notes = _fileNoteList[fileName];
        _noteTreeWidget->clear();

        for (const NoteData& note : notes) {
            auto* item = new QTreeWidgetItem();
            item->setText(0, note.name);
            item->setData(0, Qt::UserRole, note.id);
            item->setToolTip(0, noteTooltip(note));

            _noteTreeWidget->addTopLevelItem(item);
        }

        _notesFrame->show();
    } else {
        _notesFrame->hide();
    }
}

QString StoredAttachmentsDialog::getFilePath(QTreeWidgetItem* item) const {
    if (item == nullptr) {
        return QString();
    }

    if (_host == nullptr) {
        return QString();
    }

    return _host->storedFileDialogAttachmentsPath() + QDir::separator() + item->data(0, Qt::UserRole).toString();
}

void StoredAttachmentsDialog::on_deleteButton_clicked() {
    const int selectedItemsCount = _fileTreeWidget->selectedItems().count();
    if (selectedItemsCount == 0) {
        return;
    }

    if (Utils::Gui::question(this, tr("Delete selected files"),
                             tr("Delete <strong>%n</strong> selected file(s)?", "", selectedItemsCount),
                             QStringLiteral("delete-attachment-files")) != QMessageBox::Yes) {
        return;
    }

    for (QTreeWidgetItem* item : _fileTreeWidget->selectedItems()) {
        const QString filePath = getFilePath(item);
        if (QFile::remove(filePath)) {
            delete item;
        }
    }
}

bool StoredAttachmentsDialog::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);

        if (obj == _fileTreeWidget) {
            if (keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace) {
                on_deleteButton_clicked();
                return true;
            }
            return false;
        }
    }

    return MasterDialog::eventFilter(obj, event);
}

void StoredAttachmentsDialog::on_insertButton_clicked() {
    if (_host == nullptr || _fileTreeWidget->selectedItems().isEmpty()) {
        return;
    }

    const NoteData note = _host->storedFileDialogCurrentNote();
    if (note.id <= 0) {
        return;
    }

    for (QTreeWidgetItem* item : _fileTreeWidget->selectedItems()) {
        const QString filePath = getFilePath(item);
        QFileInfo fileInfo(filePath);
        const QString attachmentsUrlString =
            _host->storedFileDialogAttachmentUrlStringForFileName(note, fileInfo.fileName());
        const QString attachmentLink = QStringLiteral("[") + fileInfo.baseName() + QStringLiteral("](") +
                                       attachmentsUrlString + QStringLiteral(")\n");
        _host->storedFileDialogInsertText(attachmentLink);
    }

    refreshAttachmentFiles();
}

void StoredAttachmentsDialog::on_openFileButton_clicked() {
    QTreeWidgetItem* item = _fileTreeWidget->currentItem();
    if (item != nullptr) {
        Utils::Misc::openPath(getFilePath(item));
    }
}

void StoredAttachmentsDialog::on_openFolderButton_clicked() {
    QTreeWidgetItem* item = _fileTreeWidget->currentItem();
    if (item != nullptr) {
        Utils::Misc::openFolderSelect(getFilePath(item), QStringLiteral("show-stored-attachment-in-file-manager"));
    }
}

void StoredAttachmentsDialog::on_fileTreeWidget_customContextMenuRequested(const QPoint& pos) {
    const bool hasSelected = !_fileTreeWidget->selectedItems().isEmpty();
    const QPoint globalPos = _fileTreeWidget->mapToGlobal(pos);
    QMenu menu;

    QAction* renameAction = nullptr;
    QAction* removeAction = nullptr;
    QAction* openAction = nullptr;
    QAction* addAction = nullptr;
    if (hasSelected) {
        openAction = menu.addAction(tr("&Open attachment"));
        renameAction = menu.addAction(tr("&Rename attachment"));
        removeAction = menu.addAction(tr("&Delete attachments"));
        addAction = menu.addAction(tr("&Add attachments to current note"));
    }

    QAction* selectedItem = menu.exec(globalPos);
    if (selectedItem == nullptr) {
        return;
    }

    QTreeWidgetItem* item = _fileTreeWidget->currentItem();
    if (selectedItem == removeAction) {
        on_deleteButton_clicked();
    } else if (selectedItem == renameAction) {
        _fileTreeWidget->editItem(item);
    } else if (selectedItem == addAction) {
        on_insertButton_clicked();
    } else if (selectedItem == openAction) {
        on_openFileButton_clicked();
    }
}

void StoredAttachmentsDialog::refreshAndJumpToFileName(const QString& fileName) {
    refreshAttachmentFiles();
    auto* item = Utils::Gui::getTreeWidgetItemWithUserData(_fileTreeWidget, fileName);
    QTimer::singleShot(0, this, [this, item]() { _fileTreeWidget->setCurrentItem(item); });
}

void StoredAttachmentsDialog::on_searchLineEdit_textChanged(const QString& arg1) {
    Utils::Gui::searchForTextInTreeWidget(
        _fileTreeWidget, arg1, Utils::Gui::TreeWidgetSearchFlags(Utils::Gui::TreeWidgetSearchFlag::EveryWordSearch));
}

void StoredAttachmentsDialog::on_fileTreeWidget_itemDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(item)
    Q_UNUSED(column)
    on_insertButton_clicked();
}

void StoredAttachmentsDialog::openCurrentNote() {
    QTreeWidgetItem* item = _noteTreeWidget->currentItem();
    if (item == nullptr || _host == nullptr) {
        return;
    }

    _host->storedFileDialogOpenNoteInTab(item->data(0, Qt::UserRole).toInt());
}

void StoredAttachmentsDialog::on_noteTreeWidget_itemDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(item)
    Q_UNUSED(column)
    openCurrentNote();
}

void StoredAttachmentsDialog::on_refreshButton_clicked() {
    refreshAttachmentFiles();
}

void StoredAttachmentsDialog::on_fileTreeWidget_itemChanged(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column)

    const QString oldFileName = item->data(0, Qt::UserRole).toString();
    const QString newFileName = item->text(0);
    const QSignalBlocker blocker(_fileTreeWidget);
    Q_UNUSED(blocker)

    if (_host == nullptr) {
        return;
    }

    const QString attachmentsPath = _host->storedFileDialogAttachmentsPath();
    const QString oldFilePath = attachmentsPath + QDir::separator() + oldFileName;
    QFile oldFile(oldFilePath);
    if (!oldFile.exists()) {
        QMessageBox::warning(this, tr("File doesn't exist"),
                             tr("The file <strong>%1</strong> doesn't exist, "
                                "you cannot rename it!")
                                 .arg(oldFilePath));
        item->setText(0, oldFileName);
        return;
    }

    const QString newFilePath = attachmentsPath + QDir::separator() + newFileName;
    QFile newFile(newFilePath);

    if (newFile.exists()) {
        QMessageBox::warning(this, tr("File exists"),
                             tr("File <strong>%1</strong> already exists, "
                                "you need to remove it before choosing "
                                "<strong>%2</strong> as new filename!")
                                 .arg(newFilePath, newFileName));
        item->setText(0, oldFileName);
        return;
    }

    if (!oldFile.rename(newFilePath)) {
        QMessageBox::warning(this, tr("File renaming failed"),
                             tr("Renaming of file <strong>%1</strong> failed!").arg(oldFilePath));
        item->setText(0, oldFileName);
        return;
    }

    const auto affectedNotes = _fileNoteList[oldFileName];
    const int affectedNotesCount = affectedNotes.count();

    if (affectedNotesCount == 0) {
        refreshAndJumpToFileName(newFileName);
        return;
    }

    if (Utils::Gui::questionNoSkipOverride(nullptr, QObject::tr("File name changed"),
                                           QObject::tr("%n note(s) are using this attachment. Would you also "
                                                       "like to rename those attachments in the note(s)?",
                                                       "", affectedNotesCount),
                                           QStringLiteral("note-replace-attachments")) != QMessageBox::Yes) {
        refreshAndJumpToFileName(newFileName);
        return;
    }

    if (_host != nullptr) {
        _host->storedFileDialogRenameAttachmentReferences(noteIds(affectedNotes), oldFileName, newFileName);
    }

    refreshAndJumpToFileName(newFileName);
    if (_host != nullptr) {
        _host->storedFileDialogReloadCurrentNote(true);
    }
}

void StoredAttachmentsDialog::on_noteTreeWidget_customContextMenuRequested(const QPoint& pos) {
    const bool hasSelected = !_noteTreeWidget->selectedItems().isEmpty();
    const QPoint globalPos = _noteTreeWidget->mapToGlobal(pos);
    QMenu menu;

    QAction* openAction = nullptr;
    if (hasSelected) {
        openAction = menu.addAction(tr("&Open note"));
    }

    QAction* selectedItem = menu.exec(globalPos);
    if (selectedItem == openAction) {
        openCurrentNote();
    }
}

void StoredAttachmentsDialog::on_orphanedCheckBox_toggled(bool checked) {
    if (checked) {
        const QSignalBlocker blocker(_fileTreeWidget);
        Q_UNUSED(blocker)
        _currentNoteCheckBox->setChecked(false);
    }

    _orphanedAttachmentsOnly = checked;
    refreshAttachmentFiles();
}

void StoredAttachmentsDialog::on_currentNoteCheckBox_toggled(bool checked) {
    if (checked) {
        const QSignalBlocker blocker(_fileTreeWidget);
        Q_UNUSED(blocker)
        _orphanedCheckBox->setChecked(false);
    }

    _currentNoteOnly = checked;
    refreshAttachmentFiles();
}
