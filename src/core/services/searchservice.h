#pragma once

#include <core/data/notedata.h>
#include <core/repositories/noterepository.h>

#include <QStringList>
#include <QVector>

class SearchService {
public:
    explicit SearchService(NoteRepository* noteRepository = nullptr);

    [[nodiscard]] QVector<NoteData> searchNotes(const QString& text) const;
    [[nodiscard]] QVector<int> searchInNotes(const QString& query, bool ignoreNoteSubFolder = false,
                                             int noteSubFolderId = -1) const;
    [[nodiscard]] QStringList buildQueryStringList(QString searchString, bool escapeForRegularExpression = false,
                                                   bool removeSearchPrefix = false) const;
    [[nodiscard]] bool isNameSearch(const QString& searchTerm) const;

private:
    NoteRepository* _noteRepository;
    NoteRepository _ownedNoteRepository;
};
