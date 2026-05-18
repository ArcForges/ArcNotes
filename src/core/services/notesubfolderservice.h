#pragma once

#include <core/data/notesubfolderdata.h>
#include <core/repositories/notesubfolderrepository.h>

#include <QVector>

class NoteSubFolderService {
public:
    explicit NoteSubFolderService(NoteSubFolderRepository* subFolderRepository = nullptr);

    NoteSubFolderData subFolder(int id) const;
    QVector<NoteSubFolderData> subFolders(int parentId = 0) const;
    NoteSubFolderData subFolderByNameAndParentId(const QString& name, int parentId = 0) const;
    NoteSubFolderData subFolderByPathData(const QString& pathData,
                                          const QString& separator = QStringLiteral("/")) const;
    NoteSubFolderData createSubFolder(const QString& name, int parentId = 0) const;
    bool deleteSubFolder(int subFolderId) const;
    bool renameSubFolder(int subFolderId, const QString& name) const;
    bool moveSubFolder(int subFolderId, int destinationParentId) const;
    bool setActiveSubFolder(int subFolderId) const;
    QString relativePath(int subFolderId) const;
    QString fullPath(int subFolderId) const;

private:
    NoteSubFolderRepository* _subFolderRepository;
    NoteSubFolderRepository _ownedSubFolderRepository;
};
