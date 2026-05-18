#include "trashrepository.h"

#include <core/repositories/notefolderrepository.h>
#include <core/repositories/settingsrepository.h>
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
#include <QTextStream>

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <QRandomGenerator>
#endif

namespace {
TrashItemData trashItemFromQuery(const QSqlQuery& query) {
    TrashItemData item;
    item.id = query.value(QStringLiteral("id")).toInt();
    item.fileName = query.value(QStringLiteral("file_name")).toString();
    item.noteSubFolderPathData = query.value(QStringLiteral("note_sub_folder_path_data")).toString();
    item.fileSize = query.value(QStringLiteral("file_size")).toLongLong();
    item.created = query.value(QStringLiteral("created")).toDateTime();
    return item;
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

QString currentTrashPath() {
    return currentNoteFolderPath() + QDir::separator() + QStringLiteral("trash");
}

QString trashFilePath(int id) {
    return currentTrashPath() + QDir::separator() + QString::number(id);
}

QString noteSubFolderPathDataFromNote(const NoteData& note) {
    QString path = note.relativeNoteSubFolderPath;
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return path.split(QLatin1Char('/'), Qt::SkipEmptyParts).join(QLatin1Char('\n'));
}

QString noteSubFolderDiskPath(const QString& pathData) {
    const QString relativePath = pathData.split(QLatin1Char('\n'), Qt::SkipEmptyParts).join(QDir::separator());
    const QString rootPath = currentNoteFolderPath();
    return relativePath.isEmpty() ? rootPath : rootPath + QDir::separator() + relativePath;
}

int insertTrashItem(const TrashItemData& trashItem) {
    if (trashItem.fileName.isEmpty()) {
        return 0;
    }

    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("INSERT INTO trashItem "
                       "(file_name, file_size, note_sub_folder_path_data) "
                       "VALUES (:file_name, :file_size, :note_sub_folder_path_data)"));
    query.bindValue(QStringLiteral(":file_name"), trashItem.fileName);
    query.bindValue(QStringLiteral(":file_size"), trashItem.fileSize);
    query.bindValue(QStringLiteral(":note_sub_folder_path_data"), trashItem.noteSubFolderPathData);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        DatabaseService::closeDatabaseConnection(db, query);
        return 0;
    }

    const int id = query.lastInsertId().toInt();
    DatabaseService::closeDatabaseConnection(db, query);
    return id;
}

bool deleteTrashItemRecord(int id) {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM trashItem WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        DatabaseService::closeDatabaseConnection(db, query);
        return false;
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return true;
}

bool trashFileExists(int id) {
    const QFile file(trashFilePath(id));
    const QFileInfo fileInfo(file);
    return file.exists() && fileInfo.isFile() && fileInfo.isReadable();
}
}  // namespace

TrashItemData TrashRepository::findById(int id) const {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT * FROM trashItem WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else if (query.first()) {
        const TrashItemData item = trashItemFromQuery(query);
        DatabaseService::closeDatabaseConnection(db, query);
        return item;
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return TrashItemData();
}

QList<TrashItemData> TrashRepository::findAll(int limit) const {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QList<TrashItemData> items;

    if (!db.isValid() || !db.tables().contains(QStringLiteral("trashItem"), Qt::CaseInsensitive)) {
        return items;
    }

    QSqlQuery query(db);
    QString sql = QStringLiteral("SELECT * FROM trashItem ORDER BY created DESC");

    if (limit >= 0) {
        sql += QLatin1String(" LIMIT :limit");
    }

    query.prepare(sql);

    if (limit >= 0) {
        query.bindValue(QStringLiteral(":limit"), limit);
    }

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else {
        while (query.next()) {
            items.append(trashItemFromQuery(query));
        }
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return items;
}

QList<TrashItemData> TrashRepository::findAllExpired() const {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    QList<TrashItemData> items;
    const int days = SettingsRepository().value(QStringLiteral("localTrash/autoCleanupDays"), 30).toInt();
    const QDateTime dateTime = QDateTime::currentDateTime().addDays(-1 * days);

    query.prepare(
        QStringLiteral("SELECT * FROM trashItem WHERE created < :created "
                       "ORDER BY created DESC"));
    query.bindValue(QStringLiteral(":created"), dateTime);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else {
        while (query.next()) {
            items.append(trashItemFromQuery(query));
        }
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return items;
}

bool TrashRepository::save(const TrashItemData& trashItem) const {
    if (trashItem.fileName.isEmpty()) {
        return false;
    }

    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);

    if (trashItem.id > 0) {
        query.prepare(
            QStringLiteral("UPDATE trashItem SET "
                           "file_name = :file_name, "
                           "file_size = :file_size, "
                           "note_sub_folder_path_data = :note_sub_folder_path_data "
                           "WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), trashItem.id);
    } else {
        query.prepare(
            QStringLiteral("INSERT INTO trashItem "
                           "(file_name, file_size, note_sub_folder_path_data) "
                           "VALUES (:file_name, :file_size, :note_sub_folder_path_data)"));
    }

    query.bindValue(QStringLiteral(":file_name"), trashItem.fileName);
    query.bindValue(QStringLiteral(":file_size"), trashItem.fileSize);
    query.bindValue(QStringLiteral(":note_sub_folder_path_data"), trashItem.noteSubFolderPathData);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        DatabaseService::closeDatabaseConnection(db, query);
        return false;
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return true;
}

bool TrashRepository::remove(int id, bool withFile) const {
    if (findById(id).id == 0) {
        return false;
    }

    const QString filePath = trashFilePath(id);
    if (!deleteTrashItemRecord(id)) {
        return false;
    }

    if (withFile) {
        QFile file(filePath);
        if (file.exists()) {
            file.remove();
        }
    }

    return true;
}

bool TrashRepository::addNote(const NoteData& note) const {
    TrashItemData item;
    item.fileName = note.fileName;
    item.fileSize = note.fileSize;
    item.noteSubFolderPathData = noteSubFolderPathDataFromNote(note);
    item.fullNoteFilePath = note.fullNoteFilePath;

    const int id = insertTrashItem(item);
    if (id <= 0) {
        return false;
    }

    QDir destinationDir(currentTrashPath());
    if (!destinationDir.exists() && !destinationDir.mkpath(destinationDir.path())) {
        return false;
    }

    QFile file(item.fullNoteFilePath);
    const QString destinationFileName = trashFilePath(id);
    qDebug() << __func__ << " - 'destinationFileName': " << destinationFileName;
    return file.copy(destinationFileName);
}

bool TrashRepository::restore(int id) const {
    if (!fileExists(id)) {
        return false;
    }

    const QString newFilePath = restorationFilePath(id);
    if (newFilePath.isEmpty()) {
        return false;
    }

    QFile file(trashFilePath(id));
    if (!file.rename(newFilePath)) {
        return false;
    }

    return remove(id, false);
}

QString TrashRepository::loadFileText(int id) const {
    QFile file(trashFilePath(id));

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << __func__ << " - 'file': " << file.fileName();
        qDebug() << __func__ << " - " << file.errorString();
        return QString();
    }

    QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("UTF-8");
#endif
    const QString text = in.readAll();
    file.close();
    return text;
}

QString TrashRepository::restorationFilePath(int id) const {
    const TrashItemData item = findById(id);
    if (item.id == 0) {
        return QString();
    }

    const QString folderPath = noteSubFolderDiskPath(item.noteSubFolderPathData);
    QString filePath = folderPath + QDir::separator() + item.fileName;

    QFile file(filePath);
    if (file.exists()) {
        filePath = folderPath + QDir::separator() + QString::number(QDateTime::currentMSecsSinceEpoch() / 1000) + "_" +
                   item.fileName;
    }

    file.setFileName(filePath);
    if (file.exists()) {
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
        const quint32 number = qrand();
#else
        const quint32 number = QRandomGenerator::global()->generate();
#endif
        filePath = folderPath + QDir::separator() + QString::number(number) + "_" + item.fileName;
    }

    file.setFileName(filePath);
    if (file.exists()) {
        return QString();
    }

    return filePath;
}

bool TrashRepository::fileExists(int id) const {
    return findById(id).id > 0 && trashFileExists(id);
}

int TrashRepository::trashMode() const {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    if (SettingsRepository().contains(QStringLiteral("localTrash/mode"))) {
        const int mode = SettingsRepository().value(QStringLiteral("localTrash/mode")).toInt();
        if (mode >= 0 && mode <= 2) {
            return mode;
        }
    }
#endif

    return SettingsRepository().value(QStringLiteral("localTrash/supportEnabled"), true).toBool() ? 2 : 0;
}

bool TrashRepository::clear(bool withFiles) const {
    const QList<TrashItemData> items = findAll();
    bool ok = true;
    for (const TrashItemData& item : items) {
        ok = remove(item.id, withFiles) && ok;
    }
    return ok;
}

bool TrashRepository::expireItems() const {
    if (trashMode() != 2 ||
        !SettingsRepository().value(QStringLiteral("localTrash/autoCleanupEnabled"), true).toBool()) {
        return false;
    }

    const QList<TrashItemData> items = findAllExpired();
    for (const TrashItemData& item : items) {
        remove(item.id, true);
        qDebug() << __func__ << " - 'trashItem': " << item.id << item.fileName;
    }

    return true;
}

int TrashRepository::countAll() const {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);

    query.prepare(QStringLiteral("SELECT COUNT(*) AS cnt FROM trashItem"));

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else if (query.first()) {
        const int result = query.value(QStringLiteral("cnt")).toInt();
        DatabaseService::closeDatabaseConnection(db, query);
        return result;
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return 0;
}
