#pragma once

#include <core/data/notesubfolderdata.h>
#include <core/repositories/notesubfolderrepository.h>

#include <QVector>

class NoteSubFolderService {
public:
    explicit NoteSubFolderService(NoteSubFolderRepository* subFolderRepository = nullptr);

    [[nodiscard]] NoteSubFolderData subFolder(int id) const;
    [[nodiscard]] QVector<NoteSubFolderData> subFolders(int parentId = 0) const;
    [[nodiscard]] NoteSubFolderData subFolderByNameAndParentId(const QString& name, int parentId = 0) const;
    [[nodiscard]] NoteSubFolderData subFolderByPathData(const QString& pathData,
                                                        const QString& separator = QStringLiteral("/")) const;
    [[nodiscard]] NoteSubFolderData createSubFolder(const QString& name, int parentId = 0) const;
    [[nodiscard]] bool deleteSubFolder(int subFolderId) const;
    [[nodiscard]] bool renameSubFolder(int subFolderId, const QString& name) const;
    [[nodiscard]] bool moveSubFolder(int subFolderId, int destinationParentId) const;
    [[nodiscard]] bool setActiveSubFolder(int subFolderId) const;
    [[nodiscard]] QString relativePath(int subFolderId) const;
    [[nodiscard]] QString fullPath(int subFolderId) const;

private:
    NoteSubFolderRepository* _subFolderRepository;
    NoteSubFolderRepository _ownedSubFolderRepository;
};
