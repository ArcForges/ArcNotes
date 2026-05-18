#include "trashservice.h"

TrashService::TrashService(TrashRepository* trashRepository)
    : _trashRepository(trashRepository != nullptr ? trashRepository : &_ownedTrashRepository) {}

bool TrashService::moveNoteToTrash(const NoteData& note) const {
    return _trashRepository->addNote(note);
}

bool TrashService::restoreTrashItem(int trashItemId) const {
    return _trashRepository->restore(trashItemId);
}

bool TrashService::removeTrashItem(int trashItemId) const {
    return _trashRepository->remove(trashItemId, true);
}

bool TrashService::clearTrash() const {
    return _trashRepository->clear(true);
}

QList<TrashItemData> TrashService::trashItems(int limit) const {
    return _trashRepository->findAll(limit);
}

QString TrashService::trashItemText(int trashItemId) const {
    return _trashRepository->loadFileText(trashItemId);
}

QString TrashService::trashItemRestorationPath(int trashItemId) const {
    return _trashRepository->restorationFilePath(trashItemId);
}

bool TrashService::trashItemFileExists(int trashItemId) const {
    return _trashRepository->fileExists(trashItemId);
}
