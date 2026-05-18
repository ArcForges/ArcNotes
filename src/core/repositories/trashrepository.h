#pragma once

#include <core/data/notedata.h>
#include <core/data/trashitemdata.h>

#include <QList>

class TrashRepository {
public:
    [[nodiscard]] TrashItemData findById(int id) const;
    [[nodiscard]] QList<TrashItemData> findAll(int limit = -1) const;
    [[nodiscard]] QList<TrashItemData> findAllExpired() const;
    [[nodiscard]] bool save(const TrashItemData& trashItem) const;
    [[nodiscard]] bool remove(int id, bool withFile = false) const;
    [[nodiscard]] bool addNote(const NoteData& note) const;
    [[nodiscard]] bool restore(int id) const;
    [[nodiscard]] QString loadFileText(int id) const;
    [[nodiscard]] QString restorationFilePath(int id) const;
    [[nodiscard]] bool fileExists(int id) const;
    [[nodiscard]] int trashMode() const;
    [[nodiscard]] bool clear(bool withFiles = true) const;
    [[nodiscard]] bool expireItems() const;
    [[nodiscard]] int countAll() const;
};
