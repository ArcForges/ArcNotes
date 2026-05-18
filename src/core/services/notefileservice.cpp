#include "notefileservice.h"

#include <core/repositories/notefolderrepository.h>
#include <core/repositories/noterepository.h>
#include <core/repositories/settingsrepository.h>
#include <core/repositories/tagrepository.h>
#include <core/repositories/trashrepository.h>
#include <utils/misc.h>

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QRegularExpression>
#include <QTextStream>
#include <utility>

namespace {
QString currentNoteFolderPath() {
    QString path = NoteFolderRepository().current().localPath;
    if (path.isEmpty()) {
        path = Utils::Misc::prependPortableDataPathIfNeeded(
            SettingsRepository().value(QStringLiteral("notesPath")).toString());
    }

    if (!path.isEmpty()) {
        const QFileInfo fileInfo(path);
#ifdef Q_OS_WIN32
        path = fileInfo.canonicalFilePath();
#else
        path = fileInfo.absoluteFilePath();
#endif
    }

    path = Utils::Misc::removeIfEndsWith(std::move(path), QDir::separator());
    path = Utils::Misc::removeIfEndsWith(std::move(path), QString{Utils::Misc::dirSeparator()});
    return path;
}

QString relativeNoteFilePath(const NoteData& note) {
    return note.relativeNoteSubFolderPath.isEmpty()
               ? note.fileName
               : note.relativeNoteSubFolderPath + Utils::Misc::dirSeparator() + note.fileName;
}

QString fullNoteFilePath(const NoteData& note) {
    if (!note.fullNoteFilePath.isEmpty() && QFileInfo(note.fullNoteFilePath).fileName() == note.fileName) {
        return note.fullNoteFilePath;
    }

    return Utils::Misc::appendIfDoesNotEndWith(currentNoteFolderPath(), QStringLiteral("/")) +
           relativeNoteFilePath(note);
}

QString cleanupFileName(QString name) {
    static const QRegularExpression re(QStringLiteral(R"([\/\\:])"));
    name.remove(re);

    static const QRegularExpression whitespaceRe(QStringLiteral("\\s+"));
    name.replace(whitespaceRe, QStringLiteral(" "));
    return name;
}

QString extendedCleanupFileName(QString name) {
    static const QRegularExpression re(QStringLiteral(R"([\/\\:<>\"\|\?\*])"));
    name.replace(re, QStringLiteral(" "));
    return name;
}

QString generatedNoteFileName(const QString& name) {
    return name + QStringLiteral(".") + NoteRepository().defaultNoteFileExtension();
}

bool allowDifferentFileName() {
    const NoteFolderData folder = NoteFolderRepository().current();
    return NoteFolderRepository().settingValue(folder.id, QStringLiteral("allowDifferentNoteFileName")).toBool();
}

bool canWriteFilePath(const QString& path) {
    QFile file(path);
    const bool canWrite = file.open(QIODevice::WriteOnly);
    const bool fileExists = file.exists();

    if (file.isOpen()) {
        file.close();

        if (!fileExists) {
            file.remove();
        }
    }

    return canWrite;
}

bool applyFileNameFromText(NoteData& note) {
    QString noteText = note.noteText;
    if (noteText.startsWith(QLatin1String("---"))) {
        static const QRegularExpression frontmatterRe(
            QStringLiteral(R"(^---((\r\n)|(\n\r)|\r|\n).+?((\r\n)|(\n\r)|\r|\n)---((\r\n)|(\n\r)|\r|\n))"),
            QRegularExpression::DotMatchesEverythingOption);
        noteText.remove(frontmatterRe);
    }

    static const QRegularExpression lineFeedRe(QStringLiteral(R"((\r\n)|(\n\r)|\r|\n)"));
    const QStringList noteTextLines = noteText.trimmed().split(lineFeedRe);
    if (noteTextLines.isEmpty()) {
        return false;
    }

    QString name = noteTextLines.at(0).trimmed();
    if (name.isEmpty()) {
        return false;
    }

    static const QRegularExpression headlineRe(QStringLiteral("^#\\s"));
    name.remove(headlineRe);
    name = cleanupFileName(name);

    if (name.isEmpty()) {
        name = QObject::tr("Note");
    }

    if (name == note.name) {
        return false;
    }

    QString fileName = generatedNoteFileName(name);
    if (fileName.toLower() != note.fileName.toLower()) {
        int nameCount = 0;
        const QString nameBase = name;
        NoteRepository repository;
        while (repository.findByFileName(fileName, note.noteSubFolderId).id > 0) {
            name = nameBase + QStringLiteral(" ") + QString::number(++nameCount);
            fileName = generatedNoteFileName(name);

            if (nameCount > 1000) {
                break;
            }
        }
    }

    NoteData candidate = note;
    candidate.name = name;
    candidate.fileName = generatedNoteFileName(name);
    if (!canWriteFilePath(fullNoteFilePath(candidate))) {
        name = extendedCleanupFileName(name);
        candidate.name = name;
        candidate.fileName = generatedNoteFileName(name);
    }

    note.name = candidate.name;
    note.fileName = candidate.fileName;
    return true;
}

bool localTrashEnabled() {
    return TrashRepository().trashMode() == 2;
}

bool systemTrashEnabled() {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    return TrashRepository().trashMode() == 1;
#else
    return false;
#endif
}

bool readFileText(const QString& path, QString& text) {
    QFile file(path);
    if (!file.open(Utils::Misc::getNoteFileOpenFlags())) {
        return false;
    }

    QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("UTF-8");
#endif
    text = in.readAll();
    file.close();
    return true;
}

void addNoteToLocalTrash(const NoteData& note) {
    if (localTrashEnabled()) {
        const bool trashResult = TrashRepository().addNote(note);
        qDebug() << __func__ << " - 'trashResult': " << trashResult;
    }
}
}  // namespace

bool NoteFileService::storeNoteTextFileToDisk(NoteData& note, bool& currentNoteTextChanged,
                                              bool* wasCancelledDueToExternalModification) const {
    if (wasCancelledDueToExternalModification != nullptr) {
        *wasCancelledDueToExternalModification = false;
    }
    currentNoteTextChanged = false;

    const NoteData oldNote = note;
    const QString oldName = note.name;
    const QString oldNoteFilePath = fullNoteFilePath(note);

    if (!allowDifferentFileName()) {
        applyFileNameFromText(note);
    }

    const QString newName = note.name;
    const QString newNoteFilePath = fullNoteFilePath(note);
    bool noteFileWasRenamed = false;

    if (oldName != newName) {
        QFile oldFile(oldNoteFilePath);
        if (oldFile.exists()) {
            noteFileWasRenamed = oldFile.rename(newNoteFilePath);
            qDebug() << __func__ << " - 'noteFileWasRenamed': " << noteFileWasRenamed;
        }

        note.fileChecksum.clear();
    }

    QFile file(newNoteFilePath);
    QFile::OpenMode flags = QIODevice::WriteOnly;
    const SettingsRepository settings;
    const bool useUNIXNewline = settings.value(QStringLiteral("useUNIXNewline")).toBool();
    if (!useUNIXNewline) {
        flags |= QIODevice::Text;
    }

    const bool ignoreAllExternalModifications =
        settings.value(QStringLiteral("ignoreAllExternalModifications")).toBool();
    const bool enableNoteChecksumChecks = settings.value(QStringLiteral("enableNoteChecksumChecks"), false).toBool();

    if (enableNoteChecksumChecks && !ignoreAllExternalModifications && Utils::Misc::fileExists(newNoteFilePath) &&
        !note.fileChecksum.isEmpty()) {
        QString currentFileText;
        if (readFileText(newNoteFilePath, currentFileText)) {
            const QString currentChecksum = calculateChecksum(currentFileText);
            if (currentChecksum != note.fileChecksum) {
                qWarning() << "Note file was modified externally";
                note.noteText = currentFileText;
                note.fileChecksum = currentChecksum;
                if (wasCancelledDueToExternalModification != nullptr) {
                    *wasCancelledDueToExternalModification = true;
                }
                return false;
            }
        }
    }

    qDebug() << "storing note file: " << note.fileName;

    if (!file.open(flags)) {
        qCritical() << QObject::tr("Could not store note file: %1 - Error message: %2")
                           .arg(file.fileName(), file.errorString());
        return false;
    }

    const bool fileExists = Utils::Misc::fileExists(newNoteFilePath);

    if (oldName != newName) {
        if (!noteFileWasRenamed) {
            addNoteToLocalTrash(oldNote);
        }

        TagRepository().renameNoteFileNamesOfLinks(oldName, newName, note.relativeNoteSubFolderPath);
    }

    const QString text = Utils::Misc::transformLineFeeds(note.noteText);
    if (text.isEmpty() && note.fileSize > 100 && fileExists) {
        qWarning() << __func__ << " - Refusing to write empty note to disk when note previously had" << note.fileSize
                   << "bytes - file:" << file.fileName();
        file.close();
        return false;
    }

    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#endif
    out << text;
    out.flush();
    file.flush();
    file.close();

    note.fileChecksum = enableNoteChecksumChecks ? calculateChecksum(text) : QString();
    note.hasDirtyData = false;
    note.fileLastModified = QDateTime::currentDateTime();
    if (!fileExists || !note.fileCreated.isValid()) {
        note.fileCreated = note.fileLastModified;
    }

    const bool noteStored = NoteRepository().save(note);
    if (noteStored && note.id > 0) {
        note = NoteRepository().findById(note.id);
    }

    if (noteStored && !noteFileWasRenamed) {
        QFile oldFile(oldNoteFilePath);
        const QFileInfo oldFileInfo(oldFile);
        const QFile newFile(newNoteFilePath);
        const QFileInfo newFileInfo(newFile);
        if ((newNoteFilePath != oldNoteFilePath) && (oldFileInfo != newFileInfo)) {
            if (oldFile.exists() && oldFileInfo.isFile() && oldFileInfo.isReadable() && oldFile.remove()) {
                qInfo() << QObject::tr("Renamed note-file was removed: %1").arg(oldFile.fileName());
            } else {
                qWarning() << QObject::tr(
                                  "Could not remove renamed note-file: %1"
                                  " - Error message: %2")
                                  .arg(oldFile.fileName(), oldFile.errorString());
            }
        }
    }

    return noteStored;
}

bool NoteFileService::storeNoteTextFileToDisk(NoteData& note) const {
    bool currentNoteTextChanged = false;
    return storeNoteTextFileToDisk(note, currentNoteTextChanged);
}

bool NoteFileService::updateNoteTextFromDisk(NoteData& note) const {
    if (note.id == 0) {
        return false;
    }

    const QString path = fullNoteFilePath(note);
    QFile file(path);
    if (!file.open(Utils::Misc::getNoteFileOpenFlags())) {
        qDebug() << __func__ << " - 'file': " << file.fileName();
        qDebug() << __func__ << " - " << file.errorString();
        return false;
    }

    QFileInfo fileInfo(file);
    note.fileLastModified = fileInfo.lastModified();
    const qint64 fileSize = fileInfo.size();

    if (note.fileSize > 100 && fileSize == 0) {
        qWarning() << __func__ << " - Refusing to load empty file over existing note with size" << note.fileSize
                   << " - file:" << file.fileName();
        file.close();
        return false;
    }

    if (note.fileSize > 1000 && fileSize > 0 && fileSize < 50 && fileSize < (note.fileSize * 0.1)) {
        qWarning() << __func__ << " - Refusing to load suspiciously small file (size" << fileSize
                   << ") over existing note with size" << note.fileSize << " - file:" << file.fileName();
        file.close();
        return false;
    }

    QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("UTF-8");
#endif
    note.noteText = in.readAll();
    file.close();

    if (note.noteText.isNull()) {
        note.noteText = QLatin1String("");
    }

    if (note.noteText.isEmpty() && note.fileSize > 100) {
        qWarning() << __func__ << " - Refusing to overwrite existing note with empty content - file:" << path;
        return false;
    }

    if (SettingsRepository().value(QStringLiteral("enableNoteChecksumChecks"), false).toBool()) {
        note.fileChecksum = calculateChecksum(note.noteText);
    } else {
        note.fileChecksum.clear();
    }

    return NoteRepository().save(note);
}

bool NoteFileService::renameNoteFile(NoteData& note, const QString& newName) const {
    QString cleanName = cleanupFileName(newName);
    const QString suffix = QFileInfo(note.fileName).suffix();
    const QString newFileName = cleanName + QChar('.') + suffix;

    if (note.name == cleanName) {
        return false;
    }

    const NoteData existingNote = NoteRepository().findByName(cleanName);
    if (existingNote.id > 0 && existingNote.id != note.id) {
        return false;
    }

    addNoteToLocalTrash(note);

    const QString oldPath = fullNoteFilePath(note);
    const QString oldName = note.name;
    note.fileName = newFileName;
    note.name = cleanName;

    if (!NoteRepository().save(note)) {
        return false;
    }

    const bool renamed = QFile(oldPath).rename(fullNoteFilePath(note));
    if (renamed && note.id > 0) {
        TagRepository().renameNoteFileNamesOfLinks(oldName, note.name, note.relativeNoteSubFolderPath);
        note = NoteRepository().findById(note.id);
    }
    return renamed;
}

bool NoteFileService::removeNoteFile(const NoteData& note) const {
    const QString path = fullNoteFilePath(note);
    if (!Utils::Misc::fileExists(path)) {
        return false;
    }

    addNoteToLocalTrash(note);

    QFile file(path);
    qDebug() << __func__ << " - 'this->fileName': " << note.fileName;
    qDebug() << __func__ << " - 'file': " << file.fileName();

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    if (systemTrashEnabled()) {
        return file.moveToTrash();
    }
#endif

    return file.remove();
}

bool NoteFileService::canWriteToNoteFile(const NoteData& note) const {
    return canWriteFilePath(fullNoteFilePath(note));
}

QString NoteFileService::calculateChecksum(const QString& text) const {
    const QByteArray hash = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex());
}
