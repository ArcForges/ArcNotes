#pragma once

#include <QDateTime>
#include <QHash>
#include <QMetaType>
#include <QString>

struct TrashItemData {
    int id = 0;
    QString fileName;
    qint64 fileSize = 0;
    QString noteSubFolderPathData;
    int noteSubFolderId = 0;
    QDateTime created;
    QString fullNoteFilePath;

    bool operator==(const TrashItemData& other) const {
        return id == other.id && fileName == other.fileName && fileSize == other.fileSize &&
               noteSubFolderPathData == other.noteSubFolderPathData && noteSubFolderId == other.noteSubFolderId &&
               created == other.created && fullNoteFilePath == other.fullNoteFilePath;
    }

    bool operator!=(const TrashItemData& other) const { return !(*this == other); }
};

inline uint qHash(const TrashItemData& trashItem, uint seed = 0) {
    return qHash(trashItem.id, seed);
}

Q_DECLARE_TYPEINFO(TrashItemData, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(TrashItemData)
