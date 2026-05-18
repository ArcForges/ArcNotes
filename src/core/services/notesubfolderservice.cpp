#include "notesubfolderservice.h"

#include <core/repositories/notefolderrepository.h>
#include <core/repositories/tagrepository.h>
#include <utils/misc.h>

#include <QDir>
#include <QFileInfo>

namespace {
QString relativePathForSubFolder(const NoteSubFolderRepository& repository, const NoteSubFolderData& subFolder) {
    if (subFolder.id == 0) {
        return QString();
    }

    if (subFolder.parentId == 0) {
        return subFolder.name;
    }

    const QString parentPath = relativePathForSubFolder(repository, repository.findById(subFolder.parentId));
    return parentPath.isEmpty() ? subFolder.name : parentPath + QDir::separator() + subFolder.name;
}

QString fullPathForSubFolder(const NoteFolderData& folder, const QString& relativePath) {
    if (folder.localPath.isEmpty() || relativePath.isEmpty()) {
        return QString();
    }

    QString path = Utils::Misc::removeIfEndsWith(folder.localPath, QStringLiteral("/")) + Utils::Misc::dirSeparator() +
                   relativePath;
    const QFileInfo fileInfo(path);

    if (!fileInfo.exists()) {
        return path;
    }

#ifdef Q_OS_WIN32
    return fileInfo.canonicalFilePath();
#else
    return fileInfo.absoluteFilePath();
#endif
}

QVector<int> descendantIds(const NoteSubFolderRepository& repository, int parentId) {
    QVector<int> ids;
    for (const NoteSubFolderData& child : repository.findByParentId(parentId)) {
        ids.append(child.id);
        ids += descendantIds(repository, child.id);
    }
    return ids;
}
}  // namespace

NoteSubFolderService::NoteSubFolderService(NoteSubFolderRepository* subFolderRepository)
    : _subFolderRepository(subFolderRepository != nullptr ? subFolderRepository : &_ownedSubFolderRepository) {}

NoteSubFolderData NoteSubFolderService::subFolder(int id) const {
    return _subFolderRepository->findById(id);
}

QVector<NoteSubFolderData> NoteSubFolderService::subFolders(int parentId) const {
    return parentId <= 0 ? _subFolderRepository->findAll() : _subFolderRepository->findByParentId(parentId);
}

NoteSubFolderData NoteSubFolderService::subFolderByNameAndParentId(const QString& name, int parentId) const {
    return _subFolderRepository->findByNameAndParentId(name, parentId);
}

NoteSubFolderData NoteSubFolderService::subFolderByPathData(const QString& pathData, const QString& separator) const {
    return _subFolderRepository->findByPathData(pathData, separator);
}

NoteSubFolderData NoteSubFolderService::createSubFolder(const QString& name, int parentId) const {
    const QString folderName = name.trimmed();
    if (folderName.isEmpty() || _subFolderRepository->willFolderBeIgnored(folderName, false)) {
        return NoteSubFolderData();
    }

    const NoteFolderData currentFolder = NoteFolderRepository().current();
    const QString basePath =
        parentId > 0
            ? fullPathForSubFolder(currentFolder, relativePathForSubFolder(*_subFolderRepository,
                                                                           _subFolderRepository->findById(parentId)))
            : currentFolder.localPath;
    if (basePath.isEmpty()) {
        return NoteSubFolderData();
    }

    if (!QDir().mkpath(basePath + QDir::separator() + folderName)) {
        return NoteSubFolderData();
    }

    NoteSubFolderData data;
    data.name = folderName;
    data.parentId = parentId;
    if (!_subFolderRepository->save(data)) {
        return NoteSubFolderData();
    }
    return _subFolderRepository->findByNameAndParentId(folderName, parentId);
}

bool NoteSubFolderService::deleteSubFolder(int subFolderId) const {
    const NoteSubFolderData subFolder = _subFolderRepository->findById(subFolderId);
    if (subFolder.id <= 0) {
        return false;
    }

    const QString path = fullPath(subFolderId);
    if (!path.isEmpty() && QFileInfo::exists(path) && !QDir(path).removeRecursively()) {
        return false;
    }

    QVector<int> ids = descendantIds(*_subFolderRepository, subFolderId);
    ids.prepend(subFolderId);
    bool ok = true;
    for (const int id : ids) {
        ok = _subFolderRepository->remove(id) && ok;
    }
    return ok;
}

bool NoteSubFolderService::renameSubFolder(int subFolderId, const QString& name) const {
    NoteSubFolderData data = _subFolderRepository->findById(subFolderId);
    if (data.id <= 0) {
        return false;
    }
    const QString cleanName = name.trimmed();
    if (cleanName.isEmpty() || _subFolderRepository->willFolderBeIgnored(cleanName, false)) {
        return false;
    }

    const QString oldRelativePath = relativePath(subFolderId);
    const QString oldPath = fullPath(subFolderId);
    data.name = cleanName;
    const QString newRelativePath = relativePathForSubFolder(*_subFolderRepository, data);
    const NoteFolderData currentFolder = NoteFolderRepository().current();
    const QString newPath = fullPathForSubFolder(currentFolder, newRelativePath);
    if (oldPath != newPath && (QFileInfo::exists(newPath) || !QDir().rename(oldPath, newPath))) {
        return false;
    }

    if (!_subFolderRepository->save(data)) {
        return false;
    }

    (void)TagRepository().renameNoteSubFolderPathsOfLinks(oldRelativePath, newRelativePath);
    return true;
}

bool NoteSubFolderService::moveSubFolder(int subFolderId, int destinationParentId) const {
    NoteSubFolderData data = _subFolderRepository->findById(subFolderId);
    if (data.id <= 0 || data.parentId == destinationParentId || data.id == destinationParentId ||
        descendantIds(*_subFolderRepository, data.id).contains(destinationParentId)) {
        return false;
    }

    if (destinationParentId > 0 && _subFolderRepository->findById(destinationParentId).id <= 0) {
        return false;
    }

    if (_subFolderRepository->findByNameAndParentId(data.name, destinationParentId).id > 0) {
        return false;
    }

    const QString oldRelativePath = relativePath(subFolderId);
    const QString oldPath = fullPath(subFolderId);
    data.parentId = destinationParentId;
    const QString newRelativePath = relativePathForSubFolder(*_subFolderRepository, data);
    const NoteFolderData currentFolder = NoteFolderRepository().current();
    const QString destinationPath =
        destinationParentId == 0
            ? currentFolder.localPath
            : fullPathForSubFolder(
                  currentFolder,
                  relativePathForSubFolder(*_subFolderRepository, _subFolderRepository->findById(destinationParentId)));
    const QString newPath = fullPathForSubFolder(currentFolder, newRelativePath);

    if (destinationPath.isEmpty() || !QDir().mkpath(destinationPath) || !QDir().rename(oldPath, newPath)) {
        return false;
    }

    if (!_subFolderRepository->save(data)) {
        return false;
    }

    (void)TagRepository().renameNoteSubFolderPathsOfLinks(oldRelativePath, newRelativePath);
    return true;
}

QString NoteSubFolderService::relativePath(int subFolderId) const {
    return subFolderId <= 0
               ? QString()
               : relativePathForSubFolder(*_subFolderRepository, _subFolderRepository->findById(subFolderId));
}

QString NoteSubFolderService::fullPath(int subFolderId) const {
    if (subFolderId <= 0) {
        return NoteFolderRepository().current().localPath;
    }

    return fullPathForSubFolder(NoteFolderRepository().current(), relativePath(subFolderId));
}

bool NoteSubFolderService::setActiveSubFolder(int subFolderId) const {
    return _subFolderRepository->setActive(subFolderId);
}
