#include "editorstate.h"

EditorState::EditorState(QObject* parent) : QObject(parent) {}

int EditorState::noteId() const {
    return _noteId;
}

bool EditorState::isDirty() const {
    return _dirty;
}

bool EditorState::isSaving() const {
    return _saving;
}

bool EditorState::hasConflict() const {
    return _hasConflict;
}

int EditorState::cursorPosition() const {
    return _cursorPosition;
}

int EditorState::selectionStart() const {
    return _selectionStart;
}

int EditorState::selectionLength() const {
    return _selectionLength;
}

QString EditorState::contentHash() const {
    return _contentHash;
}

QString EditorState::baseChecksum() const {
    return _baseChecksum;
}

bool EditorState::isReadOnly() const {
    return _readOnly;
}

EditorSessionSnapshot EditorState::toSnapshot() const {
    EditorSessionSnapshot snapshot;
    snapshot.noteId = _noteId;
    snapshot.baseChecksum = _baseChecksum;
    snapshot.dirty = _dirty;
    snapshot.saving = _saving;
    snapshot.conflict = _hasConflict;
    snapshot.cursorPosition = _cursorPosition;
    return snapshot;
}

void EditorState::setNoteId(int noteId) {
    if (_noteId == noteId) {
        return;
    }

    _noteId = noteId;
    emit noteIdChanged(_noteId);
}

void EditorState::setDirty(bool dirty) {
    if (_dirty == dirty) {
        return;
    }

    _dirty = dirty;
    emit dirtyChanged(_dirty);
}

void EditorState::setSaving(bool saving) {
    if (_saving == saving) {
        return;
    }

    _saving = saving;
    emit savingChanged(_saving);
}

void EditorState::setHasConflict(bool hasConflict) {
    if (_hasConflict == hasConflict) {
        return;
    }

    _hasConflict = hasConflict;
    emit hasConflictChanged(_hasConflict);
}

void EditorState::setCursorPosition(int position) {
    if (_cursorPosition == position) {
        return;
    }

    _cursorPosition = position;
    emit cursorPositionChanged(_cursorPosition);
}

void EditorState::setSelectionStart(int position) {
    if (_selectionStart == position) {
        return;
    }

    _selectionStart = position;
    emit selectionStartChanged(_selectionStart);
}

void EditorState::setSelectionLength(int length) {
    if (_selectionLength == length) {
        return;
    }

    _selectionLength = length;
    emit selectionLengthChanged(_selectionLength);
}

void EditorState::setContentHash(const QString& hash) {
    if (_contentHash == hash) {
        return;
    }

    _contentHash = hash;
    emit contentHashChanged(_contentHash);
}

void EditorState::setBaseChecksum(const QString& checksum) {
    if (_baseChecksum == checksum) {
        return;
    }

    _baseChecksum = checksum;
    emit baseChecksumChanged(_baseChecksum);
}

void EditorState::setReadOnly(bool readOnly) {
    if (_readOnly == readOnly) {
        return;
    }

    _readOnly = readOnly;
    emit readOnlyChanged(_readOnly);
}

void EditorState::setFromSnapshot(const EditorSessionSnapshot& snapshot) {
    setNoteId(snapshot.noteId);
    setBaseChecksum(snapshot.baseChecksum);
    setDirty(snapshot.dirty);
    setSaving(snapshot.saving);
    setHasConflict(snapshot.conflict);
    setCursorPosition(snapshot.cursorPosition);
}

void EditorState::reset() {
    setFromSnapshot(EditorSessionSnapshot());
    setSelectionStart(0);
    setSelectionLength(0);
    setContentHash(QString());
    setReadOnly(false);
}
