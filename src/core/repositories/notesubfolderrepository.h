#pragma once

#include <core/data/notesubfolderdata.h>

#include <QString>
#include <QVector>

class NoteSubFolderRepository {
public:
    [[nodiscard]] NoteSubFolderData findById(int id, const QString& connectionName = QStringLiteral("memory")) const;
    [[nodiscard]] NoteSubFolderData findByNameAndParentId(
        const QString& name, int parentId, const QString& connectionName = QStringLiteral("memory")) const;
    [[nodiscard]] NoteSubFolderData findByPathData(const QString& pathData,
                                                   const QString& separator = QStringLiteral("/"),
                                                   const QString& connectionName = QStringLiteral("memory")) const;
    [[nodiscard]] QVector<NoteSubFolderData> findAll(int limit = -1) const;
    [[nodiscard]] QVector<NoteSubFolderData> findByParentId(
        int parentId, const QString& sortBy = QStringLiteral("file_last_modified DESC"),
        const QString& connectionName = QStringLiteral("memory")) const;
    [[nodiscard]] bool save(const NoteSubFolderData& subFolder) const;
    [[nodiscard]] bool remove(int id) const;
    [[nodiscard]] bool deleteAll() const;
    [[nodiscard]] bool setActive(int id) const;
    [[nodiscard]] int countAll() const;
    [[nodiscard]] QVector<int> fetchAllIds() const;
    [[nodiscard]] QString treeWidgetExpandStateSettingsKey(int noteFolderId = 0) const;
    [[nodiscard]] bool willFolderBeIgnored(const QString& folderName, bool showWarning = false) const;
    [[nodiscard]] QString defaultIgnoredSubfoldersPattern() const;
};
