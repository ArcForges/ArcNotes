#pragma once

#include <core/data/notedata.h>
#include <core/data/trashitemdata.h>

#include <QList>

class TrashRepository {
public:
    TrashItemData findById(int id) const;
    QList<TrashItemData> findAll(int limit = -1) const;
    QList<TrashItemData> findAllExpired() const;
    bool save(const TrashItemData& trashItem) const;
    bool remove(int id, bool withFile = false) const;
    bool addNote(const NoteData& note) const;
    bool restore(int id) const;
    QString loadFileText(int id) const;
    QString restorationFilePath(int id) const;
    bool fileExists(int id) const;
    int trashMode() const;
    bool clear(bool withFiles = true) const;
    bool expireItems() const;
    int countAll() const;
};
