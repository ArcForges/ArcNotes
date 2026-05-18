#include "noterepository.h"

#include <core/repositories/notefolderrepository.h>
#include <core/repositories/notesubfolderrepository.h>
#include <core/repositories/settingsrepository.h>
#include <core/repositories/trashrepository.h>
#include <services/databaseservice.h>
#include <utils/misc.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
#include <utility>

namespace {
QString relativePathForSubFolder(const NoteSubFolderRepository& repository, const NoteSubFolderData& subFolder,
                                 const QString& connectionName = QStringLiteral("memory")) {
    if (subFolder.id == 0) {
        return QString();
    }
    if (subFolder.parentId == 0) {
        return subFolder.name;
    }

    const QString parentPath =
        relativePathForSubFolder(repository, repository.findById(subFolder.parentId, connectionName), connectionName);
    return parentPath.isEmpty() ? subFolder.name : parentPath + Utils::Misc::dirSeparator() + subFolder.name;
}

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

QString fullPathForRelativeFile(const QString& relativeFilePath) {
    QString path = Utils::Misc::appendIfDoesNotEndWith(currentNoteFolderPath(), QStringLiteral("/")) + relativeFilePath;
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

QString favoriteIdentifier(const NoteData& note);
bool noteIsFavorite(const NoteData& note);

NoteData noteFromQuery(const QSqlQuery& query, const QString& connectionName = QStringLiteral("memory")) {
    NoteData note;
    note.id = query.value(QStringLiteral("id")).toInt();
    note.name = query.value(QStringLiteral("name")).toString();
    note.fileName = query.value(QStringLiteral("file_name")).toString();
    note.noteSubFolderId = query.value(QStringLiteral("note_sub_folder_id")).toInt();
    note.noteText = query.value(QStringLiteral("note_text")).toString();
    note.fileSize = query.value(QStringLiteral("file_size")).toInt();
    note.fileChecksum = query.value(QStringLiteral("file_checksum")).toString();
    note.hasDirtyData = query.value(QStringLiteral("has_dirty_data")).toInt() == 1;
    note.fileCreated = query.value(QStringLiteral("file_created")).toDateTime();
    note.fileLastModified = query.value(QStringLiteral("file_last_modified")).toDateTime();
    note.created = query.value(QStringLiteral("created")).toDateTime();
    note.modified = query.value(QStringLiteral("modified")).toDateTime();

    if (note.noteSubFolderId > 0) {
        const NoteSubFolderRepository subFolders;
        note.relativeNoteSubFolderPath = relativePathForSubFolder(
            subFolders, subFolders.findById(note.noteSubFolderId, connectionName), connectionName);
    }

    const QString relativeFilePath = note.relativeNoteSubFolderPath.isEmpty()
                                         ? note.fileName
                                         : note.relativeNoteSubFolderPath + Utils::Misc::dirSeparator() + note.fileName;
    note.fullNoteFilePath = fullPathForRelativeFile(relativeFilePath);
    note.isFavorite = noteIsFavorite(note);
    return note;
}

NoteData noteByFileName(const QString& fileName, int noteSubFolderId) {
    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("memory"));
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("SELECT * FROM note WHERE file_name = :file_name AND "
                       "note_sub_folder_id = :note_sub_folder_id"));
    query.bindValue(QStringLiteral(":file_name"), fileName);
    query.bindValue(QStringLiteral(":note_sub_folder_id"), noteSubFolderId);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else if (query.first()) {
        return noteFromQuery(query);
    }

    return NoteData();
}

NoteData noteByRelativeFilePath(const QString& relativePath) {
    const QFileInfo fileInfo(relativePath);
    const NoteSubFolderData subFolder = NoteSubFolderRepository().findByPathData(fileInfo.path(), QStringLiteral("/"));

    if ((fileInfo.path() != QStringLiteral(".")) && subFolder.id == 0) {
        return NoteData();
    }

    return noteByFileName(fileInfo.fileName(), subFolder.id);
}

int activeNoteSubFolderId() {
    const NoteFolderData currentFolder = NoteFolderRepository().current();
    if (currentFolder.activeNoteSubFolderData.isEmpty()) {
        return 0;
    }
    return NoteSubFolderRepository().findByPathData(currentFolder.activeNoteSubFolderData, QStringLiteral("\n")).id;
}

bool isNameSearch(const QString& searchTerm) {
    return searchTerm.startsWith(QStringLiteral("name:")) || searchTerm.startsWith(QStringLiteral("n:"));
}

QString removeNameSearchPrefix(QString searchTerm) {
    static const QRegularExpression re(QStringLiteral("^(name:|n:)"));
    return searchTerm.remove(re);
}

QString generatedNoteFileName(const QString& name) {
    return name + QStringLiteral(".") +
           SettingsRepository().value(QStringLiteral("defaultNoteFileExtension"), QStringLiteral("md")).toString();
}

QString favoriteIdentifier(const NoteData& note) {
    if (note.relativeNoteSubFolderPath.isEmpty()) {
        return note.fileName;
    }

    return note.relativeNoteSubFolderPath + QStringLiteral("/") + note.fileName;
}

void removeFavoriteIdentifier(const NoteData& note) {
    const int folderId = NoteFolderRepository().currentFolderId();
    if (folderId <= 0) {
        return;
    }

    const QString identifier = favoriteIdentifier(note);
    QStringList favoriteIdentifiers =
        NoteFolderRepository().settingValue(folderId, QStringLiteral("favoriteNoteIdentifiers")).toStringList();
    if (!favoriteIdentifiers.contains(identifier)) {
        return;
    }

    favoriteIdentifiers.removeAll(identifier);
    NoteFolderRepository().setSettingValue(folderId, QStringLiteral("favoriteNoteIdentifiers"), favoriteIdentifiers);
    qDebug() << __func__ << "Removed favorite:" << identifier;
}

bool noteIsFavorite(const NoteData& note) {
    const int folderId = NoteFolderRepository().currentFolderId();
    if (folderId <= 0) {
        return false;
    }

    return NoteFolderRepository()
        .settingValue(folderId, QStringLiteral("favoriteNoteIdentifiers"))
        .toStringList()
        .contains(favoriteIdentifier(note));
}

void setFavoriteIdentifier(const NoteData& note, bool favorite) {
    const int folderId = NoteFolderRepository().currentFolderId();
    if (folderId <= 0) {
        return;
    }

    QStringList favoriteIdentifiers =
        NoteFolderRepository().settingValue(folderId, QStringLiteral("favoriteNoteIdentifiers")).toStringList();
    const QString identifier = favoriteIdentifier(note);
    if (favorite) {
        if (!favoriteIdentifiers.contains(identifier)) {
            favoriteIdentifiers.append(identifier);
        }
    } else {
        favoriteIdentifiers.removeAll(identifier);
    }

    NoteFolderRepository().setSettingValue(folderId, QStringLiteral("favoriteNoteIdentifiers"), favoriteIdentifiers);
}

void removeNoteFile(const NoteData& note) {
    if (!Utils::Misc::fileExists(note.fullNoteFilePath)) {
        return;
    }

    const int trashMode = TrashRepository().trashMode();
    if (trashMode == 2) {
        const bool trashResult = TrashRepository().addNote(note);
        qDebug() << __func__ << " - 'trashResult': " << trashResult;
    }

    QFile file(note.fullNoteFilePath);
    qDebug() << __func__ << " - 'this->fileName': " << note.fileName;
    qDebug() << __func__ << " - 'file': " << file.fileName();

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    if (trashMode == 1) {
        file.moveToTrash();
        return;
    }
#endif

    file.remove();
}

void removeTagLinks(const NoteData& note) {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("DELETE FROM noteTagLink WHERE "
                       "note_file_name = :noteFileName AND "
                       "note_sub_folder_path = :noteSubFolderPath"));
    query.bindValue(QStringLiteral(":noteFileName"), note.name);
    query.bindValue(QStringLiteral(":noteSubFolderPath"), note.relativeNoteSubFolderPath);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    }

    DatabaseService::closeDatabaseConnection(db, query);
}

QStringList buildQueryStringList(QString searchString) {
    QStringList queryStrings;

    static const QRegularExpression re(QStringLiteral("\"([^\"]+)\""));
    QRegularExpressionMatchIterator iterator = re.globalMatch(searchString);
    while (iterator.hasNext()) {
        const QRegularExpressionMatch match = iterator.next();
        queryStrings.append(match.captured(1));
        searchString.remove(match.captured(0));
    }

    searchString.remove(QChar('"'));
    searchString = searchString.simplified();

    const QStringList searchStringList = searchString.split(QChar(' '));
    queryStrings.reserve(queryStrings.size() + searchStringList.size());
    for (const QString& text : searchStringList) {
        queryStrings.append(text);
    }

    queryStrings.removeAll(QLatin1String(""));
    queryStrings.removeDuplicates();

    return queryStrings;
}
}  // namespace

NoteData NoteRepository::findById(int id, const QString& connectionName) const {
    const QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT * FROM note WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else if (query.first()) {
        return noteFromQuery(query, connectionName);
    }

    return NoteData();
}

NoteData NoteRepository::findByName(const QString& name, int noteSubFolderId) const {
    if (noteSubFolderId == -1) {
        noteSubFolderId = activeNoteSubFolderId();
    }

    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("memory"));
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("SELECT * FROM note WHERE name = :name AND "
                       "note_sub_folder_id = :note_sub_folder_id"));
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":note_sub_folder_id"), noteSubFolderId);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else if (query.first()) {
        return noteFromQuery(query);
    }

    return NoteData();
}

NoteData NoteRepository::findByName(const QRegularExpression& regExp, int noteSubFolderId) const {
    const QVector<NoteData> notes = noteSubFolderId == -1 ? findAll() : findBySubFolderId(noteSubFolderId);
    for (const NoteData& note : notes) {
        if (regExp.match(note.name).hasMatch()) {
            return note;
        }
    }

    return NoteData();
}

NoteData NoteRepository::findByFileName(const QString& fileName, int noteSubFolderId) const {
    if (noteSubFolderId == -1) {
        noteSubFolderId = activeNoteSubFolderId();
    }

    return noteByFileName(fileName, noteSubFolderId);
}

NoteData NoteRepository::findByRelativeFileName(const NoteData& currentNote, const QString& fileName) const {
    QString cleanFileName = fileName.split(QChar('#')).at(0);
    if (!currentNote.relativeNoteSubFolderPath.isEmpty()) {
        cleanFileName.prepend(currentNote.relativeNoteSubFolderPath + QStringLiteral("/"));
    }
    return noteByRelativeFilePath(cleanFileName);
}

NoteData NoteRepository::findByFileUrl(const QUrl& url) const {
    return noteByRelativeFilePath(relativePathForFileUrlInCurrentFolder(url));
}

NoteData NoteRepository::findByUrlString(const QString& urlString) const {
    const QUrl url{urlString};
    const QRegularExpressionMatch match = QRegularExpression(QStringLiteral(R"(^\w+:\/\/(\d+)$)")).match(urlString);
    QString fileName = match.hasMatch() ? match.captured(1) : url.host();

    if (!url.userName().isEmpty()) {
        fileName = url.userName();
    } else {
        if (fileName.isEmpty()) {
            return NoteData();
        }

        fileName += QStringLiteral(".com");
        fileName = Utils::Misc::removeIfEndsWith(QUrl::fromAce(fileName.toLatin1()), QStringLiteral(".com"));

        if (fileName != url.host()) {
            fileName.replace(QLatin1Char('1'), QStringLiteral("[1１]"))
                .replace(QLatin1Char('2'), QStringLiteral("[2２]"))
                .replace(QLatin1Char('3'), QStringLiteral("[3３]"))
                .replace(QLatin1Char('4'), QStringLiteral("[4４]"))
                .replace(QLatin1Char('5'), QStringLiteral("[5５]"))
                .replace(QLatin1Char('6'), QStringLiteral("[6６]"))
                .replace(QLatin1Char('7'), QStringLiteral("[7７]"))
                .replace(QLatin1Char('8'), QStringLiteral("[8８]"))
                .replace(QLatin1Char('9'), QStringLiteral("[9９]"))
                .replace(QLatin1Char('0'), QStringLiteral("[0０]"));
        }
    }

    fileName.replace(QChar('-'), QChar('?')).replace(QChar('_'), QChar('?'));

    QString escapedFileName = QRegularExpression::escape(fileName);
    escapedFileName.replace(QStringLiteral("\\?"), QStringLiteral("."));
    const QRegularExpression regExp(QLatin1Char('^') + escapedFileName + QLatin1Char('$'),
                                    QRegularExpression::CaseInsensitiveOption);

    qDebug() << __func__ << " - 'regExp': " << regExp;

    const int noteSubFolderId = activeNoteSubFolderId();
    qDebug() << __func__ << " - 'noteSubFolderId': " << noteSubFolderId;

    NoteData note;
    if (noteSubFolderId > 0) {
        note = findByName(regExp, noteSubFolderId);
        qDebug() << __func__ << " - 'note in sub folder': " << note.name;
    }

    if (note.id == 0) {
        note = findByName(regExp);
        qDebug() << __func__ << " - 'note in all sub folders': " << note.name;
    }

    return note;
}

QVector<NoteData> NoteRepository::findAll(int limit, const QString& connectionName) const {
    const QSqlDatabase db = QSqlDatabase::database(connectionName);
    QVector<NoteData> notes;

    if (!db.tables().contains(QStringLiteral("note"), Qt::CaseInsensitive)) {
        return notes;
    }

    QSqlQuery query(db);
    const QString sql = limit >= 0 ? QStringLiteral(
                                         "SELECT * FROM note ORDER BY file_last_modified DESC "
                                         "LIMIT :limit")
                                   : QStringLiteral("SELECT * FROM note ORDER BY file_last_modified DESC");
    query.prepare(sql);

    if (limit >= 0) {
        notes.reserve(limit);
        query.bindValue(QStringLiteral(":limit"), limit);
    }

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return notes;
    }

    while (query.next()) {
        notes.append(noteFromQuery(query, connectionName));
    }

    return notes;
}

QVector<NoteData> NoteRepository::findBySubFolderId(int noteSubFolderId, const QString& connectionName) const {
    const QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery query(db);
    QVector<NoteData> notes;
    query.prepare(
        QStringLiteral("SELECT * FROM note WHERE note_sub_folder_id = "
                       ":note_sub_folder_id ORDER BY file_last_modified DESC"));
    query.bindValue(QStringLiteral(":note_sub_folder_id"), noteSubFolderId);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return notes;
    }

    while (query.next()) {
        notes.append(noteFromQuery(query, connectionName));
    }

    return notes;
}

QVector<NoteData> NoteRepository::search(const QString& text) const {
    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("memory"));
    QSqlQuery query(db);
    QVector<NoteData> notes;

    query.prepare(
        QStringLiteral("SELECT * FROM note WHERE note_text LIKE :text "
                       "ORDER BY file_last_modified DESC"));
    query.bindValue(QStringLiteral(":text"), QStringLiteral("%") + text + QStringLiteral("%"));

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return notes;
    }

    while (query.next()) {
        notes.append(noteFromQuery(query));
    }

    return notes;
}

QStringList NoteRepository::searchAsNameList(const QString& text, bool ignoreNoteSubFolder) const {
    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("memory"));
    QSqlQuery query(db);
    QStringList nameList;
    const QString textSearchSql = !ignoreNoteSubFolder ? QStringLiteral("OR note_text LIKE :text ") : QLatin1String("");

    query.prepare(QStringLiteral("SELECT name FROM note WHERE (name LIKE :text ") + textSearchSql +
                  QStringLiteral(") ORDER BY file_last_modified DESC"));
    query.bindValue(QStringLiteral(":text"), QStringLiteral("%") + text + QStringLiteral("%"));

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return nameList;
    }

    while (query.next()) {
        nameList.append(query.value(QStringLiteral("name")).toString());
    }

    return nameList;
}

QString NoteRepository::defaultNoteFileExtension() const {
    return SettingsRepository().value(QStringLiteral("defaultNoteFileExtension"), QStringLiteral("md")).toString();
}

QStringList NoteRepository::noteFileExtensionList(const QString& prefix) const {
    QStringList list = SettingsRepository().value(QStringLiteral("noteFileExtensionList")).toStringList();
    list.removeDuplicates();

    if (list.isEmpty()) {
        list << defaultNoteFileExtension();
    }

    if (!prefix.isEmpty()) {
        list.replaceInStrings(QRegularExpression(QStringLiteral("^")), prefix);
    }

    return list;
}

bool NoteRepository::isWikiLinkSupportEnabled() const {
    return SettingsRepository().value(QStringLiteral("Editor/wikiLinkSupport"), false).toBool();
}

bool NoteRepository::fileUrlIsNoteInCurrentFolder(const QUrl& url) const {
    if (url.scheme() != QStringLiteral("file")) {
        return false;
    }

    const QString path = url.toLocalFile();
    if (!path.startsWith(currentNoteFolderPath())) {
        return false;
    }

    QListIterator<QString> extensions(noteFileExtensionList(QStringLiteral(".")));
    while (extensions.hasNext()) {
        if (path.endsWith(extensions.next(), Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}

bool NoteRepository::fileUrlIsExistingNoteInCurrentFolder(const QUrl& url) const {
    if (url.scheme() != QStringLiteral("file")) {
        return false;
    }

    const QString path = url.toLocalFile();
    if (!QFile(path).exists()) {
        return false;
    }

    return path.startsWith(currentNoteFolderPath()) && path.endsWith(QLatin1String(".md"), Qt::CaseInsensitive);
}

QString NoteRepository::decodeNoteUrl(QString url) const {
    return QUrl::fromPercentEncoding(url.replace(QStringLiteral("&#32;"), QStringLiteral(" ")).toUtf8());
}

QString NoteRepository::fragmentFromFileName(const QString& fileName) const {
    const auto splitList = fileName.split(QChar('#'));
    const QString fragment = splitList.count() > 1 ? splitList.at(1) : QString();
    return QUrl::fromPercentEncoding(fragment.toLocal8Bit());
}

QString NoteRepository::relativePathForFileUrlInCurrentFolder(const QUrl& url) const {
    QString path = url.toLocalFile();
    qDebug() << __func__ << " - 'path': " << path;

    const QFileInfo fileInfo(path);
#ifdef Q_OS_WIN32
    path = fileInfo.absoluteFilePath();
#else
    path = fileInfo.absoluteFilePath();
#endif

    qDebug() << __func__ << " - 'canonicalFilePath': " << path;

    return path.remove(Utils::Misc::appendIfDoesNotEndWith(currentNoteFolderPath(), QStringLiteral("/")));
}

QVector<int> NoteRepository::searchInNotes(const QString& query, bool ignoreNoteSubFolder, int noteSubFolderId,
                                           const QString& connectionName) const {
    const QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery sqlQuery(db);
    QVector<int> noteIds;
    QStringList sqlList;

    if ((noteSubFolderId == -1) && !ignoreNoteSubFolder) {
        noteSubFolderId = activeNoteSubFolderId();
    }

    const QStringList queryStrings = buildQueryStringList(query);
    sqlList.reserve(queryStrings.count());
    for (const QString& queryString : queryStrings) {
        sqlList.append(isNameSearch(queryString) ? QStringLiteral("(name LIKE ? OR file_name LIKE ?)")
                                                 : QStringLiteral("(note_text LIKE ? OR name LIKE ?)"));
    }

    QString sql;
    if (ignoreNoteSubFolder) {
        sql = QStringLiteral("SELECT id FROM note WHERE ") + sqlList.join(QStringLiteral(" AND "));
        sqlQuery.prepare(sql);
    } else {
        sql = QStringLiteral(
                  "SELECT id FROM note WHERE note_sub_folder_id = "
                  ":note_sub_folder_id AND ") +
              sqlList.join(QStringLiteral(" AND "));
        sqlQuery.prepare(sql);
        sqlQuery.bindValue(0, noteSubFolderId);
    }

    for (int i = 0; i < queryStrings.count(); i++) {
        QString queryString = queryStrings[i];
        if (isNameSearch(queryString)) {
            queryString = removeNameSearchPrefix(queryString);
        }

        int pos = i * 2;
        pos = ignoreNoteSubFolder ? pos : pos + 1;
        sqlQuery.bindValue(pos, QStringLiteral("%") + queryString + QStringLiteral("%"));
        sqlQuery.bindValue(pos + 1, QStringLiteral("%") + queryString + QStringLiteral("%"));
    }

    if (!sqlQuery.exec()) {
        qWarning() << __func__ << ": " << sqlQuery.lastError();
        return noteIds;
    }

    while (sqlQuery.next()) {
        noteIds.append(sqlQuery.value(QStringLiteral("id")).toInt());
    }

    return noteIds;
}

bool NoteRepository::save(const NoteData& note) const {
    QString fileName = note.fileName;
    if (fileName.isEmpty()) {
        if (note.name.isEmpty()) {
            return false;
        }

        fileName = generatedNoteFileName(note.name);
    }

    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("memory"));
    QSqlQuery query(db);

    if (note.id > 0) {
        query.prepare(
            QStringLiteral("UPDATE note SET "
                           "name = :name,"
                           "file_name = :file_name,"
                           "file_size = :file_size,"
                           "note_sub_folder_id = :note_sub_folder_id,"
                           "note_text = :note_text,"
                           "has_dirty_data = :has_dirty_data, "
                           "file_last_modified = :file_last_modified,"
                           "file_created = :file_created,"
                           "file_checksum = :file_checksum,"
                           "modified = :modified "
                           "WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), note.id);
    } else {
        query.prepare(
            QStringLiteral("INSERT INTO note"
                           "(name, file_name, "
                           "file_size, note_text, has_dirty_data, "
                           "file_last_modified, file_created, modified, "
                           "note_sub_folder_id, file_checksum) "
                           "VALUES (:name, :file_name, :file_size, :note_text,"
                           ":has_dirty_data, :file_last_modified,"
                           ":file_created, :modified,"
                           ":note_sub_folder_id, :file_checksum)"));
    }

    const QDateTime modified = QDateTime::currentDateTime();
    const int fileSize = note.noteText.toUtf8().size();

    query.bindValue(QStringLiteral(":name"), note.name);
    query.bindValue(QStringLiteral(":file_name"), fileName);
    query.bindValue(QStringLiteral(":file_size"), fileSize);
    query.bindValue(QStringLiteral(":note_sub_folder_id"), note.noteSubFolderId);
    query.bindValue(QStringLiteral(":note_text"), note.noteText);
    query.bindValue(QStringLiteral(":has_dirty_data"), note.hasDirtyData ? 1 : 0);
    query.bindValue(QStringLiteral(":file_created"), note.fileCreated);
    query.bindValue(QStringLiteral(":file_last_modified"), note.fileLastModified);
    query.bindValue(QStringLiteral(":file_checksum"), note.fileChecksum);
    query.bindValue(QStringLiteral(":modified"), modified);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return false;
    }

    return true;
}

bool NoteRepository::remove(int id, bool withFile) const {
    const NoteData note = findById(id);
    if (note.id == 0) {
        return false;
    }

    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("memory"));
    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM note WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return false;
    }

    if (withFile) {
        removeFavoriteIdentifier(note);
        removeNoteFile(note);
        removeTagLinks(note);
    }

    return true;
}

bool NoteRepository::isFavorite(const NoteData& note) const {
    return note.id > 0 && noteIsFavorite(note);
}

bool NoteRepository::setFavorite(const NoteData& note, bool favorite) const {
    if (note.id <= 0) {
        return false;
    }

    setFavoriteIdentifier(note, favorite);
    return true;
}

bool NoteRepository::toggleFavorite(const NoteData& note) const {
    if (note.id <= 0) {
        return false;
    }

    setFavoriteIdentifier(note, !noteIsFavorite(note));
    return true;
}

bool NoteRepository::deleteAll() const {
    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("memory"));
    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM note"));

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return false;
    }

    return true;
}

int NoteRepository::countAll() const {
    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("memory"));
    QSqlQuery query(db);

    query.prepare(QStringLiteral("SELECT COUNT(*) AS cnt FROM note"));

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else if (query.first()) {
        return query.value(QStringLiteral("cnt")).toInt();
    }

    return 0;
}

QVector<int> NoteRepository::fetchAllIds(int limit, int offset) const {
    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("memory"));
    QSqlQuery query(db);
    QVector<int> ids;
    QString sql = QStringLiteral("SELECT id FROM note ORDER BY id");

    if (limit >= 0) {
        sql += QStringLiteral(" LIMIT :limit");
    }
    if (offset >= 0) {
        sql += QStringLiteral(" OFFSET :offset");
    }

    query.prepare(sql);

    if (limit >= 0) {
        ids.reserve(limit);
        query.bindValue(QStringLiteral(":limit"), limit);
    }
    if (offset >= 0) {
        query.bindValue(QStringLiteral(":offset"), offset);
    }

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return ids;
    }

    while (query.next()) {
        ids.append(query.value(QStringLiteral("id")).toInt());
    }

    return ids;
}
