#pragma once

#include <QDateTime>
#include <QHash>
#include <QMetaType>
#include <QString>

struct NoteData {
    int id = 0;
    int noteSubFolderId = 0;
    QString relativeNoteSubFolderPath;
    QString fullNoteFilePath;
    QString name;
    QString fileName;
    QString noteText;
    QString fileChecksum;
    QDateTime fileCreated;
    QDateTime fileLastModified;
    QDateTime created;
    QDateTime modified;
    int fileSize = 0;
    bool hasDirtyData = false;
    bool isFavorite = false;

    bool operator==(const NoteData& other) const {
        return id == other.id && noteSubFolderId == other.noteSubFolderId &&
               relativeNoteSubFolderPath == other.relativeNoteSubFolderPath &&
               fullNoteFilePath == other.fullNoteFilePath && name == other.name && fileName == other.fileName &&
               noteText == other.noteText && fileChecksum == other.fileChecksum && fileCreated == other.fileCreated &&
               fileLastModified == other.fileLastModified && created == other.created && modified == other.modified &&
               fileSize == other.fileSize && hasDirtyData == other.hasDirtyData && isFavorite == other.isFavorite;
    }

    bool operator!=(const NoteData& other) const { return !(*this == other); }
};

inline uint qHash(const NoteData& note, uint seed = 0) {
    return qHash(note.id, seed);
}

Q_DECLARE_TYPEINFO(NoteData, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(NoteData)
