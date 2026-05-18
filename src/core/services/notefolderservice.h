#pragma once

#include <core/data/notefolderdata.h>
#include <core/repositories/notefolderrepository.h>

#include <QList>
#include <QVector>

class NoteFolderService {
public:
    explicit NoteFolderService(NoteFolderRepository* folderRepository = nullptr);

    [[nodiscard]] NoteFolderData folder(int folderId) const;
    [[nodiscard]] QList<NoteFolderData> folders() const;
    [[nodiscard]] NoteFolderData currentFolder() const;
    [[nodiscard]] bool saveFolder(const NoteFolderData& folder) const;
    [[nodiscard]] NoteFolderData saveFolderAndReturn(const NoteFolderData& folder) const;
    [[nodiscard]] bool deleteFolder(int folderId) const;
    [[nodiscard]] int currentFolderId() const;
    void setCurrentFolderId(int folderId) const;
    [[nodiscard]] QVariant settingValue(int folderId, const QString& key,
                                        const QVariant& defaultValue = QVariant()) const;
    void setSettingValue(int folderId, const QString& key, const QVariant& value) const;
    [[nodiscard]] bool updatePriorities(const QVector<int>& folderIds) const;

private:
    NoteFolderRepository* _folderRepository;
    NoteFolderRepository _ownedFolderRepository;
};
