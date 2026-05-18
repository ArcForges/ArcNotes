#include "tagservice.h"

TagService::TagService(TagRepository* tagRepository)
    : _tagRepository(tagRepository != nullptr ? tagRepository : &_ownedTagRepository) {}

TagData TagService::createTag(const QString& name, int parentId) const {
    TagData tag;
    tag.name = name;
    tag.parentId = parentId;
    if (!_tagRepository->save(tag)) {
        return TagData();
    }
    return _tagRepository->findByName(name);
}

bool TagService::deleteTag(int tagId) const {
    return _tagRepository->remove(tagId);
}

bool TagService::renameTag(int tagId, const QString& name) const {
    TagData tag = _tagRepository->findById(tagId);
    if (tag.id <= 0) {
        return false;
    }
    tag.name = name;
    return _tagRepository->save(tag);
}

bool TagService::tagNote(const NoteData& note, int tagId) const {
    return _tagRepository->linkToNote(tagId, note);
}

bool TagService::untagNote(const NoteData& note, int tagId) const {
    return _tagRepository->unlinkFromNote(tagId, note);
}

QVector<TagData> TagService::getNoteTags(const NoteData& note) const {
    return _tagRepository->findByNote(note);
}

QVector<TagData> TagService::fetchTagTree(int parentId) const {
    return parentId <= 0 ? _tagRepository->findAll() : _tagRepository->findByParentId(parentId);
}

int TagService::countLinkedNotes(int tagId, bool fromAllSubfolders, bool recursive) const {
    return _tagRepository->countLinkedNotes(tagId, fromAllSubfolders, recursive);
}

QHash<QString, QVector<int>> TagService::allIdsByNoteFilePath() const {
    return _tagRepository->allIdsByNoteFilePath();
}

QHash<QString, QStringList> TagService::allNamesByNoteFilePath() const {
    return _tagRepository->allNamesByNoteFilePath();
}

bool TagService::moveTag(int tagId, int parentId) const {
    TagData tag = _tagRepository->findById(tagId);
    if (tag.id <= 0 || tagId == parentId || _tagRepository->hasDescendant(tagId, parentId)) {
        return false;
    }
    tag.parentId = parentId;
    return _tagRepository->save(tag);
}

bool TagService::setTagColor(int tagId, const QColor& color) const {
    TagData tag = _tagRepository->findById(tagId);
    if (tag.id <= 0) {
        return false;
    }

    tag.color = color;
    return _tagRepository->save(tag);
}
