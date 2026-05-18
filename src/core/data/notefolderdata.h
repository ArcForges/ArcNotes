#pragma once

#include <QHash>
#include <QMetaType>
#include <QString>
#include <QStringList>

struct NoteFolderData {
    int id = 0;
    QString name;
    QString localPath;
    int priority = 0;
    int activeTagId = 0;
    QString activeNoteSubFolderData;
    bool showSubfolders = false;
    bool allSubfolders = true;
    QStringList excludedSubfolderPaths;

    bool operator==(const NoteFolderData& other) const {
        return id == other.id && name == other.name && localPath == other.localPath && priority == other.priority &&
               activeTagId == other.activeTagId && activeNoteSubFolderData == other.activeNoteSubFolderData &&
               showSubfolders == other.showSubfolders && allSubfolders == other.allSubfolders &&
               excludedSubfolderPaths == other.excludedSubfolderPaths;
    }

    bool operator!=(const NoteFolderData& other) const { return !(*this == other); }
};

inline uint qHash(const NoteFolderData& folder, uint seed = 0) {
    return qHash(folder.id, seed);
}

Q_DECLARE_TYPEINFO(NoteFolderData, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(NoteFolderData)
