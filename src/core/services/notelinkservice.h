#pragma once

#include <core/data/notedata.h>

#include <QHash>
#include <QSet>
#include <QString>
#include <QVector>

class NoteLinkService {
public:
    [[nodiscard]] QVector<int> findBacklinks(const NoteData& note) const;
    [[nodiscard]] QVector<int> findLinkedNotes(const NoteData& note, const QVector<NoteData>& candidates) const;
    [[nodiscard]] NoteData resolveWikiLink(const QString& target, int currentNoteSubFolderId = -1) const;
    [[nodiscard]] QString getNoteUrlForLinkingTo(const NoteData& source, const NoteData& target,
                                                 bool forceLegacy = false) const;
    [[nodiscard]] QString relativeFilePath(const NoteData& note, const QString& path) const;
    [[nodiscard]] QString fileUrlFromFileName(const NoteData& note, const QString& fileName,
                                              bool withFragment = false) const;
    bool updateRelativeMediaFileLinks(NoteData& note) const;
    bool updateRelativeAttachmentFileLinks(NoteData& note) const;
};
