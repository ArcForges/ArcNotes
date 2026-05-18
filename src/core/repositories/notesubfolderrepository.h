#pragma once

#include <core/data/notesubfolderdata.h>

#include <QString>
#include <QVector>

class NoteSubFolderRepository {
public:
    NoteSubFolderData findById(int id, const QString& connectionName = QStringLiteral("memory")) const;
    NoteSubFolderData findByNameAndParentId(const QString& name, int parentId,
                                            const QString& connectionName = QStringLiteral("memory")) const;
    NoteSubFolderData findByPathData(const QString& pathData, const QString& separator = QStringLiteral("/"),
                                     const QString& connectionName = QStringLiteral("memory")) const;
    QVector<NoteSubFolderData> findAll(int limit = -1) const;
    QVector<NoteSubFolderData> findByParentId(int parentId,
                                              const QString& sortBy = QStringLiteral("file_last_modified DESC"),
                                              const QString& connectionName = QStringLiteral("memory")) const;
    bool save(const NoteSubFolderData& subFolder) const;
    bool remove(int id) const;
    bool deleteAll() const;
    bool setActive(int id) const;
    int countAll() const;
    QVector<int> fetchAllIds() const;
    QString treeWidgetExpandStateSettingsKey(int noteFolderId = 0) const;
    bool willFolderBeIgnored(const QString& folderName, bool showWarning = false) const;
    QString defaultIgnoredSubfoldersPattern() const;
};
