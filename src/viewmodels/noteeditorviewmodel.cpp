#include "noteeditorviewmodel.h"

#include <core/commands/commandbus.h>
#include <core/data/commands.h>
#include <core/data/notedata.h>
#include <core/state/appstate.h>
#include <core/state/editorstate.h>
#include <utils/debouncer.h>

#include <QVariant>

NoteEditorViewModel::NoteEditorViewModel(CommandBus* commandBus, AppState* appState, EditorState* editorState,
                                         QObject* parent)
    : QObject(parent),
      _commandBus(commandBus),
      _appState(appState),
      _editorState(editorState),
      _saveDebouncer(new Debouncer(this)) {
    if (_appState != nullptr) {
        connect(_appState, &AppState::currentNoteChanged, this, &NoteEditorViewModel::loadNote);
    }
    if (_editorState != nullptr) {
        connect(_editorState, &EditorState::dirtyChanged, this, &NoteEditorViewModel::syncFromEditorState);
        connect(_editorState, &EditorState::savingChanged, this, &NoteEditorViewModel::syncFromEditorState);
        connect(_editorState, &EditorState::hasConflictChanged, this, &NoteEditorViewModel::syncFromEditorState);
        connect(_editorState, &EditorState::readOnlyChanged, this, &NoteEditorViewModel::syncFromEditorState);
        connect(_editorState, &EditorState::cursorPositionChanged, this, &NoteEditorViewModel::syncFromEditorState);
        syncFromEditorState();
    }
}

NoteEditorViewModel::~NoteEditorViewModel() = default;

int NoteEditorViewModel::noteId() const {
    return _noteId;
}

QString NoteEditorViewModel::noteTitle() const {
    return _noteTitle;
}

QString NoteEditorViewModel::noteText() const {
    return _noteText;
}

bool NoteEditorViewModel::isDirty() const {
    return _dirty;
}

bool NoteEditorViewModel::isReadOnly() const {
    return _readOnly;
}

bool NoteEditorViewModel::isSaving() const {
    return _saving;
}

bool NoteEditorViewModel::hasConflict() const {
    return _hasConflict;
}

int NoteEditorViewModel::cursorPosition() const {
    return _cursorPosition;
}

void NoteEditorViewModel::setCommandBus(CommandBus* commandBus) {
    _commandBus = commandBus;
}

void NoteEditorViewModel::setAppState(AppState* appState) {
    _appState = appState;
}

void NoteEditorViewModel::setEditorState(EditorState* editorState) {
    _editorState = editorState;
    syncFromEditorState();
}

void NoteEditorViewModel::loadNote(const NoteData& note) {
    if (_noteId != note.id) {
        _noteId = note.id;
        emit noteIdChanged(_noteId);
    }
    if (_noteTitle != note.name) {
        _noteTitle = note.name;
        emit noteTitleChanged(_noteTitle);
    }
    if (_noteText != note.noteText) {
        _noteText = note.noteText;
        emit noteTextChanged(_noteText);
    }
    _baseChecksum = note.fileChecksum;
    if (_editorState != nullptr) {
        _editorState->setNoteId(note.id);
        _editorState->setBaseChecksum(note.fileChecksum);
        _editorState->setDirty(false);
        _editorState->setHasConflict(false);
    }
}

void NoteEditorViewModel::textChanged(const QString& text) {
    if (_noteText == text) {
        return;
    }
    _noteText = text;
    emit noteTextChanged(_noteText);
    if (_editorState != nullptr) {
        _editorState->setDirty(true);
    }
    _dirty = true;
    emit dirtyChanged(_dirty);
    _saveDebouncer->debounce(QStringLiteral("note-save"), [this]() { save(); }, 750);
}

void NoteEditorViewModel::save() {
    if (_commandBus == nullptr || _noteId <= 0 || _readOnly) {
        return;
    }

    SaveNoteCommand command;
    command.noteId = _noteId;
    command.text = _noteText;
    command.baseChecksum = _baseChecksum;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

void NoteEditorViewModel::formatBold() {
    emit formatRequested(QStringLiteral("bold"), {});
}

void NoteEditorViewModel::formatItalic() {
    emit formatRequested(QStringLiteral("italic"), {});
}

void NoteEditorViewModel::formatHeading(int level) {
    emit formatRequested(QStringLiteral("heading"), level);
}

void NoteEditorViewModel::insertLink(const QString& url, const QString& title) {
    emit linkInsertionRequested(url, title);
}

void NoteEditorViewModel::insertImage(const QString& sourcePath, const QString& title) {
    if (_commandBus == nullptr || _noteId <= 0) {
        return;
    }

    InsertMediaCommand command;
    command.noteId = _noteId;
    command.sourcePath = sourcePath;
    command.title = title;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

void NoteEditorViewModel::resolveConflict(const QString& resolution) {
    emit conflictResolutionRequested(resolution);
}

void NoteEditorViewModel::setCursorPosition(int position) {
    if (_cursorPosition == position) {
        return;
    }
    _cursorPosition = position;
    emit cursorPositionChanged(_cursorPosition);
    if (_editorState != nullptr) {
        _editorState->setCursorPosition(position);
    }
}

void NoteEditorViewModel::syncFromEditorState() {
    if (_editorState == nullptr) {
        return;
    }

    if (_dirty != _editorState->isDirty()) {
        _dirty = _editorState->isDirty();
        emit dirtyChanged(_dirty);
    }
    if (_saving != _editorState->isSaving()) {
        _saving = _editorState->isSaving();
        emit savingChanged(_saving);
    }
    if (_hasConflict != _editorState->hasConflict()) {
        _hasConflict = _editorState->hasConflict();
        emit conflictChanged(_hasConflict);
    }
    if (_readOnly != _editorState->isReadOnly()) {
        _readOnly = _editorState->isReadOnly();
        emit readOnlyChanged(_readOnly);
    }
    if (_cursorPosition != _editorState->cursorPosition()) {
        _cursorPosition = _editorState->cursorPosition();
        emit cursorPositionChanged(_cursorPosition);
    }
}
