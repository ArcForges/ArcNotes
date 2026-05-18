#include "services/databaseservice.h"

#include <core/repositories/notefolderrepository.h>
#include <core/repositories/settingsrepository.h>
#include <utils/misc.h>

#include <QApplication>
#include <QColor>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QUuid>

#include "services/settingsservice.h"

namespace {
constexpr int kSortAlphabetical = 0;
constexpr int kSortByLastChange = 1;
constexpr int kOrderAscending = 0;
constexpr int kOrderDescending = 1;

struct MergeTagData {
    int id = 0;
    QString name;
    int parentId = 0;
    int priority = 0;
    QString color;
    QString darkColor;
};

struct MergeNoteTagLinkData {
    int sourceTagId = 0;
    QString noteFileName;
    QString noteSubFolderPath;
};

void migrateSettingKey(SettingsService& settings, const QString& oldKey, const QString& newKey) {
    if (!settings.contains(oldKey)) {
        return;
    }

    if (!settings.contains(newKey)) {
        settings.setValue(newKey, settings.value(oldKey));
    }

    settings.remove(oldKey);
}

void migrateSettingPrefix(SettingsService& settings, const QString& oldPrefix, const QString& newPrefix) {
    const auto keys = settings.allKeys();

    for (const QString& key : keys) {
        if (!key.startsWith(oldPrefix)) {
            continue;
        }

        const QString newKey = newPrefix + key.mid(oldPrefix.size());

        if (!settings.contains(newKey)) {
            settings.setValue(newKey, settings.value(key));
        }

        settings.remove(key);
    }
}

void migrateWorkspaceSettingsToLayouts(SettingsService& settings) {
    migrateSettingKey(settings, QStringLiteral("workspaces"), QStringLiteral("layouts"));
    migrateSettingKey(settings, QStringLiteral("currentWorkspace"), QStringLiteral("currentLayout"));
    migrateSettingKey(settings, QStringLiteral("previousWorkspace"), QStringLiteral("previousLayout"));
    migrateSettingKey(settings, QStringLiteral("initialWorkspace"), QStringLiteral("initialLayout"));
    migrateSettingKey(settings, QStringLiteral("initialLayoutIdentifier"),
                      QStringLiteral("initialLayoutPresetIdentifier"));

    migrateSettingPrefix(settings, QStringLiteral("workspace-"), QStringLiteral("layout-"));
    migrateSettingPrefix(settings, QStringLiteral("Shortcuts/MainWindow-restoreWorkspace-"),
                         QStringLiteral("Shortcuts/MainWindow-restoreLayout-"));

    migrateSettingKey(settings, QStringLiteral("MessageBoxOverride/remove-workspace"),
                      QStringLiteral("MessageBoxOverride/remove-layout"));
    migrateSettingKey(settings, QStringLiteral("MessageBoxOverride/layoutwidget-use-layout"),
                      QStringLiteral("MessageBoxOverride/layoutpresetwidget-use-layout-preset"));
}

bool execChecked(QSqlQuery& query, const QString& sql) {
    if (query.exec(sql)) {
        return true;
    }

    qWarning() << __func__ << ":" << query.lastError() << sql;
    return false;
}

bool tableExists(const QSqlDatabase& db, const QString& tableName) {
    return db.tables().contains(tableName, Qt::CaseInsensitive);
}

bool columnExists(const QSqlDatabase& db, const QString& tableName, const QString& columnName) {
    const QSqlRecord record = db.record(tableName);
    return record.indexOf(columnName) >= 0;
}

bool ensureColumn(QSqlQuery& query, const QSqlDatabase& db, const QString& tableName, const QString& columnName,
                  const QString& columnDefinition) {
    if (columnExists(db, tableName, columnName)) {
        return true;
    }

    return execChecked(query, QStringLiteral("ALTER TABLE %1 ADD %2").arg(tableName, columnDefinition));
}

QString currentNoteFolderPath() {
    QString path = NoteFolderRepository().current().localPath;
    if (!path.isEmpty()) {
        const QFileInfo fileInfo(path);
#ifdef Q_OS_WIN32
        path = fileInfo.canonicalFilePath();
#else
        path = fileInfo.absoluteFilePath();
#endif
    }

    if (path.isEmpty()) {
        path = Utils::Misc::prependPortableDataPathIfNeeded(
            SettingsRepository().value(QStringLiteral("notesPath")).toString());
    }

    path = Utils::Misc::removeIfEndsWith(std::move(path), QDir::separator());
    path = Utils::Misc::removeIfEndsWith(std::move(path), QString{Utils::Misc::dirSeparator()});
    return path;
}

void convertTagLinkDirSeparators(QSqlDatabase& db) {
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("UPDATE noteTagLink SET note_sub_folder_path = replace("
                       "note_sub_folder_path, '\\', '/')"));
    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    }
}

void migrateTagDarkColors(QSqlDatabase& db) {
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT id, color FROM tag"));

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return;
    }

    QVector<QPair<int, QString>> tagColors;
    while (query.next()) {
        const QString colorName = query.value(QStringLiteral("color")).toString();
        const QColor color = colorName.isEmpty() ? QColor() : QColor(colorName);
        tagColors.append(
            {query.value(QStringLiteral("id")).toInt(), color.isValid() ? color.name() : QLatin1String("")});
    }

    query.finish();
    query.clear();

    QSqlQuery updateQuery(db);
    updateQuery.prepare(QStringLiteral("UPDATE tag SET dark_color = :color WHERE id = :id"));

    for (const auto& tagColor : Utils::asConst(tagColors)) {
        updateQuery.bindValue(QStringLiteral(":id"), tagColor.first);
        updateQuery.bindValue(QStringLiteral(":color"), tagColor.second);

        if (!updateQuery.exec()) {
            qWarning() << __func__ << ": " << updateQuery.lastError();
            break;
        }
    }
}

bool fetchMergeTags(QSqlDatabase& db, QHash<int, MergeTagData>& tagDataById) {
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT id, name, parent_id, priority, color, dark_color FROM tag"));

    if (!query.exec()) {
        qWarning() << __func__ << ":" << query.lastError();
        return false;
    }

    while (query.next()) {
        MergeTagData tagData;
        tagData.id = query.value(QStringLiteral("id")).toInt();
        tagData.name = query.value(QStringLiteral("name")).toString();
        tagData.parentId = query.value(QStringLiteral("parent_id")).toInt();
        tagData.priority = query.value(QStringLiteral("priority")).toInt();
        tagData.color = query.value(QStringLiteral("color")).toString();
        tagData.darkColor = query.value(QStringLiteral("dark_color")).toString();
        tagDataById.insert(tagData.id, tagData);
    }

    return true;
}

bool fetchMergeNoteTagLinks(QSqlDatabase& db, QVector<MergeNoteTagLinkData>& noteTagLinks) {
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT tag_id, note_file_name, note_sub_folder_path FROM noteTagLink"));

    if (!query.exec()) {
        qWarning() << __func__ << ":" << query.lastError();
        return false;
    }

    while (query.next()) {
        MergeNoteTagLinkData link;
        link.sourceTagId = query.value(QStringLiteral("tag_id")).toInt();
        link.noteFileName = query.value(QStringLiteral("note_file_name")).toString();
        link.noteSubFolderPath = query.value(QStringLiteral("note_sub_folder_path")).toString();
        noteTagLinks.append(link);
    }

    return true;
}

int fetchMergedTagId(QSqlDatabase& db, const QString& name, int parentId) {
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT id FROM tag WHERE name = :name AND parent_id = :parentId"));
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":parentId"), parentId);

    if (!query.exec()) {
        qWarning() << __func__ << ":" << query.lastError();
        return -1;
    }

    return query.first() ? query.value(QStringLiteral("id")).toInt() : 0;
}

int insertMergedTag(QSqlDatabase& db, const MergeTagData& tagData, int targetParentId) {
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("INSERT INTO tag (name, priority, parent_id, color, dark_color) "
                       "VALUES (:name, :priority, :parentId, :color, :darkColor)"));
    query.bindValue(QStringLiteral(":name"), tagData.name);
    query.bindValue(QStringLiteral(":priority"), tagData.priority);
    query.bindValue(QStringLiteral(":parentId"), targetParentId);
    query.bindValue(QStringLiteral(":color"), tagData.color);
    query.bindValue(QStringLiteral(":darkColor"), tagData.darkColor);

    if (!query.exec()) {
        qWarning() << __func__ << ":" << query.lastError();
        return -1;
    }

    return query.lastInsertId().toInt();
}

int ensureMergedTagId(QSqlDatabase& targetDb, const QHash<int, MergeTagData>& sourceTagDataById,
                      QHash<int, int>& tagIdMap, int sourceTagId) {
    if (tagIdMap.contains(sourceTagId)) {
        return tagIdMap.value(sourceTagId);
    }

    const auto it = sourceTagDataById.constFind(sourceTagId);
    if (it == sourceTagDataById.constEnd()) {
        qWarning() << __func__ << ": missing tag for id" << sourceTagId;
        return -1;
    }

    const MergeTagData& tagData = it.value();
    int targetParentId = 0;
    if (tagData.parentId > 0) {
        targetParentId = ensureMergedTagId(targetDb, sourceTagDataById, tagIdMap, tagData.parentId);
        if (targetParentId < 0) {
            return -1;
        }
    }

    int targetTagId = fetchMergedTagId(targetDb, tagData.name, targetParentId);
    if (targetTagId < 0) {
        return -1;
    }

    if (targetTagId == 0) {
        targetTagId = insertMergedTag(targetDb, tagData, targetParentId);
        if (targetTagId < 0) {
            return -1;
        }
    }

    tagIdMap.insert(sourceTagId, targetTagId);
    return targetTagId;
}

bool mergeTagsFromDatabase(QSqlDatabase& db) {
    QSqlDatabase noteFolderDB = DatabaseService::getNoteFolderDatabase();

    const bool isSameTagTable = DatabaseService::generateDatabaseTableSha1Signature(db, "tag") ==
                                DatabaseService::generateDatabaseTableSha1Signature(noteFolderDB, "tag");
    const bool isSameNoteTagLinkTable =
        DatabaseService::generateDatabaseTableSha1Signature(db, "noteTagLink") ==
        DatabaseService::generateDatabaseTableSha1Signature(noteFolderDB, "noteTagLink");

    if (isSameTagTable && isSameNoteTagLinkTable) {
        qDebug() << "Tag and tagLinkTable were the same in conflicting note folder database";
        return true;
    }

    QHash<int, MergeTagData> sourceTagDataById;
    QVector<MergeNoteTagLinkData> noteTagLinks;
    if (!fetchMergeTags(db, sourceTagDataById) || !fetchMergeNoteTagLinks(db, noteTagLinks)) {
        return false;
    }

    if (!noteFolderDB.transaction()) {
        qWarning() << __func__ << ":" << noteFolderDB.lastError();
        return false;
    }

    QHash<int, int> tagIdMap;
    for (auto it = sourceTagDataById.constBegin(); it != sourceTagDataById.constEnd(); ++it) {
        if (ensureMergedTagId(noteFolderDB, sourceTagDataById, tagIdMap, it.key()) < 0) {
            noteFolderDB.rollback();
            return false;
        }
    }

    QSqlQuery linkInsertQuery(noteFolderDB);
    linkInsertQuery.prepare(
        QStringLiteral("INSERT OR IGNORE INTO noteTagLink (tag_id, note_file_name, "
                       "note_sub_folder_path) VALUES (:tagId, :noteFileName, "
                       ":noteSubFolderPath)"));

    for (const MergeNoteTagLinkData& link : noteTagLinks) {
        const int targetTagId = ensureMergedTagId(noteFolderDB, sourceTagDataById, tagIdMap, link.sourceTagId);
        if (targetTagId < 0) {
            noteFolderDB.rollback();
            return false;
        }

        linkInsertQuery.bindValue(QStringLiteral(":tagId"), targetTagId);
        linkInsertQuery.bindValue(QStringLiteral(":noteFileName"), link.noteFileName);
        linkInsertQuery.bindValue(QStringLiteral(":noteSubFolderPath"), link.noteSubFolderPath);

        if (!linkInsertQuery.exec()) {
            qWarning() << __func__ << ":" << linkInsertQuery.lastError();
            noteFolderDB.rollback();
            return false;
        }
    }

    if (!noteFolderDB.commit()) {
        qWarning() << __func__ << ":" << noteFolderDB.lastError();
        noteFolderDB.rollback();
        return false;
    }

    return true;
}

bool createDiskNoteFolderTable(QSqlQuery& query, const QString& tableName) {
    return execChecked(query, QStringLiteral("CREATE TABLE IF NOT EXISTS %1 ("
                                             "id INTEGER PRIMARY KEY,"
                                             "name VARCHAR(255),"
                                             "local_path VARCHAR(255),"
                                             "priority INTEGER DEFAULT 0,"
                                             "active_tag_id INTEGER DEFAULT 0,"
                                             "show_subfolders BOOLEAN DEFAULT 0,"
                                             "active_note_sub_folder_data TEXT)")
                                  .arg(tableName));
}

bool repairDiskNoteFolderSchema(QSqlDatabase& db, QSqlQuery& query) {
    const QString tableName = QStringLiteral("noteFolder");

    if (!tableExists(db, tableName)) {
        return createDiskNoteFolderTable(query, tableName);
    }

    if (!ensureColumn(query, db, tableName, QStringLiteral("name"), QStringLiteral("name VARCHAR(255)")) ||
        !ensureColumn(query, db, tableName, QStringLiteral("local_path"), QStringLiteral("local_path VARCHAR(255)")) ||
        !ensureColumn(query, db, tableName, QStringLiteral("priority"), QStringLiteral("priority INTEGER DEFAULT 0")) ||
        !ensureColumn(query, db, tableName, QStringLiteral("active_tag_id"),
                      QStringLiteral("active_tag_id INTEGER DEFAULT 0")) ||
        !ensureColumn(query, db, tableName, QStringLiteral("show_subfolders"),
                      QStringLiteral("show_subfolders BOOLEAN DEFAULT 0")) ||
        !ensureColumn(query, db, tableName, QStringLiteral("active_note_sub_folder_data"),
                      QStringLiteral("active_note_sub_folder_data TEXT"))) {
        return false;
    }

    if (!columnExists(db, tableName, QStringLiteral("remote_path")) &&
        !columnExists(db, tableName, QStringLiteral("cloud_connection_id"))) {
        return true;
    }

    const QString oldTableName = QStringLiteral("_noteFolderOld");
    return execChecked(query, QStringLiteral("DROP TABLE IF EXISTS %1").arg(oldTableName)) &&
           execChecked(query, QStringLiteral("ALTER TABLE noteFolder RENAME TO %1").arg(oldTableName)) &&
           createDiskNoteFolderTable(query, tableName) &&
           execChecked(query, QStringLiteral("INSERT INTO noteFolder ("
                                             "id, name, local_path, priority, active_tag_id, "
                                             "show_subfolders, active_note_sub_folder_data) "
                                             "SELECT id, name, local_path, priority, active_tag_id, "
                                             "show_subfolders, active_note_sub_folder_data "
                                             "FROM %1 ORDER BY id")
                                  .arg(oldTableName)) &&
           execChecked(query, QStringLiteral("DROP TABLE %1").arg(oldTableName));
}

bool repairNoteFolderSchema(QSqlDatabase& db, QSqlQuery& query, bool recreateIndexes) {
    if (!tableExists(db, QStringLiteral("tag"))) {
        if (!execChecked(query, QStringLiteral("CREATE TABLE tag ("
                                               "id INTEGER PRIMARY KEY,"
                                               "name VARCHAR(255) COLLATE NOCASE,"
                                               "priority INTEGER DEFAULT 0,"
                                               "created DATETIME DEFAULT current_timestamp,"
                                               "parent_id INTEGER DEFAULT 0,"
                                               "color VARCHAR(20),"
                                               "dark_color VARCHAR(20),"
                                               "updated DATETIME DEFAULT current_timestamp)"))) {
            return false;
        }
    } else if (!ensureColumn(query, db, QStringLiteral("tag"), QStringLiteral("parent_id"),
                             QStringLiteral("parent_id INTEGER DEFAULT 0")) ||
               !ensureColumn(query, db, QStringLiteral("tag"), QStringLiteral("color"),
                             QStringLiteral("color VARCHAR(20)")) ||
               !ensureColumn(query, db, QStringLiteral("tag"), QStringLiteral("dark_color"),
                             QStringLiteral("dark_color VARCHAR(20)")) ||
               !ensureColumn(query, db, QStringLiteral("tag"), QStringLiteral("updated"),
                             QStringLiteral("updated DATETIME"))) {
        return false;
    }

    if (recreateIndexes && (!execChecked(query, QStringLiteral("DROP INDEX IF EXISTS idxTagParent")) ||
                            !execChecked(query, QStringLiteral("DROP INDEX IF EXISTS idxUniqueTag")))) {
        return false;
    }

    if (!execChecked(query, QStringLiteral("CREATE INDEX IF NOT EXISTS idxTagParent "
                                           "ON tag( parent_id )")) ||
        !execChecked(query, QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS idxUniqueTag ON "
                                           "tag (name, parent_id)"))) {
        return false;
    }

    if (!tableExists(db, QStringLiteral("noteTagLink"))) {
        if (!execChecked(query, QStringLiteral("CREATE TABLE noteTagLink ("
                                               "id INTEGER PRIMARY KEY,"
                                               "tag_id INTEGER,"
                                               "note_file_name VARCHAR(255) DEFAULT '',"
                                               "note_sub_folder_path TEXT DEFAULT '',"
                                               "created DATETIME DEFAULT current_timestamp,"
                                               "stale_date DATETIME DEFAULT NULL)"))) {
            return false;
        }
    } else if (!ensureColumn(query, db, QStringLiteral("noteTagLink"), QStringLiteral("note_sub_folder_path"),
                             QStringLiteral("note_sub_folder_path TEXT DEFAULT ''")) ||
               !ensureColumn(query, db, QStringLiteral("noteTagLink"), QStringLiteral("stale_date"),
                             QStringLiteral("stale_date DATETIME DEFAULT NULL"))) {
        return false;
    }

    if (recreateIndexes && !execChecked(query, QStringLiteral("DROP INDEX IF EXISTS idxUniqueTagNoteLink"))) {
        return false;
    }

    if (!execChecked(query, QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS idxUniqueTagNoteLink "
                                           "ON noteTagLink (tag_id, note_file_name, "
                                           "note_sub_folder_path)"))) {
        return false;
    }

    if (!tableExists(db, QStringLiteral("trashItem"))) {
        if (!execChecked(query, QStringLiteral("CREATE TABLE trashItem ("
                                               "id INTEGER PRIMARY KEY,"
                                               "file_name VARCHAR(255),"
                                               "file_size INTEGER,"
                                               "note_sub_folder_path_data TEXT,"
                                               "created DATETIME DEFAULT current_timestamp)"))) {
            return false;
        }
    }

    return true;
}
}  // namespace

DatabaseService::DatabaseService() = default;

/**
 * Returns the path to the database (on disk)
 *
 * @return
 */
QString DatabaseService::getDiskDatabasePath() {
    QString databaseFileName =
        Utils::Misc::appDataPath() + Utils::Misc::dirSeparator() + QStringLiteral("ArcNotes.sqlite");
    qDebug() << __func__ << " - 'databaseFileName': " << databaseFileName;

    return databaseFileName;
}

/**
 * @brief Returns the path to the note folder database
 * @return string
 */
QString DatabaseService::getNoteFolderDatabasePath() {
    return currentNoteFolderPath() + Utils::Misc::dirSeparator() + QStringLiteral("notes.sqlite");
}

bool DatabaseService::removeDiskDatabase() {
    QFile file(getDiskDatabasePath());

    if (file.exists()) {
        // the database file will not get deleted under Windows if the
        // database isn't closed
        if (QCoreApplication::instance() != nullptr && QSqlDatabase::contains(QStringLiteral("disk"))) {
            QSqlDatabase dbDisk = QSqlDatabase::database(QStringLiteral("disk"));
            dbDisk.close();
        }

        // remove the file
        bool result = file.remove();

        QString text = result ? QStringLiteral("Removed") : QStringLiteral("Could not remove");
        qWarning() << text + " database file: " << file.fileName();
        return result;
    }

    return false;
}

bool DatabaseService::createConnection() {
    return createMemoryConnection() && createDiskConnection();
}

bool DatabaseService::reinitializeDiskDatabase() {
    return removeDiskDatabase() && createDiskConnection() && setupTables();
}

bool DatabaseService::checkDiskDatabaseIntegrity() {
    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("disk"));
    QSqlQuery query(db);

    if (!query.exec(QStringLiteral("PRAGMA integrity_check"))) {
        qWarning() << __func__ << ": " << query.lastError();

        return false;
    } else if (query.first()) {
        const auto result = query.value(0).toString();

        if (result == QStringLiteral("ok")) {
            return true;
        }

        qWarning() << __func__ << ": " << result;

        return false;
    }

    return false;
}

QString DatabaseService::generateConnectionName() {
    //    return "memory";
    return QString("connection-%1").arg(QUuid::createUuid().toString());
}

QSqlDatabase DatabaseService::createSharedMemoryDatabase(const QString& connectionName) {
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    db.setDatabaseName(QStringLiteral("file:memory?mode=memory&cache=shared"));
    //    db.setDatabaseName(QStringLiteral(":memory:"));
    // QSQLITE_BUSY_TIMEOUT sets the busy timeout via the Qt driver option; we also apply the
    // PRAGMA after opening (see applySharedMemoryDatabasePragmas) because shared-cache mode
    // uses table-level locks that may not respect the driver option alone
    db.setConnectOptions("QSQLITE_OPEN_URI;QSQLITE_BUSY_TIMEOUT=5000");

    return db;
}

void DatabaseService::applySharedMemoryDatabasePragmas(const QSqlDatabase& db) {
    QSqlQuery query(db);
    // busy_timeout covers SQLITE_BUSY (file-level lock contention)
    query.exec(QStringLiteral("PRAGMA busy_timeout = 5000"));
    // read_uncommitted allows background read-only connections to proceed without
    // acquiring a shared-cache table lock, eliminating SQLITE_LOCKED (error 262) errors
    // that busy_timeout does not retry. Background workers only read, so dirty reads
    // are safe here.
    query.exec(QStringLiteral("PRAGMA read_uncommitted = 1"));
}

QSqlDatabase DatabaseService::getSharedMemoryDatabase(const QString& connectionName) {
    return connectionName == QStringLiteral("memory") ? QSqlDatabase::database(QStringLiteral("memory"))
                                                      : createSharedMemoryDatabase(connectionName);
}

bool DatabaseService::createMemoryConnection() {
    QSqlDatabase dbMemory = createSharedMemoryDatabase(QStringLiteral("memory"));

    if (!dbMemory.open()) {
        QMessageBox::critical(nullptr, QWidget::tr("Cannot open memory database"),
                              QWidget::tr("Unable to establish a memory database connection."), QMessageBox::Ok);
        return false;
    }

    applySharedMemoryDatabasePragmas(dbMemory);
    return true;
}

bool DatabaseService::createDiskConnection() {
    QSqlDatabase dbDisk = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("disk"));
    QString path = getDiskDatabasePath();
    dbDisk.setDatabaseName(path);

    if (!dbDisk.open()) {
        QMessageBox::critical(nullptr, QWidget::tr("Cannot open disk database"),
                              QWidget::tr("Unable to establish a database connection with "
                                          "file '%1'.\nAre the folder and the file "
                                          "writable?")
                                  .arg(path),
                              QMessageBox::Ok);
        return false;
    }

    return true;
}

bool DatabaseService::createNoteFolderConnection() {
    QSqlDatabase dbDisk = QSqlDatabase::contains(QStringLiteral("note_folder"))
                              ? QSqlDatabase::database(QStringLiteral("note_folder"))
                              : QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("note_folder"));

    QString path = getNoteFolderDatabasePath();
    dbDisk.setDatabaseName(path);

    if (!dbDisk.open()) {
        QMessageBox::critical(nullptr, QWidget::tr("Cannot open note folder database"),
                              QWidget::tr("Unable to establish a database connection with "
                                          "file '%1'.\nAre the folder and the file "
                                          "writable?")
                                  .arg(path),
                              QMessageBox::Ok);
        return false;
    }

    return true;
}

/**
 * Creates or updates the note folder tables
 */
bool DatabaseService::setupNoteFolderTables() {
    QSqlDatabase dbDisk = getNoteFolderDatabase();
    QSqlQuery queryDisk(dbDisk);
    auto fail = [&dbDisk, &queryDisk]() {
        DatabaseService::closeDatabaseConnection(dbDisk, queryDisk);
        return false;
    };

    if (!execChecked(queryDisk, QStringLiteral("CREATE TABLE IF NOT EXISTS appData ("
                                               "name VARCHAR(255) PRIMARY KEY, "
                                               "value VARCHAR(255))"))) {
        return fail();
    }
    int version = getAppData(QStringLiteral("database_version"), QStringLiteral("note_folder")).toInt();
    int oldVersion = version;
    qDebug() << __func__ << " - 'database version': " << version;

    if (version < 1) {
        if (!execChecked(queryDisk, QStringLiteral("CREATE TABLE IF NOT EXISTS tag ("
                                                   "id INTEGER PRIMARY KEY,"
                                                   "name VARCHAR(255),"
                                                   "priority INTEGER DEFAULT 0,"
                                                   "created DATETIME DEFAULT current_timestamp)"))) {
            return fail();
        }

        if (!execChecked(queryDisk, QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS idxUniqueTag ON "
                                                   "tag (name)"))) {
            return fail();
        }

        if (!execChecked(queryDisk, QStringLiteral("CREATE TABLE IF NOT EXISTS noteTagLink ("
                                                   "id INTEGER PRIMARY KEY,"
                                                   "tag_id INTEGER,"
                                                   "note_file_name VARCHAR(255),"
                                                   "created DATETIME DEFAULT current_timestamp)"))) {
            return fail();
        }

        if (!execChecked(queryDisk, QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS idxUniqueTagNoteLink"
                                                   " ON noteTagLink (tag_id, note_file_name)"))) {
            return fail();
        }

        version = 1;
    }

    if (version < 2) {
        if (!execChecked(queryDisk, QStringLiteral("ALTER TABLE tag ADD parent_id INTEGER DEFAULT 0")) ||
            !execChecked(queryDisk, QStringLiteral("CREATE INDEX IF NOT EXISTS idxTagParent "
                                                   "ON tag( parent_id )"))) {
            return fail();
        }
        version = 2;
    }

    if (version < 3) {
        if (!execChecked(queryDisk, QStringLiteral("DROP INDEX IF EXISTS idxUniqueTag")) ||
            !execChecked(queryDisk, QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS idxUniqueTag ON "
                                                   "tag (name, parent_id)"))) {
            return fail();
        }
        version = 3;
    }

    if (version < 4) {
        if (!execChecked(queryDisk, QStringLiteral("ALTER TABLE noteTagLink ADD note_sub_folder_path TEXT"))) {
            return fail();
        }
        version = 4;
    }

    if (version < 5) {
        if (!execChecked(queryDisk, QStringLiteral("DROP INDEX IF EXISTS idxUniqueTagNoteLink")) ||
            !execChecked(queryDisk, QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS idxUniqueTagNoteLink "
                                                   "ON noteTagLink (tag_id, note_file_name, "
                                                   "note_sub_folder_path)"))) {
            return fail();
        }
        version = 5;
    }

    if (version < 6) {
        // we need to add a `DEFAULT ''` to column note_sub_folder_path
        if (!execChecked(queryDisk, QStringLiteral("ALTER TABLE noteTagLink RENAME TO _noteTagLink")) ||
            !execChecked(queryDisk, QStringLiteral("CREATE TABLE IF NOT EXISTS noteTagLink ("
                                                   "id INTEGER PRIMARY KEY,"
                                                   "tag_id INTEGER,"
                                                   "note_file_name VARCHAR(255) DEFAULT '',"
                                                   "note_sub_folder_path TEXT DEFAULT '',"
                                                   "created DATETIME DEFAULT current_timestamp)")) ||
            !execChecked(queryDisk, QStringLiteral("INSERT INTO noteTagLink (tag_id, note_file_name, "
                                                   "note_sub_folder_path, created) "
                                                   "SELECT tag_id, note_file_name, "
                                                   "note_sub_folder_path, created "
                                                   "FROM _noteTagLink ORDER BY id")) ||
            !execChecked(queryDisk, QStringLiteral("DROP INDEX IF EXISTS idxUniqueTagNoteLink")) ||
            !execChecked(queryDisk, QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS idxUniqueTagNoteLink "
                                                   "ON noteTagLink (tag_id, note_file_name, "
                                                   "note_sub_folder_path)")) ||
            !execChecked(queryDisk, QStringLiteral("DROP TABLE _noteTagLink")) ||
            !execChecked(queryDisk, QStringLiteral("UPDATE noteTagLink SET note_sub_folder_path = '' "
                                                   "WHERE note_sub_folder_path IS NULL"))) {
            return fail();
        }
        version = 6;
    }

    if (version < 7) {
        // convert backslashes to slashes in the noteTagLink table to fix
        // problems with Windows
        convertTagLinkDirSeparators(dbDisk);
        version = 7;
    }

    if (version < 8) {
        if (!execChecked(queryDisk, QStringLiteral("ALTER TABLE tag ADD color VARCHAR(20)"))) {
            return fail();
        }
        version = 8;
    }

    if (version < 9) {
        if (!execChecked(queryDisk, QStringLiteral("ALTER TABLE tag ADD dark_color VARCHAR(20)"))) {
            return fail();
        }
        version = 9;
    }

    if (version < 10) {
        // set the non-darkMode colors as darkMode colors for all tags
        migrateTagDarkColors(dbDisk);
        version = 10;
    }

    if (version < 11) {
        // create a case insensitive index
        if (!execChecked(queryDisk, QStringLiteral("DROP INDEX IF EXISTS idxUniqueTag")) ||
            !execChecked(queryDisk, QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS idxUniqueTag ON "
                                                   "tag (name COLLATE NOCASE, parent_id)"))) {
            return fail();
        }
        version = 11;
    }

    if (version < 12) {
        // create new tag table, because
        //     ALTER TABLE tag ADD updated DEFAULT CURRENT_TIMESTAMP
        // is not supported by sqlite -- you can't add a column with
        // a non-constant default value. And if collate ... is used
        // on a column, it's also defaulted to indices on that column.
        if (!execChecked(queryDisk, QStringLiteral("ALTER TABLE tag RENAME TO _tag")) ||
            !execChecked(queryDisk, QStringLiteral("CREATE TABLE IF NOT EXISTS tag ("
                                                   "id INTEGER PRIMARY KEY,"
                                                   "name VARCHAR(255) COLLATE NOCASE,"
                                                   "priority INTEGER DEFAULT 0,"
                                                   "created DATETIME DEFAULT current_timestamp,"
                                                   "parent_id INTEGER DEFAULT 0,"
                                                   "color VARCHAR(20),"
                                                   "dark_color VARCHAR(20),"
                                                   "updated DATETIME DEFAULT current_timestamp)"))) {
            return fail();
        }

        // recreate the indices
        if (!execChecked(queryDisk, QStringLiteral("DROP INDEX IF EXISTS idxUniqueTag")) ||
            !execChecked(queryDisk, QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS idxUniqueTag ON "
                                                   "tag (name, parent_id)")) ||
            !execChecked(queryDisk, QStringLiteral("DROP INDEX IF EXISTS idxTagParent")) ||
            !execChecked(queryDisk, QStringLiteral("CREATE INDEX IF NOT EXISTS idxTagParent "
                                                   "ON tag( parent_id )"))) {
            return fail();
        }

        // convert old values to new table
        if (!execChecked(queryDisk, QStringLiteral("INSERT INTO tag ( "
                                                   "id, name, priority, created, parent_id, "
                                                   "color, dark_color, updated "
                                                   ") SELECT "
                                                   "id, name, priority, created, parent_id, "
                                                   "color, dark_color, created "
                                                   "FROM _tag ORDER BY id")) ||
            !execChecked(queryDisk, QStringLiteral("DROP TABLE _tag"))) {
            return fail();
        }

        version = 12;
    }

    if (version < 13) {
        if (!execChecked(queryDisk, QStringLiteral("CREATE TABLE IF NOT EXISTS trashItem ("
                                                   "id INTEGER PRIMARY KEY,"
                                                   "file_name VARCHAR(255),"
                                                   "file_size INTEGER,"
                                                   "note_sub_folder_path_data TEXT,"
                                                   "created DATETIME DEFAULT current_timestamp)"))) {
            return fail();
        }

        version = 13;
    }

    if (version < 14) {
        // removing broken tag assignments from
        // https://github.com/pbek/ArcNotes/issues/1510
        if (!execChecked(queryDisk, QStringLiteral("DELETE FROM noteTagLink WHERE note_sub_folder_path IS NULL"))) {
            return fail();
        }

        version = 14;
    }

    if (version < 15) {
        // https://github.com/pbek/ArcNotes/issues/2292
        if (!execChecked(queryDisk, QStringLiteral("ALTER TABLE noteTagLink ADD stale_date DATETIME DEFAULT NULL"))) {
            return fail();
        }

        version = 15;
    }

    const bool repairExistingNoteFolderSchema = version < 16;
    if (!repairNoteFolderSchema(dbDisk, queryDisk, repairExistingNoteFolderSchema)) {
        return fail();
    }

    if (version < 16) {
        // Repair note folder databases that were marked as migrated but missed
        // parts of the schema, for example after importing old settings and
        // selecting a new note folder (see #3612).
        version = 16;
    }

    if (version != oldVersion) {
        if (!setAppData(QStringLiteral("database_version"), QString::number(version), QStringLiteral("note_folder"))) {
            return fail();
        }
    }

    closeDatabaseConnection(dbDisk, queryDisk);

    return true;
}

QSqlDatabase DatabaseService::getNoteFolderDatabase() {
    const QString connectionName = QStringLiteral("note_folder");

    if (!QSqlDatabase::contains(connectionName)) {
        if (currentNoteFolderPath().isEmpty()) {
            return {};
        }

        createNoteFolderConnection();
    }

#ifdef Q_OS_WIN32
    if (Utils::Misc::doAutomaticNoteFolderDatabaseClosing()) {
        // open database if it was closed in closeDatabaseConnection
        createNoteFolderConnection();
    }
#endif

    QSqlDatabase db = QSqlDatabase::database(connectionName);
    //    db.transaction();
    return db;
}

/**
 * Closes a database connection if it was open
 *
 * @param db
 */
void DatabaseService::closeDatabaseConnection(QSqlDatabase& db, QSqlQuery& query) {
    query.finish();
    query.clear();

//    db.commit();
#ifdef Q_OS_WIN32
    if (Utils::Misc::doAutomaticNoteFolderDatabaseClosing()) {
        if (db.isOpen()) {
            db.close();
        }
    }
#else
    Q_UNUSED(db)
#endif
}

bool DatabaseService::setupTables() {
    QSqlDatabase dbDisk = QSqlDatabase::database(QStringLiteral("disk"));
    QSqlQuery queryDisk(dbDisk);
    SettingsService settings;

    queryDisk.exec(
        QStringLiteral("CREATE TABLE IF NOT EXISTS appData ("
                       "name VARCHAR(255) PRIMARY KEY, "
                       "value VARCHAR(255))"));
    int version = getAppData(QStringLiteral("database_version")).toInt();
    int oldVersion = version;
    qDebug() << __func__ << " - 'database_version': " << version;

    if (version > 0) {
        settings.setValue(QStringLiteral("guiFirstRunInit"), true);
    }

    QSqlDatabase dbMemory = QSqlDatabase::database(QStringLiteral("memory"));
    QSqlQuery queryMemory(dbMemory);
    queryMemory.exec(
        QStringLiteral("CREATE TABLE IF NOT EXISTS note ("
                       "id INTEGER PRIMARY KEY,"
                       "name VARCHAR(255) COLLATE NOCASE,"
                       "file_name VARCHAR(255) COLLATE NOCASE,"
                       "file_size INT64 DEFAULT 0,"
                       "note_sub_folder_id int,"
                       "note_text TEXT,"
                       "has_dirty_data INTEGER DEFAULT 0,"
                       "file_last_modified DATETIME,"
                       "file_created DATETIME,"
                       "file_checksum VARCHAR(64),"
                       "created DATETIME default current_timestamp,"
                       "modified DATETIME default current_timestamp)"));
    queryMemory.exec(
        QStringLiteral("CREATE TABLE IF NOT EXISTS noteSubFolder ("
                       "id INTEGER PRIMARY KEY,"
                       "name VARCHAR(255),"
                       "parent_id int,"
                       "file_last_modified DATETIME,"
                       "created DATETIME default current_timestamp,"
                       "modified DATETIME default current_timestamp)"));

    if (version < 1) {
        version = 1;
    }

    if (version < 2) {
        version = 2;
    }

    if (version < 3) {
        queryDisk.exec(
            QStringLiteral("CREATE TABLE IF NOT EXISTS noteFolder ("
                           "id INTEGER PRIMARY KEY,"
                           "name VARCHAR(255),"
                           "local_path VARCHAR(255),"
                           "priority INTEGER DEFAULT 0,"
                           "active_tag_id INTEGER DEFAULT 0,"
                           "show_subfolders BOOLEAN DEFAULT 0,"
                           "active_note_sub_folder_data TEXT)"));
        version = 3;
    }

    // we need to remove the main splitter sizes for version 4 and 5
    if (version < 5) {
        // remove the main splitter sizes for the tags pane
        settings.remove(QStringLiteral("mainSplitterSizes"));

        version = 5;
    }

    if (version < 6) {
        // remove the obsolete activeTagId setting
        settings.remove(QStringLiteral("activeTagId"));

        version = 6;
    }

    if (version < 7) {
        ensureColumn(queryDisk, dbDisk, QStringLiteral("noteFolder"), QStringLiteral("active_tag_id"),
                     QStringLiteral("active_tag_id INTEGER DEFAULT 0"));
        version = 7;
    }

    if (version < 8) {
        version = 8;
    }

    if (version < 9) {
        ensureColumn(queryDisk, dbDisk, QStringLiteral("noteFolder"), QStringLiteral("show_subfolders"),
                     QStringLiteral("show_subfolders BOOLEAN DEFAULT 0"));
        version = 9;
    }

    if (version < 10) {
        ensureColumn(queryDisk, dbDisk, QStringLiteral("noteFolder"), QStringLiteral("active_note_sub_folder_data"),
                     QStringLiteral("active_note_sub_folder_data TEXT"));
        version = 10;
    }

    if (version < 11) {
        // remove the oneColumnModeEnabled setting, that wrongly
        // was turned on by default
        settings.remove(QStringLiteral("oneColumnModeEnabled"));

        version = 11;
    }

    if (version < 12) {
        bool darkModeColors = settings.value(QStringLiteral("darkModeColors")).toBool();

        // set an initial schema key
        QString schemaKey = darkModeColors ? QStringLiteral("EditorColorSchema-cdbf28fc-1ddc-4d13-bb21-6a4043316a2f")
                                           : QStringLiteral("EditorColorSchema-6033d61b-cb96-46d5-a3a8-20d5172017eb");
        settings.setValue(QStringLiteral("Editor/CurrentSchemaKey"), schemaKey);

        version = 12;
    }

    if (version < 14) {
        version = 14;
    }

    if (version < 15) {
        // turn off the DFM initially after the dock widget update
        settings.remove(QStringLiteral("DistractionFreeMode/isEnabled"));

        // remove some deprecated settings
        settings.remove(QStringLiteral("MainWindow/windowState"));
        settings.remove(QStringLiteral("windowState"));
        settings.remove(QStringLiteral("markdownViewEnabled"));
        settings.remove(QStringLiteral("tagsEnabled"));
        settings.remove(QStringLiteral("noteEditPaneEnabled"));
        settings.remove(QStringLiteral("dockWindowState"));
        settings.remove(QStringLiteral("verticalPreviewModeEnabled"));
        settings.remove(QStringLiteral("mainSplitterSizes"));
        settings.remove(QStringLiteral("DistractionFreeMode/mainSplitterSizes"));
        settings.remove(QStringLiteral("mainSplitterState-0-0-0-0"));
        settings.remove(QStringLiteral("mainSplitterState-0-0-0-1"));
        settings.remove(QStringLiteral("mainSplitterState-0-0-1-0"));
        settings.remove(QStringLiteral("mainSplitterState-0-0-1-1"));
        settings.remove(QStringLiteral("mainSplitterState-0-1-0-0"));
        settings.remove(QStringLiteral("mainSplitterState-0-1-0-1"));
        settings.remove(QStringLiteral("mainSplitterState-0-1-1-0"));
        settings.remove(QStringLiteral("mainSplitterState-0-1-1-1"));
        settings.remove(QStringLiteral("mainSplitterState-1-0-0-0"));
        settings.remove(QStringLiteral("mainSplitterState-1-0-0-1"));
        settings.remove(QStringLiteral("mainSplitterState-1-0-1-0"));
        settings.remove(QStringLiteral("mainSplitterState-1-0-1-1"));
        settings.remove(QStringLiteral("mainSplitterState-1-1-0-0"));
        settings.remove(QStringLiteral("mainSplitterState-1-1-0-1"));
        settings.remove(QStringLiteral("mainSplitterState-1-1-1-0"));
        settings.remove(QStringLiteral("mainSplitterState-1-1-1-1"));
        settings.remove(QStringLiteral("noteListSplitterState"));
        settings.remove(QStringLiteral("tagFrameSplitterState"));
        settings.remove(QStringLiteral("verticalNoteFrameSplitterState"));

        version = 15;
    }

    if (version < 16) {
        // remove some deprecated settings
        settings.remove(QStringLiteral("dockWindowGeometry"));
        settings.remove(QStringLiteral("MainWindow/showRecentNoteFolderInMainArea"));

        version = 16;
    }

    if (version < 17) {
        // remove some deprecated settings
        settings.beginGroup(QStringLiteral("LogDialog"));
        settings.remove(QString());
        settings.endGroup();

        settings.remove(QStringLiteral("LogWidget/geometry"));
        settings.remove(QStringLiteral("LogWidget/showAtStartup"));

        version = 17;
    }

    if (version < 18) {
        // set a new markdownHighlightingEnabled setting
        settings.setValue(QStringLiteral("markdownHighlightingEnabled"),
                          settings.value(QStringLiteral("markdownHighlightingInterval"), 200).toInt() > 0);

        // remove the deprecated markdownHighlightingInterval setting
        settings.remove(QStringLiteral("markdownHighlightingInterval"));

        version = 18;
    }

    if (version < 19) {
        version = 19;
    }

    if (version < 20) {
#ifdef Q_OS_MAC
        // disable restoreCursorPosition for macOS by default, because there
        // are users where it causes troubles
        settings.setValue(QStringLiteral("restoreCursorPosition"), false);
#endif

        version = 20;
    }

    if (version < 21) {
        // migrate to the new Portuguese translation
        QString locale = settings.value(QStringLiteral("interfaceLanguage")).toString();
        if (locale == QStringLiteral("pt")) {
            settings.setValue(QStringLiteral("interfaceLanguage"), QStringLiteral("pt_BR"));
        }

        version = 21;
    }

    if (version < 22) {
        version = 22;
    }

    if (version < 23) {
        version = 23;
    }

    if (version < 24) {
        version = 24;
    }

    if (version < 25) {
        // migrate old sort and order settings + set defaults if unset
        // if settings.s;
        if (settings.contains(QStringLiteral("SortingModeAlphabetically"))) {
            bool sort = settings.value(QStringLiteral("SortingModeAlphabetically")).toBool();  // read old setting
            settings.setValue(QStringLiteral("notesPanelSort"), sort ? kSortAlphabetical : kSortByLastChange);
            settings.remove(QStringLiteral("SortingModeAlphabetically"));
        }

        if (settings.contains(QStringLiteral("NoteSortOrder"))) {
            int order = static_cast<Qt::SortOrder>(settings.value(QStringLiteral("NoteSortOrder")).toInt());
            settings.setValue(QStringLiteral("notesPanelOrder"),
                              order);  // see defines in MainWindow.h
            settings.remove(QStringLiteral("NoteSortOrder"));
        }

        // set defaults for now settings if not set already
        if (!settings.contains(QStringLiteral("notesPanelSort"))) {
            settings.value(QStringLiteral("notesPanelSort"), kSortByLastChange);
        }
        if (!settings.contains(QStringLiteral("notesPanelOrder"))) {
            settings.value(QStringLiteral("notesPanelOrder"), kOrderDescending);
        }

        if (!settings.contains(QStringLiteral("noteSubfoldersPanelSort"))) {
            settings.value(QStringLiteral("noteSubfoldersPanelSort"), kSortByLastChange);
        }
        if (!settings.contains(QStringLiteral("noteSubfoldersOrder"))) {
            settings.value(QStringLiteral("noteSubfoldersOrder"), kOrderAscending);
        }

        if (!settings.contains(QStringLiteral("tagsPanelSort"))) {
            settings.value(QStringLiteral("tagsPanelSort"), kSortAlphabetical);
        }
        if (!settings.contains(QStringLiteral("tagsPanelOrder"))) {
            settings.value(QStringLiteral("tagsPanelOrder"), kOrderAscending);
        }
        version = 25;
    }

    if (version < 26) {
        // remove setting with wrong default value
        settings.remove(QStringLiteral("localTrash/autoCleanupDays"));

        version = 26;
    }

    if (version < 27) {
        // if the application was not started for the first time we want to
        // disable that the note edit is the central widget
        if (oldVersion != 0) {
            settings.setValue(QStringLiteral("noteEditIsCentralWidget"), false);
        }

        version = 27;
    }

    if (version < 28) {
#ifndef Q_OS_MAC
        // we only want one app instance on Windows and Linux by default
        settings.setValue(QStringLiteral("allowOnlyOneAppInstance"), true);
#endif

        version = 28;
    }

    if (version < 29) {
        version = 29;
    }

    if (version < 30) {
        version = 30;
    }

    if (version < 31) {
        version = 31;
    }

    if (version < 32) {
        version = 32;
    }

    if (version < 33) {
        const NoteFolderRepository noteFolders;
        for (const NoteFolderData& noteFolder : noteFolders.findAll()) {
            noteFolders.setSettingValue(noteFolder.id, QStringLiteral("allowDifferentNoteFileName"),
                                        settings.value(QStringLiteral("allowDifferentNoteFileName")));
        }

        settings.remove(QStringLiteral("allowDifferentNoteFileName"));

        version = 33;
    }

    if (version < 34) {
        version = 34;
    }

    if (version < 35) {
        // migrate setting with typo
        settings.setValue(QStringLiteral("Editor/removeTrailingSpaces"),
                          settings.value(QStringLiteral("Editor/removeTrainingSpaces")).toBool());
        settings.remove(QStringLiteral("Editor/removeTrainingSpaces"));

        version = 35;
    }

    if (version < 36) {
        // remove possibly corrupted printer dialog settings from
        // https://github.com/pbek/ArcNotes/commit/ef0475692a4baf6f0b30bb200c0ee10157e7c2a6
        // so they can be generated new
        settings.remove(QStringLiteral("Printer/NotePrinting"));
        settings.remove(QStringLiteral("Printer/NotePDFExport"));

        version = 36;
    }

    if (version < 37) {
        // add "txt" and "md" to the note file extensions, so they can also be removed
        auto extensions = settings.value(QStringLiteral("customNoteFileExtensionList")).toStringList();
        extensions << "md"
                   << "txt";

        settings.setValue(QStringLiteral("customNoteFileExtensionList"), extensions);

        version = 37;
    }

    // version 38 was spent

    if (version < 39) {
        // migrate from customNoteFileExtensionList to noteFileExtensionList
        auto extensions = settings.value(QStringLiteral("customNoteFileExtensionList")).toStringList();

        settings.setValue(QStringLiteral("noteFileExtensionList"), extensions);
        settings.remove(QStringLiteral("customNoteFileExtensionList"));

        version = 39;
    }

    if (version < 40) {
        version = 40;
    }

    if (version < 41) {
        version = 41;
    }

    if (version < 42) {
        version = 42;
    }

    if (version < 43) {
        migrateWorkspaceSettingsToLayouts(settings);
        version = 43;
    }

    if (!repairDiskNoteFolderSchema(dbDisk, queryDisk) ||
        !execChecked(queryDisk, QStringLiteral("DROP INDEX IF EXISTS idxUrl")) ||
        !execChecked(queryDisk, QStringLiteral("DROP TABLE IF EXISTS calendarItem")) ||
        !execChecked(queryDisk, QStringLiteral("DROP TABLE IF EXISTS cloudConnection")) ||
        !execChecked(queryDisk, QStringLiteral("DROP TABLE IF EXISTS script")) ||
        !execChecked(queryDisk, QStringLiteral("DROP TABLE IF EXISTS bookmark")) ||
        !execChecked(queryDisk, QStringLiteral("DROP TABLE IF EXISTS commandSnippet"))) {
        return false;
    }

    if (version < 44) {
        version = 44;
    }

    if (version != oldVersion) {
        setAppData(QStringLiteral("database_version"), QString::number(version));
    }

    return true;
}

bool DatabaseService::setAppData(const QString& name, const QString& value, const QString& connectionName) {
    QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery query(db);

    query.prepare(
        QStringLiteral("REPLACE INTO appData ( name, value ) "
                       "VALUES ( :name, :value )"));
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":value"), value);
    return query.exec();
}

QString DatabaseService::getAppData(const QString& name, const QString& connectionName) {
    QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery query(db);

    query.prepare(QStringLiteral("SELECT value FROM appData WHERE name = :name"));
    query.bindValue(QStringLiteral(":name"), name);

    if (!query.exec()) {
        qCritical() << __func__ << ": " << query.lastError();
    } else if (query.first()) {
        return query.value(QStringLiteral("value")).toString();
    }

    return QString();
}

/**
 * Tries to merge a conflicted note folder database into the current one.
 *
 * @param path
 * @return
 */
bool DatabaseService::mergeNoteFolderDatabase(const QString& path) {
    const QString connectionName = QStringLiteral("note_folder_merge");
    QSqlDatabase mergeDB = QSqlDatabase::contains(connectionName)
                               ? QSqlDatabase::database(connectionName)
                               : QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    mergeDB.setDatabaseName(path);

    if (!mergeDB.open()) {
        QMessageBox::critical(nullptr, QWidget::tr("Cannot open database"),
                              QWidget::tr("Unable to establish a database connection with "
                                          "note folder database to merge '%1'.\nAre the folder "
                                          "and the file writable?")
                                  .arg(path),
                              QMessageBox::Ok);

        return false;
    }

    const bool isTagsMerged = mergeTagsFromDatabase(mergeDB);
    mergeDB.close();
    mergeDB = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);

    // We can ignore the appData table, because data there will get updated by
    // ArcNotes itself
    // We can ignore the trashItem table, because ArcNotes will manage the
    // trashed notes itself
    return isTagsMerged;
}

/**
 * Generates a SHA1 signature for the content of a database table
 *
 * @return
 */
QByteArray DatabaseService::generateDatabaseTableSha1Signature(QSqlDatabase& db, const QString& table) {
    // Whitelist of valid table names to prevent SQL injection via table name concatenation
    static const QStringList validTables = {
        QStringLiteral("note"),        QStringLiteral("noteSubFolder"), QStringLiteral("tag"),
        QStringLiteral("noteTagLink"), QStringLiteral("noteFolder"),    QStringLiteral("trashItem"),
    };
    if (!validTables.contains(table)) {
        qCritical() << __func__ << ": invalid table name rejected:" << table;
        return QByteArray();
    }

    QCryptographicHash hash(QCryptographicHash::Sha1);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT * FROM ") + table);

    if (!query.exec()) {
        qCritical() << __func__ << ": " << query.lastError();

        return QByteArray();
    }

    // loop through all table rows
    for (int r = 0; query.next(); r++) {
        int i = 0;
        QVariant value = query.value(i);

        // add data from all query columns
        while (value.isValid() && i < 1000) {
            hash.addData(value.toByteArray());
            value = query.value(i);
            i++;
        }
    }

    const QByteArray& result = hash.result();
    qDebug() << __func__ << " - 'hash': " << result;

    // retrieve the SHA1 signature from the hash
    return result;
}
