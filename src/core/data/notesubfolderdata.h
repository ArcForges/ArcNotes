#pragma once

#include <QDateTime>
#include <QHash>
#include <QMetaType>
#include <QString>

struct NoteSubFolderData {
    int id = 0;
    int parentId = 0;
    QString name;
    QDateTime fileLastModified;
    QDateTime created;
    QDateTime modified;

    bool operator==(const NoteSubFolderData& other) const {
        return id == other.id && parentId == other.parentId && name == other.name &&
               fileLastModified == other.fileLastModified && created == other.created && modified == other.modified;
    }

    bool operator!=(const NoteSubFolderData& other) const { return !(*this == other); }
};

inline size_t qHash(const NoteSubFolderData& subFolder, size_t seed = 0) {
    return qHash(subFolder.id, seed);
}

Q_DECLARE_TYPEINFO(NoteSubFolderData, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(NoteSubFolderData)
