#include "tagrepository.h"

#include <core/repositories/notefolderrepository.h>
#include <core/repositories/settingsrepository.h>
#include <services/databaseservice.h>

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringBuilder>
#include <algorithm>

namespace {
QString colorFieldName() {
    return SettingsRepository().value(QStringLiteral("darkMode")).toBool() ? QStringLiteral("dark_color")
                                                                           : QStringLiteral("color");
}

TagData tagFromQuery(const QSqlQuery& query) {
    TagData tag;
    tag.id = query.value(QStringLiteral("id")).toInt();
    tag.name = query.value(QStringLiteral("name")).toString();
    tag.priority = query.value(QStringLiteral("priority")).toInt();
    tag.parentId = query.value(QStringLiteral("parent_id")).toInt();

    const QString colorName = query.value(colorFieldName()).toString();
    tag.color = colorName.isEmpty() ? QColor() : QColor(colorName);
    return tag;
}

QString activeNoteSubFolderPath() {
    const NoteFolderData currentFolder = NoteFolderRepository().current();
    return currentFolder.activeNoteSubFolderData.split(QLatin1Char('\n'), Qt::SkipEmptyParts).join(QLatin1Char('/'));
}

int parentTagId(int tagId) {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT parent_id FROM tag WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), tagId);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else if (query.first()) {
        const int parentId = query.value(QStringLiteral("parent_id")).toInt();
        DatabaseService::closeDatabaseConnection(db, query);
        return parentId;
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return 0;
}

void touchTagAndAncestors(int tagId) {
    int currentTagId = tagId;
    while (currentTagId > 0) {
        QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
        QSqlQuery query(db);
        query.prepare(QStringLiteral("UPDATE tag SET updated = datetime('now') WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), currentTagId);

        if (!query.exec()) {
            qWarning() << __func__ << ": " << query.lastError();
            DatabaseService::closeDatabaseConnection(db, query);
            return;
        }

        DatabaseService::closeDatabaseConnection(db, query);
        currentTagId = parentTagId(currentTagId);
    }
}

QVector<int> childTagIds(int tagId) {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    QVector<int> ids;
    query.prepare(QStringLiteral("SELECT id FROM tag WHERE parent_id = :parentId"));
    query.bindValue(QStringLiteral(":parentId"), tagId);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else {
        while (query.next()) {
            ids.append(query.value(QStringLiteral("id")).toInt());
        }
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return ids;
}
}  // namespace

TagData TagRepository::findById(int id) const {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT * FROM tag WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else if (query.first()) {
        const TagData tag = tagFromQuery(query);
        DatabaseService::closeDatabaseConnection(db, query);
        return tag;
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return TagData();
}

TagData TagRepository::findByName(const QString& name, bool startsWith) const {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    QString searchName = name;
    const QString sql = QStringLiteral("SELECT * FROM tag WHERE name ") %
                        QString(startsWith ? QStringLiteral("LIKE") : QStringLiteral("=")) %
                        QStringLiteral(" :name ORDER BY name");
    query.prepare(sql);

    if (startsWith) {
        searchName += QStringLiteral("%");
    }

    query.bindValue(QStringLiteral(":name"), searchName);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else if (query.first()) {
        const TagData tag = tagFromQuery(query);
        DatabaseService::closeDatabaseConnection(db, query);
        return tag;
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return TagData();
}

QVector<TagData> TagRepository::findAll() const {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QVector<TagData> tags;

    if (!db.isValid() || !db.tables().contains(QStringLiteral("tag"), Qt::CaseInsensitive)) {
        return tags;
    }

    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("SELECT t.id as id, t.name as name, t.priority as priority, max( "
                       "CASE "
                       "WHEN l.created > t.updated THEN l.created "
                       "ELSE t.updated "
                       "END "
                       ") AS created, t.parent_id as parent_id, "
                       "t.color as color, t.dark_color as dark_color "
                       "FROM tag t LEFT JOIN noteTagLink l ON t.id = l.tag_id "
                       "GROUP BY t.name "
                       "ORDER BY created DESC"));

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else {
        while (query.next()) {
            tags.append(tagFromQuery(query));
        }
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return tags;
}

QVector<TagData> TagRepository::findByParentId(int parentId) const {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    QVector<TagData> tags;
    query.prepare(
        QStringLiteral("SELECT t.id as id, t.name as name, t.priority as priority, max( "
                       "CASE "
                       "WHEN l.created > t.updated THEN l.created "
                       "ELSE t.updated "
                       "END "
                       ") AS created, t.parent_id as parent_id, "
                       "t.color as color, t.dark_color as dark_color "
                       "FROM tag t LEFT JOIN noteTagLink l ON t.id = l.tag_id "
                       "WHERE parent_id = :parentId "
                       "GROUP BY t.name "
                       "ORDER BY created DESC"));
    query.bindValue(QStringLiteral(":parentId"), parentId);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else {
        while (query.next()) {
            tags.append(tagFromQuery(query));
        }
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return tags;
}

QVector<TagData> TagRepository::findByNote(const NoteData& note) const {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    QVector<TagData> tags;

    query.prepare(
        QStringLiteral("SELECT t.* FROM tag t "
                       "JOIN noteTagLink l ON t.id = l.tag_id "
                       "WHERE l.note_file_name = :fileName AND "
                       "l.note_sub_folder_path = :noteSubFolderPath "
                       "ORDER BY t.priority ASC, t.name ASC"));
    query.bindValue(QStringLiteral(":fileName"), note.name);
    query.bindValue(QStringLiteral(":noteSubFolderPath"), note.relativeNoteSubFolderPath);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else {
        while (query.next()) {
            tags.append(tagFromQuery(query));
        }
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return tags;
}

bool TagRepository::linkToNote(int tagId, const NoteData& note) const {
    const int parentId = parentTagId(tagId);
    if (tagId <= 0 || findById(tagId).id == 0) {
        return false;
    }

    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("INSERT INTO noteTagLink (tag_id, note_file_name, note_sub_folder_path) "
                       "VALUES (:tagId, :noteFileName, :noteSubFolderPath)"));
    query.bindValue(QStringLiteral(":tagId"), tagId);
    query.bindValue(QStringLiteral(":noteFileName"), note.name);
    query.bindValue(QStringLiteral(":noteSubFolderPath"), note.relativeNoteSubFolderPath);

    if (!query.exec()) {
        DatabaseService::closeDatabaseConnection(db, query);
        return false;
    }

    DatabaseService::closeDatabaseConnection(db, query);
    touchTagAndAncestors(parentId);
    return true;
}

bool TagRepository::unlinkFromNote(int tagId, const NoteData& note) const {
    if (tagId <= 0 || findById(tagId).id == 0) {
        return false;
    }

    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("DELETE FROM noteTagLink WHERE tag_id = :tagId AND "
                       "note_file_name = :noteFileName AND "
                       "note_sub_folder_path = :noteSubFolderPath"));
    query.bindValue(QStringLiteral(":tagId"), tagId);
    query.bindValue(QStringLiteral(":noteFileName"), note.name);
    query.bindValue(QStringLiteral(":noteSubFolderPath"), note.relativeNoteSubFolderPath);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        DatabaseService::closeDatabaseConnection(db, query);
        return false;
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return true;
}

bool TagRepository::renameNoteFileNamesOfLinks(const QString& oldName, const QString& newName,
                                               const QString& noteSubFolderPath) const {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("UPDATE noteTagLink SET note_file_name = :newName WHERE "
                       "note_file_name = :oldName AND "
                       "note_sub_folder_path = :noteSubFolderPath"));
    query.bindValue(QStringLiteral(":oldName"), oldName);
    query.bindValue(QStringLiteral(":newName"), newName);
    query.bindValue(QStringLiteral(":noteSubFolderPath"), noteSubFolderPath);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        DatabaseService::closeDatabaseConnection(db, query);
        return false;
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return true;
}

bool TagRepository::renameNoteSubFolderPathOfLinks(const QString& noteName, const QString& oldPath,
                                                   const QString& newPath) const {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("UPDATE noteTagLink SET note_sub_folder_path = :newPath WHERE "
                       "note_file_name = :noteName AND note_sub_folder_path = :oldPath"));
    query.bindValue(QStringLiteral(":noteName"), noteName);
    query.bindValue(QStringLiteral(":oldPath"), oldPath);
    query.bindValue(QStringLiteral(":newPath"), newPath);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        DatabaseService::closeDatabaseConnection(db, query);
        return false;
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return true;
}

bool TagRepository::renameNoteSubFolderPathsOfLinks(const QString& oldPath, const QString& newPath) const {
    if (oldPath.isEmpty() || oldPath == newPath) {
        return true;
    }

    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("UPDATE noteTagLink SET note_sub_folder_path = "
                       "replace(note_sub_folder_path, :oldPrefix, :newPrefix) WHERE "
                       "note_sub_folder_path = :oldPath OR note_sub_folder_path LIKE :oldLike"));
    query.bindValue(QStringLiteral(":oldPrefix"), oldPath + QLatin1Char('/'));
    query.bindValue(QStringLiteral(":newPrefix"), newPath.isEmpty() ? QString() : newPath + QLatin1Char('/'));
    query.bindValue(QStringLiteral(":oldPath"), oldPath);
    query.bindValue(QStringLiteral(":oldLike"), oldPath + QStringLiteral("/%"));

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        DatabaseService::closeDatabaseConnection(db, query);
        return false;
    }

    query.prepare(
        QStringLiteral("UPDATE noteTagLink SET note_sub_folder_path = :newPath WHERE "
                       "note_sub_folder_path = :oldPath"));
    query.bindValue(QStringLiteral(":newPath"), newPath);
    query.bindValue(QStringLiteral(":oldPath"), oldPath);
    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        DatabaseService::closeDatabaseConnection(db, query);
        return false;
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return true;
}

bool TagRepository::save(const TagData& tag) const {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    const QString colorField = colorFieldName();

    if (tag.id > 0) {
        query.prepare(QStringLiteral("UPDATE tag SET name = :name, priority = :priority, "
                                     "parent_id = :parentId, ") %
                      colorField % QStringLiteral(" = :color, updated = datetime('now') WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), tag.id);
    } else {
        query.prepare(QStringLiteral("INSERT INTO tag (name, priority, parent_id, ") % colorField %
                      QStringLiteral(") VALUES (:name, :priority, :parentId, :color)"));
    }

    query.bindValue(QStringLiteral(":name"), tag.name);
    query.bindValue(QStringLiteral(":priority"), tag.priority);
    query.bindValue(QStringLiteral(":parentId"), tag.parentId);
    query.bindValue(QStringLiteral(":color"), tag.color.isValid() ? tag.color.name() : QString());

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        DatabaseService::closeDatabaseConnection(db, query);
        return false;
    }

    DatabaseService::closeDatabaseConnection(db, query);
    touchTagAndAncestors(tag.parentId);
    return true;
}

bool TagRepository::remove(int id) const {
    if (id <= 0 || findById(id).id == 0) {
        return false;
    }

    const QVector<int> childIds = childTagIds(id);
    for (const int childId : childIds) {
        if (!remove(childId)) {
            return false;
        }
    }

    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM tag WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        DatabaseService::closeDatabaseConnection(db, query);
        return false;
    }

    query.prepare(QStringLiteral("DELETE FROM noteTagLink WHERE tag_id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
        DatabaseService::closeDatabaseConnection(db, query);
        return false;
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return true;
}

bool TagRepository::hasDescendant(int tagId, int descendantId) const {
    if (tagId <= 0 || descendantId <= 0 || tagId == descendantId) {
        return tagId > 0 && tagId == descendantId;
    }

    const QVector<int> childIds = childTagIds(tagId);
    return std::ranges::any_of(childIds, [this, descendantId](int childId) {
        return childId == descendantId || hasDescendant(childId, descendantId);
    });
}

int TagRepository::countLinkedNotes(int tagId, bool fromAllSubfolders, bool recursive) const {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);

    if (fromAllSubfolders) {
        query.prepare(
            QStringLiteral("SELECT COUNT(note_file_name) AS cnt FROM noteTagLink "
                           "WHERE tag_id = :id"));
    } else if (recursive) {
        query.prepare(
            QStringLiteral("SELECT COUNT(note_file_name) AS cnt FROM noteTagLink "
                           "WHERE tag_id = :id AND note_sub_folder_path LIKE :noteSubFolderPath"));
        query.bindValue(QStringLiteral(":noteSubFolderPath"), activeNoteSubFolderPath() + QLatin1Char('%'));
    } else {
        query.prepare(
            QStringLiteral("SELECT COUNT(note_file_name) AS cnt FROM noteTagLink "
                           "WHERE tag_id = :id AND note_sub_folder_path = :noteSubFolderPath"));
        query.bindValue(QStringLiteral(":noteSubFolderPath"), activeNoteSubFolderPath());
    }
    query.bindValue(QStringLiteral(":id"), tagId);

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

QVector<TagData> TagRepository::searchByName(const QString& name) const {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    QVector<TagData> tags;

    query.prepare(QStringLiteral("SELECT * FROM tag WHERE name LIKE :name ORDER BY priority ASC, name ASC"));
    query.bindValue(QStringLiteral(":name"), QStringLiteral("%") + name + QStringLiteral("%"));

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else {
        while (query.next()) {
            tags.append(tagFromQuery(query));
        }
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return tags;
}

QHash<QString, QVector<int>> TagRepository::allIdsByNoteFilePath() const {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    QHash<QString, QVector<int>> tagIdsByNoteFilePath;

    query.prepare(
        QStringLiteral("SELECT tag_id, note_file_name, note_sub_folder_path FROM noteTagLink "
                       "ORDER BY note_sub_folder_path ASC, note_file_name ASC, tag_id ASC"));

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else {
        while (query.next()) {
            const QString noteFileName = query.value(QStringLiteral("note_file_name")).toString();
            const QString noteSubFolderPath = query.value(QStringLiteral("note_sub_folder_path")).toString();
            const QString key = noteSubFolderPath + QStringLiteral("/") + noteFileName;
            tagIdsByNoteFilePath[key] << query.value(QStringLiteral("tag_id")).toInt();
        }
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return tagIdsByNoteFilePath;
}

QHash<QString, QStringList> TagRepository::allNamesByNoteFilePath() const {
    QSqlDatabase db = DatabaseService::getNoteFolderDatabase();
    QSqlQuery query(db);
    QHash<QString, QStringList> tagNamesByNoteFilePath;

    query.prepare(
        QStringLiteral("SELECT l.note_file_name, l.note_sub_folder_path, t.name FROM tag t "
                       "JOIN noteTagLink l ON t.id = l.tag_id "
                       "ORDER BY l.note_sub_folder_path ASC, l.note_file_name ASC, "
                       "t.priority ASC, t.name ASC"));

    if (!query.exec()) {
        qWarning() << __func__ << ": " << query.lastError();
    } else {
        while (query.next()) {
            const QString noteFileName = query.value(QStringLiteral("note_file_name")).toString();
            const QString noteSubFolderPath = query.value(QStringLiteral("note_sub_folder_path")).toString();
            const QString key = noteSubFolderPath + QStringLiteral("/") + noteFileName;
            tagNamesByNoteFilePath[key] << query.value(QStringLiteral("name")).toString();
        }
    }

    DatabaseService::closeDatabaseConnection(db, query);
    return tagNamesByNoteFilePath;
}
