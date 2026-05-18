/*
 * Copyright (c) 2014-2026 Patrizio Bekerle -- <patrizio@bekerle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#pragma once

#include <core/data/notedata.h>
#include <core/data/notehistorydata.h>
#include <dialogs/linkdialoghost.h>
#include <dialogs/localtrashdialoghost.h>
#include <dialogs/notedialoghost.h>
#include <dialogs/storedfiledialoghost.h>
#include <utils/urlhandler.h>
#include <widgets/arcnotesmarkdowntextedithost.h>
#include <widgets/logwidget.h>

#include <QHash>
#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <QVariantMap>
#include <QVector>
#include <memory>

#define SORT_ALPHABETICAL 0
#define SORT_BY_LAST_CHANGE 1
#define ORDER_ASCENDING 0
#define ORDER_DESCENDING 1

class QAction;
class AppCoordinator;
class QLabel;
class QComboBox;
class QDockWidget;
class QFile;
class QLineEdit;
class QMenu;
class QMimeData;
class ArcNotesMarkdownTextEdit;
class QPrinter;
class QSplitter;
class QTabWidget;
class QTextDocument;
class NoteEditorView;
class NoteListView;
class NotePreviewView;
class NoteSubFolderView;
class TagTreeView;
class NavigationView;
class SearchPanelView;
class LogView;
class NoteRelationView;

class MainWindow : public QMainWindow,
                   public UrlHandlerContext,
                   public ArcNotesMarkdownTextEditHost,
                   public LinkDialogHost,
                   public LocalTrashDialogHost,
                   public NoteDialogHost,
                   public StoredFileDialogHost {
    Q_OBJECT
    Q_PROPERTY(NoteData currentNote READ getCurrentNote WRITE setCurrentNote NOTIFY currentNoteChanged)

Q_SIGNALS:
    void currentNoteChanged(const NoteData& note);
    void log(LogWidget::LogType logType, QString text);
    void settingsChanged();

public:
    enum CreateNewNoteOption {
        None = 0x0000,
        UseNameAsHeadline = 0x0001,
        CursorAtEnd = 0x0002,
        DisableLoadNoteDirectoryList = 0x0004,
    };
    Q_DECLARE_FLAGS(CreateNewNoteOptions, CreateNewNoteOption)

    explicit MainWindow(QWidget* parent = nullptr);
    explicit MainWindow(AppCoordinator* coordinator, QWidget* parent = nullptr);
    ~MainWindow() override;

    bool isInDistractionFreeMode() const;

    void triggerStartupMenuAction();
    void setCurrentNoteText(const QString& text);
    void setCurrentNote(const NoteData& note, bool updateNoteText = true, bool updateSelectedNote = true,
                        bool addPreviousNoteToHistory = true);
    void setCurrentNoteFromNoteId(int noteId);
    void reloadCurrentNoteByNoteId(bool force = false);
    QString currentNoteFolderPath() const;
    void createNewNote(QString noteName = QString(), bool withNameAppend = true);
    void createNewNote(QString name, QString text, CreateNewNoteOptions options = CreateNewNoteOption::None);
    void createNewNoteSubFolder();
    void createNewNoteSubFolder(const QString& name);
    void doSearchInNote(QString searchText);
    const NoteData& getCurrentNote() const;
    void openSettingsDialog(int page = 0);
    Q_INVOKABLE void applyDarkModeSettings();
    void showStatusBarMessage(const QString& message, const QString& symbol = QString(), int timeout = 4000);
    void showStatusBarMessage(const QString& message, int timeout = 4000);
    void handleInsertingFromMimeData(const QMimeData* mimeData);
    ArcNotesMarkdownTextEdit* activeNoteTextEdit();
    ArcNotesMarkdownTextEdit* noteTextEdit();
    QList<QMenu*> menuList();
    QAction* findAction(const QString& objectName);
    void writeToNoteTextEdit(const QString& text);
    QString selectedNoteTextEditText();
    void storeUpdatedNotesToDisk();
    bool isNoteDiffDialogOpen() const;
    bool doNoteEditingCheck() const;
    void allowNoteEditing();
    void disallowNoteEditing();
    void buildNotesIndexAndLoadNoteDirectoryList(bool forceBuild = false, bool forceLoad = false);
    bool showNotesFromAllNoteSubFolders() const;
    void setShowNotesFromAllNoteSubFolders(bool enabled);
    void clearNoteDirectoryWatcher();
    void updateNoteDirectoryWatcher();
    void openCurrentNoteInTab();
    void openNoteInTab(const NoteData& note, bool setCurrent = false);
    void onNavigationWidgetPositionClicked(int position);
    void jumpToNoteSubFolder(int noteSubFolderId);
    void refreshNotePreview();
    void setCurrentLayout(const QString& layoutUuid);
    int getMaxImageWidth() const;
    void printTextDocument(QTextDocument* document, bool selectedOnly = false);
    void exportNoteAsPDF(QTextDocument* document, bool selectedOnly = false);

    NoteData urlCurrentNote() const override;
    QString urlCurrentNoteFolderPath() const override;
    QString urlCurrentNoteSubFolderRelativePath() const override;
    QString urlFileUrlFromCurrentNoteFileName(const QString& fileName, bool withFragment) const override;
    QTextDocument* urlActiveNoteDocument() const override;
    ArcNotesMarkdownTextEdit* urlNoteTextEdit() const override;
    QString urlFragmentFromFileName(const QString& fileName) const override;
    bool urlFileUrlIsNoteInCurrentFolder(const QUrl& url) const override;
    bool urlFileUrlIsExistingNoteInCurrentFolder(const QUrl& url) const override;
    QString urlRelativePathForFileUrlInCurrentFolder(const QUrl& url) const override;
    NoteData urlNoteById(int noteId) const override;
    NoteData urlNoteByFileUrl(const QUrl& url) const override;
    NoteData urlNoteByUrlString(const QString& urlString) const override;
    bool urlWikiLinkSupportEnabled() const override;
    NoteData urlResolveWikiLink(const QString& target, int currentNoteSubFolderId) const override;
    bool urlCurrentFolderHasSubfolders() const override;
    int urlNoteSubFolderIdByPath(const QString& pathData) const override;
    int urlEnsureNoteSubFolderPath(const QString& pathData) override;
    int urlEnsureChildNoteSubFolder(const QString& name, int parentId) override;
    void urlSetActiveNoteSubFolder(int noteSubFolderId) override;
    void urlOpenNote(const NoteData& note, bool openInNewTab) override;
    void urlCreateNote(const QString& name, bool withNameAppend) override;
    void urlRefreshNoteFolders() override;
    void urlJumpToNoteSubFolder(int noteSubFolderId) override;
    void urlJumpToEditorPosition(int position) override;
    bool urlDoNoteEditingCheck() const override;
    void urlRefreshNotePreview() override;

    bool editorIsInDistractionFreeMode() const override;
    void editorShowStatusMessage(const QString& message, const QString& symbol, int timeout) override;
    QAction* editorAction(const QString& objectName) override;
    NoteData editorCurrentNote() const override;
    QVector<NoteData> editorAllNotes() const override;
    QString editorCurrentNoteFolderPath() const override;
    QString editorRenderTextToHtml(const QString& text, bool forExport) const override;
    bool editorWikiLinkSupportEnabled() const override;
    bool editorFileUrlIsNoteInCurrentFolder(const QUrl& url) const override;
    QVariant editorSettingValue(const QString& key, const QVariant& defaultValue = QVariant()) const override;
    void editorSetSettingValue(const QString& key, const QVariant& value) override;
    QString editorSelectedText() const override;
    int editorMaxImageWidth() const override;
    void editorPrintDocument(QTextDocument* document, bool selectedOnly) override;
    void editorExportNoteAsPDF(QTextDocument* document, bool selectedOnly) override;
    void editorHandleMimeData(const QMimeData* mimeData) override;
    bool editorDoNoteEditingCheck() const override;
    void editorAllowNoteEditing() override;
    void editorDisallowNoteEditing() override;

    QVector<NoteData> linkDialogAllNotes() const override;
    NoteData linkDialogNoteById(int noteId) const override;
    QStringList linkDialogSearchNoteNames(const QString& query) const override;
    QHash<QString, QStringList> linkDialogTagNamesByNoteFilePath() const override;
    QString linkDialogRelativeFilePathFromCurrentNote(const QString& path) const override;
    bool linkDialogShowSubfolders() const override;
    bool linkDialogWikiLinkSupportEnabled() const override;
    bool linkDialogDarkModeColors() const override;
    QUrl linkDialogLastSelectedFileUrl() const override;
    void linkDialogSetLastSelectedFileUrl(const QString& fileUrlString) override;
    QUrl linkDialogLastSelectedDirectoryUrl() const override;
    void linkDialogSetLastSelectedDirectoryUrl(const QString& directoryUrlString) override;

    QVector<TrashItemData> localTrashItems() const override;
    QString localTrashItemText(int trashItemId) const override;
    QString localTrashItemRestorationPath(int trashItemId) const override;
    bool localTrashItemFileExists(int trashItemId) const override;
    bool localTrashRestoreItems(const QVector<int>& trashItemIds) override;
    bool localTrashRemoveItems(const QVector<int>& trashItemIds) override;

    NoteData noteDialogNoteById(int noteId) const override;
    QString noteDialogRenderNoteToHtml(const NoteData& note, const QString& noteFolderPath) const override;

    NoteData storedFileDialogCurrentNote() const override;
    QVector<NoteData> storedFileDialogAllNotes() const override;
    QString storedFileDialogMediaPath() const override;
    QString storedFileDialogAttachmentsPath() const override;
    QStringList storedFileDialogMediaFiles(const NoteData& note) const override;
    QStringList storedFileDialogAttachmentFiles(const NoteData& note) const override;
    QString storedFileDialogMediaUrlStringForFileName(const NoteData& note, const QString& fileName) const override;
    QString storedFileDialogAttachmentUrlStringForFileName(const NoteData& note,
                                                           const QString& fileName) const override;
    bool storedFileDialogRenameMediaReferences(const QVector<int>& noteIds, const QString& oldFileName,
                                               const QString& newFileName) override;
    bool storedFileDialogRenameAttachmentReferences(const QVector<int>& noteIds, const QString& oldFileName,
                                                    const QString& newFileName) override;
    void storedFileDialogInsertText(const QString& text) override;
    void storedFileDialogOpenNoteInTab(int noteId) override;
    void storedFileDialogReloadCurrentNote(bool force) override;

    QAction* newNoteAction();
    QAction* reloadNoteFolderAction();
    QAction* insertTextLinkAction();
    QAction* toggleCheckboxesAction();
    QAction* createOrderedListAction();
    QAction* createAlphabeticalListAction();
    QAction* createUnorderedListAction();
    QAction* createCheckboxListAction();
    QAction* clearListFormattingAction();
    QAction* orderCheckboxesAction();
    QAction* increaseHeadingDepthAction();
    QAction* decreaseHeadingDepthAction();
    QAction* searchTextOnWebAction();
    QAction* findNoteAction();
    QAction* selectEnclosedTextAction();
    QAction* pasteImageAction();
    QAction* autocompleteAction();
    QAction* splitNoteAtPosAction();

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QAction* ensureAction(const QString& objectName, const QString& text = QString());
    void setupShell();
    void setupActions();
    void setupMenus();
    void setupToolBar();
    void applySavedToolBarConfiguration();
    void setupCentralLayout();
    void setupStatusBarWidgets();
    void wireViewModels();
    void wireActions();
    void initStyling();
    void restoreCurrentLayoutFromSettings();
    void storeCurrentLayoutToSettings();
    void applyDockLocking(bool unlocked);
    void restoreDockTitleBars();
    void updateLayoutComboBox();
    void applyDistractionFreeMode(bool enabled, bool persistSetting = true);
    void handleTextNoteLinking(int page);
    void handleImageInsertion();
    bool insertMedia(QFile* file, QString title);
    void handleAttachmentInsertion();
    bool insertAttachment(QFile* file, const QString& title = QString(), const QString& fileName = QString());
    void insertNoteText(const QString& text);
    void addMenuAction(QMenu* menu, const QString& objectName);
    void showNoteListContextMenu(const QPoint& pos);
    void showTagTreeContextMenu(const QPoint& pos);
    void showNoteSubFolderContextMenu(const QPoint& pos);
    void showNavigationContextMenu(const QPoint& pos);
    void storeNoteBookmark(int slot);
    void gotoNoteBookmark(int slot);
    void deleteNoteBookmark(int slot);
    void openNoteBookmarkDialog();
    void updateNoteBookmarkDisplay();
    NoteHistoryItemData currentNoteHistoryItem() const;
    QVector<int> selectedNoteIds() const;
    QVector<int> selectedTagIds() const;
    QVector<int> selectedNoteSubFolderIds() const;
    void buildNoteSubFolderMenuTree(QMenu* parentMenu, bool copyNotes, const QVector<int>& selectedNotes,
                                    int parentId = 0);
    void buildTagMoveMenuTree(QMenu* parentMenu, const QVector<int>& selectedTags, int parentTagId = 0);
    void buildNoteTagMenuTree(QMenu* parentMenu, const QVector<int>& selectedNotes, int parentTagId = 0);
    void buildSubFolderMoveMenuTree(QMenu* parentMenu, const QVector<int>& selectedFolders, int parentId = 0);
    void copyNoteFilenameToClipboard();
    void renameCurrentHeadingFromNavigation();
    QVariant persistentSetting(const QString& key, const QVariant& defaultValue = QVariant()) const;
    QVector<QVariantMap> persistentSettingsArray(const QString& arrayName, const QStringList& keys) const;
    void setPersistentSetting(const QString& key, const QVariant& value);
    void removePersistentSetting(const QString& key);
    void configureAction(const QString& objectName, const QString& text, const QString& iconPath,
                         const QString& shortcut = QString(), bool checkable = false);
    void createDefaultNote();
    void onCurrentNoteChanged(const NoteData& note);
    void updateNoteFolderComboBoxVisibility();
    void updateCursorStatus();
    void updatePreview();
    QDockWidget* createDockPanel(const QString& objectName, const QString& title, QWidget* widget,
                                 Qt::DockWidgetArea area, Qt::Orientation orientation = Qt::Vertical);
    QString currentNoteReference(const NoteData& note) const;

    std::unique_ptr<AppCoordinator> _ownedCoordinator;
    AppCoordinator* _coordinator = nullptr;
    NoteData _currentNote;
    ArcNotesMarkdownTextEdit* _noteTextEdit = nullptr;
    QHash<QString, QAction*> _actions;
    bool _showNotesFromAllNoteSubFolders = true;

    QSplitter* _mainSplitter = nullptr;
    QComboBox* _layoutComboBox = nullptr;
    QComboBox* _noteFolderComboBox = nullptr;
    QLineEdit* _searchLineEdit = nullptr;
    QLineEdit* _noteSubFolderLineEdit = nullptr;
    QLineEdit* _tagLineEdit = nullptr;
    QLineEdit* _navigationLineEdit = nullptr;
    QTabWidget* _navigationTabWidget = nullptr;
    QTabWidget* _noteEditTabWidget = nullptr;
    QLabel* _filePathLabel = nullptr;
    QLabel* _cursorPositionLabel = nullptr;
    QHash<QString, QWidget*> _dockTitleBars;

    NoteEditorView* _noteEditorView = nullptr;
    NoteListView* _noteListView = nullptr;
    NoteSubFolderView* _noteSubFolderView = nullptr;
    TagTreeView* _tagTreeView = nullptr;
    NavigationView* _navigationView = nullptr;
    SearchPanelView* _searchPanelView = nullptr;
    NotePreviewView* _notePreviewView = nullptr;
    NoteRelationView* _noteRelationView = nullptr;
    LogView* _logView = nullptr;
    QDockWidget* _noteSubFolderDockWidget = nullptr;
    QDockWidget* _taggingDockWidget = nullptr;
    QDockWidget* _noteSearchDockWidget = nullptr;
    QDockWidget* _noteFolderDockWidget = nullptr;
    QDockWidget* _noteListDockWidget = nullptr;
    QDockWidget* _noteNavigationDockWidget = nullptr;
    QDockWidget* _noteTagDockWidget = nullptr;
    QDockWidget* _notePreviewDockWidget = nullptr;
    QDockWidget* _noteGraphicsViewDockWidget = nullptr;
    QDockWidget* _logDockWidget = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MainWindow::CreateNewNoteOptions)
