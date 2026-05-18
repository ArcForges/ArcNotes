/*
 * Copyright (c) 2014-2026 Patrizio Bekerle -- <patrizio@bekerle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <coordinator/actionregistry.h>
#include <coordinator/appcoordinator.h>
#include <core/data/notedata.h>
#include <core/state/appstate.h>
#include <core/state/uistate.h>
#include <dialogs/aboutdialog.h>
#include <dialogs/attachmentdialog.h>
#include <dialogs/commandbar.h>
#include <dialogs/filedialog.h>
#include <dialogs/imagedialog.h>
#include <dialogs/layoutdialog.h>
#include <dialogs/linkdialog.h>
#include <dialogs/localtrashdialog.h>
#include <dialogs/notedialog.h>
#include <dialogs/notediffdialog.h>
#include <dialogs/settingsdialog.h>
#include <dialogs/storedattachmentsdialog.h>
#include <dialogs/storedimagesdialog.h>
#include <dialogs/tabledialog.h>
#include <dialogs/tagadddialog.h>
#include <helpers/arcnotesmarkdownhighlighter.h>
#include <helpers/toolbarcontainer.h>
#include <utils/gui.h>
#include <utils/listutils.h>
#include <utils/misc.h>
#include <utils/schema.h>
#include <viewmodels/navigationviewmodel.h>
#include <viewmodels/noteeditorviewmodel.h>
#include <viewmodels/notefolderviewmodel.h>
#include <viewmodels/notelistviewmodel.h>
#include <viewmodels/notesubfolderviewmodel.h>
#include <viewmodels/searchviewmodel.h>
#include <viewmodels/settingsviewmodel.h>
#include <viewmodels/tagtreeviewmodel.h>
#include <views/logview.h>
#include <views/navigationview.h>
#include <views/noteeditorview.h>
#include <views/notelistview.h>
#include <views/notepreviewview.h>
#include <views/notesubfolderview.h>
#include <views/searchpanelview.h>
#include <views/tagtreeview.h>
#include <widgets/arcnotesmarkdowntextedit.h>
#include <widgets/combobox.h>
#include <widgets/lineedit.h>

#include <QAbstractItemModel>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QCursor>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QIcon>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QPageLayout>
#include <QPageSetupDialog>
#include <QPageSize>
#include <QPlainTextEdit>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QtGlobal>
#include <algorithm>

#include "mainwindow.h"
#include "version.h"

void MainWindow::triggerStartupMenuAction() {
    const QString actionName = qApp->property("startupAction").toString();
    if (!actionName.isEmpty()) {
        findAction(actionName)->trigger();
    }
}

void MainWindow::setCurrentNoteText(const QString& text) {
    if (_noteTextEdit != nullptr && _noteTextEdit->toPlainText() != text) {
        const QSignalBlocker blocker(_noteTextEdit);
        _noteTextEdit->setText(text);
    }
}

void MainWindow::setCurrentNote(const NoteData& note, bool updateNoteText, bool, bool) {
    _currentNote = note;
    if (updateNoteText) {
        setCurrentNoteText(note.noteText);
    }
    emit currentNoteChanged(_currentNote);
}

void MainWindow::setCurrentNoteFromNoteId(int noteId) {
    if (_coordinator != nullptr) {
        _coordinator->noteListViewModel()->openNote(noteId);
    }
}

void MainWindow::reloadCurrentNoteByNoteId(bool) {
    if (_currentNote.id > 0) {
        setCurrentNoteFromNoteId(_currentNote.id);
    }
}

QString MainWindow::currentNoteFolderPath() const {
    if (_coordinator == nullptr) {
        return QString();
    }

    return _coordinator->appState()->currentNoteFolder().localPath;
}

void MainWindow::createNewNote(QString noteName, bool) {
    if (_coordinator == nullptr) {
        return;
    }

    if (noteName.isEmpty()) {
        createDefaultNote();
    } else {
        _coordinator->noteListViewModel()->createNote(noteName, _coordinator->appState()->currentNoteFolder().id,
                                                      _coordinator->noteSubFolderViewModel()->activeSubFolderId());
    }
}

void MainWindow::createNewNote(QString name, QString text, CreateNewNoteOptions) {
    if (_coordinator == nullptr) {
        return;
    }

    _coordinator->noteListViewModel()->createNote(name, _coordinator->appState()->currentNoteFolder().id,
                                                  _coordinator->noteSubFolderViewModel()->activeSubFolderId());
    if (!text.isEmpty()) {
        setCurrentNoteText(text);
        _coordinator->noteEditorViewModel()->textChanged(text);
        _coordinator->noteEditorViewModel()->save();
    }
}

void MainWindow::createNewNoteSubFolder() {
    bool ok = false;
    const QString name =
        QInputDialog::getText(this, tr("Create a new folder"), tr("Folder name:"), QLineEdit::Normal, QString(), &ok)
            .trimmed();

    if (!ok || name.isEmpty()) {
        return;
    }

    createNewNoteSubFolder(name);
}

void MainWindow::createNewNoteSubFolder(const QString& name) {
    if (_coordinator != nullptr) {
        _coordinator->noteSubFolderViewModel()->createSubFolder(
            name, _coordinator->noteSubFolderViewModel()->activeSubFolderId());
    }
}

void MainWindow::doSearchInNote(QString searchText) {
    if (_noteTextEdit == nullptr) {
        return;
    }
    if (searchText.isEmpty() && _searchLineEdit != nullptr) {
        searchText = _searchLineEdit->text();
    }
    if (!searchText.isEmpty()) {
        _noteTextEdit->find(searchText);
    } else {
        _noteTextEdit->setFocus();
    }
}

const NoteData& MainWindow::getCurrentNote() const {
    return _currentNote;
}

void MainWindow::openSettingsDialog(int page) {
    auto* dialog =
        new SettingsDialog(page, this, _coordinator == nullptr ? nullptr : _coordinator->settingsViewModel());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
}

void MainWindow::applyDarkModeSettings() {
    initStyling();
    emit settingsChanged();
    updatePreview();
}

void MainWindow::showStatusBarMessage(const QString& message, const QString&, int timeout) {
    statusBar()->showMessage(message, timeout);
}

void MainWindow::showStatusBarMessage(const QString& message, int timeout) {
    showStatusBarMessage(message, QString(), timeout);
}

void MainWindow::handleInsertingFromMimeData(const QMimeData* mimeData) {
    if (_noteTextEdit == nullptr || mimeData == nullptr) {
        return;
    }
    if (mimeData->hasText()) {
        _noteTextEdit->textCursor().insertText(mimeData->text());
    }
}

ArcNotesMarkdownTextEdit* MainWindow::activeNoteTextEdit() {
    return _noteTextEdit;
}

ArcNotesMarkdownTextEdit* MainWindow::noteTextEdit() {
    return _noteTextEdit;
}

QList<QMenu*> MainWindow::menuList() {
    return menuBar()->findChildren<QMenu*>();
}

QAction* MainWindow::findAction(const QString& objectName) {
    return ensureAction(objectName);
}

void MainWindow::writeToNoteTextEdit(const QString& text) {
    if (_noteTextEdit != nullptr) {
        _noteTextEdit->textCursor().insertText(text);
    }
}

QString MainWindow::selectedNoteTextEditText() {
    return _noteTextEdit == nullptr ? QString() : _noteTextEdit->textCursor().selectedText();
}

NoteData MainWindow::storedFileDialogCurrentNote() const {
    return _coordinator == nullptr ? NoteData() : _coordinator->appState()->currentNote();
}

QVector<NoteData> MainWindow::storedFileDialogAllNotes() const {
    return _coordinator == nullptr ? QVector<NoteData>() : _coordinator->allNotes();
}

QString MainWindow::storedFileDialogMediaPath() const {
    return currentNoteFolderPath() + QDir::separator() + QStringLiteral("media");
}

QString MainWindow::storedFileDialogAttachmentsPath() const {
    return currentNoteFolderPath() + QDir::separator() + QStringLiteral("attachments");
}

QStringList MainWindow::storedFileDialogMediaFiles(const NoteData& note) const {
    return _coordinator == nullptr ? QStringList() : _coordinator->mediaFileList(note);
}

QStringList MainWindow::storedFileDialogAttachmentFiles(const NoteData& note) const {
    return _coordinator == nullptr ? QStringList() : _coordinator->attachmentFileList(note);
}

QString MainWindow::storedFileDialogMediaUrlStringForFileName(const NoteData& note, const QString& fileName) const {
    return _coordinator == nullptr ? QString() : _coordinator->mediaUrlStringForFileName(note, fileName);
}

QString MainWindow::storedFileDialogAttachmentUrlStringForFileName(const NoteData& note,
                                                                   const QString& fileName) const {
    return _coordinator == nullptr ? QString() : _coordinator->attachmentUrlStringForFileName(note, fileName);
}

bool MainWindow::storedFileDialogRenameMediaReferences(const QVector<int>& noteIds, const QString& oldFileName,
                                                       const QString& newFileName) {
    return _coordinator != nullptr && _coordinator->renameMediaReferences(noteIds, oldFileName, newFileName);
}

bool MainWindow::storedFileDialogRenameAttachmentReferences(const QVector<int>& noteIds, const QString& oldFileName,
                                                            const QString& newFileName) {
    return _coordinator != nullptr && _coordinator->renameAttachmentReferences(noteIds, oldFileName, newFileName);
}

void MainWindow::storedFileDialogInsertText(const QString& text) {
    writeToNoteTextEdit(text);
}

void MainWindow::storedFileDialogOpenNoteInTab(int noteId) {
    setCurrentNoteFromNoteId(noteId);
    openCurrentNoteInTab();
}

void MainWindow::storedFileDialogReloadCurrentNote(bool force) {
    reloadCurrentNoteByNoteId(force);
}

void MainWindow::handleTextNoteLinking(int page) {
    ArcNotesMarkdownTextEdit* textEdit = activeNoteTextEdit();
    if (textEdit == nullptr) {
        return;
    }

    auto* dialog = new LinkDialog(page, QString(), this);
    if (_coordinator != nullptr) {
        dialog->setCurrentNote(_coordinator->appState()->currentNote());
    }

    const QString selectedText = textEdit->textCursor().selectedText();
    if (!selectedText.isEmpty()) {
        const QString trimmedSelectedText = selectedText.trimmed();
        const QUrl selectedUrl(trimmedSelectedText);

        if (selectedUrl.isValid() && selectedUrl.scheme().startsWith(QStringLiteral("http"))) {
            dialog->setLinkName(QString());
            dialog->setURL(trimmedSelectedText);
        } else {
            dialog->setLinkName(selectedText);
        }
    }

    dialog->exec();

    if (dialog->result() == QDialog::Accepted) {
        const QString url = dialog->getURL();
        const QString linkName = dialog->getLinkName();
        const QString linkDescription = dialog->getLinkDescription();
        QString noteName = dialog->getSelectedNoteName().remove(QStringLiteral("]"));

        if (!noteName.isEmpty() || !url.isEmpty()) {
            QString newText;
            QString chosenLinkName = linkName.isEmpty() ? textEdit->textCursor().selectedText() : linkName;
            chosenLinkName.remove(QStringLiteral("]"));

            if (!url.isEmpty()) {
                newText = !chosenLinkName.isEmpty()
                              ? QStringLiteral("[") + chosenLinkName + QStringLiteral("](") + url + QStringLiteral(")")
                              : QStringLiteral("<") + url + QStringLiteral(">");
            } else if (dialog->isWikiLink()) {
                const NoteData selectedNote = dialog->getSelectedNote();
                const QString wikiName = selectedNote.id > 0 ? selectedNote.name : noteName;
                newText = QStringLiteral("[[") + wikiName + QStringLiteral("]]");
            } else {
                const NoteData selectedNote = dialog->getSelectedNote();
                const NoteData currentNote =
                    _coordinator == nullptr ? NoteData() : _coordinator->appState()->currentNote();
                const QString noteUrl = _coordinator != nullptr && currentNote.id > 0
                                            ? _coordinator->noteUrlForLinkingTo(currentNote, selectedNote)
                                            : selectedNote.name;
                const QString heading = dialog->getSelectedHeading();
                const QString headingText =
                    heading.isEmpty() ? QString() : QStringLiteral("#") + QUrl::toPercentEncoding(heading);

                if (!chosenLinkName.isEmpty()) {
                    noteName = chosenLinkName;
                } else if (!heading.isEmpty()) {
                    noteName += QStringLiteral(" - ") + heading;
                }

                newText =
                    QStringLiteral("[") + noteName + QStringLiteral("](") + noteUrl + headingText + QStringLiteral(")");
            }

            if (!linkDescription.isEmpty()) {
                newText += QStringLiteral(" ") + linkDescription;
            }

            textEdit->textCursor().insertText(newText);
        }
    }

    delete dialog;
}

void MainWindow::handleImageInsertion() {
    auto* dialog = new ImageDialog(this);
    const int ret = dialog->exec();

    if (ret == QDialog::Accepted) {
        QString title = dialog->getImageTitle();

        if (dialog->isDisableCopying()) {
            QString pathOrUrl = dialog->getFilePathOrUrl();
            const QUrl url(pathOrUrl);

            if (!url.isValid()) {
                delete dialog;
                return;
            }

            if (url.scheme() == QStringLiteral("file")) {
                pathOrUrl = url.toLocalFile();
            }

            if (!url.scheme().startsWith(QStringLiteral("http")) && _currentNote.id > 0 && _coordinator != nullptr) {
                pathOrUrl = _coordinator->noteRelativeFilePath(_currentNote, pathOrUrl);
            }

#ifdef Q_OS_WIN32
            if (Utils::Misc::fileExists(pathOrUrl)) {
                pathOrUrl = QStringLiteral("file:///") + QString::fromUtf8(QUrl::toPercentEncoding(pathOrUrl));
            }
#endif

            if (title.isEmpty()) {
                title = QStringLiteral("img");
            }

            insertNoteText(QStringLiteral("![") + title + QStringLiteral("](") + pathOrUrl + QStringLiteral(")"));
        } else {
            QFile* file = dialog->getImageFile();

            if (file != nullptr && file->size() > 0) {
                insertMedia(file, title);
            }
        }
    }

    delete dialog;
}

bool MainWindow::insertMedia(QFile* file, QString title) {
    if (file == nullptr || _currentNote.id <= 0 || _coordinator == nullptr) {
        return false;
    }

    const QString text = _coordinator->insertMediaMarkdown(_currentNote, file->fileName(), title);

    if (!text.isEmpty()) {
        insertNoteText(text);
        return true;
    }

    return false;
}

void MainWindow::handleAttachmentInsertion() {
    auto* dialog = new AttachmentDialog(this);
    dialog->exec();

    if (dialog->result() == QDialog::Accepted) {
        insertAttachment(dialog->getFile(), dialog->getTitle());
    }

    delete dialog;
}

bool MainWindow::insertAttachment(QFile* file, const QString& title, const QString& fileName) {
    if (file == nullptr || _currentNote.id <= 0 || _coordinator == nullptr) {
        return false;
    }

    QString text = _coordinator->insertAttachmentMarkdown(_currentNote, file->fileName(), title, fileName);

    if (text.isEmpty()) {
        return false;
    }

    ArcNotesMarkdownTextEdit* textEdit = activeNoteTextEdit();
    if (textEdit == nullptr) {
        return false;
    }

    QTextCursor cursor = textEdit->textCursor();

    if (cursor.block() == textEdit->document()->firstBlock()) {
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        textEdit->setTextCursor(cursor);
    }

    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
    if (!cursor.atBlockStart() && cursor.selectedText() != QStringLiteral(" ")) {
        text = QStringLiteral(" ") + text;
    }

    cursor = textEdit->textCursor();
    cursor.insertText(text);
    return true;
}

void MainWindow::insertNoteText(const QString& text) {
    ArcNotesMarkdownTextEdit* textEdit = activeNoteTextEdit();
    if (textEdit == nullptr) {
        return;
    }

    QTextCursor cursor = textEdit->textCursor();

    if (cursor.block() == textEdit->document()->firstBlock()) {
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        textEdit->setTextCursor(cursor);
    }

    cursor.insertText(text);
}

void MainWindow::storeUpdatedNotesToDisk() {
    if (_coordinator != nullptr) {
        _coordinator->noteEditorViewModel()->save();
    }
}

bool MainWindow::isNoteDiffDialogOpen() const {
    const QList<NoteDiffDialog*> dialogs = findChildren<NoteDiffDialog*>();
    return std::ranges::any_of(dialogs,
                               [](NoteDiffDialog* dialog) { return dialog != nullptr && dialog->isVisible(); });
}

bool MainWindow::doNoteEditingCheck() const {
    return true;
}

void MainWindow::allowNoteEditing() {
    if (_noteTextEdit != nullptr) {
        _noteTextEdit->setReadOnly(false);
    }
}

void MainWindow::disallowNoteEditing() {
    if (_noteTextEdit != nullptr) {
        _noteTextEdit->setReadOnly(true);
    }
}

void MainWindow::buildNotesIndexAndLoadNoteDirectoryList(bool, bool) {
    if (_coordinator != nullptr) {
        _coordinator->refreshModels();
    }
}

bool MainWindow::showNotesFromAllNoteSubFolders() const {
    return _showNotesFromAllNoteSubFolders;
}

void MainWindow::setShowNotesFromAllNoteSubFolders(bool enabled) {
    _showNotesFromAllNoteSubFolders = enabled;
    if (_coordinator != nullptr) {
        _coordinator->noteSubFolderViewModel()->setShowNotesFromAll(enabled);
    }
}

void MainWindow::clearNoteDirectoryWatcher() {
    if (_coordinator != nullptr) {
        _coordinator->noteListViewModel()->setNotes({});
    }
}

void MainWindow::updateNoteDirectoryWatcher() {
    if (_coordinator != nullptr) {
        _coordinator->refreshModels();
    }
}

void MainWindow::openCurrentNoteInTab() {
    openNoteInTab(_currentNote, true);
}

void MainWindow::openNoteInTab(const NoteData& note, bool setCurrent) {
    if (note.id <= 0 || _noteEditTabWidget == nullptr) {
        return;
    }

    const int noteId = note.id;
    for (int i = 0; i < _noteEditTabWidget->count(); ++i) {
        QWidget* page = _noteEditTabWidget->widget(i);
        if (page != nullptr && page->property("note-id").toInt() == noteId) {
            _noteEditTabWidget->setCurrentIndex(i);
            if (setCurrent) {
                setCurrentNote(note);
            }
            return;
        }
    }

    QWidget* page = _noteEditorView;
    int tabIndex = _noteEditTabWidget->indexOf(page);
    if (tabIndex < 0) {
        tabIndex = _noteEditTabWidget->addTab(page, note.name);
    }

    page->setProperty("note-id", noteId);
    _noteEditTabWidget->setTabText(tabIndex, note.name);
    _noteEditTabWidget->setCurrentIndex(tabIndex);

    if (setCurrent) {
        setCurrentNote(note);
    }
}

void MainWindow::onNavigationWidgetPositionClicked(int position) {
    if (_noteTextEdit == nullptr) {
        return;
    }
    QTextCursor cursor = _noteTextEdit->textCursor();
    cursor.setPosition(qMax(0, position));
    _noteTextEdit->setTextCursor(cursor);
}

void MainWindow::jumpToNoteSubFolder(int noteSubFolderId) {
    if (_coordinator != nullptr) {
        _coordinator->noteSubFolderViewModel()->selectSubFolder(noteSubFolderId);
    }
}

void MainWindow::refreshNotePreview() {
    updatePreview();
}

NoteData MainWindow::urlCurrentNote() const {
    return _coordinator == nullptr ? NoteData() : _coordinator->appState()->currentNote();
}

QString MainWindow::urlCurrentNoteFolderPath() const {
    return currentNoteFolderPath();
}

QString MainWindow::urlCurrentNoteSubFolderRelativePath() const {
    return _coordinator == nullptr ? QString() : _coordinator->appState()->currentNote().relativeNoteSubFolderPath;
}

QString MainWindow::urlFileUrlFromCurrentNoteFileName(const QString& fileName, bool withFragment) const {
    return _coordinator == nullptr
               ? QString()
               : _coordinator->noteFileUrlFromFileName(_coordinator->appState()->currentNote(), fileName, withFragment);
}

QTextDocument* MainWindow::urlActiveNoteDocument() const {
    return _noteTextEdit != nullptr ? _noteTextEdit->document() : nullptr;
}

ArcNotesMarkdownTextEdit* MainWindow::urlNoteTextEdit() const {
    return _noteTextEdit;
}

QString MainWindow::urlFragmentFromFileName(const QString& fileName) const {
    return _coordinator == nullptr ? QString() : _coordinator->noteUrlFragmentFromFileName(fileName);
}

bool MainWindow::urlFileUrlIsNoteInCurrentFolder(const QUrl& url) const {
    return _coordinator != nullptr && _coordinator->noteFileUrlIsInCurrentFolder(url);
}

bool MainWindow::urlFileUrlIsExistingNoteInCurrentFolder(const QUrl& url) const {
    return _coordinator != nullptr && _coordinator->noteFileUrlIsExistingInCurrentFolder(url);
}

QString MainWindow::urlRelativePathForFileUrlInCurrentFolder(const QUrl& url) const {
    return _coordinator == nullptr ? QString() : _coordinator->noteRelativePathForFileUrlInCurrentFolder(url);
}

NoteData MainWindow::urlNoteById(int noteId) const {
    return _coordinator == nullptr ? NoteData() : _coordinator->note(noteId);
}

NoteData MainWindow::urlNoteByFileUrl(const QUrl& url) const {
    return _coordinator == nullptr ? NoteData() : _coordinator->noteByFileUrl(url);
}

NoteData MainWindow::urlNoteByUrlString(const QString& urlString) const {
    return _coordinator == nullptr ? NoteData() : _coordinator->noteByUrlString(urlString);
}

bool MainWindow::urlWikiLinkSupportEnabled() const {
    return _coordinator != nullptr && _coordinator->wikiLinkSupportEnabled();
}

NoteData MainWindow::urlResolveWikiLink(const QString& target, int currentNoteSubFolderId) const {
    return _coordinator == nullptr ? NoteData() : _coordinator->resolveWikiLink(target, currentNoteSubFolderId);
}

bool MainWindow::urlCurrentFolderHasSubfolders() const {
    return _coordinator != nullptr && _coordinator->appState()->currentNoteFolder().showSubfolders;
}

int MainWindow::urlNoteSubFolderIdByPath(const QString& pathData) const {
    return _coordinator == nullptr ? 0 : _coordinator->noteSubFolderByPath(pathData).id;
}

int MainWindow::urlEnsureNoteSubFolderPath(const QString& pathData) {
    return _coordinator == nullptr ? 0 : _coordinator->ensureNoteSubFolderPath(pathData).id;
}

int MainWindow::urlEnsureChildNoteSubFolder(const QString& name, int parentId) {
    return _coordinator == nullptr ? 0 : _coordinator->ensureChildNoteSubFolder(name, parentId).id;
}

void MainWindow::urlSetActiveNoteSubFolder(int noteSubFolderId) {
    if (_coordinator != nullptr) {
        _coordinator->setActiveNoteSubFolder(noteSubFolderId);
    }
}

void MainWindow::urlOpenNote(const NoteData& note, bool openInNewTab) {
    if (note.id <= 0) {
        return;
    }

    if (openInNewTab) {
        openNoteInTab(note, true);
    } else {
        setCurrentNoteFromNoteId(note.id);
    }
}

void MainWindow::urlCreateNote(const QString& name, bool withNameAppend) {
    createNewNote(name, withNameAppend);
}

void MainWindow::urlRefreshNoteFolders() {
    buildNotesIndexAndLoadNoteDirectoryList(true, true);
}

void MainWindow::urlJumpToNoteSubFolder(int noteSubFolderId) {
    jumpToNoteSubFolder(noteSubFolderId);
}

void MainWindow::urlJumpToEditorPosition(int position) {
    onNavigationWidgetPositionClicked(position);
}

bool MainWindow::urlDoNoteEditingCheck() const {
    return doNoteEditingCheck();
}

void MainWindow::urlRefreshNotePreview() {
    refreshNotePreview();
}

bool MainWindow::editorIsInDistractionFreeMode() const {
    return isInDistractionFreeMode();
}

void MainWindow::editorShowStatusMessage(const QString& message, const QString& symbol, int timeout) {
    showStatusBarMessage(message, symbol, timeout);
}

QAction* MainWindow::editorAction(const QString& objectName) {
    return findAction(objectName);
}

NoteData MainWindow::editorCurrentNote() const {
    return _coordinator == nullptr ? NoteData() : _coordinator->appState()->currentNote();
}

QVector<NoteData> MainWindow::editorAllNotes() const {
    return _coordinator == nullptr ? QVector<NoteData>() : _coordinator->allNotes();
}

QString MainWindow::editorCurrentNoteFolderPath() const {
    return currentNoteFolderPath();
}

QString MainWindow::editorRenderTextToHtml(const QString& text, bool forExport) const {
    if (_coordinator == nullptr) {
        return {};
    }

    return _coordinator->renderTextToHtml(_coordinator->appState()->currentNote(), text, currentNoteFolderPath(),
                                          getMaxImageWidth(), forExport);
}

bool MainWindow::editorWikiLinkSupportEnabled() const {
    return _coordinator != nullptr && _coordinator->wikiLinkSupportEnabled();
}

bool MainWindow::editorFileUrlIsNoteInCurrentFolder(const QUrl& url) const {
    return _coordinator != nullptr && _coordinator->noteFileUrlIsInCurrentFolder(url);
}

QVariant MainWindow::editorSettingValue(const QString& key, const QVariant& defaultValue) const {
    return persistentSetting(key, defaultValue);
}

void MainWindow::editorSetSettingValue(const QString& key, const QVariant& value) {
    setPersistentSetting(key, value);
}

QString MainWindow::editorSelectedText() const {
    return _noteTextEdit == nullptr ? QString() : _noteTextEdit->textCursor().selectedText();
}

int MainWindow::editorMaxImageWidth() const {
    return getMaxImageWidth();
}

void MainWindow::editorPrintDocument(QTextDocument* document, bool selectedOnly) {
    printTextDocument(document, selectedOnly);
}

void MainWindow::editorExportNoteAsPDF(QTextDocument* document, bool selectedOnly) {
    exportNoteAsPDF(document, selectedOnly);
}

void MainWindow::editorHandleMimeData(const QMimeData* mimeData) {
    handleInsertingFromMimeData(mimeData);
}

bool MainWindow::editorDoNoteEditingCheck() const {
    return doNoteEditingCheck();
}

void MainWindow::editorAllowNoteEditing() {
    allowNoteEditing();
}

void MainWindow::editorDisallowNoteEditing() {
    disallowNoteEditing();
}

QVector<NoteData> MainWindow::linkDialogAllNotes() const {
    return _coordinator == nullptr ? QVector<NoteData>() : _coordinator->allNotes();
}

NoteData MainWindow::linkDialogNoteById(int noteId) const {
    return _coordinator == nullptr ? NoteData() : _coordinator->note(noteId);
}

QStringList MainWindow::linkDialogSearchNoteNames(const QString& query) const {
    return _coordinator == nullptr ? QStringList() : _coordinator->searchNoteNames(query);
}

QHash<QString, QStringList> MainWindow::linkDialogTagNamesByNoteFilePath() const {
    return _coordinator == nullptr ? QHash<QString, QStringList>() : _coordinator->tagNamesByNoteFilePath();
}

QString MainWindow::linkDialogRelativeFilePathFromCurrentNote(const QString& path) const {
    if (_coordinator == nullptr) {
        return {};
    }

    return _coordinator->noteRelativeFilePath(_coordinator->appState()->currentNote(), path);
}

bool MainWindow::linkDialogShowSubfolders() const {
    return _coordinator != nullptr && _coordinator->appState()->currentNoteFolder().showSubfolders;
}

bool MainWindow::linkDialogWikiLinkSupportEnabled() const {
    return _coordinator != nullptr && _coordinator->wikiLinkSupportEnabled();
}

bool MainWindow::linkDialogDarkModeColors() const {
    return persistentSetting(QStringLiteral("darkModeColors")).toBool();
}

QUrl MainWindow::linkDialogLastSelectedFileUrl() const {
    return persistentSetting(QStringLiteral("LinkDialog/lastSelectedFileUrl")).toUrl();
}

void MainWindow::linkDialogSetLastSelectedFileUrl(const QString& fileUrlString) {
    setPersistentSetting(QStringLiteral("LinkDialog/lastSelectedFileUrl"), fileUrlString);
}

QUrl MainWindow::linkDialogLastSelectedDirectoryUrl() const {
    return persistentSetting(QStringLiteral("LinkDialog/lastSelectedDirectoryUrl")).toUrl();
}

void MainWindow::linkDialogSetLastSelectedDirectoryUrl(const QString& directoryUrlString) {
    setPersistentSetting(QStringLiteral("LinkDialog/lastSelectedDirectoryUrl"), directoryUrlString);
}

QVector<TrashItemData> MainWindow::localTrashItems() const {
    return _coordinator == nullptr ? QVector<TrashItemData>() : _coordinator->trashItems();
}

QString MainWindow::localTrashItemText(int trashItemId) const {
    return _coordinator == nullptr ? QString() : _coordinator->trashItemText(trashItemId);
}

QString MainWindow::localTrashItemRestorationPath(int trashItemId) const {
    return _coordinator == nullptr ? QString() : _coordinator->trashItemRestorationPath(trashItemId);
}

bool MainWindow::localTrashItemFileExists(int trashItemId) const {
    return _coordinator != nullptr && _coordinator->trashItemFileExists(trashItemId);
}

bool MainWindow::localTrashRestoreItems(const QVector<int>& trashItemIds) {
    return _coordinator != nullptr && _coordinator->restoreTrashItems(trashItemIds);
}

bool MainWindow::localTrashRemoveItems(const QVector<int>& trashItemIds) {
    return _coordinator != nullptr && _coordinator->removeTrashItems(trashItemIds);
}

NoteData MainWindow::noteDialogNoteById(int noteId) const {
    return _coordinator == nullptr ? NoteData() : _coordinator->note(noteId);
}

QString MainWindow::noteDialogRenderNoteToHtml(const NoteData& note, const QString& noteFolderPath) const {
    return _coordinator == nullptr ? QString() : _coordinator->renderTextToHtml(note, note.noteText, noteFolderPath);
}

void MainWindow::setCurrentLayout(const QString& layoutUuid) {
    if (layoutUuid.isEmpty()) {
        return;
    }

    const QString currentUuid = persistentSetting(QStringLiteral("currentLayout")).toString();
    if (!currentUuid.isEmpty()) {
        storeCurrentLayoutToSettings();
        setPersistentSetting(QStringLiteral("previousLayout"), currentUuid);
    }

    setPersistentSetting(QStringLiteral("currentLayout"), layoutUuid);
    restoreCurrentLayoutFromSettings();
    applyDockLocking(findAction(QStringLiteral("actionUnlock_panels"))->isChecked());
    updatePreview();

    if (_coordinator != nullptr) {
        _coordinator->uiState()->setCurrentLayoutUuid(layoutUuid);
    }
}
