#pragma once

#include <core/data/notedata.h>
#include <core/data/notefolderdata.h>
#include <core/data/notehistorydata.h>
#include <core/data/notesubfolderdata.h>
#include <core/data/tagdata.h>
#include <core/data/trashitemdata.h>
#include <models/bookmarktablemodel.h>

#include <QHash>
#include <QList>
#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <QVariantMap>
#include <memory>

class ActionRegistry;
class AppState;
class BookmarkTableModel;
class CommandBus;
class ColorModeRepository;
class DialogCoordinator;
class EditorState;
class ExportService;
class QColor;
class MarkdownRenderService;
class MediaService;
class NavigationViewModel;
class NoteCommandHandlers;
class NoteEditorViewModel;
class NoteFileService;
class NoteFolderRepository;
class NoteFolderSettingsViewModel;
class NoteFolderService;
class NoteFolderViewModel;
class NoteHistoryRepository;
class NoteLinkService;
class NoteListViewModel;
class NoteRepository;
class NoteService;
class NoteSubFolderRepository;
class NoteSubFolderService;
class NoteSubFolderViewModel;
class SearchService;
class SearchState;
class SearchViewModel;
class SettingsRepository;
class SettingsViewModel;
class TagRepository;
class TagService;
class TagTreeViewModel;
class TrashRepository;
class TrashService;
class TrashViewModel;
class UiState;

class AppCoordinator : public QObject {
    Q_OBJECT

public:
    explicit AppCoordinator(QObject* parent = nullptr);
    ~AppCoordinator() override;

    void initialize();

    CommandBus* commandBus() const;
    ActionRegistry* actionRegistry() const;
    DialogCoordinator* dialogCoordinator() const;
    AppState* appState() const;
    EditorState* editorState() const;
    SearchState* searchState() const;
    UiState* uiState() const;

    NoteListViewModel* noteListViewModel() const;
    NoteEditorViewModel* noteEditorViewModel() const;
    TagTreeViewModel* tagTreeViewModel() const;
    NavigationViewModel* navigationViewModel() const;
    SearchViewModel* searchViewModel() const;
    NoteFolderViewModel* noteFolderViewModel() const;
    NoteFolderSettingsViewModel* noteFolderSettingsViewModel() const;
    NoteSubFolderViewModel* noteSubFolderViewModel() const;
    SettingsViewModel* settingsViewModel() const;
    TrashViewModel* trashViewModel() const;
    QVector<NoteData> allNotes() const;
    NoteData note(int noteId) const;
    QList<NoteFolderData> noteFolders() const;
    QVector<TagData> tags(int parentId = 0) const;
    QVector<TagData> tagsForNotes(const QVector<int>& noteIds) const;
    QVector<NoteSubFolderData> noteSubFolders(int parentId = 0) const;
    NoteSubFolderData noteSubFolder(int subFolderId) const;
    QString noteSubFolderFullPath(int subFolderId) const;
    QVariant currentNoteFolderSettingValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    NoteData noteByFileUrl(const QUrl& url) const;
    NoteData noteByUrlString(const QString& urlString) const;
    NoteData resolveWikiLink(const QString& target, int currentNoteSubFolderId) const;
    QStringList searchNoteNames(const QString& query) const;
    QHash<QString, QStringList> tagNamesByNoteFilePath() const;
    bool wikiLinkSupportEnabled() const;
    bool noteFileUrlIsInCurrentFolder(const QUrl& url) const;
    bool noteFileUrlIsExistingInCurrentFolder(const QUrl& url) const;
    QString noteUrlFragmentFromFileName(const QString& fileName) const;
    QString noteRelativePathForFileUrlInCurrentFolder(const QUrl& url) const;
    QString noteUrlForLinkingTo(const NoteData& source, const NoteData& target, bool forceLegacy = false) const;
    QString noteRelativeFilePath(const NoteData& note, const QString& path) const;
    QString noteFileUrlFromFileName(const NoteData& note, const QString& fileName, bool withFragment = false) const;
    QStringList mediaFileList(const NoteData& note) const;
    QStringList attachmentFileList(const NoteData& note) const;
    QString mediaUrlStringForFileName(const NoteData& note, const QString& fileName) const;
    QString attachmentUrlStringForFileName(const NoteData& note, const QString& fileName) const;
    QString insertMediaMarkdown(const NoteData& note, const QString& sourcePath, const QString& title = QString(),
                                bool returnUrlOnly = false);
    QString insertAttachmentMarkdown(const NoteData& note, const QString& sourcePath, const QString& title = QString(),
                                     const QString& fileName = QString(), bool returnUrlOnly = false);
    QString renderTextToHtml(const NoteData& note, const QString& text, const QString& notesPath,
                             int maxImageWidth = 980, bool forExport = false, bool base64Images = false) const;
    bool renameMediaReferences(const QVector<int>& noteIds, const QString& oldFileName, const QString& newFileName);
    bool renameAttachmentReferences(const QVector<int>& noteIds, const QString& oldFileName,
                                    const QString& newFileName);
    NoteSubFolderData noteSubFolderByPath(const QString& pathData) const;
    NoteSubFolderData ensureNoteSubFolderPath(const QString& pathData);
    NoteSubFolderData ensureChildNoteSubFolder(const QString& name, int parentId);
    bool setActiveNoteSubFolder(int noteSubFolderId);
    QVariant settingValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    QVector<QVariantMap> settingsArrayValues(const QString& arrayName, const QStringList& keys) const;
    QVector<TrashItemData> trashItems() const;
    QString trashItemText(int trashItemId) const;
    QString trashItemRestorationPath(int trashItemId) const;
    bool trashItemFileExists(int trashItemId) const;
    bool restoreTrashItems(const QVector<int>& trashItemIds);
    bool removeTrashItems(const QVector<int>& trashItemIds);
    QVector<BookmarkItemData> noteBookmarks() const;
    NoteHistoryItemData noteBookmark(int slot) const;
    NoteData noteForHistoryItem(const NoteHistoryItemData& item) const;
    void storeNoteBookmark(int slot, const NoteHistoryItemData& item);
    void removeNoteBookmark(int slot);
    bool renameNote(int noteId, const QString& name);
    bool moveNotesToSubFolder(const QVector<int>& noteIds, int subFolderId);
    bool copyNotesToSubFolder(const QVector<int>& noteIds, int subFolderId);
    bool toggleFavoriteNote(int noteId);
    bool tagNotes(const QVector<int>& noteIds, int tagId);
    bool untagNotes(const QVector<int>& noteIds, int tagId);
    bool renameTag(int tagId, const QString& name);
    bool moveTags(const QVector<int>& tagIds, int parentId);
    bool deleteTags(const QVector<int>& tagIds);
    bool setTagColor(const QVector<int>& tagIds, const QColor& color);
    bool renameSubFolder(int subFolderId, const QString& name);
    bool moveSubFolders(const QVector<int>& subFolderIds, int parentId);
    bool deleteSubFolders(const QVector<int>& subFolderIds);

public slots:
    void refreshModels();

signals:
    void initialized();

private:
    void registerCommandHandlers();
    void connectFlows();

    std::unique_ptr<NoteRepository> _noteRepository;
    std::unique_ptr<TagRepository> _tagRepository;
    std::unique_ptr<NoteFolderRepository> _noteFolderRepository;
    std::unique_ptr<NoteSubFolderRepository> _noteSubFolderRepository;
    std::unique_ptr<TrashRepository> _trashRepository;
    std::unique_ptr<ColorModeRepository> _colorModeRepository;
    std::unique_ptr<NoteHistoryRepository> _noteHistoryRepository;
    std::unique_ptr<SettingsRepository> _settingsRepository;

    std::unique_ptr<NoteFileService> _noteFileService;
    std::unique_ptr<NoteService> _noteService;
    std::unique_ptr<TagService> _tagService;
    std::unique_ptr<NoteFolderService> _noteFolderService;
    std::unique_ptr<NoteSubFolderService> _noteSubFolderService;
    std::unique_ptr<SearchService> _searchService;
    std::unique_ptr<NoteLinkService> _noteLinkService;
    std::unique_ptr<MediaService> _mediaService;
    std::unique_ptr<ExportService> _exportService;
    std::unique_ptr<TrashService> _trashService;
    std::unique_ptr<MarkdownRenderService> _markdownRenderService;

    std::unique_ptr<CommandBus> _commandBus;
    std::unique_ptr<AppState> _appState;
    std::unique_ptr<EditorState> _editorState;
    std::unique_ptr<SearchState> _searchState;
    std::unique_ptr<UiState> _uiState;
    std::unique_ptr<ActionRegistry> _actionRegistry;
    std::unique_ptr<DialogCoordinator> _dialogCoordinator;

    std::unique_ptr<NoteListViewModel> _noteListViewModel;
    std::unique_ptr<NoteEditorViewModel> _noteEditorViewModel;
    std::unique_ptr<TagTreeViewModel> _tagTreeViewModel;
    std::unique_ptr<NavigationViewModel> _navigationViewModel;
    std::unique_ptr<SearchViewModel> _searchViewModel;
    std::unique_ptr<NoteFolderViewModel> _noteFolderViewModel;
    std::unique_ptr<NoteFolderSettingsViewModel> _noteFolderSettingsViewModel;
    std::unique_ptr<NoteSubFolderViewModel> _noteSubFolderViewModel;
    std::unique_ptr<SettingsViewModel> _settingsViewModel;
    std::unique_ptr<TrashViewModel> _trashViewModel;

    bool _handlersRegistered = false;
    bool _flowsConnected = false;
};
