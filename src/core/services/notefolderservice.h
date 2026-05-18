#pragma once

#include <core/data/notefolderdata.h>
#include <core/repositories/notefolderrepository.h>

#include <QList>
#include <QVector>

class NoteFolderService {
public:
    explicit NoteFolderService(NoteFolderRepository* folderRepository = nullptr);

    NoteFolderData folder(int folderId) const;
    QList<NoteFolderData> folders() const;
    NoteFolderData currentFolder() const;
    bool saveFolder(const NoteFolderData& folder) const;
    NoteFolderData saveFolderAndReturn(const NoteFolderData& folder) const;
    bool deleteFolder(int folderId) const;
    int currentFolderId() const;
    void setCurrentFolderId(int folderId) const;
    QVariant settingValue(int folderId, const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setSettingValue(int folderId, const QString& key, const QVariant& value) const;
    bool updatePriorities(const QVector<int>& folderIds) const;

private:
    NoteFolderRepository* _folderRepository;
    NoteFolderRepository _ownedFolderRepository;
};
