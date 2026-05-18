#include "notesubfolderrepository.h"

#include <core/repositories/notefolderrepository.h>
#include <core/repositories/settingsrepository.h>
#include <utils/misc.h>

#ifndef INTEGRATION_TESTS
#include <utils/gui.h>
#endif

#include <QDateTime>
#include <QDebug>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

namespace {
NoteSubFolderData subFolderFromQuery(const QSqlQuery& query) {
    NoteSubFolderData subFolder;
    subFolder.id = query.value(QStringLiteral("id")).toInt();
    subFolder.parentId = query.value(QStringLiteral("parent_id")).toInt();
    subFolder.name = query.value(QStringLiteral("name")).toString();
    subFolder.fileLastModified = query.value(QStringLiteral("file_last_modified")).toDateTime();
    subFolder.created = query.value(QStringLiteral("created")).toDateTime();
    subFolder.modified = query.value(QStringLiteral("modified")).toDateTime();
    return subFolder;
}

QString subFolderPathData(int id, const QString& connectionName) {
    const NoteSubFolderRepository repository;
    const NoteSubFolderData subFolder = repository.findById(id, connectionName);
    if (subFolder.id == 0) {
        return QString();
    }

    if (subFolder.parentId == 0) {
        return subFolder.name;
    }

    const QString parentPath = subFolderPathData(subFolder.parentId, connectionName);
    return parentPath.isEmpty() ? subFolder.name : parentPath + QChar('\n') + subFolder.name;
}
}  // namespace

NoteSubFolderData NoteSubFolderRepository::findById(int id, const QString& connectionName) const {
    const QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT * FROM noteSubFolder WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else if (query.first()) {
        return subFolderFromQuery(query);
    }

    return NoteSubFolderData();
}

NoteSubFolderData NoteSubFolderRepository::findByNameAndParentId(const QString& name, int parentId,
                                                                 const QString& connectionName) const {
    const QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery query(db);

    query.prepare(
        QStringLiteral("SELECT * FROM noteSubFolder WHERE name = :name "
                       "AND parent_id = :parent_id"));
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":parent_id"), parentId);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else if (query.first()) {
        return subFolderFromQuery(query);
    }

    return NoteSubFolderData();
}

NoteSubFolderData NoteSubFolderRepository::findByPathData(const QString& pathData, const QString& separator,
                                                          const QString& connectionName) const {
    if (pathData.isEmpty()) {
        return NoteSubFolderData();
    }

    QString cleanPathData = Utils::Misc::removeIfStartsWith(pathData, separator);
    const QStringList pathList = cleanPathData.split(separator);
    NoteSubFolderData subFolder;
    for (const QString& name : pathList) {
        subFolder = findByNameAndParentId(name, subFolder.id, connectionName);
        if (subFolder.id == 0) {
            return NoteSubFolderData();
        }
    }

    return subFolder;
}

QVector<NoteSubFolderData> NoteSubFolderRepository::findAll(int limit) const {
    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("memory"));
    QVector<NoteSubFolderData> subFolders;

    if (!db.tables().contains(QStringLiteral("noteSubFolder"), Qt::CaseInsensitive)) {
        return subFolders;
    }

    QSqlQuery query(db);
    QString sql = QStringLiteral(
        "SELECT * FROM noteSubFolder "
        "ORDER BY file_last_modified DESC");

    if (limit >= 0) {
        sql += QStringLiteral(" LIMIT :limit");
    }

    query.prepare(sql);

    if (limit >= 0) {
        subFolders.reserve(limit);
        query.bindValue(QStringLiteral(":limit"), limit);
    }

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return subFolders;
    }

    while (query.next()) {
        subFolders.append(subFolderFromQuery(query));
    }

    return subFolders;
}

QVector<NoteSubFolderData> NoteSubFolderRepository::findByParentId(int parentId, const QString& sortBy,
                                                                   const QString& connectionName) const {
    const QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery query(db);
    QVector<NoteSubFolderData> subFolders;
    const QString sql = QStringLiteral(
                            "SELECT * FROM noteSubFolder WHERE parent_id = "
                            ":parent_id ORDER BY ") +
                        sortBy;

    query.prepare(sql);
    query.bindValue(QStringLiteral(":parent_id"), parentId);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return subFolders;
    }

    while (query.next()) {
        subFolders.append(subFolderFromQuery(query));
    }

    return subFolders;
}

bool NoteSubFolderRepository::save(const NoteSubFolderData& subFolder) const {
    if (subFolder.name.isEmpty()) {
        return false;
    }

    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("memory"));
    QSqlQuery query(db);

    if (subFolder.id > 0) {
        query.prepare(
            QStringLiteral("UPDATE noteSubFolder SET parent_id = :parent_id, name = :name, "
                           "file_last_modified = :file_last_modified, modified = :modified "
                           "WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), subFolder.id);
    } else {
        query.prepare(
            QStringLiteral("INSERT INTO noteSubFolder "
                           "(name, file_last_modified, parent_id, modified) "
                           "VALUES (:name, :file_last_modified, :parent_id, :modified)"));
    }

    const QDateTime currentDateTime = QDateTime::currentDateTime();
    query.bindValue(QStringLiteral(":name"), subFolder.name);
    query.bindValue(QStringLiteral(":parent_id"), subFolder.parentId);
    query.bindValue(QStringLiteral(":file_last_modified"), subFolder.fileLastModified);
    query.bindValue(QStringLiteral(":modified"), currentDateTime);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return false;
    }

    return true;
}

bool NoteSubFolderRepository::remove(int id) const {
    if (findById(id).id == 0) {
        return false;
    }

    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("memory"));
    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM noteSubFolder WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return false;
    }

    return true;
}

bool NoteSubFolderRepository::deleteAll() const {
    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("memory"));
    QSqlQuery query(db);

    query.prepare(QStringLiteral("DELETE FROM noteSubFolder"));
    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return false;
    }

    return true;
}

bool NoteSubFolderRepository::setActive(int id) const {
    NoteFolderRepository noteFolders;
    NoteFolderData currentFolder = noteFolders.current();
    if (currentFolder.id == 0) {
        return false;
    }

    currentFolder.activeNoteSubFolderData = id == 0 ? QString() : subFolderPathData(id, QStringLiteral("memory"));
    return noteFolders.save(currentFolder);
}

int NoteSubFolderRepository::countAll() const {
    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("memory"));
    QSqlQuery query(db);

    query.prepare(QStringLiteral("SELECT COUNT(*) AS cnt FROM noteSubFolder"));

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else if (query.first()) {
        return query.value(QStringLiteral("cnt")).toInt();
    }

    return 0;
}

QVector<int> NoteSubFolderRepository::fetchAllIds() const {
    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("memory"));
    QSqlQuery query(db);
    QVector<int> ids;

    query.prepare(QStringLiteral("SELECT id FROM noteSubFolder"));

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return ids;
    }

    while (query.next()) {
        ids.append(query.value(QStringLiteral("id")).toInt());
    }

    return ids;
}

QString NoteSubFolderRepository::treeWidgetExpandStateSettingsKey(int noteFolderId) const {
    if (noteFolderId == 0) {
        noteFolderId = NoteFolderRepository().currentFolderId();
    }

    return QStringLiteral("MainWindow/noteSubFolderTreeWidgetExpandState-") + QString::number(noteFolderId);
}

bool NoteSubFolderRepository::willFolderBeIgnored(const QString& folderName, bool showWarning) const {
    const QStringList internalFolders{QStringLiteral("."), QStringLiteral(".."), QStringLiteral("media"),
                                      QStringLiteral("attachments"), QStringLiteral("trash")};

    if (internalFolders.contains(folderName)) {
#ifndef INTEGRATION_TESTS
        if (showWarning) {
            Utils::Gui::warning(nullptr, QObject::tr("Folder will be hidden!"),
                                QObject::tr("Folder with name <b>%1</b> can't be created, "
                                            "because it's internally used by the "
                                            "application!")
                                    .arg(folderName),
                                "note-subfolder-hidden-internal");
        }
#else
        Q_UNUSED(showWarning)
#endif

        return true;
    }

    const SettingsRepository settings;
    const QStringList ignoredFolderRegExpList =
        settings.value(QStringLiteral("ignoreNoteSubFolders"), defaultIgnoredSubfoldersPattern())
            .toString()
            .split(QLatin1Char(';'));

    if (Utils::Misc::regExpInListMatches(folderName, ignoredFolderRegExpList)) {
#ifndef INTEGRATION_TESTS
        if (showWarning) {
            Utils::Gui::warning(nullptr, QObject::tr("Folder will be hidden!"),
                                QObject::tr("Folder with name <b>%1</b> can't be created, "
                                            "because it's on the list of ignored subfolders! "
                                            "You can change that in the <i>Panels settings</i>.")
                                    .arg(folderName),
                                "note-subfolder-hidden-settings");
        }
#endif

        return true;
    }

    return false;
}

QString NoteSubFolderRepository::defaultIgnoredSubfoldersPattern() const {
    return QStringLiteral("^\\.");
}
