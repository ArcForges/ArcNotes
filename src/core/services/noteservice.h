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

    NoteData createNote(const QString& name, int folderId = 0, int subFolderId = 0) const;
    bool saveNoteText(int noteId, const QString& text) const;
    bool deleteNote(int noteId, bool withFile = false) const;
    NoteData duplicateNote(int noteId, const QString& newName = QString(), int targetSubFolderId = -1) const;
    bool moveNote(NoteData& note, int targetSubFolderId) const;
    bool toggleFavorite(int noteId) const;
    NoteData getNote(int noteId) const;
    NoteData getNoteByFileUrl(const QUrl& url) const;
    NoteData getNoteByUrlString(const QString& urlString) const;
    QVector<NoteData> allNotes(int limit = -1) const;
    QStringList searchNoteNames(const QString& text, bool ignoreNoteSubFolder = false) const;
    bool isWikiLinkSupportEnabled() const;
    bool fileUrlIsNoteInCurrentFolder(const QUrl& url) const;
    bool fileUrlIsExistingNoteInCurrentFolder(const QUrl& url) const;
    QString fragmentFromFileName(const QString& fileName) const;
    QString relativePathForFileUrlInCurrentFolder(const QUrl& url) const;
    bool buildNotesIndex(bool forceRebuild = false) const;

private:
    bool buildNotesIndexForFolder(const QString& rootPath, int noteSubFolderId, QVector<int>& seenNoteIds,
                                  QVector<int>& seenSubFolderIds) const;

    NoteRepository* _noteRepository;
    NoteRepository _ownedNoteRepository;
};
