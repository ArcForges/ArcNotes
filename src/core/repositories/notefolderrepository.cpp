#include "notefolderrepository.h"

#include <core/repositories/settingsrepository.h>
#include <utils/misc.h>

#include <QDebug>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

namespace {
QString noteFolderSettingsKey(int folderId, const QString& key) {
    return QStringLiteral("NoteFolder-%1/%2").arg(folderId).arg(key);
}

void applySettings(NoteFolderData& folder) {
    const SettingsRepository settings;
    folder.allSubfolders =
        settings.value(noteFolderSettingsKey(folder.id, QStringLiteral("allSubfolders")), true).toBool();
    folder.excludedSubfolderPaths =
        settings.value(noteFolderSettingsKey(folder.id, QStringLiteral("excludedSubfolderPaths"))).toStringList();
}

NoteFolderData folderFromQuery(const QSqlQuery& query) {
    NoteFolderData folder;
    folder.id = query.value(QStringLiteral("id")).toInt();
    folder.name = query.value(QStringLiteral("name")).toString();
    folder.priority = query.value(QStringLiteral("priority")).toInt();
    folder.activeTagId = query.value(QStringLiteral("active_tag_id")).toInt();
    folder.showSubfolders = query.value(QStringLiteral("show_subfolders")).toBool();
    folder.activeNoteSubFolderData = query.value(QStringLiteral("active_note_sub_folder_data")).toString();
    folder.localPath =
        Utils::Misc::prependPortableDataPathIfNeeded(query.value(QStringLiteral("local_path")).toString());
    applySettings(folder);
    return folder;
}

void bindFolderValues(QSqlQuery& query, const NoteFolderData& folder) {
    query.bindValue(QStringLiteral(":name"), folder.name);
    query.bindValue(QStringLiteral(":priority"), folder.priority);
    query.bindValue(QStringLiteral(":activeTagId"), folder.activeTagId);
    query.bindValue(QStringLiteral(":showSubfolders"), folder.showSubfolders);
    query.bindValue(QStringLiteral(":activeNoteSubFolderData"), folder.activeNoteSubFolderData);
    query.bindValue(QStringLiteral(":localPath"),
                    Utils::Misc::makePathRelativeToPortableDataPathIfNeeded(folder.localPath));
}

void storeFolderSettings(const NoteFolderData& folder) {
    const SettingsRepository settings;
    settings.setValue(noteFolderSettingsKey(folder.id, QStringLiteral("allSubfolders")), folder.allSubfolders);
    settings.setValue(noteFolderSettingsKey(folder.id, QStringLiteral("excludedSubfolderPaths")),
                      folder.excludedSubfolderPaths);
}
}  // namespace

NoteFolderData NoteFolderRepository::findById(int id) const {
    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("disk"));
    QSqlQuery query(db);

    query.prepare(QStringLiteral("SELECT * FROM noteFolder WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else if (query.first()) {
        return folderFromQuery(query);
    }

    return NoteFolderData();
}

QList<NoteFolderData> NoteFolderRepository::findAll() const {
    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("disk"));
    QList<NoteFolderData> folders;

    if (!db.tables().contains(QStringLiteral("noteFolder"), Qt::CaseInsensitive)) {
        return folders;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT * FROM noteFolder ORDER BY priority ASC, id ASC"));
    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return folders;
    }

    while (query.next()) {
        folders.append(folderFromQuery(query));
    }

    return folders;
}

NoteFolderData NoteFolderRepository::current() const {
    return findById(currentFolderId());
}

bool NoteFolderRepository::save(const NoteFolderData& folder) const {
    return saveAndReturn(folder).id > 0;
}

NoteFolderData NoteFolderRepository::saveAndReturn(const NoteFolderData& folder) const {
    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("disk"));
    QSqlQuery query(db);
    NoteFolderData stored = folder;

    if (stored.id > 0) {
        query.prepare(
            QStringLiteral("UPDATE noteFolder SET name = :name, local_path = :localPath, "
                           "priority = :priority, active_tag_id = :activeTagId, "
                           "show_subfolders = :showSubfolders, "
                           "active_note_sub_folder_data = :activeNoteSubFolderData "
                           "WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), stored.id);
    } else {
        query.prepare(
            QStringLiteral("INSERT INTO noteFolder (name, local_path, priority, active_tag_id, "
                           "show_subfolders, active_note_sub_folder_data) "
                           "VALUES (:name, :localPath, :priority, :activeTagId, "
                           ":showSubfolders, :activeNoteSubFolderData)"));
    }

    bindFolderValues(query, stored);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return NoteFolderData();
    }

    if (stored.id == 0) {
        stored.id = query.lastInsertId().toInt();
    }

    storeFolderSettings(stored);
    return stored;
}

bool NoteFolderRepository::remove(int id) const {
    if (findById(id).id == 0) {
        return false;
    }

    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("disk"));
    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM noteFolder WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        return false;
    }

    const SettingsRepository settings;
    settings.remove(QStringLiteral("NoteHistory-") + QString::number(id));
    settings.remove(QStringLiteral("NoteHistoryCurrentIndex-") + QString::number(id));
    settings.remove(QStringLiteral("NoteFolder-") + QString::number(id));
    return true;
}

int NoteFolderRepository::countAll() const {
    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("disk"));
    QSqlQuery query(db);

    query.prepare(QStringLiteral("SELECT COUNT(*) AS cnt FROM noteFolder"));

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else if (query.first()) {
        return query.value(QStringLiteral("cnt")).toInt();
    }

    return 0;
}

int NoteFolderRepository::currentFolderId() const {
    const SettingsRepository settings;
    return settings.value(QStringLiteral("currentNoteFolderId")).toInt();
}

void NoteFolderRepository::setCurrentFolderId(int id) const {
    const NoteFolderData folder = findById(id);
    if (folder.id == 0) {
        return;
    }

    const SettingsRepository settings;
    settings.setValue(QStringLiteral("currentNoteFolderId"), folder.id);
    settings.setValue(QStringLiteral("notesPath"),
                      Utils::Misc::makePathRelativeToPortableDataPathIfNeeded(folder.localPath));
}

bool NoteFolderRepository::migrateToNoteFolders() const {
    SettingsRepository settings;
    const QString notesPath =
        Utils::Misc::prependPortableDataPathIfNeeded(settings.value(QStringLiteral("notesPath")).toString());

    if (countAll() > 0 && current().localPath == notesPath) {
        return false;
    }

    int priority = 0;

    if (!notesPath.isEmpty()) {
        NoteFolderData folder;
        folder.name = QObject::tr("default");
        folder.localPath = notesPath;
        folder.priority = priority++;
        folder.showSubfolders = settings.value(QStringLiteral("showNoteSubFolders")).toBool();
        folder = saveAndReturn(folder);

        settings.remove(QStringLiteral("showNoteSubFolders"));

        if (folder.id > 0) {
            setCurrentFolderId(folder.id);
        }
    }

    const QStringList recentNoteFolders = settings.value(QStringLiteral("recentNoteFolders")).toStringList();
    for (const QString& recentNoteFolderPath : recentNoteFolders) {
        if (notesPath == recentNoteFolderPath) {
            continue;
        }

        NoteFolderData folder;
        folder.name = recentNoteFolderPath;
        folder.localPath = recentNoteFolderPath;
        folder.priority = priority++;
        save(folder);
    }

    return priority > 0;
}

QVariant NoteFolderRepository::settingValue(int folderId, const QString& key, const QVariant& defaultValue) const {
    const SettingsRepository settings;
    return settings.value(noteFolderSettingsKey(folderId, key), defaultValue);
}

void NoteFolderRepository::setSettingValue(int folderId, const QString& key, const QVariant& value) const {
    const SettingsRepository settings;
    settings.setValue(noteFolderSettingsKey(folderId, key), value);
}
