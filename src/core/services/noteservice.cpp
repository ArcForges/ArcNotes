#include "noteservice.h"

#include <core/repositories/notefolderrepository.h>
#include <core/repositories/notesubfolderrepository.h>
#include <core/repositories/settingsrepository.h>
#include <core/repositories/tagrepository.h>
#include <core/repositories/trashrepository.h>
#include <core/services/notefileservice.h>
#include <services/databaseservice.h>
#include <utils/misc.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QSet>
#include <QTextStream>
#include <algorithm>

namespace {
QString relativePathForSubFolder(const NoteSubFolderRepository& repository, const NoteSubFolderData& subFolder) {
    if (subFolder.id == 0) {
        return QString();
    }
    if (subFolder.parentId == 0) {
        return subFolder.name;
    }

    const QString parentPath = relativePathForSubFolder(repository, repository.findById(subFolder.parentId));
    return parentPath.isEmpty() ? subFolder.name : parentPath + Utils::Misc::dirSeparator() + subFolder.name;
}

QString relativePathForSubFolderId(const NoteSubFolderRepository& repository, int subFolderId) {
    return subFolderId <= 0 ? QString() : relativePathForSubFolder(repository, repository.findById(subFolderId));
}

QString fullPathForSubFolderId(const NoteFolderData& folder, const NoteSubFolderRepository& repository,
                               int subFolderId) {
    QString path = Utils::Misc::removeIfEndsWith(folder.localPath, QDir::separator());
    path = Utils::Misc::removeIfEndsWith(path, QString{Utils::Misc::dirSeparator()});
    const QString relativePath = relativePathForSubFolderId(repository, subFolderId);
    return relativePath.isEmpty() ? path : path + QDir::separator() + relativePath;
}

QString noteFileNameFromName(const QString& name) {
    return name + QStringLiteral(".") + NoteRepository().defaultNoteFileExtension();
}

QString noteHeader(const QString& name) {
    QString header = name.trimmed() + QStringLiteral("\n");
    const auto len = std::min<int>(name.length(), 40);
    header.reserve(header.size() + len + 2);
    header.append(QString(QChar('=')).repeated(len));
    header.append(QStringLiteral("\n\n"));
    return header;
}

bool isSubfolderPathExcluded(const NoteFolderData& folder, const QString& relativePath) {
    if (!folder.showSubfolders || folder.allSubfolders || folder.id == 0) {
        return false;
    }

    return std::ranges::any_of(folder.excludedSubfolderPaths, [&relativePath](const QString& excludedPath) {
        return relativePath == excludedPath || relativePath.startsWith(excludedPath + QLatin1Char('/'));
    });
}

void applyIgnoredNotesSetting(QStringList& fileNames) {
    const QStringList ignoredFileRegExpList =
        SettingsRepository().value(QStringLiteral("ignoredNoteFiles")).toString().split(QLatin1Char(';'));

    if (ignoredFileRegExpList.isEmpty()) {
        return;
    }

    QStringList filteredFileNames;
    for (const QString& fileName : fileNames) {
        if (!Utils::Misc::regExpInListMatches(fileName, ignoredFileRegExpList)) {
            filteredFileNames.append(fileName);
        }
    }

    fileNames = filteredFileNames;
}

NoteData loadNoteFromFile(QFile& file, int noteSubFolderId) {
    if (!file.open(Utils::Misc::getNoteFileOpenFlags())) {
        return NoteData();
    }

    QFileInfo fileInfo(file);
    const bool allowEmptyNotes = SettingsRepository().value(QStringLiteral("allowEmptyNotes"), true).toBool();

    const qint64 fileSize = fileInfo.size();
    if (fileSize == 0) {
        if (allowEmptyNotes) {
            qWarning() << __func__ << " - Opened empty file:" << file.fileName();
        } else {
            qWarning() << __func__ << " - Skipping empty file:" << file.fileName();
            file.close();
            return NoteData();
        }
    }

    QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("UTF-8");
#endif

    const QString noteText = in.readAll();
    file.close();

    if (fileSize > 0 && noteText.isEmpty()) {
        qWarning() << __func__ << " - Read empty text from file:" << file.fileName();
        if (!allowEmptyNotes) {
            return NoteData();
        }
    }

    QString name = fileInfo.fileName();
    const int lastPoint = name.lastIndexOf(QLatin1Char('.'));
    name = name.left(lastPoint);

    NoteData note;
    note.name = name;
    note.fileName = fileInfo.fileName();
    note.noteSubFolderId = noteSubFolderId;
    note.noteText = noteText;
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    note.fileCreated = fileInfo.created();
#else
    note.fileCreated = fileInfo.birthTime();
#endif
    note.fileLastModified = fileInfo.lastModified();

    if (SettingsRepository().value(QStringLiteral("enableNoteChecksumChecks"), false).toBool()) {
        note.fileChecksum = NoteFileService().calculateChecksum(note.noteText);
    }

    return note;
}
}  // namespace

NoteService::NoteService(NoteRepository* noteRepository)
    : _noteRepository(noteRepository != nullptr ? noteRepository : &_ownedNoteRepository) {}

NoteData NoteService::createNote(const QString& name, int, int subFolderId) const {
    NoteData note;
    note.name = name;
    note.fileName = noteFileNameFromName(name);
    note.noteText = noteHeader(name);
    note.noteSubFolderId = subFolderId;

    if (!_noteRepository->save(note)) {
        return NoteData();
    }

    return _noteRepository->findByFileName(note.fileName, subFolderId);
}

bool NoteService::saveNoteText(int noteId, const QString& text) const {
    NoteData note = _noteRepository->findById(noteId);
    if (note.id == 0 || !QFileInfo(note.fullNoteFilePath).isWritable() || !Utils::Misc::isNoteEditingAllowed()) {
        return false;
    }

    if (text.isEmpty() && note.fileSize > 100) {
        qWarning() << __func__ << " - Refusing to store empty text over existing note with size" << note.fileSize
                   << " - file:" << note.fullNoteFilePath;
        return false;
    }

    note.noteText = text;
    note.hasDirtyData = true;
    return _noteRepository->save(note);
}

bool NoteService::deleteNote(int noteId, bool withFile) const {
    return _noteRepository->remove(noteId, withFile);
}

NoteData NoteService::duplicateNote(int noteId, const QString& newName, int targetSubFolderId) const {
    const NoteData sourceData = _noteRepository->findById(noteId);
    if (sourceData.id <= 0) {
        return NoteData();
    }

    const QString copyName = newName.isEmpty() ? QObject::tr("%1 copy").arg(sourceData.name) : newName;
    NoteData copy = createNote(copyName, 0, targetSubFolderId >= 0 ? targetSubFolderId : sourceData.noteSubFolderId);
    if (copy.id > 0) {
        saveNoteText(copy.id, sourceData.noteText);
        copy = _noteRepository->findById(copy.id);
    }
    return copy;
}

bool NoteService::moveNote(NoteData& note, int targetSubFolderId) const {
    if (note.id <= 0) {
        return false;
    }

    const NoteFolderData currentFolder = NoteFolderRepository().current();
    NoteSubFolderRepository subFolders;
    const QString oldPath = note.fullNoteFilePath;
    const QString oldRelativePath = note.relativeNoteSubFolderPath;
    const QString destinationPath = fullPathForSubFolderId(currentFolder, subFolders, targetSubFolderId);
    if (destinationPath.isEmpty() || !QDir().mkpath(destinationPath)) {
        return false;
    }

    const QString newPath = destinationPath + QDir::separator() + note.fileName;
    if (QFileInfo(oldPath).absoluteFilePath() != QFileInfo(newPath).absoluteFilePath()) {
        if (QFileInfo::exists(newPath) || !QFile(oldPath).rename(newPath)) {
            return false;
        }
    }

    note.noteSubFolderId = targetSubFolderId;
    const bool ok = _noteRepository->save(note);
    if (ok && note.id > 0) {
        const QString newRelativePath = relativePathForSubFolderId(subFolders, targetSubFolderId);
        TagRepository().renameNoteSubFolderPathOfLinks(note.name, oldRelativePath, newRelativePath);
        note = _noteRepository->findById(note.id);
    }
    return ok;
}

bool NoteService::toggleFavorite(int noteId) const {
    const NoteData note = _noteRepository->findById(noteId);
    return note.id > 0 && _noteRepository->toggleFavorite(note);
}

NoteData NoteService::getNote(int noteId) const {
    return _noteRepository->findById(noteId);
}

NoteData NoteService::getNoteByFileUrl(const QUrl& url) const {
    return _noteRepository->findByFileUrl(url);
}

NoteData NoteService::getNoteByUrlString(const QString& urlString) const {
    return _noteRepository->findByUrlString(urlString);
}

QVector<NoteData> NoteService::allNotes(int limit) const {
    return _noteRepository->findAll(limit);
}

QStringList NoteService::searchNoteNames(const QString& text, bool ignoreNoteSubFolder) const {
    return _noteRepository->searchAsNameList(text, ignoreNoteSubFolder);
}

bool NoteService::isWikiLinkSupportEnabled() const {
    return _noteRepository->isWikiLinkSupportEnabled();
}

bool NoteService::fileUrlIsNoteInCurrentFolder(const QUrl& url) const {
    return _noteRepository->fileUrlIsNoteInCurrentFolder(url);
}

bool NoteService::fileUrlIsExistingNoteInCurrentFolder(const QUrl& url) const {
    return _noteRepository->fileUrlIsExistingNoteInCurrentFolder(url);
}

QString NoteService::fragmentFromFileName(const QString& fileName) const {
    return _noteRepository->fragmentFromFileName(fileName);
}

QString NoteService::relativePathForFileUrlInCurrentFolder(const QUrl& url) const {
    return _noteRepository->relativePathForFileUrlInCurrentFolder(url);
}

bool NoteService::buildNotesIndex(bool forceRebuild) const {
    const NoteFolderData currentFolder = NoteFolderRepository().current();
    const QString rootPath = Utils::Misc::removeIfEndsWith(currentFolder.localPath, QDir::separator());
    if (rootPath.isEmpty() || !QDir(rootPath).exists()) {
        return false;
    }

    NoteSubFolderRepository subFolders;
    QVector<int> beforeNoteIds = _noteRepository->fetchAllIds();
    QVector<int> beforeSubFolderIds = subFolders.fetchAllIds();
    QVector<int> seenNoteIds;
    QVector<int> seenSubFolderIds;

    if (forceRebuild) {
        _noteRepository->deleteAll();
        subFolders.deleteAll();
        beforeNoteIds.clear();
        beforeSubFolderIds.clear();
    }

    bool wasModified = buildNotesIndexForFolder(rootPath, 0, seenNoteIds, seenSubFolderIds);

    const QSet<int> seenNoteSet(seenNoteIds.begin(), seenNoteIds.end());
    for (int noteId : beforeNoteIds) {
        if (!seenNoteSet.contains(noteId)) {
            wasModified = _noteRepository->remove(noteId) || wasModified;
        }
    }

    const QSet<int> seenSubFolderSet(seenSubFolderIds.begin(), seenSubFolderIds.end());
    for (int subFolderId : beforeSubFolderIds) {
        if (!seenSubFolderSet.contains(subFolderId)) {
            if (subFolders.findById(subFolderId).id > 0) {
                wasModified = subFolders.remove(subFolderId) || wasModified;
            }
        }
    }

    DatabaseService::createNoteFolderConnection();
    DatabaseService::setupNoteFolderTables();
    TrashRepository().expireItems();
    return wasModified;
}

bool NoteService::buildNotesIndexForFolder(const QString& rootPath, int noteSubFolderId, QVector<int>& seenNoteIds,
                                           QVector<int>& seenSubFolderIds) const {
    NoteSubFolderRepository subFolders;
    NoteSubFolderData noteSubFolderData;
    QString relativePath;
    QString notePath = rootPath;
    if (noteSubFolderId > 0) {
        noteSubFolderData = subFolders.findById(noteSubFolderId);
        if (noteSubFolderData.id == 0) {
            return false;
        }
        relativePath = relativePathForSubFolder(subFolders, noteSubFolderData);
        notePath += QDir::separator() + relativePath;
    }

    QDir notesDir(notePath);
    QStringList files =
        notesDir.entryList(_noteRepository->noteFileExtensionList(QStringLiteral("*.")), QDir::Files, QDir::Time);
    applyIgnoredNotesSetting(files);

    bool wasModified = false;
    const int maxNoteFileSize = Utils::Misc::getMaximumNoteFileSize();
    for (QString fileName : files) {
        if (noteSubFolderId > 0) {
            fileName.prepend(relativePath + QDir::separator());
        }

        QFile file(rootPath + QDir::separator() + fileName);
        if (file.size() > maxNoteFileSize) {
            continue;
        }

        const QFileInfo fileInfo(file);
        NoteData note = _noteRepository->findByFileName(fileInfo.fileName(), noteSubFolderId);
        if ((fileInfo.size() != note.fileSize) || (fileInfo.lastModified() > note.modified)) {
            NoteData fileNote = loadNoteFromFile(file, noteSubFolderId);
            if (fileNote.fileName.isEmpty()) {
                continue;
            }

            fileNote.id = note.id;
            if (!_noteRepository->save(fileNote)) {
                continue;
            }

            note = fileNote.id > 0 ? _noteRepository->findById(fileNote.id)
                                   : _noteRepository->findByFileName(fileNote.fileName, noteSubFolderId);
            wasModified = true;
        }

        if (note.id > 0) {
            seenNoteIds.append(note.id);
        }
    }

    const NoteFolderData currentFolder = NoteFolderRepository().current();
    if (!currentFolder.showSubfolders) {
        return wasModified;
    }

    const QStringList folders = notesDir.entryList(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Time);
    for (const QString& folder : folders) {
        if (subFolders.willFolderBeIgnored(folder)) {
            continue;
        }

        NoteSubFolderData child = subFolders.findByNameAndParentId(folder, noteSubFolderId);
        if (child.id == 0) {
            child.name = folder;
            child.parentId = noteSubFolderId;
            if (subFolders.save(child)) {
                wasModified = true;
            }
        }

        if (child.id == 0) {
            child = subFolders.findByNameAndParentId(folder, noteSubFolderId);
        }
        if (child.id == 0) {
            continue;
        }

        const QString childRelativePath = relativePathForSubFolder(subFolders, child);
        if (isSubfolderPathExcluded(currentFolder, childRelativePath)) {
            continue;
        }

        seenSubFolderIds.append(child.id);
        wasModified = buildNotesIndexForFolder(rootPath, child.id, seenNoteIds, seenSubFolderIds) || wasModified;
    }

    return wasModified;
}
