#pragma once

#include <core/data/notedata.h>
#include <core/data/tagdata.h>

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

class TagRepository {
public:
    [[nodiscard]] TagData findById(int id) const;
    [[nodiscard]] TagData findByName(const QString& name, bool startsWith = false) const;
    [[nodiscard]] QVector<TagData> findAll() const;
    [[nodiscard]] QVector<TagData> findByParentId(int parentId) const;
    [[nodiscard]] QVector<TagData> findByNote(const NoteData& note) const;
    [[nodiscard]] bool linkToNote(int tagId, const NoteData& note) const;
    [[nodiscard]] bool unlinkFromNote(int tagId, const NoteData& note) const;
    [[nodiscard]] bool renameNoteFileNamesOfLinks(const QString& oldName, const QString& newName,
                                                  const QString& noteSubFolderPath) const;
    [[nodiscard]] bool renameNoteSubFolderPathOfLinks(const QString& noteName, const QString& oldPath,
                                                      const QString& newPath) const;
    [[nodiscard]] bool renameNoteSubFolderPathsOfLinks(const QString& oldPath, const QString& newPath) const;
    [[nodiscard]] bool save(const TagData& tag) const;
    [[nodiscard]] bool remove(int id) const;
    [[nodiscard]] bool hasDescendant(int tagId, int descendantId) const;
    [[nodiscard]] int countLinkedNotes(int tagId, bool fromAllSubfolders, bool recursive = true) const;
    [[nodiscard]] QVector<TagData> searchByName(const QString& name) const;
    [[nodiscard]] QHash<QString, QVector<int>> allIdsByNoteFilePath() const;
    [[nodiscard]] QHash<QString, QStringList> allNamesByNoteFilePath() const;
};
