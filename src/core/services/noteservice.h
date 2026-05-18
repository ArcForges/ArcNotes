#pragma once

#include <core/data/notedata.h>
#include <core/repositories/noterepository.h>

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

class NoteService {
public:
    explicit NoteService(NoteRepository* noteRepository = nullptr);

    [[nodiscard]] NoteData createNote(const QString& name, int folderId = 0, int subFolderId = 0) const;
    [[nodiscard]] bool saveNoteText(int noteId, const QString& text) const;
    [[nodiscard]] bool deleteNote(int noteId, bool withFile = false) const;
    [[nodiscard]] NoteData duplicateNote(int noteId, const QString& newName = QString(),
                                         int targetSubFolderId = -1) const;
    bool moveNote(NoteData& note, int targetSubFolderId) const;
    [[nodiscard]] bool toggleFavorite(int noteId) const;
    [[nodiscard]] NoteData getNote(int noteId) const;
    [[nodiscard]] NoteData getNoteByFileUrl(const QUrl& url) const;
    [[nodiscard]] NoteData getNoteByUrlString(const QString& urlString) const;
    [[nodiscard]] QVector<NoteData> allNotes(int limit = -1) const;
    [[nodiscard]] QStringList searchNoteNames(const QString& text, bool ignoreNoteSubFolder = false) const;
    [[nodiscard]] bool isWikiLinkSupportEnabled() const;
    [[nodiscard]] bool fileUrlIsNoteInCurrentFolder(const QUrl& url) const;
    [[nodiscard]] bool fileUrlIsExistingNoteInCurrentFolder(const QUrl& url) const;
    [[nodiscard]] QString fragmentFromFileName(const QString& fileName) const;
    [[nodiscard]] QString relativePathForFileUrlInCurrentFolder(const QUrl& url) const;
    [[nodiscard]] bool buildNotesIndex(bool forceRebuild = false) const;

private:
    bool buildNotesIndexForFolder(const QString& rootPath, int noteSubFolderId, QVector<int>& seenNoteIds,
                                  QVector<int>& seenSubFolderIds) const;

    NoteRepository* _noteRepository;
    NoteRepository _ownedNoteRepository;
};
