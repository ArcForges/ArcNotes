#pragma once

#include <core/data/notedata.h>
#include <core/data/trashitemdata.h>
#include <core/repositories/trashrepository.h>

#include <QList>

class TrashService {
public:
    explicit TrashService(TrashRepository* trashRepository = nullptr);

    bool moveNoteToTrash(const NoteData& note) const;
    bool restoreTrashItem(int trashItemId) const;
    bool removeTrashItem(int trashItemId) const;
    bool clearTrash() const;
    QList<TrashItemData> trashItems(int limit = -1) const;
    QString trashItemText(int trashItemId) const;
    QString trashItemRestorationPath(int trashItemId) const;
    bool trashItemFileExists(int trashItemId) const;

private:
    TrashRepository* _trashRepository;
    TrashRepository _ownedTrashRepository;
};
