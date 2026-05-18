#include "appcoordinator.h"

#include <core/commands/commandbus.h>
#include <core/commands/exportcommandhandlers.h>
#include <core/commands/foldercommandhandlers.h>
#include <core/commands/mediacommandhandlers.h>
#include <core/commands/notecommandhandlers.h>
#include <core/commands/searchcommandhandlers.h>
#include <core/commands/settingscommandhandlers.h>
#include <core/commands/tagcommandhandlers.h>
#include <core/commands/trashcommandhandlers.h>
#include <core/data/commands.h>
#include <core/repositories/colormoderepository.h>
#include <core/repositories/notefolderrepository.h>
#include <core/repositories/notehistoryrepository.h>
#include <core/repositories/noterepository.h>
#include <core/repositories/notesubfolderrepository.h>
#include <core/repositories/settingsrepository.h>
#include <core/repositories/tagrepository.h>
#include <core/repositories/trashrepository.h>
#include <core/services/exportservice.h>
#include <core/services/markdownrenderservice.h>
#include <core/services/mediaservice.h>
#include <core/services/notefileservice.h>
#include <core/services/notefolderservice.h>
#include <core/services/notelinkservice.h>
#include <core/services/noteservice.h>
#include <core/services/notesubfolderservice.h>
#include <core/services/searchservice.h>
#include <core/services/tagservice.h>
#include <core/services/trashservice.h>
#include <core/state/appstate.h>
#include <core/state/editorstate.h>
#include <core/state/searchstate.h>
#include <core/state/uistate.h>
#include <viewmodels/navigationviewmodel.h>
#include <viewmodels/noteeditorviewmodel.h>
#include <viewmodels/notefoldersettingsviewmodel.h>
#include <viewmodels/notefolderviewmodel.h>
#include <viewmodels/notelistviewmodel.h>
#include <viewmodels/notesubfolderviewmodel.h>
#include <viewmodels/searchviewmodel.h>
#include <viewmodels/settingsviewmodel.h>
#include <viewmodels/tagtreeviewmodel.h>
#include <viewmodels/trashviewmodel.h>
#include <viewmodels/viewmodellocator.h>

#include <QColor>
#include <QSet>
#include <algorithm>

#include "actionregistry.h"
#include "dialogcoordinator.h"

namespace {
constexpr int AllNotesTagId = -1;
constexpr int AllUntaggedNotesTagId = -2;

QString noteTagKey(const NoteData& note) {
    return note.relativeNoteSubFolderPath + QStringLiteral("/") + note.name;
}

QString noteBookmarkKey(int slot) {
    return QStringLiteral("NoteBookmark%1").arg(slot);
}
}  // namespace

AppCoordinator::AppCoordinator(QObject* parent) : QObject(parent) {
    _noteRepository = std::make_unique<NoteRepository>();
    _tagRepository = std::make_unique<TagRepository>();
    _noteFolderRepository = std::make_unique<NoteFolderRepository>();
    _noteSubFolderRepository = std::make_unique<NoteSubFolderRepository>();
    _trashRepository = std::make_unique<TrashRepository>();
    _colorModeRepository = std::make_unique<ColorModeRepository>();
    _noteHistoryRepository = std::make_unique<NoteHistoryRepository>();
    _settingsRepository = std::make_unique<SettingsRepository>();

    _noteFileService = std::make_unique<NoteFileService>();
    _noteService = std::make_unique<NoteService>(_noteRepository.get());
    _tagService = std::make_unique<TagService>(_tagRepository.get());
    _noteFolderService = std::make_unique<NoteFolderService>(_noteFolderRepository.get());
    _noteSubFolderService = std::make_unique<NoteSubFolderService>(_noteSubFolderRepository.get());
    _searchService = std::make_unique<SearchService>(_noteRepository.get());
    _noteLinkService = std::make_unique<NoteLinkService>();
    _mediaService = std::make_unique<MediaService>();
    _exportService = std::make_unique<ExportService>();
    _trashService = std::make_unique<TrashService>(_trashRepository.get());
    _markdownRenderService = std::make_unique<MarkdownRenderService>();

    _commandBus = std::make_unique<CommandBus>();
    _appState = std::make_unique<AppState>();
    _editorState = std::make_unique<EditorState>();
    _searchState = std::make_unique<SearchState>();
    _uiState = std::make_unique<UiState>();
    _actionRegistry = std::make_unique<ActionRegistry>();
    _dialogCoordinator = std::make_unique<DialogCoordinator>();

    _noteListViewModel = std::make_unique<NoteListViewModel>(_commandBus.get(), _appState.get());
    _noteEditorViewModel =
        std::make_unique<NoteEditorViewModel>(_commandBus.get(), _appState.get(), _editorState.get());
    _tagTreeViewModel = std::make_unique<TagTreeViewModel>(_commandBus.get(), _appState.get());
    _navigationViewModel = std::make_unique<NavigationViewModel>();
    _searchViewModel = std::make_unique<SearchViewModel>(_commandBus.get(), _searchState.get());
    _noteFolderViewModel = std::make_unique<NoteFolderViewModel>(_commandBus.get(), _appState.get());
    _noteFolderSettingsViewModel = std::make_unique<NoteFolderSettingsViewModel>(
        _commandBus.get(), _noteFolderRepository.get(), _noteSubFolderRepository.get());
    ViewModelLocator::setNoteFolderSettingsViewModel(_noteFolderSettingsViewModel.get());
    _noteSubFolderViewModel = std::make_unique<NoteSubFolderViewModel>(_commandBus.get(), _appState.get());
    _settingsViewModel =
        std::make_unique<SettingsViewModel>(_commandBus.get(), _settingsRepository.get(), _colorModeRepository.get());
    ViewModelLocator::setSettingsViewModel(_settingsViewModel.get());
    _trashViewModel = std::make_unique<TrashViewModel>(_commandBus.get());
}

AppCoordinator::~AppCoordinator() = default;

void AppCoordinator::initialize() {
    _actionRegistry->registerDefaultActions();
    registerCommandHandlers();
    connectFlows();
    (void)_noteService->buildNotesIndex(false);
    refreshModels();
    emit initialized();
}

CommandBus* AppCoordinator::commandBus() const {
    return _commandBus.get();
}

ActionRegistry* AppCoordinator::actionRegistry() const {
    return _actionRegistry.get();
}

DialogCoordinator* AppCoordinator::dialogCoordinator() const {
    return _dialogCoordinator.get();
}

AppState* AppCoordinator::appState() const {
    return _appState.get();
}

EditorState* AppCoordinator::editorState() const {
    return _editorState.get();
}

SearchState* AppCoordinator::searchState() const {
    return _searchState.get();
}

UiState* AppCoordinator::uiState() const {
    return _uiState.get();
}

NoteListViewModel* AppCoordinator::noteListViewModel() const {
    return _noteListViewModel.get();
}

NoteEditorViewModel* AppCoordinator::noteEditorViewModel() const {
    return _noteEditorViewModel.get();
}

TagTreeViewModel* AppCoordinator::tagTreeViewModel() const {
    return _tagTreeViewModel.get();
}

NavigationViewModel* AppCoordinator::navigationViewModel() const {
    return _navigationViewModel.get();
}

SearchViewModel* AppCoordinator::searchViewModel() const {
    return _searchViewModel.get();
}

NoteFolderViewModel* AppCoordinator::noteFolderViewModel() const {
    return _noteFolderViewModel.get();
}

NoteFolderSettingsViewModel* AppCoordinator::noteFolderSettingsViewModel() const {
    return _noteFolderSettingsViewModel.get();
}

NoteSubFolderViewModel* AppCoordinator::noteSubFolderViewModel() const {
    return _noteSubFolderViewModel.get();
}

SettingsViewModel* AppCoordinator::settingsViewModel() const {
    return _settingsViewModel.get();
}

TrashViewModel* AppCoordinator::trashViewModel() const {
    return _trashViewModel.get();
}

QVector<NoteData> AppCoordinator::allNotes() const {
    return _noteService == nullptr ? QVector<NoteData>() : _noteService->allNotes();
}

NoteData AppCoordinator::note(int noteId) const {
    return _noteService == nullptr ? NoteData() : _noteService->getNote(noteId);
}

QList<NoteFolderData> AppCoordinator::noteFolders() const {
    return _noteFolderService == nullptr ? QList<NoteFolderData>() : _noteFolderService->folders();
}

QVector<TagData> AppCoordinator::tags(int parentId) const {
    if (_tagRepository == nullptr) {
        return {};
    }

    if (parentId < 0) {
        return _tagRepository->findAll();
    }
    return _tagRepository->findByParentId(parentId);
}

QVector<TagData> AppCoordinator::tagsForNotes(const QVector<int>& noteIds) const {
    QVector<TagData> tags;
    QSet<int> seenIds;
    if (_tagService == nullptr || _noteService == nullptr) {
        return tags;
    }

    for (const int noteId : noteIds) {
        for (const TagData& tag : _tagService->getNoteTags(_noteService->getNote(noteId))) {
            if (!seenIds.contains(tag.id)) {
                tags.append(tag);
                seenIds.insert(tag.id);
            }
        }
    }
    return tags;
}

QVector<NoteSubFolderData> AppCoordinator::noteSubFolders(int parentId) const {
    if (_noteSubFolderRepository == nullptr) {
        return {};
    }

    return parentId < 0 ? _noteSubFolderRepository->findAll()
                        : _noteSubFolderRepository->findByParentId(parentId, QStringLiteral("name ASC"));
}

NoteSubFolderData AppCoordinator::noteSubFolder(int subFolderId) const {
    return _noteSubFolderService == nullptr ? NoteSubFolderData() : _noteSubFolderService->subFolder(subFolderId);
}

QString AppCoordinator::noteSubFolderFullPath(int subFolderId) const {
    return _noteSubFolderService == nullptr ? QString() : _noteSubFolderService->fullPath(subFolderId);
}

QVariant AppCoordinator::currentNoteFolderSettingValue(const QString& key, const QVariant& defaultValue) const {
    const int folderId = _appState == nullptr ? 0 : _appState->currentNoteFolder().id;
    return (_noteFolderRepository == nullptr || folderId <= 0)
               ? defaultValue
               : _noteFolderRepository->settingValue(folderId, key, defaultValue);
}

NoteData AppCoordinator::noteByFileUrl(const QUrl& url) const {
    return _noteService == nullptr ? NoteData() : _noteService->getNoteByFileUrl(url);
}

NoteData AppCoordinator::noteByUrlString(const QString& urlString) const {
    return _noteService == nullptr ? NoteData() : _noteService->getNoteByUrlString(urlString);
}

NoteData AppCoordinator::resolveWikiLink(const QString& target, int currentNoteSubFolderId) const {
    return _noteLinkService == nullptr ? NoteData() : _noteLinkService->resolveWikiLink(target, currentNoteSubFolderId);
}

QStringList AppCoordinator::searchNoteNames(const QString& query) const {
    return _noteService == nullptr ? QStringList() : _noteService->searchNoteNames(query, true);
}

QHash<QString, QStringList> AppCoordinator::tagNamesByNoteFilePath() const {
    return _tagService == nullptr ? QHash<QString, QStringList>() : _tagService->allNamesByNoteFilePath();
}

bool AppCoordinator::wikiLinkSupportEnabled() const {
    return _noteService != nullptr && _noteService->isWikiLinkSupportEnabled();
}

bool AppCoordinator::noteFileUrlIsInCurrentFolder(const QUrl& url) const {
    return _noteService != nullptr && _noteService->fileUrlIsNoteInCurrentFolder(url);
}

bool AppCoordinator::noteFileUrlIsExistingInCurrentFolder(const QUrl& url) const {
    return _noteService != nullptr && _noteService->fileUrlIsExistingNoteInCurrentFolder(url);
}

QString AppCoordinator::noteUrlFragmentFromFileName(const QString& fileName) const {
    return _noteService == nullptr ? QString() : _noteService->fragmentFromFileName(fileName);
}

QString AppCoordinator::noteRelativePathForFileUrlInCurrentFolder(const QUrl& url) const {
    return _noteService == nullptr ? QString() : _noteService->relativePathForFileUrlInCurrentFolder(url);
}

QString AppCoordinator::noteUrlForLinkingTo(const NoteData& source, const NoteData& target, bool forceLegacy) const {
    return _noteLinkService == nullptr ? QString()
                                       : _noteLinkService->getNoteUrlForLinkingTo(source, target, forceLegacy);
}

QString AppCoordinator::noteRelativeFilePath(const NoteData& note, const QString& path) const {
    return _noteLinkService == nullptr ? QString() : _noteLinkService->relativeFilePath(note, path);
}

QString AppCoordinator::noteFileUrlFromFileName(const NoteData& note, const QString& fileName,
                                                bool withFragment) const {
    return _noteLinkService == nullptr ? QString()
                                       : _noteLinkService->fileUrlFromFileName(note, fileName, withFragment);
}

QStringList AppCoordinator::mediaFileList(const NoteData& note) const {
    return _mediaService == nullptr ? QStringList() : _mediaService->mediaFileList(note);
}

QStringList AppCoordinator::attachmentFileList(const NoteData& note) const {
    return _mediaService == nullptr ? QStringList() : _mediaService->attachmentFileList(note);
}

QString AppCoordinator::mediaUrlStringForFileName(const NoteData& note, const QString& fileName) const {
    return _mediaService == nullptr ? QString() : _mediaService->mediaUrlStringForFileName(note, fileName);
}

QString AppCoordinator::attachmentUrlStringForFileName(const NoteData& note, const QString& fileName) const {
    return _mediaService == nullptr ? QString() : _mediaService->attachmentUrlStringForFileName(note, fileName);
}

QString AppCoordinator::insertMediaMarkdown(const NoteData& note, const QString& sourcePath, const QString& title,
                                            bool returnUrlOnly) {
    if (_commandBus == nullptr || note.id <= 0 || sourcePath.isEmpty()) {
        return QString();
    }

    InsertMediaCommand command;
    command.noteId = note.id;
    command.sourcePath = sourcePath;
    command.title = title;
    command.returnUrlOnly = returnUrlOnly;
    const CommandResult result = _commandBus->dispatch(command);
    return result.success ? result.resultData.toString() : QString();
}

QString AppCoordinator::insertAttachmentMarkdown(const NoteData& note, const QString& sourcePath, const QString& title,
                                                 const QString& fileName, bool returnUrlOnly) {
    if (_commandBus == nullptr || note.id <= 0 || sourcePath.isEmpty()) {
        return QString();
    }

    InsertAttachmentCommand command;
    command.noteId = note.id;
    command.sourcePath = sourcePath;
    command.title = title;
    command.fileName = fileName;
    command.returnUrlOnly = returnUrlOnly;
    const CommandResult result = _commandBus->dispatch(command);
    return result.success ? result.resultData.toString() : QString();
}

QString AppCoordinator::renderTextToHtml(const NoteData& note, const QString& text, const QString& notesPath,
                                         int maxImageWidth, bool forExport, bool base64Images) const {
    return _markdownRenderService == nullptr ? QString()
                                             : _markdownRenderService->renderTextToHtml(
                                                   note, text, notesPath, maxImageWidth, forExport, base64Images);
}

bool AppCoordinator::renameMediaReferences(const QVector<int>& noteIds, const QString& oldFileName,
                                           const QString& newFileName) {
    if (_commandBus == nullptr) {
        return false;
    }

    RenameMediaReferencesCommand command;
    command.noteIds = noteIds;
    command.oldFileName = oldFileName;
    command.newFileName = newFileName;
    return _commandBus->dispatch(command).success;
}

bool AppCoordinator::renameAttachmentReferences(const QVector<int>& noteIds, const QString& oldFileName,
                                                const QString& newFileName) {
    if (_commandBus == nullptr) {
        return false;
    }

    RenameAttachmentReferencesCommand command;
    command.noteIds = noteIds;
    command.oldFileName = oldFileName;
    command.newFileName = newFileName;
    return _commandBus->dispatch(command).success;
}

NoteSubFolderData AppCoordinator::noteSubFolderByPath(const QString& pathData) const {
    return _noteSubFolderService == nullptr ? NoteSubFolderData()
                                            : _noteSubFolderService->subFolderByPathData(pathData);
}

NoteSubFolderData AppCoordinator::ensureNoteSubFolderPath(const QString& pathData) {
    if (_noteSubFolderService == nullptr || _commandBus == nullptr) {
        return {};
    }

    NoteSubFolderData existing = _noteSubFolderService->subFolderByPathData(pathData);
    if (existing.id > 0) {
        return existing;
    }

    int parentId = 0;
    NoteSubFolderData current;
    const QStringList names = pathData.split(QChar('/'),
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                                             QString::SkipEmptyParts);
#else
                                             Qt::SkipEmptyParts);
#endif
    for (const QString& name : names) {
        current = _noteSubFolderService->subFolderByNameAndParentId(name, parentId);
        if (current.id <= 0) {
            CreateSubFolderCommand command;
            command.name = name;
            command.parentId = parentId;
            const CommandResult result = _commandBus->dispatch(command);
            if (!result.success) {
                return {};
            }
            current = _noteSubFolderService->subFolderByNameAndParentId(name, parentId);
        }

        if (current.id <= 0) {
            return {};
        }
        parentId = current.id;
    }
    return current;
}

NoteSubFolderData AppCoordinator::ensureChildNoteSubFolder(const QString& name, int parentId) {
    if (_noteSubFolderService == nullptr || _commandBus == nullptr) {
        return {};
    }

    NoteSubFolderData subFolder = _noteSubFolderService->subFolderByNameAndParentId(name, parentId);
    if (subFolder.id > 0) {
        return subFolder;
    }

    CreateSubFolderCommand command;
    command.name = name;
    command.parentId = parentId;
    const CommandResult result = _commandBus->dispatch(command);
    return result.success ? _noteSubFolderService->subFolderByNameAndParentId(name, parentId) : NoteSubFolderData();
}

bool AppCoordinator::setActiveNoteSubFolder(int noteSubFolderId) {
    if (_commandBus == nullptr) {
        return false;
    }

    SetActiveSubFolderCommand command;
    command.subFolderId = noteSubFolderId;
    return _commandBus->dispatch(command).success;
}

QVariant AppCoordinator::settingValue(const QString& key, const QVariant& defaultValue) const {
    return _settingsRepository == nullptr ? defaultValue : _settingsRepository->value(key, defaultValue);
}

QVector<QVariantMap> AppCoordinator::settingsArrayValues(const QString& arrayName, const QStringList& keys) const {
    return _settingsRepository == nullptr ? QVector<QVariantMap>() : _settingsRepository->arrayValues(arrayName, keys);
}

QVector<TrashItemData> AppCoordinator::trashItems() const {
    QVector<TrashItemData> trashItems;
    if (_trashService == nullptr) {
        return trashItems;
    }

    for (const TrashItemData& trashItem : _trashService->trashItems()) {
        trashItems.append(trashItem);
    }
    return trashItems;
}

QString AppCoordinator::trashItemText(int trashItemId) const {
    return _trashService == nullptr ? QString() : _trashService->trashItemText(trashItemId);
}

QString AppCoordinator::trashItemRestorationPath(int trashItemId) const {
    return _trashService == nullptr ? QString() : _trashService->trashItemRestorationPath(trashItemId);
}

bool AppCoordinator::trashItemFileExists(int trashItemId) const {
    return _trashService != nullptr && _trashService->trashItemFileExists(trashItemId);
}

bool AppCoordinator::restoreTrashItems(const QVector<int>& trashItemIds) {
    if (_commandBus == nullptr) {
        return false;
    }

    RestoreTrashCommand command;
    command.trashItemIds = trashItemIds;
    return _commandBus->dispatch(command).success;
}

bool AppCoordinator::removeTrashItems(const QVector<int>& trashItemIds) {
    if (_commandBus == nullptr) {
        return false;
    }

    RemoveTrashCommand command;
    command.trashItemIds = trashItemIds;
    return _commandBus->dispatch(command).success;
}

QVector<BookmarkItemData> AppCoordinator::noteBookmarks() const {
    QVector<BookmarkItemData> bookmarks;
    for (int slot = 0; slot <= 9; ++slot) {
        const NoteHistoryItemData item = noteBookmark(slot);
        if (item.noteName.isEmpty() || noteForHistoryItem(item).id <= 0) {
            continue;
        }
        bookmarks.append(BookmarkItemData{slot, item});
    }
    std::sort(bookmarks.begin(), bookmarks.end(),
              [](const BookmarkItemData& left, const BookmarkItemData& right) { return left.slot < right.slot; });
    return bookmarks;
}

NoteHistoryItemData AppCoordinator::noteBookmark(int slot) const {
    if (_settingsRepository == nullptr || slot < 0 || slot > 9) {
        return {};
    }

    return NoteHistoryRepository::itemDataFromVariant(_settingsRepository->value(noteBookmarkKey(slot)));
}

NoteData AppCoordinator::noteForHistoryItem(const NoteHistoryItemData& item) const {
    if (item.noteName.isEmpty() || _noteService == nullptr) {
        return {};
    }

    NoteData fallback;
    const QVector<NoteData> notes = _noteService->allNotes();
    for (const NoteData& note : notes) {
        if (note.name != item.noteName) {
            continue;
        }
        if (note.relativeNoteSubFolderPath == item.noteSubFolderPathData) {
            return note;
        }
        if (fallback.id <= 0 && item.noteSubFolderPathData.isEmpty()) {
            fallback = note;
        }
    }
    return fallback;
}

void AppCoordinator::storeNoteBookmark(int slot, const NoteHistoryItemData& item) {
    if (_settingsRepository == nullptr || slot < 0 || slot > 9 || item.noteName.isEmpty()) {
        return;
    }

    _settingsRepository->setValue(noteBookmarkKey(slot), NoteHistoryRepository::variantFromItemData(item));
}

void AppCoordinator::removeNoteBookmark(int slot) {
    if (_settingsRepository == nullptr || slot < 0 || slot > 9) {
        return;
    }

    _settingsRepository->remove(noteBookmarkKey(slot));
}

bool AppCoordinator::renameNote(int noteId, const QString& name) {
    if (_commandBus == nullptr) {
        return false;
    }

    RenameNoteCommand command;
    command.noteId = noteId;
    command.newName = name;
    return _commandBus->dispatch(command).success;
}

bool AppCoordinator::moveNotesToSubFolder(const QVector<int>& noteIds, int subFolderId) {
    if (_commandBus == nullptr) {
        return false;
    }

    MoveNoteCommand command;
    command.noteIds = noteIds;
    command.targetSubFolderId = subFolderId;
    return _commandBus->dispatch(command).success;
}

bool AppCoordinator::copyNotesToSubFolder(const QVector<int>& noteIds, int subFolderId) {
    if (_commandBus == nullptr) {
        return false;
    }

    bool ok = true;
    for (const int noteId : noteIds) {
        DuplicateNoteCommand command;
        command.noteId = noteId;
        command.targetSubFolderId = subFolderId;
        ok = _commandBus->dispatch(command).success && ok;
    }
    return ok;
}

bool AppCoordinator::toggleFavoriteNote(int noteId) {
    if (_commandBus == nullptr) {
        return false;
    }

    ToggleFavoriteNoteCommand command;
    command.noteId = noteId;
    return _commandBus->dispatch(command).success;
}

bool AppCoordinator::tagNotes(const QVector<int>& noteIds, int tagId) {
    if (_commandBus == nullptr) {
        return false;
    }

    TagNoteCommand command;
    command.noteIds = noteIds;
    command.tagId = tagId;
    return _commandBus->dispatch(command).success;
}

bool AppCoordinator::untagNotes(const QVector<int>& noteIds, int tagId) {
    if (_commandBus == nullptr) {
        return false;
    }

    UntagNoteCommand command;
    command.noteIds = noteIds;
    command.tagId = tagId;
    return _commandBus->dispatch(command).success;
}

bool AppCoordinator::renameTag(int tagId, const QString& name) {
    if (_commandBus == nullptr) {
        return false;
    }

    RenameTagCommand command;
    command.tagId = tagId;
    command.newName = name;
    return _commandBus->dispatch(command).success;
}

bool AppCoordinator::moveTags(const QVector<int>& tagIds, int parentId) {
    if (_commandBus == nullptr) {
        return false;
    }

    bool ok = true;
    for (const int tagId : tagIds) {
        MoveTagCommand command;
        command.tagId = tagId;
        command.parentId = parentId;
        ok = _commandBus->dispatch(command).success && ok;
    }
    return ok;
}

bool AppCoordinator::deleteTags(const QVector<int>& tagIds) {
    if (_commandBus == nullptr) {
        return false;
    }

    bool ok = true;
    for (const int tagId : tagIds) {
        DeleteTagCommand command;
        command.tagId = tagId;
        ok = _commandBus->dispatch(command).success && ok;
    }
    return ok;
}

bool AppCoordinator::setTagColor(const QVector<int>& tagIds, const QColor& color) {
    if (_commandBus == nullptr) {
        return false;
    }

    bool ok = true;
    for (const int tagId : tagIds) {
        SetTagColorCommand command;
        command.tagId = tagId;
        command.color = color;
        ok = _commandBus->dispatch(command).success && ok;
    }
    return ok;
}

bool AppCoordinator::renameSubFolder(int subFolderId, const QString& name) {
    if (_commandBus == nullptr) {
        return false;
    }

    RenameSubFolderCommand command;
    command.subFolderId = subFolderId;
    command.name = name;
    return _commandBus->dispatch(command).success;
}

bool AppCoordinator::moveSubFolders(const QVector<int>& subFolderIds, int parentId) {
    if (_commandBus == nullptr) {
        return false;
    }

    bool ok = true;
    for (const int subFolderId : subFolderIds) {
        MoveSubFolderCommand command;
        command.subFolderId = subFolderId;
        command.destinationParentId = parentId;
        ok = _commandBus->dispatch(command).success && ok;
    }
    return ok;
}

bool AppCoordinator::deleteSubFolders(const QVector<int>& subFolderIds) {
    if (_commandBus == nullptr) {
        return false;
    }

    bool ok = true;
    for (const int subFolderId : subFolderIds) {
        DeleteSubFolderCommand command;
        command.subFolderId = subFolderId;
        ok = _commandBus->dispatch(command).success && ok;
    }
    return ok;
}

void AppCoordinator::refreshModels() {
    const QVector<NoteData> allNotes = _noteService->allNotes();
    QVector<NoteData> subFolderNotes;
    const int activeSubFolderId = _noteSubFolderViewModel->activeSubFolderId();
    const bool showNotesFromAllSubFolders = _noteSubFolderViewModel->showNotesFromAll() || activeSubFolderId <= 0;
    if (showNotesFromAllSubFolders) {
        subFolderNotes = allNotes;
    } else {
        subFolderNotes.reserve(allNotes.count());
        for (const NoteData& note : allNotes) {
            if (note.noteSubFolderId == activeSubFolderId) {
                subFolderNotes.append(note);
            }
        }
    }

    const QVector<TagData> tags = _tagService->fetchTagTree();
    const QHash<QString, QVector<int>> tagIdsByNotePath = _tagService->allIdsByNoteFilePath();

    QHash<int, int> noteCounts;
    noteCounts.insert(AllNotesTagId, subFolderNotes.count());
    int untaggedNoteCount = 0;
    for (const NoteData& note : subFolderNotes) {
        if (!tagIdsByNotePath.contains(noteTagKey(note))) {
            ++untaggedNoteCount;
        }
    }
    noteCounts.insert(AllUntaggedNotesTagId, untaggedNoteCount);
    for (const TagData& tag : tags) {
        noteCounts.insert(tag.id, _tagService->countLinkedNotes(tag.id, showNotesFromAllSubFolders));
    }

    QVector<NoteData> notes = subFolderNotes;
    const int activeTagId = _tagTreeViewModel->activeTagId();
    if (activeTagId == AllUntaggedNotesTagId) {
        notes.clear();
        notes.reserve(subFolderNotes.count());
        for (const NoteData& note : subFolderNotes) {
            if (!tagIdsByNotePath.contains(noteTagKey(note))) {
                notes.append(note);
            }
        }
    } else if (activeTagId > 0) {
        notes.clear();
        notes.reserve(subFolderNotes.count());
        for (const NoteData& note : subFolderNotes) {
            if (tagIdsByNotePath.value(noteTagKey(note)).contains(activeTagId)) {
                notes.append(note);
            }
        }
    }

    _noteListViewModel->setNotes(notes);
    _tagTreeViewModel->setTags(tags, noteCounts);

    QVector<NoteFolderData> folders;
    for (const NoteFolderData& folder : _noteFolderService->folders()) {
        folders.append(folder);
    }
    _noteFolderViewModel->setFolders(folders);
    _appState->setCurrentNoteFolder(_noteFolderService->currentFolder());

    _noteSubFolderViewModel->setSubFolders(_noteSubFolderService->subFolders());

    QVector<TrashItemData> trashItems;
    for (const TrashItemData& trashItem : _trashService->trashItems()) {
        trashItems.append(trashItem);
    }
    _trashViewModel->setTrashItems(trashItems);

    if (_appState->currentNote().id <= 0 && !notes.isEmpty()) {
        _appState->setCurrentNote(notes.first());
    }
}

void AppCoordinator::registerCommandHandlers() {
    if (_handlersRegistered) {
        return;
    }

    NoteCommandHandlers::registerHandlers(_commandBus.get(), _noteService.get(), _noteFileService.get(),
                                          _trashService.get(), _noteLinkService.get(), _appState.get(),
                                          _editorState.get());
    TagCommandHandlers::registerHandlers(_commandBus.get(), _tagService.get(), _noteService.get());
    FolderCommandHandlers::registerHandlers(_commandBus.get(), _noteFolderService.get(), _noteSubFolderService.get(),
                                            _appState.get());
    SearchCommandHandlers::registerHandlers(_commandBus.get(), _searchService.get(), _searchState.get());
    SettingsCommandHandlers::registerHandlers(_commandBus.get());
    TrashCommandHandlers::registerHandlers(_commandBus.get(), _trashService.get());
    ExportCommandHandlers::registerHandlers(_commandBus.get(), _exportService.get(), _noteService.get());
    MediaCommandHandlers::registerHandlers(_commandBus.get(), _mediaService.get(), _noteService.get());

    _handlersRegistered = true;
}

void AppCoordinator::connectFlows() {
    if (_flowsConnected) {
        return;
    }

    connect(_appState.get(), &AppState::currentNoteChanged, _navigationViewModel.get(),
            [this](const NoteData& note) { _navigationViewModel->parseDocument(note.noteText); });
    connect(_noteListViewModel.get(), &NoteListViewModel::refreshRequested, this, &AppCoordinator::refreshModels);
    connect(_noteSubFolderViewModel.get(), &NoteSubFolderViewModel::activeSubFolderChanged, this,
            &AppCoordinator::refreshModels);
    connect(_noteSubFolderViewModel.get(), &NoteSubFolderViewModel::showNotesFromAllChanged, this,
            &AppCoordinator::refreshModels);
    connect(_appState.get(), &AppState::activeTagIdChanged, this, &AppCoordinator::refreshModels);
    connect(_commandBus.get(), &CommandBus::commandCompleted, this,
            [this](const QString&, const CommandResult&) { refreshModels(); });

    _flowsConnected = true;
}
