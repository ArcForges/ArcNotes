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

    TagData createTag(const QString& name, int parentId = 0) const;
    bool deleteTag(int tagId) const;
    bool renameTag(int tagId, const QString& name) const;
    bool tagNote(const NoteData& note, int tagId) const;
    bool untagNote(const NoteData& note, int tagId) const;
    QVector<TagData> getNoteTags(const NoteData& note) const;
    QVector<TagData> fetchTagTree(int parentId = 0) const;
    int countLinkedNotes(int tagId, bool fromAllSubfolders, bool recursive = true) const;
    QHash<QString, QVector<int>> allIdsByNoteFilePath() const;
    QHash<QString, QStringList> allNamesByNoteFilePath() const;
    bool moveTag(int tagId, int parentId) const;
    bool setTagColor(int tagId, const QColor& color) const;

private:
    TagRepository* _tagRepository;
    TagRepository _ownedTagRepository;
};
