#include "notefolderservice.h"

NoteFolderService::NoteFolderService(NoteFolderRepository* folderRepository)
    : _folderRepository(folderRepository != nullptr ? folderRepository : &_ownedFolderRepository) {}

NoteFolderData NoteFolderService::folder(int folderId) const {
    return _folderRepository->findById(folderId);
}

QList<NoteFolderData> NoteFolderService::folders() const {
    return _folderRepository->findAll();
}

NoteFolderData NoteFolderService::currentFolder() const {
    return _folderRepository->current();
}

bool NoteFolderService::saveFolder(const NoteFolderData& folder) const {
    return _folderRepository->save(folder);
}

NoteFolderData NoteFolderService::saveFolderAndReturn(const NoteFolderData& folder) const {
    return _folderRepository->saveAndReturn(folder);
}

bool NoteFolderService::deleteFolder(int folderId) const {
    return _folderRepository->remove(folderId);
}

int NoteFolderService::currentFolderId() const {
    return _folderRepository->currentFolderId();
}

void NoteFolderService::setCurrentFolderId(int folderId) const {
    _folderRepository->setCurrentFolderId(folderId);
}

QVariant NoteFolderService::settingValue(int folderId, const QString& key, const QVariant& defaultValue) const {
    return _folderRepository->settingValue(folderId, key, defaultValue);
}

void NoteFolderService::setSettingValue(int folderId, const QString& key, const QVariant& value) const {
    _folderRepository->setSettingValue(folderId, key, value);
}

bool NoteFolderService::updatePriorities(const QVector<int>& folderIds) const {
    for (int index = 0; index < folderIds.size(); ++index) {
        NoteFolderData folder = _folderRepository->findById(folderIds.at(index));
        if (folder.id <= 0) {
            continue;
        }

        folder.priority = index;
        if (!_folderRepository->save(folder)) {
            return false;
        }
    }

    return true;
}
