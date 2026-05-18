#pragma once

#include <core/data/notedata.h>
#include <core/data/tagdata.h>
#include <core/repositories/tagrepository.h>

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

class TagService {
public:
    explicit TagService(TagRepository* tagRepository = nullptr);

    [[nodiscard]] TagData createTag(const QString& name, int parentId = 0) const;
    [[nodiscard]] bool deleteTag(int tagId) const;
    [[nodiscard]] bool renameTag(int tagId, const QString& name) const;
    [[nodiscard]] bool tagNote(const NoteData& note, int tagId) const;
    [[nodiscard]] bool untagNote(const NoteData& note, int tagId) const;
    [[nodiscard]] QVector<TagData> getNoteTags(const NoteData& note) const;
    [[nodiscard]] QVector<TagData> fetchTagTree(int parentId = 0) const;
    [[nodiscard]] int countLinkedNotes(int tagId, bool fromAllSubfolders, bool recursive = true) const;
    [[nodiscard]] QHash<QString, QVector<int>> allIdsByNoteFilePath() const;
    [[nodiscard]] QHash<QString, QStringList> allNamesByNoteFilePath() const;
    [[nodiscard]] bool moveTag(int tagId, int parentId) const;
    [[nodiscard]] bool setTagColor(int tagId, const QColor& color) const;

private:
    TagRepository* _tagRepository;
    TagRepository _ownedTagRepository;
};
