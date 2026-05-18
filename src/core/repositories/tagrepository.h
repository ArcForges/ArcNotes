#pragma once

#include <core/data/notedata.h>
#include <core/data/tagdata.h>

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

class TagRepository {
public:
    TagData findById(int id) const;
    TagData findByName(const QString& name, bool startsWith = false) const;
    QVector<TagData> findAll() const;
    QVector<TagData> findByParentId(int parentId) const;
    QVector<TagData> findByNote(const NoteData& note) const;
    bool linkToNote(int tagId, const NoteData& note) const;
    bool unlinkFromNote(int tagId, const NoteData& note) const;
    bool renameNoteFileNamesOfLinks(const QString& oldName, const QString& newName,
                                    const QString& noteSubFolderPath) const;
    bool renameNoteSubFolderPathOfLinks(const QString& noteName, const QString& oldPath, const QString& newPath) const;
    bool renameNoteSubFolderPathsOfLinks(const QString& oldPath, const QString& newPath) const;
    bool save(const TagData& tag) const;
    bool remove(int id) const;
    bool hasDescendant(int tagId, int descendantId) const;
    int countLinkedNotes(int tagId, bool fromAllSubfolders, bool recursive = true) const;
    QVector<TagData> searchByName(const QString& name) const;
    QHash<QString, QVector<int>> allIdsByNoteFilePath() const;
    QHash<QString, QStringList> allNamesByNoteFilePath() const;
};
