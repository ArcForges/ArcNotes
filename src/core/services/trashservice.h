#pragma once

#include <core/data/notedata.h>
#include <core/data/trashitemdata.h>
#include <core/repositories/trashrepository.h>

#include <QList>

class TrashService {
public:
    explicit TrashService(TrashRepository* trashRepository = nullptr);

    [[nodiscard]] bool moveNoteToTrash(const NoteData& note) const;
    [[nodiscard]] bool restoreTrashItem(int trashItemId) const;
    [[nodiscard]] bool removeTrashItem(int trashItemId) const;
    [[nodiscard]] bool clearTrash() const;
    [[nodiscard]] QList<TrashItemData> trashItems(int limit = -1) const;
    [[nodiscard]] QString trashItemText(int trashItemId) const;
    [[nodiscard]] QString trashItemRestorationPath(int trashItemId) const;
    [[nodiscard]] bool trashItemFileExists(int trashItemId) const;

private:
    TrashRepository* _trashRepository;
    TrashRepository _ownedTrashRepository;
};
